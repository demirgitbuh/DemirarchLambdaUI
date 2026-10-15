// Demirarch Lambda UI — istemci köprüsü: yaşam döngüsü, gamepad/klasik mod, girdi uygulama.
// Standart ve Lambda başlıkları SDK başlıklarından önce (SDK/Windows makroları etkilemesin).
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "lambda/core/fileio.h"
#include "lambda/core/log.h"
#include "lambda/core/mode.h"
#include "lambda/core/profile_store.h"
#include "lambda/core/recovery.h"
#include "lambda/core/resolver.h"
#include "lambda/core/settings.h"
#include "lambda/platform/gamepad.h"
#include "lambda/version.h"

#include "hud.h"
#include "cl_util.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "in_defs.h"

#include "lambda_bridge/client_bridge.h"
#include "lambda_bridge/gl.h"
#include "bridge_internal.h"

namespace lambda::bridge
{
namespace
{
constexpr const char* kStateFile = "state.cfg";

GameId CurrentGame()
{
	const auto g = GameIdFromString(LAMBDA_GAME_ID);
	return g ? *g : GameId::HalfLife;
}

double Now()
{
	using namespace std::chrono;
	static const auto start = steady_clock::now();
	return duration<double>(steady_clock::now() - start).count();
}

void Print(const std::string& text)
{
	gEngfuncs.Con_Printf("%s\n", text.c_str());
}

struct State
{
	bool initialized = false;
	bool safe_mode = false;
	GameId game = GameId::HalfLife;
	std::string mod_dir;
	std::string data_dir;
	ModeController mode{InterfaceMode::Gamepad};
	SettingJournal journal;
	platform::GamepadManager pads;
	BindingResolver resolver;
	ControllerSettings settings;
	CameraController camera;
	RecoveryCombo recovery;
	platform::PadState pad;
	bool pad_valid = false;
	bool input_enabled = false;
	bool overlay_enabled = false;
	bool multiplayer = false;
	int waterlevel = 0;
	std::string loaded_profile_family;
	std::vector<ActionEvent> events;
	std::vector<platform::DeviceEvent> device_events;
	bool gameui_active_logged = false;
};

State g;

// ---------------------------------------------------------------- kalıcı durum

void SaveState()
{
	if (g.data_dir.empty())
		return;
	SettingsDocument doc;
	StampDocument(doc, "state");
	doc.Set("state", "mode", InterfaceModeId(g.mode.Current()));
	g.journal.WriteTo(doc);
	fileio::MakeDirectories(g.data_dir);
	if (!fileio::WriteFileAtomic(fileio::Join(g.data_dir, kStateFile), doc.Serialize()))
		LAMBDA_LOG_ERROR("could not write %s", kStateFile);
}

InterfaceMode LoadStateMode()
{
	auto text = fileio::ReadFile(fileio::Join(g.data_dir, kStateFile));
	if (!text)
		return InterfaceMode::Gamepad; // ilk kurulum: varsayılan gamepad modu
	auto doc = SettingsDocument::Parse(*text);
	std::string err;
	if (!doc || !CheckDocument(*doc, "state", &err))
	{
		LAMBDA_LOG_WARN("state file is damaged; starting in gamepad mode");
		return InterfaceMode::Gamepad;
	}
	g.journal.ReadFrom(*doc);
	const auto m = InterfaceModeFromId(doc->GetString("state", "mode", "gamepad"));
	return m ? *m : InterfaceMode::Gamepad;
}

// ---------------------------------------------------------------- profiller

std::string FamilyProfileName(platform::GlyphFamily f)
{
	switch (f)
	{
	case platform::GlyphFamily::Xbox: return "xbox";
	case platform::GlyphFamily::PlayStation: return "playstation";
	case platform::GlyphFamily::SteamDeck: return "steamdeck";
	case platform::GlyphFamily::Generic: break;
	}
	return "generic";
}

void ApplySettingsToResolver()
{
	g.resolver.SetBindings(g.settings.bindings);
	g.resolver.SetTiming(g.settings.timing);
	for (int i = 1; i < kActionCount; ++i)
		g.resolver.SetToggle(static_cast<Action>(i), g.settings.toggle[i]);
}

void LoadProfiles(const std::string& family)
{
	g.settings = ControllerSettings::Defaults(g.game);
	g.loaded_profile_family = family;
	if (g.safe_mode)
	{
		LAMBDA_LOG_INFO("safe mode: profiles are not loaded or changed");
		ApplySettingsToResolver();
		return;
	}
	ProfileStore store(fileio::Join(g.data_dir, "profiles"));
	SettingsDocument merged;
	for (const std::string& name : {std::string("common"), family})
	{
		auto r = store.Load(name);
		if (!r.message.empty())
		{
			LAMBDA_LOG_WARN("%s", r.message.c_str());
			Print("[Lambda UI] " + r.message);
		}
		if (r.source == LoadSource::Primary || r.source == LoadSource::Backup)
			merged.Overlay(r.document);
	}
	std::vector<std::string> warnings;
	g.settings = ReadControllerSettings(merged, g.game, &warnings);
	for (const auto& w : warnings)
	{
		LAMBDA_LOG_WARN("%s", w.c_str());
		Print("[Lambda UI] " + w);
	}
	ApplySettingsToResolver();
	LAMBDA_LOG_INFO("profile loaded for device family '%s'", family.c_str());
}

// ---------------------------------------------------------------- mod alt sistemleri

class InputSubsystem final : public ModeSubsystem
{
public:
	const char* Name() const override { return "gamepad input"; }
	void EnterGamepad() override
	{
		IN_LambdaCloseNativeJoystick();
		std::string err;
		if (!g.pads.Init(&err))
			LAMBDA_LOG_ERROR("SDL3 gamepad init failed: %s", err.c_str());
		g.input_enabled = true;
		g.camera.Reset();
	}
	void EnterClassic() override
	{
		std::vector<ActionEvent> released;
		g.resolver.ReleaseAll(released);
		g.pads.StopAllEffects();
		g.pads.Shutdown();
		g.input_enabled = false;
		g.pad_valid = false;
	}
};

class InterfaceSubsystem final : public ModeSubsystem
{
public:
	const char* Name() const override { return "gamepad interface"; }
	void EnterGamepad() override { g.overlay_enabled = true; }
	void EnterClassic() override { g.overlay_enabled = false; }
};

class EngineSettingsSubsystem final : public ModeSubsystem
{
public:
	const char* Name() const override { return "engine settings"; }
	void EnterGamepad() override {}
	void EnterClassic() override
	{
		g.journal.RestoreAll([](const std::string& name, const std::string& value) {
			gEngfuncs.Cvar_Set(name.c_str(), value.c_str());
			LAMBDA_LOG_INFO("restored %s = %s", name.c_str(), value.c_str());
		});
	}
};

InputSubsystem g_input_sub;
InterfaceSubsystem g_interface_sub;
EngineSettingsSubsystem g_engine_sub;

// ---------------------------------------------------------------- komutlar

void ReportTransition(const ModeTransitionReport& r)
{
	if (!r.changed)
	{
		Print(std::string("[Lambda UI] Already in ") + (r.to == InterfaceMode::Gamepad ? "gamepad" : "classic") + " mode.");
		return;
	}
	if (r.to == InterfaceMode::Classic)
		Print("[Lambda UI] Classic mode is on. The gamepad interface, HUD, input layer, aim assist and controller effects are off. "
			  "Your gamepad profiles were kept. Type hlgamepad to switch back.");
	else
		Print("[Lambda UI] Gamepad mode is on. Type hlclassic to return to the original controls.");
	for (const auto& note : r.restart_notes)
		Print("[Lambda UI] " + note);
	LAMBDA_LOG_INFO("mode switched to %s", std::string(InterfaceModeId(r.to)).c_str());
}

void Cmd_Classic()
{
	ReportTransition(g.mode.SwitchTo(InterfaceMode::Classic));
	SaveState();
}

void Cmd_Gamepad()
{
	ReportTransition(g.mode.SwitchTo(InterfaceMode::Gamepad));
	SaveState();
}

void Cmd_Status()
{
	Print(std::string("[Lambda UI] ") + LAMBDA_PRODUCT_NAME + " " + LAMBDA_VERSION_STRING + " (" + LAMBDA_GIT_REVISION + ")");
	Print(std::string("  Mode: ") + (g.mode.Current() == InterfaceMode::Gamepad ? "gamepad" : "classic") + (g.safe_mode ? " (safe start)" : ""));
	Print("  Diagnostic log: " + log::Sanitize(log::Path()));
	if (!g.pads.Initialized())
	{
		Print("  Controller layer: off");
		return;
	}
	const auto active = g.pads.Active();
	if (g.pads.Devices().empty())
		Print("  No controller detected. The interface can be used with keyboard and mouse.");
	for (const auto& d : g.pads.Devices())
	{
		std::string line = "  " + std::string(active && *active == d.id ? "* " : "  ") + d.name + " [" + platform::PadFamilyName(d.family) + ", " +
						   platform::ConnectionName(d.connection) + "]";
		if (d.steam_virtual)
			line += " via Steam Input";
		if (d.battery_percent >= 0)
			line += " battery " + std::to_string(d.battery_percent) + "%";
		Print(line);
	}
	if (!active && !g.pads.Devices().empty())
		Print("  Press A / Cross or Start on the controller you want to use.");
	if (g.pads.SteamInputOverlapSuspected())
		Print("  Steam Input and a direct connection are both visible. Only the active controller is read; see docs/steam-input.md.");
}

void Cmd_Recover()
{
	if (gEngfuncs.Cmd_Argc() < 2 || std::strcmp(gEngfuncs.Cmd_Argv(1), "confirm") != 0)
	{
		Print("[Lambda UI] lambda_recover restores the default controls for this game.");
		Print("  Your current profile is kept as a backup. Type: lambda_recover confirm");
		return;
	}
	ResetGroup(g.settings, SettingsGroup::All, g.game);
	ApplySettingsToResolver();
	if (!g.safe_mode)
	{
		ProfileStore store(fileio::Join(g.data_dir, "profiles"));
		SettingsDocument doc;
		WriteControllerSettings(g.settings, doc);
		store.Save(g.loaded_profile_family.empty() ? "generic" : g.loaded_profile_family, doc);
	}
	Print("[Lambda UI] Default controls restored.");
	LAMBDA_LOG_INFO("controls restored to defaults by console command");
}

// ---------------------------------------------------------------- eylemler

bool ActionIs(Action a) { return g.resolver.IsActive(a); }

void HandleActionEvent(const ActionEvent& e)
{
	const ActionInfo& info = GetActionInfo(e.action);
	if (info.singleplayer_only && g.multiplayer)
		return;
	if (!e.begin)
	{
		switch (e.action)
		{
		case Action::Scoreboard: gEngfuncs.pfnClientCmd("-showscores"); break;
		case Action::PushToTalk: gEngfuncs.pfnClientCmd("-voicerecord"); break;
		default: break;
		}
		return;
	}
	switch (e.action)
	{
	case Action::Flashlight: gEngfuncs.pfnClientCmd("impulse 100"); break;
	case Action::Spray: gEngfuncs.pfnClientCmd("impulse 201"); break;
	case Action::NextWeapon: gEngfuncs.pfnClientCmd("invnext"); break;
	case Action::PreviousWeapon: gEngfuncs.pfnClientCmd("invprev"); break;
	case Action::LastWeapon: gEngfuncs.pfnClientCmd("lastinv"); break;
	case Action::QuickSave: gEngfuncs.pfnClientCmd("save quick"); break;
	case Action::QuickLoad: gEngfuncs.pfnClientCmd("load quick"); break;
	case Action::Scoreboard: gEngfuncs.pfnClientCmd("+showscores"); break;
	case Action::PushToTalk: gEngfuncs.pfnClientCmd("+voicerecord"); break;
	case Action::Chat: gEngfuncs.pfnClientCmd("messagemode"); break;
	case Action::TeamChat: gEngfuncs.pfnClientCmd("messagemode2"); break;
	case Action::Pause:
		if (VGui().baseui)
			VGui().baseui->ActivateGameUI();
		break;
	case Action::Console:
		if (VGui().baseui)
		{
			VGui().baseui->ActivateGameUI();
			VGui().baseui->ShowConsole();
		}
		break;
	default: break;
	}
}

void UpdateContexts()
{
	std::vector<BindContext> stack;
	const bool menu = VGui().gameui && VGui().gameui->IsGameUIActive();
	if (menu)
	{
		stack.push_back(BindContext::Menu);
	}
	else
	{
		if (g.waterlevel >= 2)
			stack.push_back(BindContext::Water);
		if (g.multiplayer)
			stack.push_back(BindContext::Multiplayer);
		stack.push_back(BindContext::Gameplay);
	}
	g.resolver.SetContexts(std::move(stack));
}

void ProcessDeviceEvents()
{
	for (const auto& e : g.device_events)
	{
		const auto* d = g.pads.Find(e.id);
		switch (e.type)
		{
		case platform::DeviceEventType::Added:
			LAMBDA_LOG_INFO("controller connected: %s (family %s, %s, vid %04x pid %04x, steam virtual %d)", d ? d->name.c_str() : "?",
				d ? platform::PadFamilyName(d->family) : "?", d ? platform::ConnectionName(d->connection) : "?", d ? d->vendor : 0,
				d ? d->product : 0, d ? d->steam_virtual : 0);
			break;
		case platform::DeviceEventType::Removed: LAMBDA_LOG_INFO("controller disconnected (id %u)", e.id); break;
		case platform::DeviceEventType::ActiveLost:
		{
			LAMBDA_LOG_WARN("active controller disconnected");
			std::vector<ActionEvent> released;
			g.resolver.ReleaseAll(released);
			for (const auto& r : released)
				HandleActionEvent(r);
			Print("[Lambda UI] Controller disconnected. Reconnect it, or continue with keyboard and mouse.");
			// Tek oyunculuda oyunu duraklat (menü açılır; çok oyunculuda dünya durdurulmaz).
			if (!g.multiplayer && VGui().baseui && !(VGui().gameui && VGui().gameui->IsGameUIActive()))
				VGui().baseui->ActivateGameUI();
			break;
		}
		case platform::DeviceEventType::ActiveChanged:
			if (d)
			{
				LAMBDA_LOG_INFO("active controller: %s", d->name.c_str());
				const bool deck = VGui().gameui && VGui().gameui->IsRunningOnSteamDeck();
				const std::string family = FamilyProfileName(platform::GlyphsFor(*d, deck));
				if (family != g.loaded_profile_family)
					LoadProfiles(family);
			}
			break;
		case platform::DeviceEventType::BatteryLow:
			if (d)
				Print("[Lambda UI] Controller battery is low (" + std::to_string(d->battery_percent) + "%).");
			break;
		}
	}
	g.device_events.clear();
}
} // namespace

bool GamepadLayerActive() { return g.initialized && g.input_enabled; }
bool OverlayEnabled() { return g.initialized && g.overlay_enabled; }
bool OverlayWantsInput() { return false; }

void OnInitialize()
{
	if (g.initialized)
		return;
	g.game = CurrentGame();
	const char* dir = gEngfuncs.pfnGetGameDirectory ? gEngfuncs.pfnGetGameDirectory() : nullptr;
	g.mod_dir = (dir && *dir) ? dir : LAMBDA_MOD_DIR;
	g.data_dir = fileio::Join(g.mod_dir, "lambda");
	log::Open(fileio::Join(g.data_dir, "logs"));
	LAMBDA_LOG_INFO("%s %s (%s) starting, game=%s, mod dir=%s", LAMBDA_PRODUCT_NAME, LAMBDA_VERSION_STRING, LAMBDA_GIT_REVISION,
		std::string(GameIdString(g.game)).c_str(), g.mod_dir.c_str());

	g.safe_mode = gEngfuncs.CheckParm("-lambdasafe", nullptr) != 0;
	if (g.safe_mode)
		LAMBDA_LOG_INFO("safe start requested with -lambdasafe");

	const InterfaceMode saved = g.safe_mode ? InterfaceMode::Gamepad : LoadStateMode();
	g.mode = ModeController(saved);
	g.mode.Register(&g_engine_sub);
	g.mode.Register(&g_interface_sub);
	g.mode.Register(&g_input_sub);
	LoadProfiles("generic");
	g.initialized = true;
	g.mode.Apply();
	LAMBDA_LOG_INFO("started in %s mode", std::string(InterfaceModeId(g.mode.Current())).c_str());
}

void OnHudInit()
{
	gEngfuncs.pfnAddCommand("hlclassic", Cmd_Classic);
	gEngfuncs.pfnAddCommand("hlgamepad", Cmd_Gamepad);
	gEngfuncs.pfnAddCommand("lambda_status", Cmd_Status);
	gEngfuncs.pfnAddCommand("lambda_recover", Cmd_Recover);
	if (g.safe_mode)
		Print("[Lambda UI] Safe start: default controls are active and saved profiles are not loaded or changed.");
}

void OnVidInit()
{
	gl::Load();
	LAMBDA_LOG_INFO("video init (OpenGL entry points %s)", gl::Loaded() ? "resolved" : "NOT resolved");
}

void OnFrame(double)
{
	if (!g.initialized)
		return;
	g.multiplayer = gEngfuncs.GetMaxClients() > 1;
	if (!g.input_enabled)
		return;

	g.pads.Pump(g.device_events);
	ProcessDeviceEvents();

	g.pad_valid = g.pads.ReadActive(g.pad);
	const ButtonSet buttons = g.pad_valid ? g.pad.buttons : ButtonSet{};
	const double now = Now();

	if (g.recovery.Update(buttons, now))
	{
		LAMBDA_LOG_INFO("recovery combination held");
		Print("[Lambda UI] Recovery: type lambda_recover confirm in the console, or use the recovery dialog, to restore default controls.");
	}

	UpdateContexts();
	g.events.clear();
	g.resolver.Update(buttons, now, g.events);
	for (const auto& e : g.events)
		HandleActionEvent(e);

	if (VGui().gameui)
	{
		const bool active = VGui().gameui->IsGameUIActive();
		if (active != g.gameui_active_logged)
		{
			g.gameui_active_logged = active;
			LAMBDA_LOG_DEBUG("GameUI active: %d", active);
		}
	}
}

bool OnRedraw(float, int) { return false; }

void OnPlayerMove(int waterlevel, int)
{
	g.waterlevel = waterlevel;
}

int ButtonBits()
{
	if (!GamepadLayerActive())
		return 0;
	int bits = 0;
	if (ActionIs(Action::PrimaryAttack))
		bits |= IN_ATTACK;
	if (ActionIs(Action::SecondaryAttack))
		bits |= IN_ATTACK2;
	if (ActionIs(Action::Jump))
		bits |= IN_JUMP;
	if (ActionIs(Action::Crouch))
		bits |= IN_DUCK;
	if (ActionIs(Action::Use))
		bits |= IN_USE;
	if (ActionIs(Action::Reload))
		bits |= IN_RELOAD;
	return bits;
}

void OnInMove(float frametime, usercmd_s* cmd)
{
	if (!GamepadLayerActive() || !g.pad_valid || !cmd)
		return;
	if (VGui().gameui && VGui().gameui->IsGameUIActive())
		return;

	const Vec2 move_raw = g.settings.movement.swap_sticks ? g.pad.right : g.pad.left;
	const Vec2 look_raw = g.settings.movement.swap_sticks ? g.pad.left : g.pad.right;
	Vec2 move = ProcessStick(move_raw, g.settings.left_stick);
	const Vec2 look = ProcessStick(look_raw, g.settings.right_stick);

	float viewangles[3];
	gEngfuncs.GetViewAngles(viewangles);
	const CameraDelta d = g.camera.Update(look, frametime, g.settings.camera);
	viewangles[YAW] += d.yaw;
	viewangles[PITCH] += d.pitch;
	if (g.settings.movement.movement_stick_turns)
	{
		viewangles[YAW] -= move.x * g.settings.movement.turn_speed * frametime;
		move.x = 0.0f;
	}
	const float pitch_up = gEngfuncs.pfnGetCvarFloat("cl_pitchup");
	const float pitch_down = gEngfuncs.pfnGetCvarFloat("cl_pitchdown");
	viewangles[PITCH] = std::fmax(-(pitch_up > 0 ? pitch_up : 89.0f), std::fmin(pitch_down > 0 ? pitch_down : 89.0f, viewangles[PITCH]));
	viewangles[YAW] = WrapDegrees(viewangles[YAW]);
	gEngfuncs.SetViewAngles(viewangles); // CL_CreateMove bu açıyı komuta kopyalar

	// Kademeli hareket: tam sapma = özgün azami hız (cl_forwardspeed/cl_sidespeed, sunucu
	// sv_maxspeed ile sınırlar). Yeni sprint yok.
	const float fwd_speed = gEngfuncs.pfnGetCvarFloat("cl_forwardspeed");
	const float back_speed = gEngfuncs.pfnGetCvarFloat("cl_backspeed");
	const float side_speed = gEngfuncs.pfnGetCvarFloat("cl_sidespeed");
	float forward = -move.y * (move.y <= 0 ? fwd_speed : back_speed);
	float side = move.x * side_speed;
	float up = 0.0f;
	const float up_speed = gEngfuncs.pfnGetCvarFloat("cl_upspeed");
	if (ActionIs(Action::SwimUp))
		up += up_speed;
	if (ActionIs(Action::SwimDown))
		up -= up_speed;
	if (ActionIs(Action::Walk))
	{
		const float k = gEngfuncs.pfnGetCvarFloat("cl_movespeedkey");
		forward *= k;
		side *= k;
		up *= k;
	}
	cmd->forwardmove += forward;
	cmd->sidemove += side;
	cmd->upmove += up;

	const float max_speed = gEngfuncs.GetClientMaxspeed();
	if (max_speed > 0.0f)
	{
		const float len = std::sqrt(cmd->forwardmove * cmd->forwardmove + cmd->sidemove * cmd->sidemove + cmd->upmove * cmd->upmove);
		if (len > max_speed)
		{
			const float k = max_speed / len;
			cmd->forwardmove *= k;
			cmd->sidemove *= k;
			cmd->upmove *= k;
		}
	}
}

int OnKeyEvent(int, int, const char*) { return 1; }

void OnShutdown()
{
	if (!g.initialized)
		return;
	LAMBDA_LOG_INFO("shutting down");
	g.pads.StopAllEffects();
	g.pads.Shutdown();
	SaveState();
	log::Close();
	g.initialized = false;
}

void ThinkOverlay()
{
	auto& v = VGui();
	if (!OverlayEnabled() || !v.panel || !v.root_panel)
		return;
	// GameUI kendini öne aldığında üstte kalmak için.
	v.panel->MoveToFront(v.root_panel);
}

void PaintOverlay(int w, int h)
{
	if (!gl::Loaded() && !gl::Load())
		return;
	// Prototip göstergesi: sağ üstte Lambda turuncusu şerit ve kontrolcü durumu.
	// Tam arayüz Aşama 7'de bu çizim yolunu kullanır.
	gl::Begin2D(w, h);
	const float s = static_cast<float>(h) / 1080.0f;
	const float bw = 220.0f * s, bh = 10.0f * s;
	const float x = static_cast<float>(w) - bw - 24.0f * s, y = 24.0f * s;
	gl::FillRectGradient(x, y, bw, bh, {255, 160, 30, 230}, {200, 90, 0, 230});
	const bool connected = g.pads.Initialized() && !g.pads.Devices().empty();
	const bool active = g.pads.Active().has_value();
	gl::Color dot = active ? gl::Color{120, 230, 120, 255} : (connected ? gl::Color{255, 210, 80, 255} : gl::Color{150, 150, 150, 255});
	gl::FillRect(x + bw + 6.0f * s, y, bh, bh, dot);
	gl::End2D();
}
} // namespace lambda::bridge

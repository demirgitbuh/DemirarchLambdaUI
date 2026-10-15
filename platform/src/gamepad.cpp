// SPDX-License-Identifier: MIT
#include "lambda/platform/gamepad.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>

namespace lambda::platform
{
namespace
{
constexpr std::uint16_t kValveVendor = 0x28DE;
constexpr std::uint16_t kSteamDeckProduct = 0x1205;
constexpr std::uint16_t kSteamVirtualProduct = 0x11FF;
constexpr std::uint16_t kSonyVendor = 0x054C;

PadFamily FromSdlType(SDL_GamepadType t)
{
	switch (t)
	{
	case SDL_GAMEPAD_TYPE_XBOX360: return PadFamily::Xbox360;
	case SDL_GAMEPAD_TYPE_XBOXONE: return PadFamily::XboxOne;
	case SDL_GAMEPAD_TYPE_PS3: return PadFamily::PS3;
	case SDL_GAMEPAD_TYPE_PS4: return PadFamily::PS4;
	case SDL_GAMEPAD_TYPE_PS5: return PadFamily::PS5;
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
	case SDL_GAMEPAD_TYPE_GAMECUBE: return PadFamily::Nintendo;
	case SDL_GAMEPAD_TYPE_STANDARD: return PadFamily::Generic;
	default: return PadFamily::Unknown;
	}
}

float AxisToFloat(Sint16 v)
{
	return v < 0 ? static_cast<float>(v) / 32768.0f : static_cast<float>(v) / 32767.0f;
}

std::uint16_t Scale16(float v) { return static_cast<std::uint16_t>(Clamp(v, 0.0f, 1.0f) * 65535.0f + 0.5f); }
} // namespace

struct GamepadManager::Open
{
	DeviceId id = 0;
	SDL_Gamepad* pad = nullptr;
	bool sensors_on = false;
	bool low_battery_reported = false;
};

const char* PadFamilyName(PadFamily f)
{
	switch (f)
	{
	case PadFamily::Xbox360: return "Xbox 360";
	case PadFamily::XboxOne: return "Xbox One / Series";
	case PadFamily::PS3: return "DualShock 3";
	case PadFamily::PS4: return "DualShock 4";
	case PadFamily::PS5: return "DualSense";
	case PadFamily::SteamDeck: return "Steam Deck";
	case PadFamily::SteamVirtual: return "Steam Input virtual controller";
	case PadFamily::Nintendo: return "Nintendo";
	case PadFamily::Generic: return "Generic controller";
	case PadFamily::Unknown: break;
	}
	return "Unknown controller";
}

const char* ConnectionName(Connection c)
{
	switch (c)
	{
	case Connection::Wired: return "Wired";
	case Connection::Wireless: return "Wireless";
	case Connection::Unknown: break;
	}
	return "Unknown";
}

GlyphFamily GlyphsFor(const DeviceInfo& info, bool running_on_steam_deck)
{
	PadFamily f = info.family;
	if (f == PadFamily::SteamVirtual)
	{
		if (running_on_steam_deck && info.reported_real_family == PadFamily::Unknown)
			return GlyphFamily::SteamDeck;
		f = info.reported_real_family;
	}
	switch (f)
	{
	case PadFamily::Xbox360:
	case PadFamily::XboxOne: return GlyphFamily::Xbox;
	case PadFamily::PS3:
	case PadFamily::PS4:
	case PadFamily::PS5: return GlyphFamily::PlayStation;
	case PadFamily::SteamDeck: return GlyphFamily::SteamDeck;
	default: return GlyphFamily::Generic;
	}
}

GamepadManager::~GamepadManager() { Shutdown(); }

bool GamepadManager::Init(std::string* error)
{
	if (initialized_)
		return true;
	// SDL3'ün kendi penceresi yok; odak motor penceresinde. Arka plan olaylarına izin verilir,
	// odak dışındayken girdiyi köprü katmanı yok sayar.
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC | SDL_INIT_SENSOR))
	{
		if (error)
			*error = SDL_GetError();
		return false;
	}
	initialized_ = true;
	std::vector<DeviceEvent> ignored;
	int count = 0;
	if (SDL_JoystickID* ids = SDL_GetGamepads(&count))
	{
		for (int i = 0; i < count; ++i)
			AddDevice(ids[i], ignored);
		SDL_free(ids);
	}
	return true;
}

void GamepadManager::Shutdown()
{
	if (!initialized_)
		return;
	StopAllEffects();
	for (Open* o : open_)
	{
		if (o->pad)
		{
			if (o->sensors_on)
			{
				SDL_SetGamepadSensorEnabled(o->pad, SDL_SENSOR_GYRO, false);
				SDL_SetGamepadSensorEnabled(o->pad, SDL_SENSOR_ACCEL, false);
			}
			SDL_CloseGamepad(o->pad);
		}
		delete o;
	}
	open_.clear();
	devices_.clear();
	active_.reset();
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC | SDL_INIT_SENSOR);
	initialized_ = false;
}

GamepadManager::Open* GamepadManager::FindOpen(DeviceId id)
{
	for (Open* o : open_)
		if (o->id == id)
			return o;
	return nullptr;
}

const DeviceInfo* GamepadManager::Find(DeviceId id) const
{
	for (const DeviceInfo& d : devices_)
		if (d.id == id)
			return &d;
	return nullptr;
}

void GamepadManager::RefreshInfo(DeviceInfo& info)
{
	Open* o = FindOpen(info.id);
	if (!o || !o->pad)
		return;
	SDL_Gamepad* pad = o->pad;
	const char* name = SDL_GetGamepadName(pad);
	info.name = name ? name : "";
	info.vendor = SDL_GetGamepadVendor(pad);
	info.product = SDL_GetGamepadProduct(pad);
	info.steam_input_handle = SDL_GetGamepadSteamHandle(pad) != 0;
	info.steam_virtual = info.vendor == kValveVendor && info.product == kSteamVirtualProduct;

	info.family = FromSdlType(SDL_GetGamepadType(pad));
	if (info.vendor == kValveVendor && info.product == kSteamDeckProduct)
		info.family = PadFamily::SteamDeck;
	if (info.steam_virtual)
	{
		// SDL, Steam'in yayımladığı bilgiden gerçek türü bildirebilir; bildirmezse bilinmiyor kalır.
		const PadFamily real = FromSdlType(SDL_GetRealGamepadType(pad));
		info.reported_real_family = (real == PadFamily::Generic) ? PadFamily::Unknown : real;
		info.family = PadFamily::SteamVirtual;
	}

	switch (SDL_GetGamepadConnectionState(pad))
	{
	case SDL_JOYSTICK_CONNECTION_WIRED: info.connection = Connection::Wired; break;
	case SDL_JOYSTICK_CONNECTION_WIRELESS: info.connection = Connection::Wireless; break;
	default: info.connection = Connection::Unknown; break;
	}

	const SDL_PropertiesID props = SDL_GetGamepadProperties(pad);
	info.caps.rumble = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
	info.caps.trigger_rumble = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_TRIGGER_RUMBLE_BOOLEAN, false);
	info.caps.rgb_led = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN, false);
	info.caps.player_led = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_PLAYER_LED_BOOLEAN, false);
	info.caps.gyro = SDL_GamepadHasSensor(pad, SDL_SENSOR_GYRO);
	info.caps.accel = SDL_GamepadHasSensor(pad, SDL_SENSOR_ACCEL);
	info.caps.touchpads = SDL_GetNumGamepadTouchpads(pad);
	info.caps.paddles = SDL_GamepadHasButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1) ||
						SDL_GamepadHasButton(pad, SDL_GAMEPAD_BUTTON_LEFT_PADDLE1);
	// DS5 efekt paketi yalnızca SDL'in doğrudan sürdüğü (HIDAPI) gerçek DualSense'te anlamlıdır;
	// Steam sanal aygıtında gönderilemez.
	info.caps.ds5_effects = !info.steam_virtual && info.family == PadFamily::PS5 && info.vendor == kSonyVendor;

	int percent = -1;
	switch (SDL_GetGamepadPowerInfo(pad, &percent))
	{
	case SDL_POWERSTATE_ON_BATTERY: info.power = PowerState::OnBattery; break;
	case SDL_POWERSTATE_NO_BATTERY: info.power = PowerState::Wired; break;
	case SDL_POWERSTATE_CHARGING: info.power = PowerState::Charging; break;
	case SDL_POWERSTATE_CHARGED: info.power = PowerState::Charged; break;
	default: info.power = PowerState::Unknown; break;
	}
	info.battery_percent = (info.power == PowerState::Unknown || info.power == PowerState::Wired) ? -1 : percent;
}

void GamepadManager::AddDevice(DeviceId id, std::vector<DeviceEvent>& events)
{
	if (FindOpen(id))
		return;
	SDL_Gamepad* pad = SDL_OpenGamepad(id);
	if (!pad)
		return;
	Open* o = new Open;
	o->id = id;
	o->pad = pad;
	open_.push_back(o);
	DeviceInfo info;
	info.id = id;
	devices_.push_back(info);
	RefreshInfo(devices_.back());
	events.push_back({DeviceEventType::Added, id});
}

void GamepadManager::RemoveDevice(DeviceId id, std::vector<DeviceEvent>& events)
{
	auto it = std::find_if(open_.begin(), open_.end(), [&](Open* o) { return o->id == id; });
	if (it != open_.end())
	{
		if ((*it)->pad)
			SDL_CloseGamepad((*it)->pad);
		delete *it;
		open_.erase(it);
	}
	devices_.erase(std::remove_if(devices_.begin(), devices_.end(), [&](const DeviceInfo& d) { return d.id == id; }), devices_.end());
	events.push_back({DeviceEventType::Removed, id});
	if (active_ && *active_ == id)
	{
		active_.reset();
		events.push_back({DeviceEventType::ActiveLost, id});
	}
}

void GamepadManager::Pump(std::vector<DeviceEvent>& events)
{
	if (!initialized_)
		return;
	SDL_UpdateGamepads();
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_EVENT_GAMEPAD_ADDED: AddDevice(e.gdevice.which, events); break;
		case SDL_EVENT_GAMEPAD_REMOVED: RemoveDevice(e.gdevice.which, events); break;
		case SDL_EVENT_GAMEPAD_REMAPPED:
		case SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED:
			for (DeviceInfo& d : devices_)
				if (d.id == e.gdevice.which)
					RefreshInfo(d);
			break;
		case SDL_EVENT_JOYSTICK_BATTERY_UPDATED:
			for (DeviceInfo& d : devices_)
			{
				if (d.id != e.jbattery.which)
					continue;
				RefreshInfo(d);
				Open* o = FindOpen(d.id);
				if (o && d.power == PowerState::OnBattery && d.battery_percent >= 0 && d.battery_percent <= 15)
				{
					if (!o->low_battery_reported)
						events.push_back({DeviceEventType::BatteryLow, d.id});
					o->low_battery_reported = true;
				}
				else if (o)
				{
					o->low_battery_reported = false;
				}
			}
			break;
		default: break;
		}
	}

	if (!active_ && auto_select_)
	{
		for (Open* o : open_)
		{
			if (SDL_GetGamepadButton(o->pad, SDL_GAMEPAD_BUTTON_SOUTH) || SDL_GetGamepadButton(o->pad, SDL_GAMEPAD_BUTTON_START))
			{
				active_ = o->id;
				events.push_back({DeviceEventType::ActiveChanged, o->id});
				break;
			}
		}
	}
}

void GamepadManager::SetActive(DeviceId id)
{
	if (FindOpen(id))
		active_ = id;
}

void GamepadManager::ClearActive() { active_.reset(); }

bool GamepadManager::ReadState(DeviceId id, PadState& out, float trigger_threshold)
{
	out = PadState{};
	Open* o = FindOpen(id);
	if (!o || !o->pad || !SDL_GamepadConnected(o->pad))
		return false;
	SDL_Gamepad* pad = o->pad;
	for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT && b < kPadButtonCount; ++b)
		out.buttons.set(static_cast<size_t>(b), SDL_GetGamepadButton(pad, static_cast<SDL_GamepadButton>(b)));
	out.left = {AxisToFloat(SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX)), AxisToFloat(SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY))};
	out.right = {AxisToFloat(SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTX)), AxisToFloat(SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTY))};
	out.left_trigger = Clamp(AxisToFloat(SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)), 0.0f, 1.0f);
	out.right_trigger = Clamp(AxisToFloat(SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)), 0.0f, 1.0f);
	out.buttons.set(static_cast<size_t>(PadButton::LeftTrigger), out.left_trigger >= trigger_threshold);
	out.buttons.set(static_cast<size_t>(PadButton::RightTrigger), out.right_trigger >= trigger_threshold);

	if (o->sensors_on)
	{
		float g[3];
		if (SDL_GetGamepadSensorData(pad, SDL_SENSOR_GYRO, g, 3))
		{
			out.gyro_valid = true;
			out.gyro = {g[0], g[1], g[2]};
		}
		float a[3];
		if (SDL_GetGamepadSensorData(pad, SDL_SENSOR_ACCEL, a, 3))
		{
			out.accel_valid = true;
			out.accel = {a[0], a[1], a[2]};
		}
	}

	if (SDL_GetNumGamepadTouchpads(pad) > 0)
	{
		const int fingers = std::min(2, SDL_GetNumGamepadTouchpadFingers(pad, 0));
		for (int f = 0; f < fingers; ++f)
		{
			bool down = false;
			float x = 0, y = 0, p = 0;
			if (SDL_GetGamepadTouchpadFinger(pad, 0, f, &down, &x, &y, &p))
				out.touch[f] = {down, x, y, p};
		}
	}
	return true;
}

bool GamepadManager::ReadActive(PadState& out, float trigger_threshold)
{
	if (!active_)
	{
		out = PadState{};
		return false;
	}
	return ReadState(*active_, out, trigger_threshold);
}

bool GamepadManager::Rumble(DeviceId id, float low, float high, std::uint32_t ms)
{
	Open* o = FindOpen(id);
	return o && SDL_RumbleGamepad(o->pad, Scale16(low), Scale16(high), ms);
}

bool GamepadManager::RumbleTriggers(DeviceId id, float left, float right, std::uint32_t ms)
{
	Open* o = FindOpen(id);
	return o && SDL_RumbleGamepadTriggers(o->pad, Scale16(left), Scale16(right), ms);
}

bool GamepadManager::SetLed(DeviceId id, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
	Open* o = FindOpen(id);
	return o && SDL_SetGamepadLED(o->pad, r, g, b);
}

bool GamepadManager::SendEffect(DeviceId id, const void* data, int size)
{
	Open* o = FindOpen(id);
	return o && SDL_SendGamepadEffect(o->pad, data, size);
}

bool GamepadManager::SetSensors(DeviceId id, bool enabled)
{
	Open* o = FindOpen(id);
	if (!o)
		return false;
	bool ok = SDL_SetGamepadSensorEnabled(o->pad, SDL_SENSOR_GYRO, enabled);
	SDL_SetGamepadSensorEnabled(o->pad, SDL_SENSOR_ACCEL, enabled);
	o->sensors_on = enabled && ok;
	return ok;
}

void GamepadManager::StopAllEffects()
{
	for (Open* o : open_)
	{
		if (!o->pad)
			continue;
		SDL_RumbleGamepad(o->pad, 0, 0, 0);
		SDL_RumbleGamepadTriggers(o->pad, 0, 0, 0);
	}
}

bool GamepadManager::SteamInputOverlapSuspected() const
{
	bool virt = false;
	bool physical = false;
	for (const DeviceInfo& d : devices_)
	{
		if (d.steam_virtual)
			virt = true;
		else
			physical = true;
	}
	return virt && physical;
}
} // namespace lambda::platform

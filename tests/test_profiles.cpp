// SPDX-License-Identifier: MIT
#include "lambda_test.h"

#include "lambda/core/fileio.h"
#include "lambda/core/mode.h"
#include "lambda/core/profile_store.h"
#include "lambda/core/settings.h"

#include <cstdlib>
#include <ctime>

using namespace lambda;

namespace
{
std::string TempDir(const char* tag)
{
#ifdef _WIN32
	const char* base = std::getenv("TEMP");
#else
	const char* base = std::getenv("TMPDIR");
	if (!base)
		base = "/tmp";
#endif
	static int counter = 0;
	std::string dir = std::string(base ? base : ".") + "/lambda_test_" + tag + "_" + std::to_string(std::time(nullptr)) + "_" +
					  std::to_string(++counter);
	fileio::MakeDirectories(dir);
	return dir;
}
} // namespace

TEST_CASE("document: parse, typed access and serialize round trip")
{
	auto doc = SettingsDocument::Parse("# comment\n[meta]\nformat = 1\n[stick.left]\ninner_deadzone = 0.15\ninvert_y = yes\nname = Hello World \n");
	CHECK(doc.has_value());
	CHECK_NEAR(doc->GetFloat("stick.left", "inner_deadzone", 0), 0.15f, 1e-6);
	CHECK(doc->GetBool("stick.left", "invert_y", false));
	CHECK(doc->GetString("stick.left", "name", "") == "Hello World");
	CHECK(doc->GetInt("meta", "format", 0) == 1);
	CHECK_NEAR(doc->GetFloat("stick.left", "missing", 7.0f), 7.0f, 1e-6);
	auto again = SettingsDocument::Parse(doc->Serialize());
	CHECK(again && again->Serialize() == doc->Serialize());
}

TEST_CASE("document: locale-independent numbers and bad values fall back")
{
	CHECK_NEAR(*ParseFloatInvariant("0.25"), 0.25f, 1e-7);
	CHECK_NEAR(*ParseFloatInvariant("-1.5e2"), -150.0f, 1e-4);
	CHECK(!ParseFloatInvariant("0,25"));
	CHECK(!ParseFloatInvariant("abc"));
	CHECK(!ParseFloatInvariant("1e999"));
	CHECK(FormatFloatInvariant(0.5f) == "0.5");
	auto doc = SettingsDocument::Parse("[a]\nx = nan\ny = 12abc\n");
	CHECK(doc.has_value());
	CHECK_NEAR(doc->GetFloat("a", "x", 3.0f), 3.0f, 1e-6);
	CHECK(doc->GetInt("a", "y", 4) == 4);
}

TEST_CASE("document: garbage and binary data are rejected")
{
	CHECK(!SettingsDocument::Parse("this is not\na profile\nat all\n"));
	CHECK(!SettingsDocument::Parse(std::string("[a]\nx=1\n\0\0\0", 11)));
	// Tek bir bozuk satır tolere edilir ve raporlanır
	std::vector<ParseIssue> issues;
	auto doc = SettingsDocument::Parse("[a]\nx = 1\ny = 2\nbroken line\n", &issues);
	CHECK(doc.has_value());
	CHECK(issues.size() == 1);
}

TEST_CASE("settings: write/read round trip preserves every group")
{
	ControllerSettings s = ControllerSettings::Defaults(GameId::OpposingForce);
	s.left_stick.inner_deadzone = 0.2f;
	s.right_stick.invert_y = true;
	s.camera.yaw_speed = 300.0f;
	s.movement.swap_sticks = true;
	s.timing.hold_time = 0.5f;
	s.toggle[static_cast<int>(Action::Crouch)] = true;
	Binding b;
	b.context = BindContext::Gameplay;
	b.input = PadButton::RightPaddle1;
	b.action = Action::Flashlight;
	s.bindings.Set(b);

	SettingsDocument doc;
	WriteControllerSettings(s, doc);
	const ControllerSettings r = ReadControllerSettings(doc, GameId::OpposingForce);
	CHECK_NEAR(r.left_stick.inner_deadzone, 0.2f, 1e-6);
	CHECK(r.right_stick.invert_y);
	CHECK_NEAR(r.camera.yaw_speed, 300.0f, 1e-3);
	CHECK(r.movement.swap_sticks);
	CHECK_NEAR(r.timing.hold_time, 0.5f, 1e-6);
	CHECK(r.toggle[static_cast<int>(Action::Crouch)]);
	CHECK(r.bindings.All().size() == s.bindings.All().size());
	auto paddle = r.bindings.FindSlot(BindContext::Gameplay, Trigger::Press, PadButton::RightPaddle1, std::nullopt);
	CHECK(paddle && paddle->action == Action::Flashlight);
}

TEST_CASE("settings: unusable saved layout falls back to defaults without erasing it")
{
	SettingsDocument doc;
	doc.Set("bindings", "gameplay.press.south", "jump"); // menüde kabul/geri yok, duraklatma yok
	std::vector<std::string> warnings;
	const ControllerSettings r = ReadControllerSettings(doc, GameId::HalfLife, &warnings);
	CHECK(!warnings.empty());
	CHECK(r.bindings.Validate().empty());
	CHECK(doc.Get("bindings", "gameplay.press.south").has_value()); // belge değiştirilmedi
}

TEST_CASE("settings: group reset only touches the selected group")
{
	ControllerSettings s = ControllerSettings::Defaults(GameId::HalfLife);
	s.camera.yaw_speed = 999.0f;
	s.left_stick.inner_deadzone = 0.5f;
	ResetGroup(s, SettingsGroup::Camera, GameId::HalfLife);
	CHECK_NEAR(s.camera.yaw_speed, CameraSettings{}.yaw_speed, 1e-4);
	CHECK_NEAR(s.left_stick.inner_deadzone, 0.5f, 1e-6);
}

TEST_CASE("profile store: save, backups and corrupt file recovery never delete user data")
{
	ProfileStore store(TempDir("profiles"));
	SettingsDocument a;
	a.SetInt("test", "value", 1);
	CHECK(store.Save("hl_xbox", a));
	a.SetInt("test", "value", 2);
	CHECK(store.Save("hl_xbox", a));
	a.SetInt("test", "value", 3);
	CHECK(store.Save("hl_xbox", a));

	auto r = store.Load("hl_xbox");
	CHECK(r.source == LoadSource::Primary);
	CHECK(r.document.GetInt("test", "value", 0) == 3);
	CHECK(fileio::Exists(store.PathFor("hl_xbox") + ".bak1"));
	CHECK(fileio::Exists(store.PathFor("hl_xbox") + ".bak2"));

	// Ana dosyayı boz
	CHECK(fileio::WriteFileAtomic(store.PathFor("hl_xbox"), std::string("\0\0garbage\0", 10)));
	r = store.Load("hl_xbox");
	CHECK(r.source == LoadSource::Backup);
	CHECK(r.backup_index == 1);
	CHECK(r.document.GetInt("test", "value", 0) == 2);
	CHECK(!r.quarantined_path.empty());
	CHECK(fileio::Exists(r.quarantined_path)); // bozuk dosya korunur
	CHECK(!r.message.empty());
}

TEST_CASE("profile store: missing file, invalid names and isolation between games")
{
	ProfileStore store(TempDir("iso"));
	CHECK(store.Load("nothing").source == LoadSource::NotFound);
	CHECK(store.Load("../evil").source == LoadSource::Failed);
	CHECK(!store.Save("a/b", SettingsDocument{}));

	SettingsDocument hl, op4;
	hl.SetFloat("camera", "yaw_speed", 111.0f);
	op4.SetFloat("camera", "yaw_speed", 222.0f);
	CHECK(store.Save("hl_default", hl));
	CHECK(store.Save("opfor_default", op4));
	CHECK_NEAR(store.Load("hl_default").document.GetFloat("camera", "yaw_speed", 0), 111.0f, 1e-3);
	CHECK_NEAR(store.Load("opfor_default").document.GetFloat("camera", "yaw_speed", 0), 222.0f, 1e-3);
	const auto names = store.List();
	CHECK(names.size() == 2);
}

TEST_CASE("profile store: export and import validation")
{
	const std::string dir = TempDir("export");
	ProfileStore store(dir);
	SettingsDocument d;
	d.SetInt("x", "y", 5);
	CHECK(store.Save("p", d));
	const std::string out = fileio::Join(dir, "exported.txt");
	CHECK(store.Export("p", out));
	std::string err;
	auto imported = ProfileStore::ReadImport(out, &err);
	CHECK(imported && imported->GetInt("x", "y", 0) == 5);

	CHECK(fileio::WriteFileAtomic(fileio::Join(dir, "foreign.txt"), "[meta]\nformat = 1\nkind = something_else\n"));
	CHECK(!ProfileStore::ReadImport(fileio::Join(dir, "foreign.txt"), &err));
	CHECK(!err.empty());
	CHECK(fileio::WriteFileAtomic(fileio::Join(dir, "future.txt"), "[meta]\nformat = 99\nkind = profile\n"));
	CHECK(!ProfileStore::ReadImport(fileio::Join(dir, "future.txt"), &err));
}

namespace
{
struct FakeSubsystem : ModeSubsystem
{
	const char* name;
	std::vector<std::string>* log;
	bool enabled = false;
	bool needs_restart = false;
	FakeSubsystem(const char* n, std::vector<std::string>* l) : name(n), log(l) {}
	const char* Name() const override { return name; }
	void EnterGamepad() override
	{
		enabled = true;
		log->push_back(std::string("+") + name);
	}
	void EnterClassic() override
	{
		enabled = false;
		log->push_back(std::string("-") + name);
	}
	std::optional<std::string> RestartRequirement(InterfaceMode) const override
	{
		if (needs_restart)
			return std::string(name) + " needs a restart.";
		return std::nullopt;
	}
};
} // namespace

TEST_CASE("mode: classic disables every subsystem in reverse order and gamepad restores them")
{
	std::vector<std::string> log;
	FakeSubsystem input("input", &log), aim("aim", &log), ui("ui", &log);
	ui.needs_restart = true;
	ModeController m(InterfaceMode::Gamepad);
	m.Register(&ui);
	m.Register(&aim);
	m.Register(&input);
	m.Apply();
	CHECK(input.enabled && aim.enabled && ui.enabled);

	log.clear();
	auto r = m.SwitchTo(InterfaceMode::Classic);
	CHECK(r.changed);
	CHECK(!input.enabled && !aim.enabled && !ui.enabled);
	CHECK(log.size() == 3 && log[0] == "-input" && log[2] == "-ui");
	CHECK(r.restart_notes.size() == 1);

	auto same = m.SwitchTo(InterfaceMode::Classic);
	CHECK(!same.changed);

	log.clear();
	m.SwitchTo(InterfaceMode::Gamepad);
	CHECK(input.enabled && aim.enabled && ui.enabled);
	CHECK(log[0] == "+ui");
}

TEST_CASE("mode: setting journal records first original value, persists and restores")
{
	SettingJournal j;
	j.RecordOriginal("crosshair", "1");
	j.RecordOriginal("crosshair", "0"); // ikinci kayıt özgün değeri ezmez
	j.RecordOriginal("hud_draw", "1");
	SettingsDocument doc;
	j.WriteTo(doc);
	SettingJournal loaded;
	loaded.ReadFrom(doc);
	CHECK(loaded.Has("crosshair"));
	std::vector<std::pair<std::string, std::string>> applied;
	loaded.RestoreAll([&](const std::string& k, const std::string& v) { applied.emplace_back(k, v); });
	CHECK(applied.size() == 2);
	bool ok = false;
	for (auto& [k, v] : applied)
		ok |= k == "crosshair" && v == "1";
	CHECK(ok);
	CHECK(loaded.Empty());
	CHECK(InterfaceModeFromId("classic") == InterfaceMode::Classic);
	CHECK(!InterfaceModeFromId("-hlclassic"));
}

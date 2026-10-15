// SPDX-License-Identifier: MIT
#include "lambda_test.h"

#include "lambda/core/recovery.h"
#include "lambda/core/resolver.h"

#include <algorithm>

using namespace lambda;

namespace
{
ButtonSet Pressed(std::initializer_list<PadButton> buttons)
{
	ButtonSet s;
	for (PadButton b : buttons)
		s.set(static_cast<size_t>(b));
	return s;
}

bool Has(const std::vector<ActionEvent>& ev, Action a, bool begin)
{
	return std::find(ev.begin(), ev.end(), ActionEvent{a, begin}) != ev.end();
}

Binding Make(BindContext c, Trigger t, PadButton in, Action a, std::optional<PadButton> mod = std::nullopt)
{
	Binding b;
	b.context = c;
	b.trigger = t;
	b.input = in;
	b.action = a;
	b.modifier = mod;
	return b;
}
} // namespace

TEST_CASE("bindings: default layout matches the specification and has no issues")
{
	for (GameId g : {GameId::HalfLife, GameId::OpposingForce, GameId::BlueShift})
	{
		const BindingSet s = DefaultBindings(g);
		CHECK(s.Validate().empty());
		auto is = [&](PadButton b, Action a) {
			auto f = s.FindSlot(BindContext::Gameplay, Trigger::Press, b, std::nullopt);
			return f && f->action == a;
		};
		CHECK(is(PadButton::RightTrigger, Action::PrimaryAttack));
		CHECK(is(PadButton::LeftTrigger, Action::SecondaryAttack));
		CHECK(is(PadButton::South, Action::Jump));
		CHECK(is(PadButton::East, Action::Crouch));
		CHECK(is(PadButton::West, Action::Use));
		// Yeniden doldurma X/kare üzerine yığılmaz
		CHECK(!is(PadButton::West, Action::Reload));
		CHECK(s.FindByAction(Action::Reload).size() == 1);
		// Varsayılan düzen yalnızca tek basış kullanır
		for (const Binding& b : s.All())
		{
			CHECK(b.trigger == Trigger::Press);
			CHECK(!b.modifier);
		}
		// Listelenen tüm eylemlerin bir varsayılan ataması vardır
		for (Action a : {Action::Reload, Action::WeaponWheel, Action::Flashlight, Action::Pause, Action::Scoreboard, Action::Console,
				 Action::Chat, Action::TeamChat, Action::PushToTalk, Action::QuickSave, Action::QuickLoad, Action::AimSnap, Action::AimLock,
				 Action::LongJump, Action::SwimUp, Action::SwimDown, Action::MessageWheel})
			CHECK(!s.FindByAction(a).empty());
	}
}

TEST_CASE("bindings: validation finds conflicts, wrong context, missing essentials and reserved buttons")
{
	BindingSet s = DefaultBindings(GameId::HalfLife);
	s.Remove(Make(BindContext::Menu, Trigger::Press, PadButton::East, Action::None));
	bool missing_back = false;
	for (const auto& i : s.Validate())
		missing_back |= i.kind == BindingIssueKind::MissingEssential && i.binding.action == Action::MenuBack;
	CHECK(missing_back);
	CHECK(s.HasBlockingIssues());

	BindingSet w = DefaultBindings(GameId::HalfLife);
	w.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::Misc1, Action::MenuAccept));
	w.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::Guide, Action::Reload));
	w.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::South, Action::Use, PadButton::South));
	int wrong = 0, reserved = 0, self_mod = 0;
	for (const auto& i : w.Validate())
	{
		wrong += i.kind == BindingIssueKind::WrongContext;
		reserved += i.kind == BindingIssueKind::ReservedButton;
		self_mod += i.kind == BindingIssueKind::ModifierIsInput;
	}
	CHECK(wrong == 1);
	CHECK(reserved == 1);
	CHECK(self_mod == 1);
}

TEST_CASE("bindings: Set replaces the same slot instead of creating a conflict")
{
	BindingSet s;
	auto old = s.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::North, Action::Reload));
	CHECK(!old);
	old = s.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::North, Action::WeaponWheel));
	CHECK(old && old->action == Action::Reload);
	CHECK(s.All().size() == 1);
}

TEST_CASE("bindings: key/value round trip including modifiers")
{
	const Binding b = Make(BindContext::Multiplayer, Trigger::DoublePress, PadButton::DpadUp, Action::Spray, PadButton::LeftShoulder);
	const std::string key = BindingSet::SlotKey(b);
	CHECK(key == "multiplayer.double.left_shoulder+dpad_up");
	auto back = BindingSet::FromKeyValue(key, "spray");
	CHECK(back && *back == b);
	CHECK(!BindingSet::FromKeyValue("gameplay.press.not_a_button", "jump"));
	CHECK(!BindingSet::FromKeyValue("gameplay.press.south", "not_an_action"));
	CHECK(!BindingSet::FromKeyValue("garbage", "jump"));
}

TEST_CASE("resolver: single press fires immediately and releases")
{
	BindingResolver r;
	r.SetBindings(DefaultBindings(GameId::HalfLife));
	r.SetContexts({BindContext::Gameplay});
	std::vector<ActionEvent> ev;
	r.Update(Pressed({PadButton::South}), 0.0, ev);
	CHECK(Has(ev, Action::Jump, true));
	CHECK(r.IsActive(Action::Jump));
	ev.clear();
	r.Update(Pressed({}), 0.05, ev);
	CHECK(Has(ev, Action::Jump, false));
	CHECK(!r.IsActive(Action::Jump));
}

TEST_CASE("resolver: context stack shadows gameplay bindings underwater and in multiplayer")
{
	BindingResolver r;
	r.SetBindings(DefaultBindings(GameId::HalfLife));
	r.SetContexts({BindContext::Water, BindContext::Gameplay});
	std::vector<ActionEvent> ev;
	r.Update(Pressed({PadButton::South}), 0.0, ev);
	CHECK(Has(ev, Action::SwimUp, true));
	CHECK(!Has(ev, Action::Jump, true));
	r.Update(Pressed({}), 0.1, ev);

	r.SetContexts({BindContext::Multiplayer, BindContext::Gameplay});
	ev.clear();
	r.Update(Pressed({PadButton::DpadLeft}), 0.2, ev);
	CHECK(Has(ev, Action::MessageWheel, true));
	CHECK(!Has(ev, Action::QuickSave, true));
	ev.clear();
	r.Update(Pressed({PadButton::DpadLeft, PadButton::RightTrigger}), 0.3, ev);
	CHECK(Has(ev, Action::PrimaryAttack, true)); // gölgelenmeyen atamalar çalışır
}

TEST_CASE("resolver: action begun in one context is released after a context switch")
{
	BindingResolver r;
	r.SetBindings(DefaultBindings(GameId::HalfLife));
	r.SetContexts({BindContext::Gameplay});
	std::vector<ActionEvent> ev;
	r.Update(Pressed({PadButton::South}), 0.0, ev);
	r.SetContexts({BindContext::Water, BindContext::Gameplay});
	ev.clear();
	r.Update(Pressed({}), 0.1, ev);
	CHECK(Has(ev, Action::Jump, false));
	CHECK(!r.IsActive(Action::Jump));
}

TEST_CASE("resolver: hold binding and tap on the same button")
{
	BindingSet s;
	s.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::North, Action::LastWeapon));
	s.Set(Make(BindContext::Gameplay, Trigger::Hold, PadButton::North, Action::WeaponWheel));
	BindingResolver r;
	r.SetBindings(s);
	r.SetContexts({BindContext::Gameplay});
	ResolverTiming t;
	t.hold_time = 0.3f;
	r.SetTiming(t);
	std::vector<ActionEvent> ev;

	// Kısa dokunma -> Press, bir sonraki karede bırakılır
	r.Update(Pressed({PadButton::North}), 0.0, ev);
	CHECK(ev.empty());
	r.Update(Pressed({}), 0.1, ev);
	CHECK(Has(ev, Action::LastWeapon, true));
	ev.clear();
	r.Update(Pressed({}), 0.12, ev);
	CHECK(Has(ev, Action::LastWeapon, false));

	// Uzun basış -> Hold, Press tetiklenmez
	ev.clear();
	r.Update(Pressed({PadButton::North}), 1.0, ev);
	r.Update(Pressed({PadButton::North}), 1.2, ev);
	CHECK(ev.empty());
	r.Update(Pressed({PadButton::North}), 1.31, ev);
	CHECK(Has(ev, Action::WeaponWheel, true));
	ev.clear();
	r.Update(Pressed({}), 1.5, ev);
	CHECK(Has(ev, Action::WeaponWheel, false));
	CHECK(!Has(ev, Action::LastWeapon, true));
}

TEST_CASE("resolver: double press timing window")
{
	BindingSet s;
	s.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::DpadUp, Action::Flashlight));
	s.Set(Make(BindContext::Gameplay, Trigger::DoublePress, PadButton::DpadUp, Action::Spray));
	BindingResolver r;
	r.SetBindings(s);
	r.SetContexts({BindContext::Gameplay});
	ResolverTiming t;
	t.double_press_window = 0.25f;
	r.SetTiming(t);
	std::vector<ActionEvent> ev;

	r.Update(Pressed({PadButton::DpadUp}), 0.0, ev);
	r.Update(Pressed({}), 0.05, ev);
	r.Update(Pressed({PadButton::DpadUp}), 0.15, ev);
	CHECK(Has(ev, Action::Spray, true));
	CHECK(!Has(ev, Action::Flashlight, true));
	r.Update(Pressed({}), 0.2, ev);

	ev.clear();
	r.Update(Pressed({PadButton::DpadUp}), 1.0, ev);
	r.Update(Pressed({}), 1.05, ev);
	CHECK(ev.empty());
	r.Update(Pressed({}), 1.2, ev);
	CHECK(ev.empty()); // pencere henüz dolmadı
	r.Update(Pressed({}), 1.31, ev);
	CHECK(Has(ev, Action::Flashlight, true));
	CHECK(!Has(ev, Action::Spray, true));
}

TEST_CASE("resolver: modifier combination suppresses the modifier's own action")
{
	BindingSet s;
	s.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::LeftShoulder, Action::AimSnap));
	s.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::South, Action::Jump));
	s.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::South, Action::LongJump, PadButton::LeftShoulder));
	BindingResolver r;
	r.SetBindings(s);
	r.SetContexts({BindContext::Gameplay});
	std::vector<ActionEvent> ev;

	r.Update(Pressed({PadButton::LeftShoulder}), 0.0, ev);
	CHECK(ev.empty()); // değiştirici: kendi eylemi ertelenir
	r.Update(Pressed({PadButton::LeftShoulder, PadButton::South}), 0.1, ev);
	CHECK(Has(ev, Action::LongJump, true));
	CHECK(!Has(ev, Action::Jump, true));
	ev.clear();
	r.Update(Pressed({}), 0.2, ev);
	CHECK(Has(ev, Action::LongJump, false));
	CHECK(!Has(ev, Action::AimSnap, true));

	// Kombinasyon kullanılmadan bırakılırsa değiştiricinin kendi eylemi çalışır
	ev.clear();
	r.Update(Pressed({PadButton::LeftShoulder}), 1.0, ev);
	r.Update(Pressed({}), 1.1, ev);
	CHECK(Has(ev, Action::AimSnap, true));
}

TEST_CASE("resolver: toggle mode for crouch")
{
	BindingResolver r;
	r.SetBindings(DefaultBindings(GameId::HalfLife));
	r.SetContexts({BindContext::Gameplay});
	r.SetToggle(Action::Crouch, true);
	std::vector<ActionEvent> ev;
	r.Update(Pressed({PadButton::East}), 0.0, ev);
	r.Update(Pressed({}), 0.1, ev);
	CHECK(r.IsActive(Action::Crouch));
	CHECK(Has(ev, Action::Crouch, true));
	CHECK(!Has(ev, Action::Crouch, false));
	ev.clear();
	r.Update(Pressed({PadButton::East}), 0.5, ev);
	CHECK(Has(ev, Action::Crouch, false));
	CHECK(!r.IsActive(Action::Crouch));
}

TEST_CASE("resolver: ReleaseAll leaves no stuck input after disconnect")
{
	BindingResolver r;
	r.SetBindings(DefaultBindings(GameId::HalfLife));
	r.SetContexts({BindContext::Gameplay});
	r.SetToggle(Action::Crouch, true);
	std::vector<ActionEvent> ev;
	r.Update(Pressed({PadButton::RightTrigger, PadButton::South, PadButton::East}), 0.0, ev);
	CHECK(r.IsActive(Action::PrimaryAttack));
	CHECK(r.IsActive(Action::Crouch));
	ev.clear();
	r.ReleaseAll(ev);
	CHECK(Has(ev, Action::PrimaryAttack, false));
	CHECK(Has(ev, Action::Jump, false));
	CHECK(Has(ev, Action::Crouch, false));
	for (int i = 0; i < kActionCount; ++i)
		CHECK(!r.IsActive(static_cast<Action>(i)));
	// Kopma anında basılı kalan tuş, yeniden basılana kadar eylem üretmez
	ev.clear();
	r.Update(Pressed({PadButton::RightTrigger}), 0.1, ev);
	CHECK(ev.empty());
	r.Update(Pressed({}), 0.2, ev);
	CHECK(ev.empty());
	r.Update(Pressed({PadButton::RightTrigger}), 0.3, ev);
	CHECK(Has(ev, Action::PrimaryAttack, true));
}

TEST_CASE("resolver: auto repeat toggles output while held and stops on release")
{
	BindingResolver r;
	r.SetBindings(DefaultBindings(GameId::HalfLife));
	r.SetContexts({BindContext::Gameplay});
	r.SetAutoRepeat(Action::PrimaryAttack, 10.0f);
	std::vector<ActionEvent> ev;
	r.Update(Pressed({PadButton::RightTrigger}), 0.0, ev);
	int begins = 0, ends = 0;
	for (int i = 1; i <= 20; ++i)
		r.Update(Pressed({PadButton::RightTrigger}), i * 0.025, ev);
	for (const auto& e : ev)
		(e.action == Action::PrimaryAttack && e.begin) ? ++begins : (e.action == Action::PrimaryAttack ? ++ends : 0);
	CHECK(begins >= 5); // 0.5 sn'de ~5 atış
	ev.clear();
	r.Update(Pressed({}), 0.6, ev);
	r.ClearAutoRepeat();
	CHECK(!r.IsActive(Action::PrimaryAttack));
}

TEST_CASE("resolver: menu context only produces menu actions")
{
	BindingResolver r;
	r.SetBindings(DefaultBindings(GameId::HalfLife));
	r.SetContexts({BindContext::Menu});
	std::vector<ActionEvent> ev;
	r.Update(Pressed({PadButton::South, PadButton::RightTrigger}), 0.0, ev);
	CHECK(Has(ev, Action::MenuAccept, true));
	CHECK(Has(ev, Action::MenuPageNext, true));
	CHECK(!Has(ev, Action::Jump, true));
	CHECK(!Has(ev, Action::PrimaryAttack, true));
}

TEST_CASE("resolver: effective binding lookup for button hints follows rebinding")
{
	BindingSet s = DefaultBindings(GameId::HalfLife);
	BindingResolver r;
	r.SetBindings(s);
	r.SetContexts({BindContext::Gameplay});
	auto b = r.EffectiveBindingFor(Action::Reload);
	CHECK(b && b->input == PadButton::RightShoulder);
	s.Remove(*b);
	s.Set(Make(BindContext::Gameplay, Trigger::Press, PadButton::DpadUp, Action::Reload));
	r.SetBindings(s);
	b = r.EffectiveBindingFor(Action::Reload);
	CHECK(b && b->input == PadButton::DpadUp);
}

TEST_CASE("recovery: Start+Back must be held for three seconds")
{
	RecoveryCombo c;
	const ButtonSet both = Pressed({PadButton::Start, PadButton::Back});
	CHECK(!c.Update(both, 0.0));
	CHECK(!c.Update(both, 2.9));
	CHECK_NEAR(c.Progress(1.5), 0.5f, 1e-4);
	CHECK(c.Update(both, 3.0));
	CHECK(!c.Update(both, 3.5)); // bir kez
	CHECK(!c.Update(Pressed({PadButton::Start}), 3.6));
	CHECK(!c.Update(both, 4.0));
	CHECK(c.Update(both, 7.1));
}

// SPDX-License-Identifier: MIT
#include "lambda/core/bindings.h"

#include <algorithm>
#include <array>

namespace lambda
{
namespace
{
constexpr std::array<std::string_view, 3> kGameIds = {"hl", "opfor", "bshift"};
constexpr std::array<std::string_view, static_cast<int>(BindContext::Count)> kContextIds = {"gameplay", "water", "multiplayer", "menu"};
constexpr std::array<std::string_view, static_cast<int>(BindContext::Count)> kContextNames = {"Gameplay", "Underwater", "Multiplayer", "Menus"};
constexpr std::array<std::string_view, static_cast<int>(Trigger::Count)> kTriggerIds = {"press", "hold", "double"};
constexpr std::array<std::string_view, static_cast<int>(Trigger::Count)> kTriggerNames = {"Press", "Hold", "Double Press"};

// Menü bağlamında menü eylemleri dışında yalnızca bu sistem eylemlerine izin verilir.
bool AllowedInMenu(Action a) { return GetActionInfo(a).menu_action || a == Action::Pause || a == Action::None; }

bool AllowedInGameplay(Action a) { return !GetActionInfo(a).menu_action; }

std::string Describe(const Binding& b)
{
	std::string s(BindContextDisplayName(b.context));
	s += ": ";
	if (b.modifier)
	{
		s += PadButtonId(*b.modifier);
		s += " + ";
	}
	s += PadButtonId(b.input);
	s += " (";
	s += TriggerDisplayName(b.trigger);
	s += ")";
	return s;
}

void Add(BindingSet& set, BindContext c, PadButton input, Action a, Trigger t = Trigger::Press)
{
	Binding b;
	b.context = c;
	b.trigger = t;
	b.input = input;
	b.action = a;
	set.Set(b);
}

void AddMenuDefaults(BindingSet& s)
{
	using P = PadButton;
	const auto M = BindContext::Menu;
	Add(s, M, P::South, Action::MenuAccept);
	Add(s, M, P::East, Action::MenuBack);
	Add(s, M, P::West, Action::MenuAction1);
	Add(s, M, P::North, Action::MenuAction2);
	Add(s, M, P::LeftShoulder, Action::MenuTabPrevious);
	Add(s, M, P::RightShoulder, Action::MenuTabNext);
	Add(s, M, P::LeftTrigger, Action::MenuPagePrevious);
	Add(s, M, P::RightTrigger, Action::MenuPageNext);
	Add(s, M, P::DpadUp, Action::MenuUp);
	Add(s, M, P::DpadDown, Action::MenuDown);
	Add(s, M, P::DpadLeft, Action::MenuLeft);
	Add(s, M, P::DpadRight, Action::MenuRight);
	Add(s, M, P::Start, Action::Pause);
}
} // namespace

std::string_view GameIdString(GameId g) { return kGameIds[static_cast<int>(g)]; }

std::optional<GameId> GameIdFromString(std::string_view s)
{
	for (int i = 0; i < static_cast<int>(kGameIds.size()); ++i)
		if (kGameIds[i] == s)
			return static_cast<GameId>(i);
	return std::nullopt;
}

std::string_view BindContextId(BindContext c) { return kContextIds[static_cast<int>(c)]; }
std::string_view BindContextDisplayName(BindContext c) { return kContextNames[static_cast<int>(c)]; }

std::optional<BindContext> BindContextFromId(std::string_view id)
{
	for (int i = 0; i < static_cast<int>(kContextIds.size()); ++i)
		if (kContextIds[i] == id)
			return static_cast<BindContext>(i);
	return std::nullopt;
}

std::string_view TriggerId(Trigger t) { return kTriggerIds[static_cast<int>(t)]; }
std::string_view TriggerDisplayName(Trigger t) { return kTriggerNames[static_cast<int>(t)]; }

std::optional<Trigger> TriggerFromId(std::string_view id)
{
	for (int i = 0; i < static_cast<int>(kTriggerIds.size()); ++i)
		if (kTriggerIds[i] == id)
			return static_cast<Trigger>(i);
	return std::nullopt;
}

std::optional<Binding> BindingSet::Set(const Binding& b)
{
	for (auto& existing : bindings_)
	{
		if (existing.SameSlot(b))
		{
			Binding old = existing;
			existing = b;
			return old;
		}
	}
	bindings_.push_back(b);
	return std::nullopt;
}

bool BindingSet::Remove(const Binding& slot)
{
	const auto it = std::find_if(bindings_.begin(), bindings_.end(), [&](const Binding& b) { return b.SameSlot(slot); });
	if (it == bindings_.end())
		return false;
	bindings_.erase(it);
	return true;
}

std::vector<Binding> BindingSet::FindByAction(Action a) const
{
	std::vector<Binding> out;
	for (const auto& b : bindings_)
		if (b.action == a)
			out.push_back(b);
	return out;
}

std::optional<Binding> BindingSet::FindSlot(BindContext c, Trigger t, PadButton input, std::optional<PadButton> modifier) const
{
	for (const auto& b : bindings_)
		if (b.context == c && b.trigger == t && b.input == input && b.modifier == modifier)
			return b;
	return std::nullopt;
}

std::vector<BindingIssue> BindingSet::Validate() const
{
	std::vector<BindingIssue> issues;
	for (size_t i = 0; i < bindings_.size(); ++i)
	{
		const Binding& b = bindings_[i];
		for (size_t j = i + 1; j < bindings_.size(); ++j)
		{
			const Binding& o = bindings_[j];
			if (b.SameSlot(o) && b.action != o.action)
				issues.push_back({BindingIssueKind::Conflict, b, o,
					Describe(b) + " is assigned to both " + std::string(GetActionInfo(b.action).display_name) +
						" and " + std::string(GetActionInfo(o.action).display_name) + "."});
		}
		const bool menu = b.context == BindContext::Menu;
		if ((menu && !AllowedInMenu(b.action)) || (!menu && !AllowedInGameplay(b.action)))
			issues.push_back({BindingIssueKind::WrongContext, b, std::nullopt,
				std::string(GetActionInfo(b.action).display_name) + " cannot be used in " +
					std::string(BindContextDisplayName(b.context)) + "."});
		if (b.modifier && *b.modifier == b.input)
			issues.push_back({BindingIssueKind::ModifierIsInput, b, std::nullopt,
				Describe(b) + " uses the same button as its own modifier."});
		if (b.input == PadButton::Guide || (b.modifier && *b.modifier == PadButton::Guide))
			issues.push_back({BindingIssueKind::ReservedButton, b, std::nullopt,
				"The Guide/PS/Steam button is reserved by the system and cannot be assigned."});
	}

	// Menüden çıkılamayan ya da duraklatılamayan düzenler engellenir.
	auto reachable = [&](BindContext c, Action a) {
		return std::any_of(bindings_.begin(), bindings_.end(), [&](const Binding& b) {
			return b.context == c && b.action == a && !b.modifier && b.input != PadButton::Guide;
		});
	};
	const std::pair<BindContext, Action> essentials[] = {
		{BindContext::Menu, Action::MenuAccept},
		{BindContext::Menu, Action::MenuBack},
		{BindContext::Gameplay, Action::Pause},
	};
	for (const auto& [ctx, act] : essentials)
	{
		if (!reachable(ctx, act))
		{
			Binding b;
			b.context = ctx;
			b.action = act;
			issues.push_back({BindingIssueKind::MissingEssential, b, std::nullopt,
				std::string(GetActionInfo(act).display_name) + " must be assigned to a single button in " +
					std::string(BindContextDisplayName(ctx)) + "."});
		}
	}
	return issues;
}

bool BindingSet::HasBlockingIssues() const
{
	// Tüm sorun türleri kaydı engeller; kullanıcı düzeltmeli ya da varsayılana dönmelidir.
	return !Validate().empty();
}

std::string BindingSet::SlotKey(const Binding& b)
{
	std::string key(BindContextId(b.context));
	key += '.';
	key += TriggerId(b.trigger);
	key += '.';
	if (b.modifier)
	{
		key += PadButtonId(*b.modifier);
		key += '+';
	}
	key += PadButtonId(b.input);
	return key;
}

std::optional<Binding> BindingSet::FromKeyValue(std::string_view key, std::string_view value)
{
	const size_t d1 = key.find('.');
	if (d1 == std::string_view::npos)
		return std::nullopt;
	const size_t d2 = key.find('.', d1 + 1);
	if (d2 == std::string_view::npos)
		return std::nullopt;
	const auto ctx = BindContextFromId(key.substr(0, d1));
	const auto trig = TriggerFromId(key.substr(d1 + 1, d2 - d1 - 1));
	std::string_view rest = key.substr(d2 + 1);
	std::optional<PadButton> modifier;
	if (const size_t plus = rest.find('+'); plus != std::string_view::npos)
	{
		modifier = PadButtonFromId(rest.substr(0, plus));
		if (!modifier)
			return std::nullopt;
		rest = rest.substr(plus + 1);
	}
	const auto input = PadButtonFromId(rest);
	const auto action = ActionFromId(value);
	if (!ctx || !trig || !input || !action)
		return std::nullopt;
	Binding b;
	b.context = *ctx;
	b.trigger = *trig;
	b.input = *input;
	b.modifier = modifier;
	b.action = *action;
	return b;
}

BindingSet DefaultBindings(GameId game)
{
	(void)game; // Üç oyun aynı eylem setini kullanır; OF'de el feneri eylemi gece görüşünü açar.
	using P = PadButton;
	BindingSet s;
	const auto G = BindContext::Gameplay;
	// Şartnamede kesinleşmiş temel eşleşmeler
	Add(s, G, P::RightTrigger, Action::PrimaryAttack);
	Add(s, G, P::LeftTrigger, Action::SecondaryAttack);
	Add(s, G, P::South, Action::Jump);
	Add(s, G, P::East, Action::Crouch);
	Add(s, G, P::West, Action::Use);
	// Kalan eylemler — hepsi tek basış, çakışmasız
	Add(s, G, P::RightShoulder, Action::Reload);
	Add(s, G, P::North, Action::WeaponWheel);
	Add(s, G, P::LeftShoulder, Action::AimSnap);
	Add(s, G, P::RightStick, Action::AimLock);
	Add(s, G, P::LeftStick, Action::LongJump);
	Add(s, G, P::DpadUp, Action::Flashlight);
	Add(s, G, P::DpadDown, Action::LastWeapon);
	Add(s, G, P::DpadLeft, Action::QuickSave);
	Add(s, G, P::DpadRight, Action::QuickLoad);
	Add(s, G, P::Start, Action::Pause);
	Add(s, G, P::Back, Action::Console);
	Add(s, G, P::TouchpadSwipeLeft, Action::PreviousWeapon);
	Add(s, G, P::TouchpadSwipeRight, Action::NextWeapon);

	// Su altı: yüzme için açık yukarı/aşağı eylemleri
	const auto W = BindContext::Water;
	Add(s, W, P::South, Action::SwimUp);
	Add(s, W, P::East, Action::SwimDown);

	// Çok oyunculu: kayıt ve oynanış otomasyonu yerine iletişim eylemleri
	const auto MP = BindContext::Multiplayer;
	Add(s, MP, P::Back, Action::Scoreboard);
	Add(s, MP, P::Touchpad, Action::Scoreboard);
	Add(s, MP, P::DpadLeft, Action::MessageWheel);
	Add(s, MP, P::DpadRight, Action::PushToTalk);
	Add(s, MP, P::LeftShoulder, Action::Chat);
	Add(s, MP, P::RightStick, Action::TeamChat);
	Add(s, MP, P::LeftStick, Action::Walk);

	AddMenuDefaults(s);
	return s;
}

BindingSet OneHandedBindings(GameId game, OneHandedLayout layout)
{
	(void)game;
	using P = PadButton;
	BindingSet s;
	const auto G = BindContext::Gameplay;
	const auto W = BindContext::Water;
	const auto MP = BindContext::Multiplayer;
	if (layout == OneHandedLayout::Left)
	{
		// Sol el: sol çubuk + D-pad + sol omuz/tetik. Kamera jiroskop ya da "tank" dönüşüyle.
		Add(s, G, P::LeftTrigger, Action::PrimaryAttack);
		Add(s, G, P::LeftShoulder, Action::SecondaryAttack);
		Add(s, G, P::DpadDown, Action::Jump);
		Add(s, G, P::DpadRight, Action::Crouch);
		Add(s, G, P::DpadLeft, Action::Use);
		Add(s, G, P::DpadUp, Action::Reload);
		Add(s, G, P::LeftStick, Action::WeaponWheel);
		Add(s, G, P::Back, Action::Pause);
		Add(s, G, P::LeftPaddle1, Action::Flashlight);
		Add(s, G, P::LeftPaddle2, Action::LastWeapon);
		Add(s, W, P::DpadDown, Action::SwimUp);
		Add(s, W, P::DpadRight, Action::SwimDown);
		Add(s, MP, P::LeftPaddle1, Action::Scoreboard);
		Add(s, BindContext::Menu, P::DpadDown, Action::MenuDown);
	}
	else
	{
		// Sağ el: sağ çubuk hareket, yüz tuşları + sağ omuz/tetik.
		Add(s, G, P::RightTrigger, Action::PrimaryAttack);
		Add(s, G, P::RightShoulder, Action::SecondaryAttack);
		Add(s, G, P::South, Action::Jump);
		Add(s, G, P::East, Action::Crouch);
		Add(s, G, P::West, Action::Use);
		Add(s, G, P::North, Action::Reload);
		Add(s, G, P::RightStick, Action::WeaponWheel);
		Add(s, G, P::Start, Action::Pause);
		Add(s, G, P::RightPaddle1, Action::Flashlight);
		Add(s, G, P::RightPaddle2, Action::LastWeapon);
		Add(s, W, P::South, Action::SwimUp);
		Add(s, W, P::East, Action::SwimDown);
		Add(s, MP, P::RightPaddle1, Action::Scoreboard);
	}
	// Tek elle düzenlerde menüler her iki tarafta da kullanılabilir kalır.
	const auto M = BindContext::Menu;
	if (layout == OneHandedLayout::Left)
	{
		Add(s, M, P::DpadUp, Action::MenuUp);
		Add(s, M, P::DpadLeft, Action::MenuBack);
		Add(s, M, P::DpadRight, Action::MenuAccept);
		Add(s, M, P::LeftShoulder, Action::MenuTabPrevious);
		Add(s, M, P::LeftTrigger, Action::MenuTabNext);
		Add(s, M, P::LeftStick, Action::MenuAction1);
		Add(s, M, P::Back, Action::Pause);
	}
	else
	{
		Add(s, M, P::South, Action::MenuAccept);
		Add(s, M, P::East, Action::MenuBack);
		Add(s, M, P::West, Action::MenuAction1);
		Add(s, M, P::North, Action::MenuAction2);
		Add(s, M, P::RightShoulder, Action::MenuTabNext);
		Add(s, M, P::RightTrigger, Action::MenuTabPrevious);
		Add(s, M, P::Start, Action::Pause);
	}
	return s;
}
} // namespace lambda

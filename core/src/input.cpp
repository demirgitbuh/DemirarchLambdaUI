// SPDX-License-Identifier: MIT
#include "lambda/core/input.h"

#include <array>

namespace lambda
{
namespace
{
constexpr std::array<std::string_view, kPadButtonCount> kButtonIds = {
	"south", "east", "west", "north", "back", "guide", "start", "left_stick", "right_stick",
	"left_shoulder", "right_shoulder", "dpad_up", "dpad_down", "dpad_left", "dpad_right", "misc1",
	"right_paddle1", "left_paddle1", "right_paddle2", "left_paddle2", "touchpad", "misc2", "misc3",
	"misc4", "misc5", "misc6", "left_trigger", "right_trigger", "touchpad_click_left",
	"touchpad_click_right", "touchpad_swipe_up", "touchpad_swipe_down", "touchpad_swipe_left",
	"touchpad_swipe_right"};

using C = ActionCategory;

// Sıra Action enum'uyla birebir aynı olmalı (static_assert ve test ile korunur).
constexpr std::array<ActionInfo, kActionCount> kActions = {{
	{Action::None, "none", "Unassigned", C::System, false, false, false},
	{Action::Jump, "jump", "Jump", C::Movement, false, false, false},
	{Action::Crouch, "crouch", "Crouch", C::Movement, false, false, true},
	{Action::Walk, "walk", "Walk", C::Movement, false, false, true},
	{Action::SwimUp, "swim_up", "Swim Up", C::Movement, false, false, false},
	{Action::SwimDown, "swim_down", "Swim Down", C::Movement, false, false, false},
	{Action::LongJump, "long_jump", "Long Jump (Assisted)", C::Movement, false, true, false},
	{Action::PrimaryAttack, "primary_attack", "Primary Fire", C::Combat, false, false, false},
	{Action::SecondaryAttack, "secondary_attack", "Secondary Fire", C::Combat, false, false, false},
	{Action::Reload, "reload", "Reload", C::Combat, false, false, false},
	{Action::WeaponWheel, "weapon_wheel", "Weapon Wheel", C::Weapons, false, false, false},
	{Action::NextWeapon, "next_weapon", "Next Weapon", C::Weapons, false, false, false},
	{Action::PreviousWeapon, "previous_weapon", "Previous Weapon", C::Weapons, false, false, false},
	{Action::LastWeapon, "last_weapon", "Last Weapon", C::Weapons, false, false, false},
	{Action::Use, "use", "Use", C::Interaction, false, false, false},
	{Action::Flashlight, "flashlight", "Flashlight", C::Interaction, false, false, false},
	{Action::Spray, "spray", "Spray Logo", C::Interaction, false, false, false},
	{Action::Pause, "pause", "Pause Menu", C::System, false, false, false},
	{Action::Console, "console", "Console", C::System, false, false, false},
	{Action::QuickSave, "quick_save", "Quick Save", C::System, false, true, false},
	{Action::QuickLoad, "quick_load", "Quick Load", C::System, false, true, false},
	{Action::Scoreboard, "scoreboard", "Scoreboard", C::System, false, false, true},
	{Action::Chat, "chat", "Chat (All)", C::Communication, false, false, false},
	{Action::TeamChat, "team_chat", "Chat (Team)", C::Communication, false, false, false},
	{Action::PushToTalk, "push_to_talk", "Push to Talk", C::Communication, false, false, true},
	{Action::MessageWheel, "message_wheel", "Message Wheel", C::Communication, false, false, false},
	{Action::AimSnap, "aim_snap", "Aim Snap", C::AimAssist, false, true, false},
	{Action::AimLock, "aim_lock", "Target Lock", C::AimAssist, false, true, false},
	{Action::MenuUp, "menu_up", "Navigate Up", C::Menu, true, false, false},
	{Action::MenuDown, "menu_down", "Navigate Down", C::Menu, true, false, false},
	{Action::MenuLeft, "menu_left", "Navigate Left", C::Menu, true, false, false},
	{Action::MenuRight, "menu_right", "Navigate Right", C::Menu, true, false, false},
	{Action::MenuAccept, "menu_accept", "Select", C::Menu, true, false, false},
	{Action::MenuBack, "menu_back", "Back", C::Menu, true, false, false},
	{Action::MenuTabPrevious, "menu_tab_previous", "Previous Tab", C::Menu, true, false, false},
	{Action::MenuTabNext, "menu_tab_next", "Next Tab", C::Menu, true, false, false},
	{Action::MenuPagePrevious, "menu_page_previous", "Page Up", C::Menu, true, false, false},
	{Action::MenuPageNext, "menu_page_next", "Page Down", C::Menu, true, false, false},
	{Action::MenuAction1, "menu_action1", "Option", C::Menu, true, false, false},
	{Action::MenuAction2, "menu_action2", "Details", C::Menu, true, false, false},
}};

constexpr bool ActionTableOrdered()
{
	for (int i = 0; i < kActionCount; ++i)
		if (static_cast<int>(kActions[i].action) != i)
			return false;
	return true;
}
static_assert(ActionTableOrdered(), "Action metadata table must follow the Action enum order");
} // namespace

std::string_view PadButtonId(PadButton b)
{
	const int i = static_cast<int>(b);
	return (i >= 0 && i < kPadButtonCount) ? kButtonIds[i] : std::string_view{};
}

std::optional<PadButton> PadButtonFromId(std::string_view id)
{
	for (int i = 0; i < kPadButtonCount; ++i)
		if (kButtonIds[i] == id)
			return static_cast<PadButton>(i);
	return std::nullopt;
}

const ActionInfo& GetActionInfo(Action a)
{
	const int i = static_cast<int>(a);
	return kActions[(i >= 0 && i < kActionCount) ? i : 0];
}

std::optional<Action> ActionFromId(std::string_view id)
{
	for (const auto& info : kActions)
		if (info.id == id)
			return info.action;
	return std::nullopt;
}

std::string_view ActionCategoryName(ActionCategory c)
{
	switch (c)
	{
	case C::Movement: return "Movement";
	case C::Combat: return "Combat";
	case C::Weapons: return "Weapons";
	case C::Interaction: return "Interaction";
	case C::System: return "System";
	case C::Communication: return "Communication";
	case C::AimAssist: return "Aim Assist";
	case C::Menu: return "Menu";
	}
	return "Other";
}
} // namespace lambda

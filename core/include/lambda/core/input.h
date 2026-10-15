// SPDX-License-Identifier: MIT
// Fiziksel kontrolcü girdileri ve Lambda UI eylemleri.
// Kullanıcıya görünen adlar İngilizcedir; profil dosyalarında sabit kimlikler (id) kullanılır.
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace lambda
{
// SDL3 SDL_GamepadButton sırasını izler (konumsal adlar: South = Xbox A / PS çarpı).
// Tetikler eşikli dijital girdiler olarak sona eklenmiştir. Touchpad hareketleri sanal girdidir.
enum class PadButton : std::uint8_t
{
	South = 0,
	East,
	West,
	North,
	Back,
	Guide,
	Start,
	LeftStick,
	RightStick,
	LeftShoulder,
	RightShoulder,
	DpadUp,
	DpadDown,
	DpadLeft,
	DpadRight,
	Misc1,        // Share / Capture / Mic
	RightPaddle1, // Elite P1 / Edge sağ arka
	LeftPaddle1,
	RightPaddle2,
	LeftPaddle2,
	Touchpad,     // touchpad tıklaması (bölgesiz)
	Misc2,
	Misc3,
	Misc4,
	Misc5,
	Misc6,
	LeftTrigger,
	RightTrigger,
	TouchpadClickLeft,
	TouchpadClickRight,
	TouchpadSwipeUp,
	TouchpadSwipeDown,
	TouchpadSwipeLeft,
	TouchpadSwipeRight,
	Count
};

constexpr int kPadButtonCount = static_cast<int>(PadButton::Count);

std::string_view PadButtonId(PadButton b);
std::optional<PadButton> PadButtonFromId(std::string_view id);

enum class ActionCategory : std::uint8_t
{
	Movement,
	Combat,
	Weapons,
	Interaction,
	System,
	Communication,
	AimAssist,
	Menu,
};

enum class Action : std::uint16_t
{
	None = 0,
	// Hareket
	Jump,
	Crouch,
	Walk,
	SwimUp,
	SwimDown,
	LongJump,
	// Savaş
	PrimaryAttack,
	SecondaryAttack,
	Reload,
	// Silahlar
	WeaponWheel,
	NextWeapon,
	PreviousWeapon,
	LastWeapon,
	// Etkileşim
	Use,
	Flashlight,
	Spray,
	// Sistem
	Pause,
	Console,
	QuickSave,
	QuickLoad,
	Scoreboard,
	// İletişim
	Chat,
	TeamChat,
	PushToTalk,
	MessageWheel,
	// Nişan yardımı
	AimSnap,
	AimLock,
	// Menü
	MenuUp,
	MenuDown,
	MenuLeft,
	MenuRight,
	MenuAccept,
	MenuBack,
	MenuTabPrevious,
	MenuTabNext,
	MenuPagePrevious,
	MenuPageNext,
	MenuAction1,
	MenuAction2,
	Count
};

constexpr int kActionCount = static_cast<int>(Action::Count);

struct ActionInfo
{
	Action action;
	std::string_view id;           // profil kimliği, ör. "jump"
	std::string_view display_name; // İngilizce UI metni
	ActionCategory category;
	bool menu_action;       // yalnızca Menu bağlamında geçerli
	bool singleplayer_only; // çok oyunculuda devre dışı (otomasyon, kayıt vb.)
	bool supports_toggle;   // tut yerine aç/kapa seçeneği sunulabilir
};

const ActionInfo& GetActionInfo(Action a);
std::optional<Action> ActionFromId(std::string_view id);
std::string_view ActionCategoryName(ActionCategory c);
} // namespace lambda

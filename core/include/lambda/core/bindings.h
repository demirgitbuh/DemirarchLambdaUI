// SPDX-License-Identifier: MIT
// Bağlama (context) duyarlı tuş atamaları, çakışma denetimi ve varsayılan düzen.
#pragma once

#include "lambda/core/input.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lambda
{
enum class GameId : std::uint8_t
{
	HalfLife = 0,
	OpposingForce,
	BlueShift,
};

std::string_view GameIdString(GameId g);
std::optional<GameId> GameIdFromString(std::string_view s);

// Daha özel bağlamlar daha genel olanları gölgeler: Water ve Multiplayer, Gameplay'in üstüne biner.
enum class BindContext : std::uint8_t
{
	Gameplay = 0,
	Water,
	Multiplayer,
	Menu,
	Count
};

std::string_view BindContextId(BindContext c);
std::string_view BindContextDisplayName(BindContext c);
std::optional<BindContext> BindContextFromId(std::string_view id);

enum class Trigger : std::uint8_t
{
	Press = 0,   // tek basış (basılı tutuldukça etkin)
	Hold,        // hold_time boyunca basılı tutunca
	DoublePress, // double_window içinde iki basış
	Count
};

std::string_view TriggerId(Trigger t);
std::string_view TriggerDisplayName(Trigger t);
std::optional<Trigger> TriggerFromId(std::string_view id);

struct Binding
{
	BindContext context = BindContext::Gameplay;
	Trigger trigger = Trigger::Press;
	PadButton input = PadButton::South;
	std::optional<PadButton> modifier; // değiştirici tuş (basılı tutulmalı)
	Action action = Action::None;

	bool SameSlot(const Binding& o) const
	{
		return context == o.context && trigger == o.trigger && input == o.input && modifier == o.modifier;
	}
	bool operator==(const Binding& o) const { return SameSlot(o) && action == o.action; }
};

enum class BindingIssueKind : std::uint8_t
{
	Conflict,             // aynı yuvada iki farklı eylem
	WrongContext,         // menü eylemi oynanış bağlamında ya da tersi
	MissingEssential,     // menüde kabul/geri ya da duraklatma erişilemez
	ModifierIsInput,      // değiştirici ile ana tuş aynı
	ReservedButton,       // Guide gibi sistem tuşu
};

struct BindingIssue
{
	BindingIssueKind kind;
	Binding binding;
	std::optional<Binding> other;
	std::string message; // İngilizce, UI'da gösterilir
};

class BindingSet
{
public:
	const std::vector<Binding>& All() const { return bindings_; }

	// Aynı yuvadaki eski atamanın yerini alır; değiştirilen atamayı döndürür.
	std::optional<Binding> Set(const Binding& b);
	bool Remove(const Binding& slot);
	void Clear() { bindings_.clear(); }
	std::vector<Binding> FindByAction(Action a) const;
	std::optional<Binding> FindSlot(BindContext c, Trigger t, PadButton input, std::optional<PadButton> modifier) const;

	std::vector<BindingIssue> Validate() const;
	bool HasBlockingIssues() const;

	// Profil anahtar/değer biçimi: "<context>.<trigger>.[<modifier>+]<input>" = "<action id>"
	static std::string SlotKey(const Binding& b);
	static std::optional<Binding> FromKeyValue(std::string_view key, std::string_view value);

private:
	std::vector<Binding> bindings_;
};

// Modern FPS varsayılan düzeni (bkz. docs/controls.md). Oyuna özgü farklar burada uygulanır.
BindingSet DefaultBindings(GameId game);

// Tek elle kullanım profilleri (erişilebilirlik).
enum class OneHandedLayout : std::uint8_t
{
	Left,
	Right
};
BindingSet OneHandedBindings(GameId game, OneHandedLayout layout);
} // namespace lambda

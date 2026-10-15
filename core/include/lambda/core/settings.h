// SPDX-License-Identifier: MIT
// Tipli kontrolcü ayarları ve profil belgesine çift yönlü eşleme.
// Her alan okunurken geçerli aralığa çekilir (bozuk/elle düzenlenmiş profil güvenliği).
#pragma once

#include "lambda/core/bindings.h"
#include "lambda/core/camera.h"
#include "lambda/core/document.h"
#include "lambda/core/resolver.h"
#include "lambda/core/stick.h"

#include <array>

namespace lambda
{
struct MovementSettings
{
	bool swap_sticks = false;        // hareket sağ, kamera sol
	bool movement_stick_turns = false; // tek elle "tank" dönüşü: hareket çubuğunun yatay ekseni döndürür
	float turn_speed = 160.0f;       // movement_stick_turns için derece/sn

	void Sanitize();
};

struct ControllerSettings
{
	StickSettings left_stick{};
	StickSettings right_stick{};
	CameraSettings camera{};
	MovementSettings movement{};
	ResolverTiming timing{};
	// Aç/kapa olarak kullanılan eylemler (ör. çömelme)
	std::array<bool, kActionCount> toggle{};
	BindingSet bindings{};

	static ControllerSettings Defaults(GameId game);
	void Sanitize();
};

// Groups: "sticks", "camera", "movement", "timing", "toggles", "bindings"
void WriteControllerSettings(const ControllerSettings& s, SettingsDocument& doc);
ControllerSettings ReadControllerSettings(const SettingsDocument& doc, GameId game, std::vector<std::string>* warnings = nullptr);

enum class SettingsGroup
{
	Sticks,
	Camera,
	Movement,
	Timing,
	Toggles,
	Bindings,
	All,
};
// Onaylı sıfırlama: yalnızca seçilen grubu varsayılana döndürür.
void ResetGroup(ControllerSettings& s, SettingsGroup group, GameId game);
} // namespace lambda

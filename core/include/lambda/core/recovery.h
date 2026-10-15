// SPDX-License-Identifier: MIT
// Yeniden atanamaz kurtarma kombinasyonu: Start + Back birlikte 3 saniye basılı tutulur.
// Tuş atamalarından bağımsız çalışır; kullanıcı düzeni bozsa bile varsayılan kontrollere
// dönme penceresini açar (onaylı). Klavyede karşılığı: konsol komutu "lambda_recover"
// ve başlatma parametresi "-lambdasafe".
#pragma once

#include "lambda/core/resolver.h"

namespace lambda
{
class RecoveryCombo
{
public:
	static constexpr float kHoldSeconds = 3.0f;

	// Kombinasyon tamamlandığı karede true döner (bir kez); tuşlar bırakılınca yeniden kurulur.
	bool Update(const ButtonSet& down, double now);
	// 0..1 ilerleme (UI'da halka göstergesi için)
	float Progress(double now) const;
	bool Active() const { return holding_; }

private:
	bool holding_ = false;
	bool fired_ = false;
	double start_ = 0.0;
};
} // namespace lambda

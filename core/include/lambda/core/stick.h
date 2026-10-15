// SPDX-License-Identifier: MIT
// Analog çubuk işleme: kalibrasyon ofseti, radyal iç/dış ölü bölge, tepki eğrisi,
// eksen hassasiyeti ve ters çevirme. Kare hızından bağımsızdır (durumsuz).
#pragma once

#include "lambda/core/math.h"

#include <cstdint>

namespace lambda
{
enum class ResponseCurve : std::uint8_t
{
	Linear = 0,
	Quadratic,
	Cubic,
	// response = |x|^exponent (exponent ayarı kullanılır)
	Power,
	// Küçük hareketlerde hassas, ortada doğrusal; hedef alma için yaygın S eğrisi
	SCurve,
};

struct StickCalibration
{
	// Kullanıcı onayıyla ölçülmüş merkez kayması (drift) ve gürültü yarıçapı.
	Vec2 center_offset{};
	float noise_radius = 0.0f;
	bool valid = false;
};

struct StickSettings
{
	float inner_deadzone = 0.12f; // 0..0.9
	float outer_deadzone = 0.04f; // 0..0.5 (1 - outer = tam sapma kabul eşiği)
	ResponseCurve curve = ResponseCurve::Linear;
	float curve_exponent = 2.0f; // Power için 1..5
	float sensitivity_x = 1.0f;  // 0.1..4
	float sensitivity_y = 1.0f;
	bool invert_x = false;
	bool invert_y = false;
	StickCalibration calibration{};

	// Ayarları geçerli aralığa çeker; bozuk profil verisi için güvenlik ağı.
	void Sanitize();
};

// Ham değer [-1,1] aralığında (y aşağı pozitif değil: yukarı = -1 SDL kuralı bu katmanda
// çevrilmez; çağıran tutarlı olmalıdır). Çıktı: büyüklük [0,1] aralığında, yönü korunmuş.
Vec2 ProcessStick(Vec2 raw, const StickSettings& settings);

// Yalnızca büyüklük eğrisini uygular (test ve önizleme için).
float ApplyResponseCurve(float magnitude, ResponseCurve curve, float exponent);

// Drift kalibrasyonu örnekleyicisi. Kullanıcı çubuğu bırakmışken örnek toplar;
// sonuç yalnızca kullanıcı onaylarsa ayarlara yazılır (Finish() bir öneri döndürür).
class StickCalibrator
{
public:
	void Reset();
	void AddSample(Vec2 raw);
	int SampleCount() const { return count_; }
	// En az min_samples örnek varsa ve hareket görülmediyse geçerli kalibrasyon döner.
	StickCalibration Finish(int min_samples = 60, float max_noise = 0.25f) const;

private:
	int count_ = 0;
	double sum_x_ = 0.0;
	double sum_y_ = 0.0;
	float min_x_ = 1.0f, max_x_ = -1.0f, min_y_ = 1.0f, max_y_ = -1.0f;
};
} // namespace lambda

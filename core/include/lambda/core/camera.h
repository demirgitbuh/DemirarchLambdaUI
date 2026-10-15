// SPDX-License-Identifier: MIT
// Sağ analogdan kamera dönüşü: derece/saniye hızı, ivmelenme, yumuşatma ve dönüş sınırı.
// Tüm hesaplar dt (saniye) ile yapılır; kare hızı değişse de aynı girdi aynı açıyı üretir.
#pragma once

#include "lambda/core/math.h"

namespace lambda
{
struct CameraSettings
{
	float yaw_speed = 220.0f;   // tam sapmada derece/sn
	float pitch_speed = 160.0f; // tam sapmada derece/sn
	// İvmelenme: çubuk accel_threshold üzerinde tutuldukça hız accel_time içinde
	// (1 + accel_boost) katına çıkar. accel_boost = 0 ise kapalı.
	float accel_boost = 0.6f;
	float accel_threshold = 0.9f;
	float accel_time = 0.35f;
	float smoothing = 0.0f;       // zaman sabiti (sn); 0 = yumuşatma yok
	float max_turn_rate = 720.0f; // derece/sn mutlak üst sınır

	void Sanitize();
};

struct CameraDelta
{
	float yaw = 0.0f;   // derece, pozitif = sola (Half-Life yaw yönü)
	float pitch = 0.0f; // derece, pozitif = aşağı (Half-Life pitch yönü)
};

class CameraController
{
public:
	// stick: işlenmiş sağ analog (x sağ +, y aşağı +). dt saniye.
	// rate_scale: dışarıdan uygulanan çarpan (ör. hedef üzerinde yavaşlama, 0..1).
	CameraDelta Update(Vec2 stick, float dt, const CameraSettings& settings, float rate_scale = 1.0f);
	void Reset();
	float AccelerationProgress() const { return accel_progress_; }

private:
	float accel_progress_ = 0.0f;
	float smoothed_yaw_rate_ = 0.0f;
	float smoothed_pitch_rate_ = 0.0f;
};
} // namespace lambda

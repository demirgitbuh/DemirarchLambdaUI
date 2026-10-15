// SPDX-License-Identifier: MIT
#include "lambda/core/camera.h"

namespace lambda
{
namespace
{
float SafeClamp(float v, float lo, float hi, float fallback)
{
	return std::isfinite(v) ? Clamp(v, lo, hi) : fallback;
}
} // namespace

void CameraSettings::Sanitize()
{
	yaw_speed = SafeClamp(yaw_speed, 10.0f, 1000.0f, 220.0f);
	pitch_speed = SafeClamp(pitch_speed, 10.0f, 1000.0f, 160.0f);
	accel_boost = SafeClamp(accel_boost, 0.0f, 3.0f, 0.6f);
	accel_threshold = SafeClamp(accel_threshold, 0.5f, 1.0f, 0.9f);
	accel_time = SafeClamp(accel_time, 0.05f, 3.0f, 0.35f);
	smoothing = SafeClamp(smoothing, 0.0f, 0.5f, 0.0f);
	max_turn_rate = SafeClamp(max_turn_rate, 30.0f, 3600.0f, 720.0f);
}

void CameraController::Reset() { *this = CameraController{}; }

CameraDelta CameraController::Update(Vec2 stick, float dt, const CameraSettings& s, float rate_scale)
{
	if (!(dt > 0.0f) || !std::isfinite(dt))
		return {};
	// Çok uzun kareler (yükleme, duraklatma dönüşü) ani savrulma üretmesin.
	dt = std::min(dt, 0.1f);
	rate_scale = Clamp(std::isfinite(rate_scale) ? rate_scale : 1.0f, 0.0f, 1.0f);

	const float mag = stick.Length();
	if (s.accel_boost > 0.0f && mag >= s.accel_threshold)
		accel_progress_ = std::min(1.0f, accel_progress_ + dt / s.accel_time);
	else
		accel_progress_ = 0.0f;

	const float accel = 1.0f + s.accel_boost * accel_progress_;
	float yaw_rate = -stick.x * s.yaw_speed * accel * rate_scale;
	float pitch_rate = stick.y * s.pitch_speed * accel * rate_scale;

	const float prev_yaw_rate = smoothed_yaw_rate_;
	const float prev_pitch_rate = smoothed_pitch_rate_;
	const float a = SmoothingAlpha(dt, s.smoothing);
	smoothed_yaw_rate_ += (yaw_rate - smoothed_yaw_rate_) * a;
	smoothed_pitch_rate_ += (pitch_rate - smoothed_pitch_rate_) * a;

	// Yumuşatma açıkken hız kare boyunca değiştiği için yamuk integrali kullanılır; böylece
	// toplam açı kare hızından bağımsız kalır. Yumuşatma kapalıyken hız sabittir.
	float eff_yaw = smoothed_yaw_rate_;
	float eff_pitch = smoothed_pitch_rate_;
	if (s.smoothing > 0.0f)
	{
		eff_yaw = 0.5f * (prev_yaw_rate + smoothed_yaw_rate_);
		eff_pitch = 0.5f * (prev_pitch_rate + smoothed_pitch_rate_);
	}

	// Toplam açısal hız sınırı (yön korunarak).
	const float rate = std::sqrt(eff_yaw * eff_yaw + eff_pitch * eff_pitch);
	const float scale = (rate > s.max_turn_rate && rate > 0.0f) ? s.max_turn_rate / rate : 1.0f;

	return {eff_yaw * scale * dt, eff_pitch * scale * dt};
}
} // namespace lambda

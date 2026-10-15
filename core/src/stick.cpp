// SPDX-License-Identifier: MIT
#include "lambda/core/stick.h"

namespace lambda
{
void StickSettings::Sanitize()
{
	inner_deadzone = Clamp(std::isfinite(inner_deadzone) ? inner_deadzone : 0.12f, 0.0f, 0.9f);
	outer_deadzone = Clamp(std::isfinite(outer_deadzone) ? outer_deadzone : 0.04f, 0.0f, 0.5f);
	if (inner_deadzone + outer_deadzone > 0.95f)
		outer_deadzone = 0.95f - inner_deadzone;
	curve_exponent = Clamp(std::isfinite(curve_exponent) ? curve_exponent : 2.0f, 1.0f, 5.0f);
	sensitivity_x = Clamp(std::isfinite(sensitivity_x) ? sensitivity_x : 1.0f, 0.1f, 4.0f);
	sensitivity_y = Clamp(std::isfinite(sensitivity_y) ? sensitivity_y : 1.0f, 0.1f, 4.0f);
	if (static_cast<int>(curve) > static_cast<int>(ResponseCurve::SCurve))
		curve = ResponseCurve::Linear;
	if (!std::isfinite(calibration.center_offset.x) || !std::isfinite(calibration.center_offset.y) ||
		std::fabs(calibration.center_offset.x) > 0.5f || std::fabs(calibration.center_offset.y) > 0.5f)
		calibration = {};
}

float ApplyResponseCurve(float m, ResponseCurve curve, float exponent)
{
	m = Clamp(m, 0.0f, 1.0f);
	switch (curve)
	{
	case ResponseCurve::Linear:
		return m;
	case ResponseCurve::Quadratic:
		return m * m;
	case ResponseCurve::Cubic:
		return m * m * m;
	case ResponseCurve::Power:
		return std::pow(m, Clamp(exponent, 1.0f, 5.0f));
	case ResponseCurve::SCurve:
		// Smoothstep tabanlı; başı yumuşak, sonu tam sapmaya ulaşır.
		return m * m * (3.0f - 2.0f * m) * 0.5f + m * m * 0.5f;
	}
	return m;
}

Vec2 ProcessStick(Vec2 raw, const StickSettings& s)
{
	if (s.calibration.valid)
		raw = raw - s.calibration.center_offset;

	float mag = raw.Length();
	if (!std::isfinite(mag) || mag <= 0.0f)
		return {};

	// Kalibrasyonda ölçülen gürültü yarıçapı, iç ölü bölgeyi yalnızca büyütebilir.
	const float inner = std::max(s.inner_deadzone, s.calibration.valid ? s.calibration.noise_radius : 0.0f);
	const float outer_edge = 1.0f - s.outer_deadzone;
	if (mag <= inner)
		return {};

	const Vec2 dir = raw * (1.0f / mag);
	float scaled = (outer_edge > inner) ? (mag - inner) / (outer_edge - inner) : 1.0f;
	scaled = Clamp(scaled, 0.0f, 1.0f);
	const float shaped = ApplyResponseCurve(scaled, s.curve, s.curve_exponent);

	Vec2 out = dir * shaped;
	out.x *= s.sensitivity_x;
	out.y *= s.sensitivity_y;
	if (s.invert_x)
		out.x = -out.x;
	if (s.invert_y)
		out.y = -out.y;

	// Hassasiyet > 1 iken büyüklük 1'i aşmasın (hareket hızı özgün azamiyi geçmemeli).
	const float out_mag = out.Length();
	if (out_mag > 1.0f)
		out = out * (1.0f / out_mag);
	return out;
}

void StickCalibrator::Reset() { *this = StickCalibrator{}; }

void StickCalibrator::AddSample(Vec2 raw)
{
	if (!std::isfinite(raw.x) || !std::isfinite(raw.y))
		return;
	++count_;
	sum_x_ += raw.x;
	sum_y_ += raw.y;
	min_x_ = std::min(min_x_, raw.x);
	max_x_ = std::max(max_x_, raw.x);
	min_y_ = std::min(min_y_, raw.y);
	max_y_ = std::max(max_y_, raw.y);
}

StickCalibration StickCalibrator::Finish(int min_samples, float max_noise) const
{
	StickCalibration c;
	if (count_ < min_samples)
		return c;
	const float spread_x = max_x_ - min_x_;
	const float spread_y = max_y_ - min_y_;
	const float noise = std::max(spread_x, spread_y) * 0.5f;
	// Büyük yayılım = kullanıcı çubuğa dokunuyor; kalibrasyon reddedilir.
	if (noise > max_noise)
		return c;
	c.center_offset = {static_cast<float>(sum_x_ / count_), static_cast<float>(sum_y_ / count_)};
	if (std::fabs(c.center_offset.x) > 0.5f || std::fabs(c.center_offset.y) > 0.5f)
		return StickCalibration{};
	c.noise_radius = noise + 0.01f;
	c.valid = true;
	return c;
}
} // namespace lambda

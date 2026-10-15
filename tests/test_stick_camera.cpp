// SPDX-License-Identifier: MIT
#include "lambda_test.h"

#include "lambda/core/camera.h"
#include "lambda/core/stick.h"

using namespace lambda;

TEST_CASE("stick: inner deadzone removes small input")
{
	StickSettings s;
	s.inner_deadzone = 0.2f;
	CHECK(ProcessStick({0.1f, 0.1f}, s) == Vec2{});
	CHECK(ProcessStick({0.19f, 0.0f}, s) == Vec2{});
	CHECK(ProcessStick({0.3f, 0.0f}, s).x > 0.0f);
}

TEST_CASE("stick: output is graded and reaches full deflection at outer deadzone")
{
	StickSettings s;
	s.inner_deadzone = 0.1f;
	s.outer_deadzone = 0.1f;
	const float slow = ProcessStick({0.3f, 0.0f}, s).x;
	const float fast = ProcessStick({0.7f, 0.0f}, s).x;
	CHECK(slow > 0.0f && slow < fast);
	CHECK_NEAR(ProcessStick({0.9f, 0.0f}, s).x, 1.0f, 1e-4);
	CHECK_NEAR(ProcessStick({1.0f, 0.0f}, s).x, 1.0f, 1e-4);
	// Radyal: çapraz tam sapma da 1 büyüklüğünde, 1'i aşmaz
	CHECK(ProcessStick({0.8f, 0.8f}, s).Length() <= 1.0001f);
}

TEST_CASE("stick: sensitivity never exceeds original maximum speed")
{
	StickSettings s;
	s.sensitivity_x = 3.0f;
	s.sensitivity_y = 3.0f;
	CHECK(ProcessStick({1.0f, 1.0f}, s).Length() <= 1.0001f);
}

TEST_CASE("stick: response curves")
{
	CHECK_NEAR(ApplyResponseCurve(0.5f, ResponseCurve::Linear, 2), 0.5f, 1e-6);
	CHECK_NEAR(ApplyResponseCurve(0.5f, ResponseCurve::Quadratic, 2), 0.25f, 1e-6);
	CHECK_NEAR(ApplyResponseCurve(0.5f, ResponseCurve::Cubic, 2), 0.125f, 1e-6);
	CHECK_NEAR(ApplyResponseCurve(0.5f, ResponseCurve::Power, 3), 0.125f, 1e-6);
	CHECK_NEAR(ApplyResponseCurve(1.0f, ResponseCurve::SCurve, 2), 1.0f, 1e-6);
	CHECK_NEAR(ApplyResponseCurve(0.0f, ResponseCurve::SCurve, 2), 0.0f, 1e-6);
	// Monotonluk
	float prev = -1;
	for (int i = 0; i <= 100; ++i)
	{
		const float v = ApplyResponseCurve(i / 100.0f, ResponseCurve::SCurve, 2);
		CHECK(v >= prev);
		prev = v;
	}
}

TEST_CASE("stick: inversion and axis sensitivity")
{
	StickSettings s;
	s.inner_deadzone = 0;
	s.outer_deadzone = 0;
	s.invert_y = true;
	s.sensitivity_x = 0.5f;
	const Vec2 out = ProcessStick({0.6f, 0.0f}, s);
	CHECK_NEAR(out.x, 0.3f, 1e-4);
	CHECK(ProcessStick({0.0f, 0.5f}, s).y < 0.0f);
}

TEST_CASE("stick: sanitize clamps corrupt values")
{
	StickSettings s;
	s.inner_deadzone = 7.0f;
	s.outer_deadzone = -3.0f;
	s.sensitivity_x = NAN;
	s.calibration.valid = true;
	s.calibration.center_offset = {4.0f, 0.0f};
	s.Sanitize();
	CHECK(s.inner_deadzone <= 0.9f);
	CHECK(s.outer_deadzone >= 0.0f);
	CHECK_NEAR(s.sensitivity_x, 1.0f, 1e-6);
	CHECK(!s.calibration.valid);
}

TEST_CASE("calibration: drift is measured only when the stick is at rest")
{
	StickCalibrator c;
	for (int i = 0; i < 100; ++i)
		c.AddSample({0.06f + (i % 3) * 0.002f, -0.04f});
	const StickCalibration cal = c.Finish();
	CHECK(cal.valid);
	CHECK_NEAR(cal.center_offset.x, 0.062f, 0.003);
	CHECK_NEAR(cal.center_offset.y, -0.04f, 1e-4);

	StickSettings s;
	s.inner_deadzone = 0.05f;
	CHECK(ProcessStick({0.07f, -0.04f}, s).Length() > 0.0f); // kalibrasyonsuz drift sızar
	s.calibration = cal;
	CHECK(ProcessStick({0.07f, -0.04f}, s) == Vec2{});     // kalibrasyonla sızmaz

	StickCalibrator moving;
	for (int i = 0; i < 100; ++i)
		moving.AddSample({(i % 2) ? 0.8f : -0.8f, 0.0f});
	CHECK(!moving.Finish().valid);

	StickCalibrator few;
	few.AddSample({0.0f, 0.0f});
	CHECK(!few.Finish().valid);
}

TEST_CASE("camera: rotation is frame-rate independent")
{
	CameraSettings s;
	s.accel_boost = 0.0f;
	s.smoothing = 0.0f;
	auto turn = [&](float dt, int frames) {
		CameraController c;
		float yaw = 0;
		for (int i = 0; i < frames; ++i)
			yaw += c.Update({0.5f, 0.0f}, dt, s).yaw;
		return yaw;
	};
	const float at30 = turn(1.0f / 30.0f, 30);
	const float at144 = turn(1.0f / 144.0f, 144);
	CHECK_NEAR(at30, at144, 0.01);
	CHECK_NEAR(at30, -0.5f * s.yaw_speed, 0.01); // sağa = negatif yaw
}

TEST_CASE("camera: smoothing converges equally at different frame rates")
{
	CameraSettings s;
	s.accel_boost = 0.0f;
	s.smoothing = 0.1f;
	auto turn = [&](float dt, int frames) {
		CameraController c;
		float yaw = 0;
		for (int i = 0; i < frames; ++i)
			yaw += c.Update({1.0f, 0.0f}, dt, s).yaw;
		return yaw;
	};
	CHECK_NEAR(turn(1.0f / 30.0f, 60), turn(1.0f / 240.0f, 480), 0.5); // 2 sn sonunda ±0.5 derece
}

TEST_CASE("camera: acceleration ramps and resets; turn rate is capped")
{
	CameraSettings s;
	s.accel_boost = 1.0f;
	s.accel_time = 0.5f;
	s.max_turn_rate = 10000.0f;
	CameraController c;
	const float first = c.Update({1.0f, 0.0f}, 0.01f, s).yaw;
	for (int i = 0; i < 100; ++i)
		c.Update({1.0f, 0.0f}, 0.01f, s);
	const float later = c.Update({1.0f, 0.0f}, 0.01f, s).yaw;
	CHECK(std::fabs(later) > std::fabs(first) * 1.9f);
	c.Update({0.5f, 0.0f}, 0.01f, s);
	CHECK_NEAR(c.AccelerationProgress(), 0.0f, 1e-6);

	CameraSettings capped;
	capped.accel_boost = 0;
	capped.yaw_speed = 900.0f;
	capped.max_turn_rate = 100.0f;
	CameraController c2;
	CHECK_NEAR(c2.Update({1.0f, 0.0f}, 0.1f, capped).yaw, -10.0f, 1e-3);
}

TEST_CASE("camera: long frames and slowdown scale")
{
	CameraSettings s;
	s.accel_boost = 0;
	CameraController c;
	// 2 sn'lik takılma 0.1 sn ile sınırlanır
	CHECK_NEAR(c.Update({1.0f, 0.0f}, 2.0f, s).yaw, -s.yaw_speed * 0.1f, 1e-3);
	CameraController c2;
	CHECK_NEAR(c2.Update({1.0f, 0.0f}, 0.05f, s, 0.5f).yaw, -s.yaw_speed * 0.05f * 0.5f, 1e-3);
}

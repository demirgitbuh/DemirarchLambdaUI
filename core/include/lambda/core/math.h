// SPDX-License-Identifier: MIT
// Demirarch Lambda UI çekirdeği — temel matematik yardımcıları.
#pragma once

#include <algorithm>
#include <cmath>

namespace lambda
{
constexpr float kPi = 3.14159265358979323846f;

struct Vec2
{
	float x = 0.0f;
	float y = 0.0f;

	constexpr Vec2() = default;
	constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

	constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
	constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
	constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
	constexpr bool operator==(Vec2 o) const { return x == o.x && y == o.y; }
	float Length() const { return std::sqrt(x * x + y * y); }
};

struct Vec3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	constexpr Vec3() = default;
	constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	constexpr Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
	constexpr Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
	constexpr Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
	float Length() const { return std::sqrt(x * x + y * y + z * z); }
	constexpr float Dot(Vec3 o) const { return x * o.x + y * o.y + z * o.z; }
};

template <typename T>
constexpr T Clamp(T v, T lo, T hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

constexpr float Lerp(float a, float b, float t) { return a + (b - a) * t; }

// Kare süresinden bağımsız üstel yumuşatma katsayısı. time_constant <= 0 ise yumuşatma yok.
inline float SmoothingAlpha(float dt, float time_constant)
{
	if (time_constant <= 0.0f || dt <= 0.0f)
		return 1.0f;
	return 1.0f - std::exp(-dt / time_constant);
}

// Açıyı (-180, 180] aralığına indirger.
inline float WrapDegrees(float deg)
{
	deg = std::fmod(deg + 180.0f, 360.0f);
	if (deg < 0.0f)
		deg += 360.0f;
	return deg - 180.0f;
}

constexpr float DegToRad(float d) { return d * (kPi / 180.0f); }
constexpr float RadToDeg(float r) { return r * (180.0f / kPi); }
} // namespace lambda

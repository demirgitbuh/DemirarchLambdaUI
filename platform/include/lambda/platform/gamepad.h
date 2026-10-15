// SPDX-License-Identifier: MIT
// SDL3 tabanlı kontrolcü yöneticisi.
//  - Algılama ve hot-plug
//  - Birden çok kontrolcüde "ilk onay veren aktif olur" ve sonradan cihaz değiştirme
//  - Yalnızca aktif cihazın girdisi okunur: aynı fiziksel cihaz hem doğrudan hem Steam
//    sanal aygıtı olarak görünse bile çift girdi oluşmaz
//  - Yetenekler (titreşim, tetik titreşimi, LED, jiroskop, touchpad, arka tuşlar, pil)
//    SDL'in bildirdiği şekilde raporlanır; bilinmeyen değerler uydurulmaz
//  - Shutdown(): klasik moda geçişte tüm cihazlar kapatılır ve alt sistem tamamen durur
#pragma once

#include "lambda/core/math.h"
#include "lambda/core/resolver.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace lambda::platform
{
using DeviceId = std::uint32_t;

enum class PadFamily : std::uint8_t
{
	Unknown,
	Xbox360,
	XboxOne, // One / Series / Elite (SDL aynı türü bildirir)
	PS3,
	PS4,
	PS5,
	SteamDeck,
	SteamVirtual, // Steam Input sanal gamepad'i; gerçek model bilinmeyebilir
	Nintendo,
	Generic,
};

enum class GlyphFamily : std::uint8_t
{
	Xbox,
	PlayStation,
	SteamDeck,
	Generic,
};

enum class Connection : std::uint8_t
{
	Unknown,
	Wired,
	Wireless,
};

enum class PowerState : std::uint8_t
{
	Unknown,   // bilgi yok — kullanıcıya değer uydurulmaz
	Wired,     // pil yok / kablolu güç
	OnBattery,
	Charging,
	Charged,
};

struct Capabilities
{
	bool rumble = false;
	bool trigger_rumble = false; // Xbox One/Series impulse tetikler
	bool rgb_led = false;
	bool player_led = false;
	bool gyro = false;
	bool accel = false;
	int touchpads = 0;
	bool paddles = false;
	bool ds5_effects = false; // DualSense uyarlanabilir tetik efekt paketi (SDL_SendGamepadEffect)
};

struct DeviceInfo
{
	DeviceId id = 0;
	std::string name;
	std::uint16_t vendor = 0;
	std::uint16_t product = 0;
	PadFamily family = PadFamily::Unknown;
	PadFamily reported_real_family = PadFamily::Unknown; // Steam sanal aygıtta SDL'in bildirdiği gerçek tür
	bool steam_virtual = false;
	bool steam_input_handle = false; // Steam Input bu cihazı yönetiyor
	Connection connection = Connection::Unknown;
	Capabilities caps;
	PowerState power = PowerState::Unknown;
	int battery_percent = -1; // -1 = bilinmiyor
};

struct TouchFinger
{
	bool down = false;
	float x = 0.0f; // 0..1
	float y = 0.0f; // 0..1
	float pressure = 0.0f;
};

struct PadState
{
	ButtonSet buttons;     // tetikler eşikli dijital olarak dahil
	Vec2 left{};           // -1..1, y aşağı +
	Vec2 right{};
	float left_trigger = 0.0f;  // 0..1
	float right_trigger = 0.0f;
	bool gyro_valid = false;
	Vec3 gyro{};  // rad/sn (SDL: x pitch, y yaw, z roll)
	bool accel_valid = false;
	Vec3 accel{}; // m/sn^2
	TouchFinger touch[2];
};

enum class DeviceEventType : std::uint8_t
{
	Added,
	Removed,
	ActiveLost,    // aktif cihaz koptu (tek oyunculuda oyun duraklatılmalı)
	ActiveChanged, // yeni aktif cihaz seçildi
	BatteryLow,
};

struct DeviceEvent
{
	DeviceEventType type;
	DeviceId id;
};

GlyphFamily GlyphsFor(const DeviceInfo& info, bool running_on_steam_deck);
const char* PadFamilyName(PadFamily f);
const char* ConnectionName(Connection c);

class GamepadManager
{
public:
	GamepadManager() = default;
	~GamepadManager();
	GamepadManager(const GamepadManager&) = delete;
	GamepadManager& operator=(const GamepadManager&) = delete;

	bool Init(std::string* error = nullptr);
	void Shutdown();
	bool Initialized() const { return initialized_; }

	// SDL3 olay kuyruğunu işler (motorun SDL2 kuyruğuna dokunmaz).
	void Pump(std::vector<DeviceEvent>& events);

	const std::vector<DeviceInfo>& Devices() const { return devices_; }
	const DeviceInfo* Find(DeviceId id) const;
	std::optional<DeviceId> Active() const { return active_; }
	void SetActive(DeviceId id);
	void ClearActive();
	// Aktif cihaz yokken: Güney (A/çarpı) ya da Start'a ilk basan cihaz aktif olur.
	void SetAutoSelect(bool enabled) { auto_select_ = enabled; }

	bool ReadState(DeviceId id, PadState& out, float trigger_threshold = 0.35f);
	bool ReadActive(PadState& out, float trigger_threshold = 0.35f);

	bool Rumble(DeviceId id, float low, float high, std::uint32_t ms);
	bool RumbleTriggers(DeviceId id, float left, float right, std::uint32_t ms);
	bool SetLed(DeviceId id, std::uint8_t r, std::uint8_t g, std::uint8_t b);
	bool SendEffect(DeviceId id, const void* data, int size);
	bool SetSensors(DeviceId id, bool enabled);
	// Tüm cihazlarda titreşimi, tetik titreşimini ve LED'i sıfırlar (kopma/çıkış/klasik mod).
	void StopAllEffects();

	// Steam Input ile doğrudan cihaz aynı anda görünüyorsa true (kurulum yönergesi gösterilir).
	bool SteamInputOverlapSuspected() const;

private:
	struct Open;
	void AddDevice(DeviceId id, std::vector<DeviceEvent>& events);
	void RemoveDevice(DeviceId id, std::vector<DeviceEvent>& events);
	void RefreshInfo(DeviceInfo& info);
	Open* FindOpen(DeviceId id);

	bool initialized_ = false;
	bool auto_select_ = true;
	std::vector<DeviceInfo> devices_;
	std::vector<Open*> open_;
	std::optional<DeviceId> active_;
};
} // namespace lambda::platform

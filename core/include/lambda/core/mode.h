// SPDX-License-Identifier: MIT
// Gamepad / klasik mod geçişi.
//  - hlclassic: modun eklediği tüm davranışlar (UI, HUD, girdi katmanı, yardımlar, titreşim,
//    jiroskop, LED/tetik efektleri) kapanır; mod tarafından değiştirilen motor ayarları
//    günlükten geri yüklenir. Gamepad profilleri silinmez.
//  - hlgamepad: alt sistemler kayıt sırasıyla yeniden açılır.
//  - Yeniden başlatma gerektiren adımlar raporlanır; anında geçmiş gibi gösterilmez.
#pragma once

#include "lambda/core/document.h"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace lambda
{
enum class InterfaceMode
{
	Gamepad,
	Classic,
};

std::string_view InterfaceModeId(InterfaceMode m);
std::optional<InterfaceMode> InterfaceModeFromId(std::string_view id);

class ModeSubsystem
{
public:
	virtual ~ModeSubsystem() = default;
	virtual const char* Name() const = 0;
	virtual void EnterGamepad() = 0;
	virtual void EnterClassic() = 0;
	// Geçişin bu alt sistem için tam etkili olması yeniden başlatma gerektiriyorsa
	// İngilizce açıklama döndürür.
	virtual std::optional<std::string> RestartRequirement(InterfaceMode target) const
	{
		(void)target;
		return std::nullopt;
	}
};

struct ModeTransitionReport
{
	InterfaceMode from = InterfaceMode::Gamepad;
	InterfaceMode to = InterfaceMode::Gamepad;
	bool changed = false;
	std::vector<std::string> steps;           // uygulanan alt sistemler (sırasıyla)
	std::vector<std::string> restart_notes;   // yeniden başlatma gerektiren kısımlar
};

class ModeController
{
public:
	explicit ModeController(InterfaceMode initial = InterfaceMode::Gamepad) : mode_(initial) {}

	InterfaceMode Current() const { return mode_; }
	void Register(ModeSubsystem* subsystem);
	// Başlangıçta kayıtlı moda göre alt sistemleri hazırlar.
	ModeTransitionReport Apply();
	ModeTransitionReport SwitchTo(InterfaceMode target);

private:
	ModeTransitionReport Run(InterfaceMode from, InterfaceMode to, bool changed);
	InterfaceMode mode_;
	std::vector<ModeSubsystem*> subsystems_;
};

// Modun değiştirdiği dış ayarların (motor cvar'ları gibi) özgün değerlerini tutar.
// Diskte saklanır; oyun çökse bile bir sonraki açılışta geri yükleme yapılabilir.
class SettingJournal
{
public:
	// name için özgün değer daha önce kaydedilmediyse kaydeder.
	void RecordOriginal(const std::string& name, const std::string& original_value);
	bool Has(const std::string& name) const { return originals_.count(name) != 0; }
	const std::map<std::string, std::string>& Originals() const { return originals_; }
	// Her kayıt için apply(name, original) çağırır ve günlüğü boşaltır.
	void RestoreAll(const std::function<void(const std::string&, const std::string&)>& apply);
	bool Empty() const { return originals_.empty(); }

	void WriteTo(SettingsDocument& doc) const;
	void ReadFrom(const SettingsDocument& doc);

private:
	std::map<std::string, std::string> originals_;
};
} // namespace lambda

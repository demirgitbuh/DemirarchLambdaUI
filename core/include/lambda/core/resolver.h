// SPDX-License-Identifier: MIT
// Tuş durumlarını zamanla birlikte eylem olaylarına çevirir:
//  - Tek basış, basılı tutma, çift basma, değiştirici kombinasyonları
//  - Bağlam yığını (en özel bağlam önce): ör. [Water, Gameplay]
//  - Aç/kapa (toggle) eylemleri ve yalnızca tek oyunculuda açılan otomatik tekrar
//  - ReleaseAll: bağlantı kopması, menü açılması, mod geçişinde takılı girdi bırakmaz
//
// Belirsizlik kuralları (belgelenmiş davranış):
//  - Bir tuşa hem Press hem Hold atanmışsa Press, bırakıldığında "dokunma" olarak tetiklenir.
//  - Bir tuşa hem Press hem DoublePress atanmışsa Press, çift basma penceresi dolunca tetiklenir.
//  - Değiştirici olarak kullanılan bir tuşun kendi Press eylemi, kombinasyon kullanılmadan
//    bırakılırsa tetiklenir.
//  - Yalnızca Press atanmış tuşlar gecikmesiz tetiklenir (varsayılan düzen böyledir).
#pragma once

#include "lambda/core/bindings.h"

#include <array>
#include <bitset>
#include <vector>

namespace lambda
{
using ButtonSet = std::bitset<kPadButtonCount>;

struct ResolverTiming
{
	float hold_time = 0.35f;
	float double_press_window = 0.25f;

	void Sanitize();
};

struct ActionEvent
{
	Action action = Action::None;
	bool begin = false;

	bool operator==(const ActionEvent& o) const { return action == o.action && begin == o.begin; }
};

class BindingResolver
{
public:
	void SetBindings(const BindingSet& set);
	// En özel bağlam önce. Boş yığın = hiçbir eylem üretilmez.
	void SetContexts(std::vector<BindContext> stack);
	const std::vector<BindContext>& Contexts() const { return contexts_; }
	void SetTiming(const ResolverTiming& timing);
	void SetToggle(Action action, bool toggle);
	// rate_hz > 0: eylem basılı tutuldukça bu sıklıkta bırak/bas üretir. 0 = kapalı.
	void SetAutoRepeat(Action action, float rate_hz);
	void ClearAutoRepeat();

	void Update(const ButtonSet& down, double now, std::vector<ActionEvent>& out);
	// Tüm etkin eylemler için End üretir ve bekleyen durumları temizler.
	void ReleaseAll(std::vector<ActionEvent>& out);

	bool IsActive(Action action) const;
	// Belirli bir eylemi şu an tetikleyen etkin bağlama (UI ipuçları için).
	std::optional<Binding> EffectiveBindingFor(Action action) const;

private:
	struct Solo
	{
		const Binding* press = nullptr;
		const Binding* hold = nullptr;
		const Binding* dbl = nullptr;
	};

	struct PerButton
	{
		bool down = false;
		double press_time = 0.0;
		bool consumed = false;
		bool modifier_used = false;
		bool hold_fired = false;
		bool pending_double = false;
		double double_deadline = 0.0;
		std::vector<Action> begun;
	};

	struct PerAction
	{
		int count = 0;
		bool toggle = false;
		bool toggled_on = false;
		float repeat_hz = 0.0f;
		bool output_on = false;
		double next_repeat = 0.0;
	};

	void Rebuild();
	Solo SoloFor(PadButton b) const;
	bool IsModifier(PadButton b) const;
	void OnPress(int b, double now, std::vector<ActionEvent>& out);
	void OnRelease(int b, double now, std::vector<ActionEvent>& out);
	void OnHeld(int b, double now, std::vector<ActionEvent>& out);
	void OnIdle(int b, double now, std::vector<ActionEvent>& out);
	void Begin(int button, Action a, double now, std::vector<ActionEvent>& out);
	void Pulse(Action a, double now, std::vector<ActionEvent>& out);
	void Inc(Action a, double now, std::vector<ActionEvent>& out);
	void Dec(Action a, std::vector<ActionEvent>& out);

	BindingSet set_;
	std::vector<BindContext> contexts_;
	std::vector<Binding> effective_;
	ResolverTiming timing_;
	std::array<PerButton, kPadButtonCount> buttons_{};
	std::array<PerAction, kActionCount> actions_{};
	std::vector<Action> pending_pulse_ends_;
};
} // namespace lambda

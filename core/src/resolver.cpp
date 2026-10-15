// SPDX-License-Identifier: MIT
#include "lambda/core/resolver.h"

#include "lambda/core/math.h"

#include <algorithm>

namespace lambda
{
void ResolverTiming::Sanitize()
{
	hold_time = std::isfinite(hold_time) ? Clamp(hold_time, 0.15f, 2.0f) : 0.35f;
	double_press_window = std::isfinite(double_press_window) ? Clamp(double_press_window, 0.1f, 1.0f) : 0.25f;
}

void BindingResolver::SetBindings(const BindingSet& set)
{
	set_ = set;
	Rebuild();
}

void BindingResolver::SetContexts(std::vector<BindContext> stack)
{
	if (stack == contexts_)
		return;
	contexts_ = std::move(stack);
	Rebuild();
}

void BindingResolver::SetTiming(const ResolverTiming& timing)
{
	timing_ = timing;
	timing_.Sanitize();
}

void BindingResolver::SetToggle(Action action, bool toggle)
{
	auto& pa = actions_[static_cast<int>(action)];
	pa.toggle = toggle;
	if (!toggle)
		pa.toggled_on = false;
}

void BindingResolver::SetAutoRepeat(Action action, float rate_hz)
{
	actions_[static_cast<int>(action)].repeat_hz = (std::isfinite(rate_hz) && rate_hz > 0.0f) ? Clamp(rate_hz, 1.0f, 30.0f) : 0.0f;
}

void BindingResolver::ClearAutoRepeat()
{
	for (auto& pa : actions_)
		pa.repeat_hz = 0.0f;
}

void BindingResolver::Rebuild()
{
	// Her yuva için yığındaki ilk (en özel) bağlamın ataması geçerlidir.
	effective_.clear();
	for (BindContext ctx : contexts_)
	{
		for (const Binding& b : set_.All())
		{
			if (b.context != ctx)
				continue;
			const bool shadowed = std::any_of(effective_.begin(), effective_.end(), [&](const Binding& e) {
				return e.trigger == b.trigger && e.input == b.input && e.modifier == b.modifier;
			});
			if (!shadowed)
				effective_.push_back(b);
		}
	}
	// Action::None atamaları yalnızca gölgeleme içindir.
	effective_.erase(std::remove_if(effective_.begin(), effective_.end(), [](const Binding& b) { return b.action == Action::None; }),
		effective_.end());
}

BindingResolver::Solo BindingResolver::SoloFor(PadButton b) const
{
	Solo s;
	for (const Binding& e : effective_)
	{
		if (e.input != b || e.modifier)
			continue;
		switch (e.trigger)
		{
		case Trigger::Press: s.press = &e; break;
		case Trigger::Hold: s.hold = &e; break;
		case Trigger::DoublePress: s.dbl = &e; break;
		default: break;
		}
	}
	return s;
}

bool BindingResolver::IsModifier(PadButton b) const
{
	return std::any_of(effective_.begin(), effective_.end(), [&](const Binding& e) { return e.modifier && *e.modifier == b; });
}

void BindingResolver::Inc(Action a, double now, std::vector<ActionEvent>& out)
{
	auto& pa = actions_[static_cast<int>(a)];
	if (pa.count++ > 0)
		return;
	if (pa.toggle)
	{
		pa.toggled_on = !pa.toggled_on;
		out.push_back({a, pa.toggled_on});
		return;
	}
	out.push_back({a, true});
	pa.output_on = true;
	if (pa.repeat_hz > 0.0f)
		pa.next_repeat = now + 0.5 / pa.repeat_hz;
}

void BindingResolver::Dec(Action a, std::vector<ActionEvent>& out)
{
	auto& pa = actions_[static_cast<int>(a)];
	if (pa.count <= 0)
		return;
	if (--pa.count > 0)
		return;
	if (pa.toggle)
		return; // aç/kapa: bırakma durumu değiştirmez
	if (pa.output_on)
		out.push_back({a, false});
	pa.output_on = false;
}

void BindingResolver::Begin(int button, Action a, double now, std::vector<ActionEvent>& out)
{
	buttons_[button].begun.push_back(a);
	Inc(a, now, out);
}

void BindingResolver::Pulse(Action a, double now, std::vector<ActionEvent>& out)
{
	// Motor komutlarının (+jump vb.) en az bir karede etkin görünmesi için End bir sonraki
	// Update çağrısında üretilir.
	Inc(a, now, out);
	pending_pulse_ends_.push_back(a);
}

void BindingResolver::OnPress(int bi, double now, std::vector<ActionEvent>& out)
{
	PerButton& st = buttons_[bi];
	const PadButton b = static_cast<PadButton>(bi);
	st.down = true;
	st.press_time = now;
	st.consumed = false;
	st.modifier_used = false;
	st.hold_fired = false;

	// Değiştirici kombinasyonları önceliklidir.
	for (const Binding& e : effective_)
	{
		if (e.input != b || !e.modifier || e.trigger != Trigger::Press)
			continue;
		PerButton& mod = buttons_[static_cast<int>(*e.modifier)];
		if (!mod.down)
			continue;
		mod.modifier_used = true;
		st.consumed = true;
		st.pending_double = false;
		Begin(bi, e.action, now, out);
		return;
	}

	const Solo solo = SoloFor(b);
	if (st.pending_double)
	{
		st.pending_double = false;
		if (solo.dbl && now <= st.double_deadline)
		{
			st.consumed = true;
			Pulse(solo.dbl->action, now, out);
			return;
		}
	}

	const bool deferred = IsModifier(b) || solo.hold || solo.dbl;
	if (!deferred && solo.press)
		Begin(bi, solo.press->action, now, out);
}

void BindingResolver::OnHeld(int bi, double now, std::vector<ActionEvent>& out)
{
	PerButton& st = buttons_[bi];
	if (st.consumed || st.hold_fired || st.modifier_used || !st.begun.empty())
		return;
	const PadButton b = static_cast<PadButton>(bi);
	const Solo solo = SoloFor(b);
	const double held = now - st.press_time;
	if (solo.hold && held >= timing_.hold_time)
	{
		st.hold_fired = true;
		Begin(bi, solo.hold->action, now, out);
		return;
	}
	// Press + DoublePress (Hold yok): pencereden uzun basılırsa Press basılı tutulmuş sayılır.
	if (!solo.hold && solo.dbl && solo.press && !IsModifier(b) && held >= timing_.double_press_window)
		Begin(bi, solo.press->action, now, out);
}

void BindingResolver::OnRelease(int bi, double now, std::vector<ActionEvent>& out)
{
	PerButton& st = buttons_[bi];
	st.down = false;
	const bool had_begun = !st.begun.empty();
	for (Action a : st.begun)
		Dec(a, out);
	st.begun.clear();

	if (st.consumed || st.hold_fired || st.modifier_used || had_begun)
		return;

	const Solo solo = SoloFor(static_cast<PadButton>(bi));
	if (solo.dbl)
	{
		st.pending_double = true;
		st.double_deadline = now + timing_.double_press_window;
		return;
	}
	if (solo.press)
		Pulse(solo.press->action, now, out);
}

void BindingResolver::OnIdle(int bi, double now, std::vector<ActionEvent>& out)
{
	PerButton& st = buttons_[bi];
	if (!st.pending_double || now <= st.double_deadline)
		return;
	st.pending_double = false;
	if (const Solo solo = SoloFor(static_cast<PadButton>(bi)); solo.press)
		Pulse(solo.press->action, now, out);
}

void BindingResolver::Update(const ButtonSet& down, double now, std::vector<ActionEvent>& out)
{
	if (!pending_pulse_ends_.empty())
	{
		auto ends = std::move(pending_pulse_ends_);
		pending_pulse_ends_.clear();
		for (Action a : ends)
			Dec(a, out);
	}

	for (int i = 0; i < kPadButtonCount; ++i)
	{
		const bool cur = down.test(static_cast<size_t>(i));
		const bool prev = buttons_[i].down;
		if (cur && !prev)
			OnPress(i, now, out);
		else if (!cur && prev)
			OnRelease(i, now, out);
		else if (cur)
			OnHeld(i, now, out);
		else
			OnIdle(i, now, out);
	}

	// Otomatik tekrar (yalnızca köprü katmanı tek oyunculuda etkinleştirir).
	for (int i = 0; i < kActionCount; ++i)
	{
		PerAction& pa = actions_[i];
		if (pa.repeat_hz <= 0.0f || pa.toggle || pa.count <= 0 || now < pa.next_repeat)
			continue;
		pa.output_on = !pa.output_on;
		out.push_back({static_cast<Action>(i), pa.output_on});
		pa.next_repeat = now + 0.5 / pa.repeat_hz;
	}
}

void BindingResolver::ReleaseAll(std::vector<ActionEvent>& out)
{
	for (int i = 0; i < kActionCount; ++i)
	{
		PerAction& pa = actions_[i];
		const Action a = static_cast<Action>(i);
		if (pa.toggle)
		{
			if (pa.toggled_on)
				out.push_back({a, false});
			pa.toggled_on = false;
		}
		else if (pa.output_on)
		{
			out.push_back({a, false});
		}
		pa.count = 0;
		pa.output_on = false;
	}
	for (auto& st : buttons_)
	{
		const bool was_down = st.down;
		st = PerButton{};
		// Basılı tuş yeniden basılana kadar yok sayılsın diye "down" korunur ve tüketilmiş sayılır.
		st.down = was_down;
		st.consumed = was_down;
	}
	pending_pulse_ends_.clear();
}

bool BindingResolver::IsActive(Action action) const
{
	const PerAction& pa = actions_[static_cast<int>(action)];
	return pa.toggle ? pa.toggled_on : pa.count > 0;
}

std::optional<Binding> BindingResolver::EffectiveBindingFor(Action action) const
{
	// Tercih sırası: değiştiricisiz Press > Hold > DoublePress > kombinasyon
	const Binding* best = nullptr;
	auto rank = [](const Binding& b) { return (b.modifier ? 10 : 0) + static_cast<int>(b.trigger); };
	for (const Binding& e : effective_)
		if (e.action == action && (!best || rank(e) < rank(*best)))
			best = &e;
	if (!best)
		return std::nullopt;
	return *best;
}
} // namespace lambda

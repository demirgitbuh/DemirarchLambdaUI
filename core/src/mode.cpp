// SPDX-License-Identifier: MIT
#include "lambda/core/mode.h"

#include <algorithm>

namespace lambda
{
std::string_view InterfaceModeId(InterfaceMode m) { return m == InterfaceMode::Gamepad ? "gamepad" : "classic"; }

std::optional<InterfaceMode> InterfaceModeFromId(std::string_view id)
{
	if (id == "gamepad")
		return InterfaceMode::Gamepad;
	if (id == "classic")
		return InterfaceMode::Classic;
	return std::nullopt;
}

void ModeController::Register(ModeSubsystem* subsystem)
{
	if (subsystem && std::find(subsystems_.begin(), subsystems_.end(), subsystem) == subsystems_.end())
		subsystems_.push_back(subsystem);
}

ModeTransitionReport ModeController::Apply() { return Run(mode_, mode_, false); }

ModeTransitionReport ModeController::SwitchTo(InterfaceMode target)
{
	if (target == mode_)
	{
		ModeTransitionReport r;
		r.from = r.to = mode_;
		return r;
	}
	const InterfaceMode from = mode_;
	mode_ = target;
	return Run(from, target, true);
}

ModeTransitionReport ModeController::Run(InterfaceMode from, InterfaceMode to, bool changed)
{
	ModeTransitionReport r;
	r.from = from;
	r.to = to;
	r.changed = changed;
	if (to == InterfaceMode::Gamepad)
	{
		for (ModeSubsystem* s : subsystems_)
		{
			s->EnterGamepad();
			r.steps.emplace_back(s->Name());
		}
	}
	else
	{
		// Kapatma ters sırada: önce girdi bırakılır, en son UI/çekirdek kapanır.
		for (auto it = subsystems_.rbegin(); it != subsystems_.rend(); ++it)
		{
			(*it)->EnterClassic();
			r.steps.emplace_back((*it)->Name());
		}
	}
	if (changed)
		for (ModeSubsystem* s : subsystems_)
			if (auto note = s->RestartRequirement(to))
				r.restart_notes.push_back(*note);
	return r;
}

void SettingJournal::RecordOriginal(const std::string& name, const std::string& original_value)
{
	originals_.emplace(name, original_value);
}

void SettingJournal::RestoreAll(const std::function<void(const std::string&, const std::string&)>& apply)
{
	for (const auto& [name, value] : originals_)
		apply(name, value);
	originals_.clear();
}

void SettingJournal::WriteTo(SettingsDocument& doc) const
{
	doc.RemoveSection("journal");
	for (const auto& [name, value] : originals_)
		doc.Set("journal", name, value);
}

void SettingJournal::ReadFrom(const SettingsDocument& doc)
{
	originals_.clear();
	if (const auto* s = doc.GetSection("journal"))
		for (const auto& [k, v] : s->entries)
			originals_.emplace(k, v);
}
} // namespace lambda

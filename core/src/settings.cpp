// SPDX-License-Identifier: MIT
#include "lambda/core/settings.h"

namespace lambda
{
namespace
{
void WriteStick(SettingsDocument& d, const char* sec, const StickSettings& s)
{
	d.SetFloat(sec, "inner_deadzone", s.inner_deadzone);
	d.SetFloat(sec, "outer_deadzone", s.outer_deadzone);
	d.SetInt(sec, "curve", static_cast<int>(s.curve));
	d.SetFloat(sec, "curve_exponent", s.curve_exponent);
	d.SetFloat(sec, "sensitivity_x", s.sensitivity_x);
	d.SetFloat(sec, "sensitivity_y", s.sensitivity_y);
	d.SetBool(sec, "invert_x", s.invert_x);
	d.SetBool(sec, "invert_y", s.invert_y);
	d.SetBool(sec, "calibration_valid", s.calibration.valid);
	d.SetFloat(sec, "calibration_x", s.calibration.center_offset.x);
	d.SetFloat(sec, "calibration_y", s.calibration.center_offset.y);
	d.SetFloat(sec, "calibration_noise", s.calibration.noise_radius);
}

StickSettings ReadStick(const SettingsDocument& d, const char* sec, const StickSettings& def)
{
	StickSettings s;
	s.inner_deadzone = d.GetFloat(sec, "inner_deadzone", def.inner_deadzone);
	s.outer_deadzone = d.GetFloat(sec, "outer_deadzone", def.outer_deadzone);
	s.curve = static_cast<ResponseCurve>(Clamp(d.GetInt(sec, "curve", static_cast<int>(def.curve)), 0, static_cast<int>(ResponseCurve::SCurve)));
	s.curve_exponent = d.GetFloat(sec, "curve_exponent", def.curve_exponent);
	s.sensitivity_x = d.GetFloat(sec, "sensitivity_x", def.sensitivity_x);
	s.sensitivity_y = d.GetFloat(sec, "sensitivity_y", def.sensitivity_y);
	s.invert_x = d.GetBool(sec, "invert_x", def.invert_x);
	s.invert_y = d.GetBool(sec, "invert_y", def.invert_y);
	s.calibration.valid = d.GetBool(sec, "calibration_valid", false);
	s.calibration.center_offset.x = d.GetFloat(sec, "calibration_x", 0.0f);
	s.calibration.center_offset.y = d.GetFloat(sec, "calibration_y", 0.0f);
	s.calibration.noise_radius = Clamp(d.GetFloat(sec, "calibration_noise", 0.0f), 0.0f, 0.3f);
	s.Sanitize();
	return s;
}
} // namespace

void MovementSettings::Sanitize()
{
	turn_speed = std::isfinite(turn_speed) ? Clamp(turn_speed, 30.0f, 720.0f) : 160.0f;
}

ControllerSettings ControllerSettings::Defaults(GameId game)
{
	ControllerSettings s;
	// Sağ çubuk nişan için biraz daha küçük iç ölü bölge ve hafif eğri.
	s.right_stick.inner_deadzone = 0.10f;
	s.right_stick.curve = ResponseCurve::Power;
	s.right_stick.curve_exponent = 1.6f;
	s.bindings = DefaultBindings(game);
	return s;
}

void ControllerSettings::Sanitize()
{
	left_stick.Sanitize();
	right_stick.Sanitize();
	camera.Sanitize();
	movement.Sanitize();
	timing.Sanitize();
}

void WriteControllerSettings(const ControllerSettings& s, SettingsDocument& d)
{
	WriteStick(d, "stick.left", s.left_stick);
	WriteStick(d, "stick.right", s.right_stick);

	d.SetFloat("camera", "yaw_speed", s.camera.yaw_speed);
	d.SetFloat("camera", "pitch_speed", s.camera.pitch_speed);
	d.SetFloat("camera", "accel_boost", s.camera.accel_boost);
	d.SetFloat("camera", "accel_threshold", s.camera.accel_threshold);
	d.SetFloat("camera", "accel_time", s.camera.accel_time);
	d.SetFloat("camera", "smoothing", s.camera.smoothing);
	d.SetFloat("camera", "max_turn_rate", s.camera.max_turn_rate);

	d.SetBool("movement", "swap_sticks", s.movement.swap_sticks);
	d.SetBool("movement", "movement_stick_turns", s.movement.movement_stick_turns);
	d.SetFloat("movement", "turn_speed", s.movement.turn_speed);

	d.SetFloat("timing", "hold_time", s.timing.hold_time);
	d.SetFloat("timing", "double_press_window", s.timing.double_press_window);

	d.RemoveSection("toggles");
	for (int i = 1; i < kActionCount; ++i)
	{
		const ActionInfo& info = GetActionInfo(static_cast<Action>(i));
		if (info.supports_toggle)
			d.SetBool("toggles", info.id, s.toggle[i]);
	}

	d.RemoveSection("bindings");
	for (const Binding& b : s.bindings.All())
		d.Set("bindings", BindingSet::SlotKey(b), GetActionInfo(b.action).id);
}

ControllerSettings ReadControllerSettings(const SettingsDocument& d, GameId game, std::vector<std::string>* warnings)
{
	const ControllerSettings def = ControllerSettings::Defaults(game);
	ControllerSettings s = def;
	s.left_stick = ReadStick(d, "stick.left", def.left_stick);
	s.right_stick = ReadStick(d, "stick.right", def.right_stick);

	s.camera.yaw_speed = d.GetFloat("camera", "yaw_speed", def.camera.yaw_speed);
	s.camera.pitch_speed = d.GetFloat("camera", "pitch_speed", def.camera.pitch_speed);
	s.camera.accel_boost = d.GetFloat("camera", "accel_boost", def.camera.accel_boost);
	s.camera.accel_threshold = d.GetFloat("camera", "accel_threshold", def.camera.accel_threshold);
	s.camera.accel_time = d.GetFloat("camera", "accel_time", def.camera.accel_time);
	s.camera.smoothing = d.GetFloat("camera", "smoothing", def.camera.smoothing);
	s.camera.max_turn_rate = d.GetFloat("camera", "max_turn_rate", def.camera.max_turn_rate);

	s.movement.swap_sticks = d.GetBool("movement", "swap_sticks", def.movement.swap_sticks);
	s.movement.movement_stick_turns = d.GetBool("movement", "movement_stick_turns", def.movement.movement_stick_turns);
	s.movement.turn_speed = d.GetFloat("movement", "turn_speed", def.movement.turn_speed);

	s.timing.hold_time = d.GetFloat("timing", "hold_time", def.timing.hold_time);
	s.timing.double_press_window = d.GetFloat("timing", "double_press_window", def.timing.double_press_window);

	for (int i = 1; i < kActionCount; ++i)
	{
		const ActionInfo& info = GetActionInfo(static_cast<Action>(i));
		if (info.supports_toggle)
			s.toggle[i] = d.GetBool("toggles", info.id, false);
	}

	if (const auto* sec = d.GetSection("bindings"))
	{
		BindingSet loaded;
		for (const auto& [k, v] : sec->entries)
		{
			if (auto b = BindingSet::FromKeyValue(k, v))
				loaded.Set(*b);
			else if (warnings)
				warnings->push_back("Ignored unknown binding \"" + k + " = " + v + "\".");
		}
		// Kullanılamaz bir düzen kaydedilmiş olsa bile kullanıcı kilitlenmesin:
		// zorunlu eylemler eksikse varsayılan düzen kullanılır, kayıtlı düzen dosyada kalır.
		if (loaded.HasBlockingIssues())
		{
			if (warnings)
				warnings->push_back("The saved button layout is incomplete or has conflicts. Default controls are active until it is fixed.");
		}
		else
		{
			s.bindings = loaded;
		}
	}

	s.Sanitize();
	return s;
}

void ResetGroup(ControllerSettings& s, SettingsGroup group, GameId game)
{
	const ControllerSettings def = ControllerSettings::Defaults(game);
	switch (group)
	{
	case SettingsGroup::Sticks:
		s.left_stick = def.left_stick;
		s.right_stick = def.right_stick;
		break;
	case SettingsGroup::Camera: s.camera = def.camera; break;
	case SettingsGroup::Movement: s.movement = def.movement; break;
	case SettingsGroup::Timing: s.timing = def.timing; break;
	case SettingsGroup::Toggles: s.toggle = def.toggle; break;
	case SettingsGroup::Bindings: s.bindings = def.bindings; break;
	case SettingsGroup::All: s = def; break;
	}
}
} // namespace lambda

// SPDX-License-Identifier: MIT
#include "lambda/core/recovery.h"

#include "lambda/core/math.h"

namespace lambda
{
bool RecoveryCombo::Update(const ButtonSet& down, double now)
{
	const bool combo = down.test(static_cast<size_t>(PadButton::Start)) && down.test(static_cast<size_t>(PadButton::Back));
	if (!combo)
	{
		holding_ = false;
		fired_ = false;
		return false;
	}
	if (!holding_)
	{
		holding_ = true;
		start_ = now;
	}
	if (!fired_ && now - start_ >= kHoldSeconds)
	{
		fired_ = true;
		return true;
	}
	return false;
}

float RecoveryCombo::Progress(double now) const
{
	if (!holding_ || fired_)
		return fired_ ? 1.0f : 0.0f;
	return Clamp(static_cast<float>((now - start_) / kHoldSeconds), 0.0f, 1.0f);
}
} // namespace lambda

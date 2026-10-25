#pragma once

namespace vega
{
	struct ModuleCooldownRecoveryRule
	{
		float bonus = 0.f;
		float damageFreeSeconds = 0.f;
		bool perEmptyAbilitySlot = false;
	};
	struct ModuleDeferredDamageRule
	{
		float fraction = 0.f;
		float seconds = 0.f;
	};
	struct ModuleMomentumExtension
	{
		float seconds = 0.f;
		float maximumSeconds = 0.f;
	};
}

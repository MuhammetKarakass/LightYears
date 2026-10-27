#pragma once

#include "framework/Core.h"
#include "abilities/AbilityTypes.h"
#include "abilities/AbilityHandle.h"

namespace vega
{
	// Read-only view of which module systems a ship currently has enabled. The
	// ability system asks it instead of depending on the player ship class; a
	// component without a gate (non-player owners) treats every system as enabled.
	class ModuleSystemGate
	{
	public:
		virtual ~ModuleSystemGate() = default;
		virtual float GetAbilityMainEffectMultiplier(const List<GameplayTag>&) const { return 1.f; }
		virtual float GetAbilitySlotMainEffectMultiplier(sas::AbilitySlot) const { return 1.f; }
		virtual float GetAbilityGeneralEffectMultiplier(const List<GameplayTag>&) const { return 1.f; }
		virtual float GetPrimedArsenalMultiplier(sas::AbilityHandle) const { return 1.f; }
		// Loadout-composition power of the ability's Q/E/F/R set (Four Corners).
		virtual float GetAbilitySetMainEffectMultiplier(sas::AbilityHandle) const { return 1.f; }
		virtual void CommitPrimedArsenalUse(sas::AbilityHandle) {}
		virtual void RefreshPoolConversion() {}
		virtual float GetAbilitySlotCooldownMultiplier(sas::AbilitySlot) const { return 1.f; }
		virtual bool IsModuleSystemEnabled(const GameplayTag& systemTag) const = 0;
	};
}

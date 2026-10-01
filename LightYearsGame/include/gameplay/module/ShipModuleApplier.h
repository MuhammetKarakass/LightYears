#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/ship/ShipRuntime.h"

#include <cstddef>
#include <cstdint>

namespace ly
{
	class LightYearsAbilitySystemComponent;

	// Turns an installed loadout into live ship state. It owns every handle it
	// creates and always reapplies from scratch, so no separate add/remove path
	// can drift. It keeps no pointer to the loadout or the rules: the caller
	// pushes both on each Apply, so a deleted Player can never dangle here. The
	// referenced runtimes belong to the same ship that owns the applier.
	class ShipModuleApplier
	{
	public:
		ShipModuleApplier(ShipRuntime& shipRuntime, LightYearsAbilitySystemComponent& abilities);
		ShipModuleApplier(const ShipModuleApplier&) = delete;
		ShipModuleApplier& operator=(const ShipModuleApplier&) = delete;

		// Reapplies when this is the first call or the loadout revision changed.
		// Returns whether a reapply happened.
		bool Apply(const ModuleLoadout& loadout, const ModuleRulesContext& rules);

		const List<GameplayTag>& GetEnabledSystems() const { return mEnabledSystems; }
		bool IsSystemEnabled(const GameplayTag& systemTag) const;
		const ResolvedModuleEffects& GetResolvedEffects() const { return mResolved; }
		bool HasApplied() const { return mHasApplied; }
		std::uint64_t GetAppliedRevision() const { return mAppliedRevision; }
		std::size_t GetOwnedHandleCount() const
		{
			return mOwnerHandles.size() + mShipHandles.size() + mScopedRuleHandles.size();
		}

	private:
		void RemoveAll();

		ShipRuntime& mShipRuntime;
		LightYearsAbilitySystemComponent& mAbilities;
		List<sas::AttributeModifierHandle> mOwnerHandles;
		List<sas::AttributeModifierHandle> mShipHandles;
		List<std::size_t> mScopedRuleHandles;
		ResolvedModuleEffects mResolved;
		List<GameplayTag> mEnabledSystems;
		std::uint64_t mAppliedRevision = 0;
		bool mHasApplied = false;
	};
}

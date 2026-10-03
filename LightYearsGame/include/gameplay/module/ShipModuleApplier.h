#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/ModuleSystemGate.h"
#include "gameplay/ship/ShipRuntime.h"

#include <cstddef>
#include <cstdint>

namespace ly
{
	class LightYearsAbilitySystemComponent;
	class ModuleTriggerRuntime;

	// Turns an installed loadout into live ship state. It owns every handle it
	// creates and rebuilds from the full desired state on each change. It keeps no
	// pointer to the loadout or the rules: the caller pushes both on each Apply and
	// the applier stores value copies of what a later reboot expiry needs (the
	// placements, schematic and zone table), so a deleted Player can never dangle
	// here. The only retained callable is the module lookup, which must stay valid
	// for the life of the ship (the content catalog lookup is). The referenced
	// runtimes belong to the same ship that owns the applier.
	//
	// Reboot: while the ship is in combat, a newly seen placement instanceId is held
	// out of the resolved effects for the zone table's rebootDurationSeconds.
	class ShipModuleApplier final : public ModuleSystemGate
	{
	public:
		struct RebootEntry
		{
			std::uint32_t instanceId = 0;
			float remainingSeconds = 0.f;
		};

		ShipModuleApplier(ShipRuntime& shipRuntime, LightYearsAbilitySystemComponent& abilities, ModuleTriggerRuntime* triggerRuntime = nullptr);
		~ShipModuleApplier() override;
		ShipModuleApplier(const ShipModuleApplier&) = delete;
		ShipModuleApplier& operator=(const ShipModuleApplier&) = delete;

		// Reapplies when this is the first call or the loadout revision changed.
		// inCombat decides whether placements new since the last apply reboot; the first
		// apply is always immediate. Returns whether a reapply happened.
		bool Apply(const ModuleLoadout& loadout, const ModuleRulesContext& rules, bool inCombat = false);
		// Counts reboots down and reapplies the stored snapshot when one finishes.
		void Tick(float deltaSeconds);

		bool IsRebooting(std::uint32_t instanceId) const;
		// 0 when the placement is not rebooting.
		float GetRebootRemainingSeconds(std::uint32_t instanceId) const;
		const List<RebootEntry>& GetReboots() const { return mReboots; }

		const List<GameplayTag>& GetEnabledSystems() const { return mEnabledSystems; }
		bool IsSystemEnabled(const GameplayTag& systemTag) const;
		// ModuleSystemGate: a ship that never had a loadout applied is unmanaged and
		// keeps every system enabled.
		bool IsModuleSystemEnabled(const GameplayTag& systemTag) const override { return !mHasApplied || IsSystemEnabled(systemTag); }
		// Presentation read: a ship that never had a loadout applied (no schematic)
		// is unmanaged and keeps every readout, so legacy flows behave as before.
		bool IsReadoutEnabled(const GameplayTag& systemTag) const { return !mHasApplied || IsSystemEnabled(systemTag); }
		const ResolvedModuleEffects& GetResolvedEffects() const { return mResolved; }
		bool HasApplied() const { return mHasApplied; }
		std::uint64_t GetAppliedRevision() const { return mAppliedRevision; }
		std::size_t GetOwnedHandleCount() const
		{
			return mOwnerHandles.size() + mShipHandles.size() + mPrimaryHandles.size() + mScopedRuleHandles.size();
		}

	private:
		struct OwnedModifier
		{
			sas::AttributeModifier modifier;
			sas::AttributeModifierHandle handle;
		};

		struct OwnedPrimaryModifier
		{
			sas::AttributeModifier modifier;
			std::size_t handle = 0;
		};

		void Rebuild();
		void SyncPrimaryWeaponModifiers();
		static void SyncModifiers(
			sas::AttributeSystem& attributes,
			const List<sas::AttributeModifier>& desired,
			List<OwnedModifier>& owned
		);
		void RemoveScopedRules();

		ShipRuntime& mShipRuntime;
		LightYearsAbilitySystemComponent& mAbilities;
		// The trigger runtime is owned by the same ship and outlives this applier.
		ModuleTriggerRuntime* mTriggerRuntime = nullptr;
		List<OwnedModifier> mOwnerHandles;
		List<OwnedModifier> mShipHandles;
		List<OwnedPrimaryModifier> mPrimaryHandles;
		List<std::size_t> mScopedRuleHandles;
		ResolvedModuleEffects mResolved;
		List<GameplayTag> mEnabledSystems;

		// Last pushed snapshot, by value.
		List<ModulePlacement> mPlacements;
		ShipSchematic mSchematic;
		ModuleZoneTable mZoneTable;
		ModuleLookup mLookup;
		List<std::uint32_t> mKnownIds;
		List<RebootEntry> mReboots;

		std::uint64_t mAppliedRevision = 0;
		bool mHasApplied = false;
	};
}

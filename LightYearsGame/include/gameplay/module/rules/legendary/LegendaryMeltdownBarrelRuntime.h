#pragma once

#include "gameplay/module/rules/legendary/LegendaryMeltdownBarrelRules.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;
	struct MeltdownWeaponCycle
	{
		float heat = 0.f;
		float budget = 0.f;
		float focusRemaining = 0.f;
		float fireLockRemaining = 0.f;
		float pendingDamage = 0.f;
		bool active = true;
		bool startedDuringAbilityTick = false;
	};

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryMeltdownBarrelRuntime final
	{
	public:
		LegendaryMeltdownBarrelRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryMeltdownBarrelRuntime();
		LegendaryMeltdownBarrelRuntime(const LegendaryMeltdownBarrelRuntime&) = delete;
		LegendaryMeltdownBarrelRuntime& operator=(const LegendaryMeltdownBarrelRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryMeltdownBarrelRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryMeltdownBarrelRule>& GetRules() const { return mRules; }
		bool Supports(const PrimaryWeaponDefinition& definition) const;
		bool CanFire(const PrimaryWeaponRuntimeState& state) const;
		void RecordTrigger(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state);
		void RecordBeam(const PrimaryWeaponExecutionContext& context, float heatBandMultiplier, float deltaSeconds);
		void Overheat(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float naturalLock);
		void Cool(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float previousHeat, float newHeat);
		bool CoolMagazine(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float deltaSeconds);
		float GetHeatCapacity() const { return mRules.empty() ? 0.f : mRules.front().heatCapacity; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryMeltdownBarrelRule> mRules;
		// Shared cycle storage survives changing the selected weapon; no actor pointers are retained.
		List<std::weak_ptr<MeltdownWeaponCycle>> mCycles;
		std::shared_ptr<MeltdownWeaponCycle> EnsureCycle(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state);
		float NormalDamage(const PrimaryWeaponExecutionContext& context) const;
	};
}

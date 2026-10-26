#pragma once

#include "gameplay/module/rules/legendary/LegendarySecondHardpointRules.h"
#include "gameplay/weapon/runtime/PrimaryWeaponAbilityState.h"
#include <unordered_map>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendarySecondHardpointRuntime final
	{
	public:
		LegendarySecondHardpointRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendarySecondHardpointRuntime();
		LegendarySecondHardpointRuntime(const LegendarySecondHardpointRuntime&) = delete;
		LegendarySecondHardpointRuntime& operator=(const LegendarySecondHardpointRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendarySecondHardpointRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendarySecondHardpointRule>& GetRules() const { return mRules; }
		// Run acquisition is intentionally external. Only a catalog weapon ID can be assigned.
		bool AssignSecondWeapon(const std::string& weaponId);
		// Id of the assigned second weapon; empty when none.
		std::string GetSecondWeaponId() const { return mSecondWeapon ? mSecondWeapon->weaponId : std::string{}; }
		bool SwitchWeapon();
		bool IsSwitching() const { return mSwitchRemaining > 0.f; }
		float GetSwitchRemaining() const { return mSwitchRemaining; }
		bool IsSecondWeaponActive() const { return mSecondActive; }
		const PrimaryWeaponDefinition* GetSecondWeapon() const { return mSecondWeapon ? &*mSecondWeapon : nullptr; }
		PrimaryWeaponAbilityState& GetSecondWeaponState() { return *mSecondState; }
		std::shared_ptr<PrimaryWeaponRuntimeState> RetainSecondWeaponRuntime() const
		{
			return std::shared_ptr<PrimaryWeaponRuntimeState>{ mSecondState, &mSecondState->runtime };
		}
		float ResolveDirectHit(Actor& target, const sas::ContentId& weaponId);
		void AdvanceReserveReload(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float deltaSeconds);

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendarySecondHardpointRule> mRules;
		std::optional<PrimaryWeaponDefinition> mSecondWeapon;
		std::shared_ptr<PrimaryWeaponAbilityState> mSecondState = std::make_shared<PrimaryWeaponAbilityState>();
		bool mSecondActive = false;
		float mSwitchRemaining = 0.f;
		struct Mark { std::weak_ptr<Actor> target; float first = 0.f; float second = 0.f; };
		std::unordered_map<unsigned int, Mark> mMarks;
	};
}

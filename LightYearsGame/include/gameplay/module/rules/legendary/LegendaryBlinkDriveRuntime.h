#pragma once

#include "gameplay/module/rules/legendary/LegendaryBlinkDriveRules.h"
#include <SFML/System/Vector2.hpp>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryBlinkDriveRuntime final
	{
	public:
		LegendaryBlinkDriveRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryBlinkDriveRuntime();
		LegendaryBlinkDriveRuntime(const LegendaryBlinkDriveRuntime&) = delete;
		LegendaryBlinkDriveRuntime& operator=(const LegendaryBlinkDriveRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryBlinkDriveRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryBlinkDriveRule>& GetRules() const { return mRules; }
		bool IsInstalled() const { return !mRules.empty(); }
		bool TryBlink(const sf::Vector2f& worldInput);
		int GetCharges() const { return mCharges; }
		int GetMaximumCharges() const { return IsInstalled() ? mRules.front().charges : 0; }
		float GetRechargeRemaining() const { return mRechargeRemaining; }
		float GetUseCooldownRemaining() const { return mUseCooldownRemaining; }
		float GetBoostRemaining() const { return mBoostRemaining; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryBlinkDriveRule> mRules;
		int mCharges = 0;
		float mRechargeRemaining = 0.f;
		float mUseCooldownRemaining = 0.f;
		float mBoostRemaining = 0.f;
	};
}

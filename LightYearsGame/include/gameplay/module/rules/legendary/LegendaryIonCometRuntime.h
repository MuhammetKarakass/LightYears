#pragma once

#include "gameplay/module/rules/legendary/LegendaryIonCometRules.h"
#include <memory>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;
	class IonCometVisualActor;

	// ShipModuleApplier owns this runtime; referenced subsystems outlive it.
	class LegendaryIonCometRuntime final
	{
	public:
		LegendaryIonCometRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~LegendaryIonCometRuntime();
		LegendaryIonCometRuntime(const LegendaryIonCometRuntime&) = delete;
		LegendaryIonCometRuntime& operator=(const LegendaryIonCometRuntime&) = delete;
		void Replace(const List<ResolvedModuleLegendaryIonCometRule>& rules);
		void Tick(float deltaSeconds);
		void Clear();
		const List<ResolvedModuleLegendaryIonCometRule>& GetRules() const { return mRules; }
		void ObservePlayerControl(float speedBeforeInput, bool braking, float deltaSeconds);
		float GetCharge() const { return mCharge; }
		float GetFieldRadius() const { return mFieldRadius; }
		float GetEffectiveSpeed() const { return mEffectiveSpeed; }
		float GetLastBurstRadius() const { return mLastBurstRadius; }
		float GetLastBurstDamage() const { return mLastBurstDamage; }
		std::uint64_t GetBurstCount() const { return mBurstCount; }

	private:
		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		List<ResolvedModuleLegendaryIonCometRule> mRules;
		void ApplyArea(float radius, float damage, int stacks, bool burst);
		void UpdateVisual();
		float EffectiveSpeed(float speed) const;
		float Radius(float effectiveSpeed) const;
		float mCharge = 0.f;
		float mFieldRadius = 0.f;
		float mEffectiveSpeed = 0.f;
		float mTickAccumulator = 0.f;
		float mSpeedBeforeInput = 0.f;
		float mControlSeconds = 0.f;
		bool mBraking = false;
		bool mHasControlSample = false;
		bool mBrakeActive = false;
		float mBrakeRemaining = 0.f;
		float mBrakeSpeed = 0.f;
		float mBrakeRadius = 0.f;
		float mBrakeEffectiveSpeed = 0.f;
		float mLastBurstRadius = 0.f;
		float mLastBurstDamage = 0.f;
		std::uint64_t mBurstCount = 0;
		std::uint64_t mTeleportRevision = 0;
		std::weak_ptr<IonCometVisualActor> mVisual;
	};
}

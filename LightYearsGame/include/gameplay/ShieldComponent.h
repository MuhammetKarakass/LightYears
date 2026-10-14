#pragma once

#include <framework/ActorComponent.h>
#include <framework/Delegate.h>
#include "gameplay/resource/TemporaryOvercapLedger.h"

#include <string>
#include <unordered_set>

namespace ly
{
	enum class ShieldImpactSide { Shared, Front, Rear };
	// Shield shaping set by equipped modules. The defaults are the unmodified rules.
	// SpeedScaled and Continuous are applied by SpaceShip::UpdateRegeneration; the
	// re-arm fraction is read by ModuleTriggerRuntime; armorShare by SpaceShip::ReceiveDamage.
	struct ShieldModuleBehavior
	{
		enum class RegenMode { Normal, SpeedScaled, Continuous };
		RegenMode regenMode = RegenMode::Normal;
		// SpeedScaled: passive rate at the speed cap, as a multiple of the normal rate.
		float speedMultiplierAtMax = 1.f;
		float minimumSpeed = 0.f;
		// Continuous: fraction of the normal rate that applies while the shield is taking damage.
		float continuousRateFraction = 1.f;
		// A ShieldBroken event latches until the shield refills to this fraction of its maximum; 0 disables the latch.
		float breakRearmFraction = 0.f;
		// Share of the Armor damage reduction that the shield also takes for the damage it absorbs.
		float armorShare = 0.f;
		bool frontRearSplit = false;
	};

	// Not ticked automatically (mCanEverTick stays false): Tick takes per-frame
	// arguments and must run at a fixed point of the owner's regeneration step
	// (UE tick-prerequisite equivalent), so the owning actor drives it explicitly.
	class ShieldComponent : public ActorComponent
	{
	public:
		explicit ShieldComponent(Actor& owner, float shield = 0.f, float maxShield = 0.f, float rechargeDelay = 3.f);

		float GetShield() const { return mShield + mModuleTemporaryShield; }
		float GetMaxShield() const { return mMaxShield; }
		float GetModuleTemporaryShield() const { return mModuleTemporaryShield; }
		void GrantModuleTemporaryShield(const std::string& sourceId, float amount, float decaySeconds);
		void RemoveModuleTemporaryShield(const std::string& sourceId);
		float GetRechargeDelayRemaining() const { return mRechargeDelayRemaining; }
		void SetModuleBehavior(const ShieldModuleBehavior& behavior);
		float GetFrontShield() const;
		float GetRearShield() const;
		const ShieldModuleBehavior& GetModuleBehavior() const { return mModuleBehavior; }

		void SetMaxShield(float maxShield, bool preserveShieldPercent = false);
		void SetRechargeDelay(float rechargeDelay);
		// Clears only the current delay; the configured base delay remains intact.
		void ClearRechargeDelay() { mRechargeDelayRemaining = 0.f; }
		// A non-damage resource correction. Positive adjustments obey the normal
		// maximum; negative adjustments preserve combat timers and are used by
		// state-recall mechanics that must restore an earlier lower shield value.
		void ChangeShield(float amount);

		// Returns source damage consumed by the shield. The multiplier lets damage
		// types spend more or less shield capacity without leaking extra hull damage.
		float AbsorbDamage(float sourceDamage, float shieldDamageMultiplier, float extraRechargeDelay, bool moduleGenerated = false, ShieldImpactSide side = ShieldImpactSide::Shared);
		bool IsAbsorbingModuleDamage() const { return mAbsorbingModuleDamage; }
		// Adds only the portion that reaches above the normal maximum to the
		// temporary overshield ledger. The normal shield value remains one scalar
		// so existing damage and recharge rules continue to work unchanged.
		float GrantTemporaryOvershield(
			const std::string& sourceId,
			float amount,
			float holdDuration,
			float decayPerSecond
		);
		// Advances hold/decay timers independently from normal shield recharge.
		void TickTemporaryOvershields(float deltaTime);
		// Resource systems may temporarily suppress passive shield regeneration
		// while a committed resource is travelling. The source key makes nested
		// mechanics compose safely: one ability cannot re-enable regen that a
		// different active mechanic still blocks.
		void SetPassiveRegenBlocked(const std::string& sourceId, bool blocked);
		bool IsPassiveRegenBlocked() const { return !mPassiveRegenBlockers.empty(); }
		// A caller may pause both the recharge delay and regeneration while a system is active.
		void Tick(float deltaTime, float regenerationPerSecond, bool allowRecharge = true);

		Delegate<float, float, float> onShieldChanged;
		Delegate<float, float, float> onShieldDamaged;

	private:
		void BroadcastShieldChanged(float previousShield);
		void AdjustPolarNormalShield(float previousNormal);

		float mShield = 0.f;
		float mFrontShield = 0.f;
		bool mAbsorbingModuleDamage = false;
		float mMaxShield = 0.f;
		float mRechargeDelay = 3.f;
		float mRechargeDelayRemaining = 0.f;
		ShieldModuleBehavior mModuleBehavior;
		TemporaryOvercapLedger mTemporaryOvershields;
		TemporaryOvercapLedger mModuleShieldLayers;
		float mModuleTemporaryShield = 0.f;
		std::unordered_set<std::string> mPassiveRegenBlockers;
	};
}

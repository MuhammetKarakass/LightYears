#pragma once

#include <framework/Delegate.h>

#include <string>

namespace ly
{
	class ShieldComponent
	{
	public:
		explicit ShieldComponent(float shield = 0.f, float maxShield = 0.f, float rechargeDelay = 3.f);

		float GetShield() const { return mShield; }
		float GetMaxShield() const { return mMaxShield; }
		float GetRechargeDelayRemaining() const { return mRechargeDelayRemaining; }

		void SetMaxShield(float maxShield, bool preserveShieldPercent = false);
		void SetRechargeDelay(float rechargeDelay);
		// Clears only the current delay; the configured base delay remains intact.
		void ClearRechargeDelay() { mRechargeDelayRemaining = 0.f; }

		// Returns source damage consumed by the shield. The multiplier lets damage
		// types spend more or less shield capacity without leaking extra hull damage.
		float AbsorbDamage(float sourceDamage, float shieldDamageMultiplier, float extraRechargeDelay);
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
		// A caller may pause both the recharge delay and regeneration while a system is active.
		void Tick(float deltaTime, float regenerationPerSecond, bool allowRecharge = true);

		Delegate<float, float, float> onShieldChanged;
		Delegate<float, float, float> onShieldDamaged;

	private:
		struct TemporaryOvershield
		{
			std::string sourceId;
			float amount = 0.f;
			float holdRemaining = 0.f;
			float decayPerSecond = 0.f;
		};

		void ConsumeTemporaryOvershield(float amount);
		void ReconcileTemporaryOvershieldLedger();
		void BroadcastShieldChanged(float previousShield);

		float mShield = 0.f;
		float mMaxShield = 0.f;
		float mRechargeDelay = 3.f;
		float mRechargeDelayRemaining = 0.f;
		List<TemporaryOvershield> mTemporaryOvershields;
	};
}

#pragma once

#include <framework/Delegate.h>

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

		// Returns source damage consumed by the shield. The multiplier lets damage
		// types spend more or less shield capacity without leaking extra hull damage.
		float AbsorbDamage(float sourceDamage, float shieldDamageMultiplier, float extraRechargeDelay);
		// A caller may pause both the recharge delay and regeneration while a system is active.
		void Tick(float deltaTime, float regenerationPerSecond, bool allowRecharge = true);

		Delegate<float, float, float> onShieldChanged;
		Delegate<float, float, float> onShieldDamaged;

	private:
		void BroadcastShieldChanged(float previousShield);

		float mShield = 0.f;
		float mMaxShield = 0.f;
		float mRechargeDelay = 3.f;
		float mRechargeDelayRemaining = 0.f;
	};
}

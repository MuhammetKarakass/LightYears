#pragma once

#include <framework/ActorComponent.h>
#include <framework/Delegate.h>

namespace vega
{
	// Not ticked automatically (mCanEverTick stays false): see ShieldComponent.
	class EnergyComponent : public ActorComponent
	{
		VEGA_CLASS(EnergyComponent)
	public:
		explicit EnergyComponent(Actor& owner, float energy = 0.f, float maxEnergy = 0.f, float rechargeDelay = 2.f);

		float GetEnergy() const { return mEnergy; }
		float GetMaxEnergy() const { return mMaxEnergy; }
		float GetRechargeDelay() const { return mRechargeDelay; }
		float GetRechargeDelayRemaining() const { return mRechargeDelayRemaining; }

		void SetMaxEnergy(float maxEnergy, bool preserveEnergyPercent = false);
		void SetRechargeDelay(float rechargeDelay);
		// Clears only the current delay; the configured base delay remains intact.
		void ClearRechargeDelay() { mRechargeDelayRemaining = 0.f; }
		float Consume(float amount);
		void Restore(float amount);
		// A caller may pause both the recharge delay and regeneration while a system is active.
		void Tick(float deltaTime, float regenerationPerSecond, bool allowRecharge = true);

		Delegate<float, float, float> onEnergyChanged;

	private:
		void BroadcastEnergyChanged(float previousEnergy);

		float mEnergy = 0.f;
		float mMaxEnergy = 0.f;
		float mRechargeDelay = 2.f;
		float mRechargeDelayRemaining = 0.f;
	};
}

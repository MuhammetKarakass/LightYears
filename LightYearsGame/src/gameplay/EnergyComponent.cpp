#include "gameplay/EnergyComponent.h"
#include <framework/Actor.h>
#include <framework/reflection/Reflection.h>

#include <algorithm>

namespace ly
{
	VEGA_REFLECT(EnergyComponent, ActorComponent)
	{
		VEGA_ADDABLE();
		VEGA_CATEGORY("Energy");
		VEGA_READONLY("Energy", GetEnergy);
		VEGA_PROPERTY("Max Energy", GetMaxEnergy, SetMaxEnergy).Range(0.f, 100000.f);
		VEGA_PROPERTY("Recharge Delay", GetRechargeDelay, SetRechargeDelay).Range(0.f, 1000.f).Speed(0.1f);
		VEGA_READONLY("Recharge Delay Remaining", GetRechargeDelayRemaining);
	}

	EnergyComponent::EnergyComponent(Actor& owner, float energy, float maxEnergy, float rechargeDelay)
		: ActorComponent{ owner }
		, mEnergy{ std::clamp(energy, 0.f, std::max(0.f, maxEnergy)) }
		, mMaxEnergy{ std::max(0.f, maxEnergy) }
		, mRechargeDelay{ std::max(0.f, rechargeDelay) }
	{
	}

	void EnergyComponent::SetMaxEnergy(float maxEnergy, bool preserveEnergyPercent)
	{
		const float clampedMaxEnergy = std::max(0.f, maxEnergy);
		if (clampedMaxEnergy == mMaxEnergy)
		{
			return;
		}

		const float previousEnergy = mEnergy;
		const float previousMaxEnergy = mMaxEnergy;
		mMaxEnergy = clampedMaxEnergy;
		if (preserveEnergyPercent && previousMaxEnergy > 0.f)
		{
			mEnergy = mMaxEnergy * (previousEnergy / previousMaxEnergy);
		}
		else if (previousMaxEnergy <= 0.f && mMaxEnergy > 0.f)
		{
			mEnergy = mMaxEnergy;
		}
		else
		{
			mEnergy = std::min(mEnergy, mMaxEnergy);
		}

		BroadcastEnergyChanged(previousEnergy);
	}

	void EnergyComponent::SetRechargeDelay(float rechargeDelay)
	{
		mRechargeDelay = std::max(0.f, rechargeDelay);
	}

	float EnergyComponent::Consume(float amount)
	{
		if (amount <= 0.f || mEnergy <= 0.f)
		{
			return 0.f;
		}

		const float previousEnergy = mEnergy;
		const float consumedEnergy = std::min(mEnergy, amount);
		mEnergy -= consumedEnergy;
		mRechargeDelayRemaining = std::max(mRechargeDelayRemaining, mRechargeDelay);
		BroadcastEnergyChanged(previousEnergy);
		return consumedEnergy;
	}

	void EnergyComponent::Restore(float amount)
	{
		if (amount <= 0.f) return;
		const float previousEnergy = mEnergy;
		mEnergy = std::min(mMaxEnergy, mEnergy + amount);
		BroadcastEnergyChanged(previousEnergy);
	}

	void EnergyComponent::Tick(float deltaTime, float regenerationPerSecond, bool allowRecharge)
	{
		if (!allowRecharge || deltaTime <= 0.f || mEnergy >= mMaxEnergy)
		{
			return;
		}

		float regenerationTime = deltaTime;
		if (mRechargeDelayRemaining > 0.f)
		{
			const float consumedDelay = std::min(mRechargeDelayRemaining, regenerationTime);
			mRechargeDelayRemaining -= consumedDelay;
			regenerationTime -= consumedDelay;
		}

		const float regeneration = std::max(0.f, regenerationPerSecond);
		if (regenerationTime <= 0.f || regeneration <= 0.f)
		{
			return;
		}

		const float previousEnergy = mEnergy;
		mEnergy = std::min(mMaxEnergy, mEnergy + regeneration * regenerationTime);
		BroadcastEnergyChanged(previousEnergy);
	}

	void EnergyComponent::BroadcastEnergyChanged(float previousEnergy)
	{
		if (mEnergy != previousEnergy)
		{
			onEnergyChanged.Broadcast(mEnergy - previousEnergy, mEnergy, mMaxEnergy);
		}
	}
}

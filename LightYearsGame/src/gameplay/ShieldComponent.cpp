#include "gameplay/ShieldComponent.h"

#include <algorithm>

namespace ly
{
	ShieldComponent::ShieldComponent(float shield, float maxShield, float rechargeDelay)
		: mShield{ std::clamp(shield, 0.f, std::max(0.f, maxShield)) }
		, mMaxShield{ std::max(0.f, maxShield) }
		, mRechargeDelay{ std::max(0.f, rechargeDelay) }
	{
	}

	void ShieldComponent::SetMaxShield(float maxShield, bool preserveShieldPercent)
	{
		const float clampedMaxShield = std::max(0.f, maxShield);
		if (clampedMaxShield == mMaxShield)
		{
			return;
		}

		const float previousShield = mShield;
		const float previousMaxShield = mMaxShield;
		mMaxShield = clampedMaxShield;
		if (preserveShieldPercent && previousMaxShield > 0.f)
		{
			mShield = mMaxShield * (previousShield / previousMaxShield);
		}
		else if (previousMaxShield <= 0.f && mMaxShield > 0.f)
		{
			mShield = mMaxShield;
		}
		else
		{
			mShield = std::min(mShield, mMaxShield);
		}

		BroadcastShieldChanged(previousShield);
	}

	void ShieldComponent::SetRechargeDelay(float rechargeDelay)
	{
		mRechargeDelay = std::max(0.f, rechargeDelay);
	}

	float ShieldComponent::AbsorbDamage(float sourceDamage, float shieldDamageMultiplier, float extraRechargeDelay)
	{
		if (sourceDamage <= 0.f || mMaxShield <= 0.f)
		{
			return 0.f;
		}

		mRechargeDelayRemaining = std::max(
			mRechargeDelayRemaining,
			mRechargeDelay + std::max(0.f, extraRechargeDelay)
		);

		const float multiplier = std::max(0.f, shieldDamageMultiplier);
		if (mShield <= 0.f || multiplier <= 0.f)
		{
			return 0.f;
		}

		const float previousShield = mShield;
		const float absorbedShieldDamage = std::min(mShield, sourceDamage * multiplier);
		mShield -= absorbedShieldDamage;
		BroadcastShieldChanged(previousShield);
		onShieldDamaged.Broadcast(absorbedShieldDamage, mShield, mMaxShield);
		return absorbedShieldDamage / multiplier;
	}

	void ShieldComponent::Tick(float deltaTime, float regenerationPerSecond, bool allowRecharge)
	{
		if (!allowRecharge || deltaTime <= 0.f || mShield >= mMaxShield)
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

		const float previousShield = mShield;
		mShield = std::min(mMaxShield, mShield + regeneration * regenerationTime);
		BroadcastShieldChanged(previousShield);
	}

	void ShieldComponent::BroadcastShieldChanged(float previousShield)
	{
		if (mShield != previousShield)
		{
			onShieldChanged.Broadcast(mShield - previousShield, mShield, mMaxShield);
		}
	}
}

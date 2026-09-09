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
		mTemporaryOvershields.Reconcile(mShield, mMaxShield);

		BroadcastShieldChanged(previousShield);
	}

	void ShieldComponent::SetRechargeDelay(float rechargeDelay)
	{
		mRechargeDelay = std::max(0.f, rechargeDelay);
	}

	void ShieldComponent::ChangeShield(float amount)
	{
		if (amount == 0.f)
		{
			return;
		}

		const float previousShield = mShield;
		mShield = std::max(0.f, mShield + amount);
		if (amount > 0.f)
		{
			mShield = std::min(mShield, mMaxShield);
		}
		else
		{
			const float previousExcess = std::max(0.f, previousShield - mMaxShield);
			const float currentExcess = std::max(0.f, mShield - mMaxShield);
			mTemporaryOvershields.Consume(
				std::max(0.f, previousExcess - currentExcess)
			);
		}
		BroadcastShieldChanged(previousShield);
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
		const float previousOvershield = std::max(0.f, previousShield - mMaxShield);
		const float absorbedShieldDamage = std::min(mShield, sourceDamage * multiplier);
		mShield -= absorbedShieldDamage;
		const float currentOvershield = std::max(0.f, mShield - mMaxShield);
		mTemporaryOvershields.Consume(
			std::max(0.f, previousOvershield - currentOvershield)
		);
		BroadcastShieldChanged(previousShield);
		onShieldDamaged.Broadcast(absorbedShieldDamage, mShield, mMaxShield);
		return absorbedShieldDamage / multiplier;
	}

	float ShieldComponent::GrantTemporaryOvershield(
		const std::string& sourceId,
		float amount,
		float holdDuration,
		float decayPerSecond
	)
	{
		const float safeAmount = std::max(0.f, amount);
		if (safeAmount <= 0.f || mMaxShield <= 0.f)
		{
			return 0.f;
		}

		const float previousShield = mShield;
		const float previousOvershield = std::max(0.f, previousShield - mMaxShield);
		mShield += safeAmount;
		const float newOvershield = std::max(0.f, mShield - mMaxShield);
		const float temporaryPortion = std::max(0.f, newOvershield - previousOvershield);
		if (temporaryPortion > 0.f)
		{
			mTemporaryOvershields.Add(TemporaryOvercapRequest{
				sourceId,
				temporaryPortion,
				std::max(0.f, holdDuration),
				std::max(0.f, decayPerSecond)
			});
		}
		BroadcastShieldChanged(previousShield);
		return safeAmount;
	}

	void ShieldComponent::TickTemporaryOvershields(float deltaTime)
	{
		const float decayAmount = mTemporaryOvershields.Tick(
			deltaTime,
			mShield,
			mMaxShield
		);
		if (decayAmount <= 0.f)
		{
			return;
		}

		const float previousShield = mShield;
		mShield = std::max(mMaxShield, mShield - decayAmount);
		BroadcastShieldChanged(previousShield);
	}

	void ShieldComponent::SetPassiveRegenBlocked(
		const std::string& sourceId,
		bool blocked
	)
	{
		if (sourceId.empty())
		{
			return;
		}
		if (blocked)
		{
			mPassiveRegenBlockers.insert(sourceId);
		}
		else
		{
			mPassiveRegenBlockers.erase(sourceId);
		}
	}

	void ShieldComponent::Tick(float deltaTime, float regenerationPerSecond, bool allowRecharge)
	{
		if (!allowRecharge || IsPassiveRegenBlocked() || deltaTime <= 0.f ||
			mShield >= mMaxShield)
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

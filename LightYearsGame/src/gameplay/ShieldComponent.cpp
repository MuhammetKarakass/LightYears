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
		ReconcileTemporaryOvershieldLedger();

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
		const float previousOvershield = std::max(0.f, previousShield - mMaxShield);
		const float absorbedShieldDamage = std::min(mShield, sourceDamage * multiplier);
		mShield -= absorbedShieldDamage;
		const float currentOvershield = std::max(0.f, mShield - mMaxShield);
		ConsumeTemporaryOvershield(
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
			mTemporaryOvershields.push_back(TemporaryOvershield{
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
		if (deltaTime <= 0.f || mTemporaryOvershields.empty())
		{
			return;
		}

		ReconcileTemporaryOvershieldLedger();
		if (mTemporaryOvershields.empty())
		{
			return;
		}

		float decayAmount = 0.f;
		for (TemporaryOvershield& overshield : mTemporaryOvershields)
		{
			overshield.holdRemaining = std::max(
				0.f,
				overshield.holdRemaining - deltaTime
			);
			if (overshield.holdRemaining <= 0.f && overshield.decayPerSecond > 0.f)
			{
				const float entryDecay = std::min(
					overshield.amount,
					overshield.decayPerSecond * deltaTime
				);
				overshield.amount -= entryDecay;
				decayAmount += entryDecay;
			}
		}

		if (decayAmount <= 0.f)
		{
			ReconcileTemporaryOvershieldLedger();
			return;
		}

		const float previousShield = mShield;
		mShield = std::max(mMaxShield, mShield - decayAmount);
		ReconcileTemporaryOvershieldLedger();
		BroadcastShieldChanged(previousShield);
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

	void ShieldComponent::ConsumeTemporaryOvershield(float amount)
	{
		float remaining = std::max(0.f, amount);
		for (TemporaryOvershield& overshield : mTemporaryOvershields)
		{
			if (remaining <= 0.f)
			{
				break;
			}
			const float consumed = std::min(overshield.amount, remaining);
			overshield.amount -= consumed;
			remaining -= consumed;
		}
	}

	void ShieldComponent::ReconcileTemporaryOvershieldLedger()
	{
		const float currentOvershield = std::max(0.f, mShield - mMaxShield);
		float recordedOvershield = 0.f;
		for (const TemporaryOvershield& overshield : mTemporaryOvershields)
		{
			recordedOvershield += std::max(0.f, overshield.amount);
		}

		if (recordedOvershield > currentOvershield)
		{
			ConsumeTemporaryOvershield(recordedOvershield - currentOvershield);
		}

		mTemporaryOvershields.erase(
			std::remove_if(
				mTemporaryOvershields.begin(),
				mTemporaryOvershields.end(),
				[](const TemporaryOvershield& overshield)
				{
					return overshield.amount <= 0.001f;
				}
			),
			mTemporaryOvershields.end()
		);
	}

	void ShieldComponent::BroadcastShieldChanged(float previousShield)
	{
		if (mShield != previousShield)
		{
			onShieldChanged.Broadcast(mShield - previousShield, mShield, mMaxShield);
		}
	}
}

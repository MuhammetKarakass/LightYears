#include "gameplay/ShieldComponent.h"
#include <framework/Actor.h>
#include <framework/reflection/Reflection.h>

#include <algorithm>
#include <cmath>

namespace vega
{
	VEGA_REFLECT(ShieldComponent, ActorComponent)
	{
		VEGA_ADDABLE();
		VEGA_CATEGORY("Shield");
		VEGA_READONLY("Shield", GetShield);
		VEGA_PROPERTY("Max Shield", GetMaxShield, SetMaxShield).Range(0.f, 100000.f);
		VEGA_PROPERTY("Recharge Delay", GetRechargeDelay, SetRechargeDelay).Range(0.f, 1000.f).Speed(0.1f);
		VEGA_READONLY("Recharge Delay Remaining", GetRechargeDelayRemaining);
		VEGA_READONLY("Overshield", GetOvershield);
		VEGA_READONLY("Front Shield", GetFrontShield);
		VEGA_READONLY("Rear Shield", GetRearShield);
		VEGA_READONLY("Passive Regen Blocked", IsPassiveRegenBlocked);
	}

	ShieldComponent::ShieldComponent(Actor& owner, float shield, float maxShield, float rechargeDelay)
		: ActorComponent{ owner }
		, mShield{ std::clamp(shield, 0.f, std::max(0.f, maxShield)) }
		, mMaxShield{ std::max(0.f, maxShield) }
		, mRechargeDelay{ std::max(0.f, rechargeDelay) }
	{
	}

	void ShieldComponent::SetModuleBehavior(const ShieldModuleBehavior& behavior)
	{
		if (behavior.frontRearSplit != mModuleBehavior.frontRearSplit) mFrontShield = behavior.frontRearSplit ? std::min(mShield, mMaxShield) * .5f : 0.f;
		mModuleBehavior = behavior;
		mBatteryReserve = std::min(mBatteryReserve, GetBatteryCap());
	}
	float ShieldComponent::GetFrontShield() const { return mModuleBehavior.frontRearSplit ? mFrontShield : std::min(mShield, mMaxShield) * .5f; }
	float ShieldComponent::GetRearShield() const { return std::max(0.f, std::min(mShield, mMaxShield) - GetFrontShield()); }
	void ShieldComponent::AdjustPolarNormalShield(float previousNormal)
	{
		if (!mModuleBehavior.frontRearSplit) return;
		const float normal = std::min(mShield, mMaxShield);
		if (normal < previousNormal) mFrontShield = previousNormal > 0.f ? mFrontShield * normal / previousNormal : 0.f;
		else
		{
			const float rear = std::max(0.f, previousNormal - mFrontShield), increase = normal - previousNormal;
			if (mFrontShield < rear) { const float equalize = std::min(increase, rear - mFrontShield); mFrontShield += equalize + (increase - equalize) * .5f; }
			else { const float equalize = std::min(increase, mFrontShield - rear); mFrontShield += (increase - equalize) * .5f; }
		}
		mFrontShield = std::clamp(mFrontShield, 0.f, mMaxShield * .5f);
	}
	void ShieldComponent::SetMaxShield(float maxShield, bool preserveShieldPercent)
	{
		const float clampedMaxShield = std::max(0.f, maxShield);
		if (clampedMaxShield == mMaxShield)
		{
			return;
		}

		const float previousShield = mShield;
		const float previousTotalShield = GetShield();
		const float previousMaxShield = mMaxShield;
		const float previousFront = GetFrontShield(), previousRear = GetRearShield();
		float normalShield = std::min(previousShield, previousMaxShield);
		const float excessShield = std::max(0.f, previousShield - previousMaxShield);
		mMaxShield = clampedMaxShield;
		if (excessShield > 0.f) normalShield = mMaxShield;
		else if (preserveShieldPercent && previousMaxShield > 0.f)
		{
			normalShield = mMaxShield * (normalShield / previousMaxShield);
		}
		else if (previousMaxShield <= 0.f && mMaxShield > 0.f)
		{
			normalShield = mMaxShield;
		}
		mShield = std::min(normalShield, mMaxShield) + excessShield;
		if (mModuleBehavior.frontRearSplit)
		{
			const float cap = mMaxShield * .5f;
			if (excessShield > 0.f || previousMaxShield <= 0.f) mFrontShield = cap;
			else if (preserveShieldPercent) mFrontShield = previousFront * mMaxShield / previousMaxShield;
			else mFrontShield = std::min(cap, previousFront);
			const float rear = excessShield > 0.f || previousMaxShield <= 0.f ? cap : preserveShieldPercent ? previousRear * mMaxShield / previousMaxShield : std::min(cap, previousRear);
			mShield = mFrontShield + rear + excessShield;
		}
		mTemporaryOvershields.Reconcile(mShield, mMaxShield);
		mBatteryReserve = std::min(mBatteryReserve, GetBatteryCap());

		if (mShield != previousShield || mMaxShield != previousMaxShield)
		{
			onShieldChanged.Broadcast(GetShield() - previousTotalShield, GetShield(), mMaxShield);
		}
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
		if (amount > 0.f && mShield >= mMaxShield)
		{
			return;
		}

		const float previousShield = mShield;
		const float previousTotalShield = GetShield();
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
		AdjustPolarNormalShield(std::min(previousShield, mMaxShield));
		BroadcastShieldChanged(previousTotalShield);
	}

	float ShieldComponent::AbsorbDamage(float sourceDamage, float shieldDamageMultiplier, float extraRechargeDelay, bool moduleGenerated, ShieldImpactSide side)
	{
		if (sourceDamage <= 0.f || (mMaxShield <= 0.f && GetShield() <= 0.f))
		{
			return 0.f;
		}

		mRechargeDelayRemaining = std::max(
			mRechargeDelayRemaining,
			mRechargeDelay + std::max(0.f, extraRechargeDelay)
		);

		const float multiplier = std::max(0.f, shieldDamageMultiplier);
		// The battery reserve is spent before the normal shield; the rest of the hit continues through the normal path.
		const float reserveAbsorbed = std::min(mBatteryReserve, sourceDamage * multiplier);
		if (reserveAbsorbed > 0.f)
		{
			mBatteryReserve -= reserveAbsorbed;
			const float reserveDamage = reserveAbsorbed / multiplier;
			onShieldDamaged.Broadcast(reserveAbsorbed, GetShield(), mMaxShield);
			return reserveDamage + AbsorbDamage(sourceDamage - reserveDamage, shieldDamageMultiplier, 0.f, moduleGenerated, side);
		}
		if (GetShield() <= 0.f || multiplier <= 0.f)
		{
			return 0.f;
		}

		const float previousShield = mShield;
		const float previousTotalShield = GetShield();
		const float previousOvershield = std::max(0.f, previousShield - mMaxShield);
		const float normal = std::min(mShield, mMaxShield);
		const float directional = !mModuleBehavior.frontRearSplit || side == ShieldImpactSide::Shared ? normal : side == ShieldImpactSide::Front ? GetFrontShield() : GetRearShield();
		const float absorbedShieldDamage = std::min(previousOvershield + directional, sourceDamage * multiplier);
		struct DamageScope
		{
			bool& flag;
			bool previous;
			~DamageScope() { flag = previous; }
		} scope{ mAbsorbingModuleDamage, mAbsorbingModuleDamage };
		mAbsorbingModuleDamage = moduleGenerated;
		const float sharedDamage = std::min(previousOvershield, absorbedShieldDamage);
		const float normalDamage = absorbedShieldDamage - sharedDamage;
		if (mModuleBehavior.frontRearSplit)
		{
			if (side == ShieldImpactSide::Front) mFrontShield = std::max(0.f, mFrontShield - normalDamage);
			else if (side == ShieldImpactSide::Shared) mFrontShield = normal > 0.f ? mFrontShield * std::max(0.f, normal - normalDamage) / normal : 0.f;
		}
		mShield -= sharedDamage + normalDamage;
		const float currentOvershield = std::max(0.f, mShield - mMaxShield);
		mTemporaryOvershields.Consume(
			std::max(0.f, previousOvershield - currentOvershield)
		);
		BroadcastShieldChanged(previousTotalShield);
		onShieldDamaged.Broadcast(absorbedShieldDamage, GetShield(), mMaxShield);
		return absorbedShieldDamage / multiplier;
	}

	float ShieldComponent::GrantShield(const std::string& sourceId, float amount)
	{
		if (sourceId.empty() || !std::isfinite(amount) || amount <= 0.f) return 0.f;
		const float previous = mShield;
		const float fill = std::min(amount, std::max(0.f, mMaxShield - mShield));
		const float increase = mTemporaryOvershields.RefreshMaximum({ sourceId, amount - fill, OvershieldRule::HoldSeconds, OvershieldRule::DecayPerSecond });
		mShield += fill + increase;
		AdjustPolarNormalShield(std::min(previous, mMaxShield));
		BroadcastShieldChanged(previous);
		return mShield - previous;
	}

	void ShieldComponent::RemoveShieldGrant(const std::string& sourceId)
	{
		const float previous = mShield;
		mShield = std::max(0.f, mShield - mTemporaryOvershields.RemoveSource(sourceId));
		BroadcastShieldChanged(previous);
	}

	void ShieldComponent::TickTemporaryOvershields(float deltaTime)
	{
		const float previous = GetShield();
		const float decay = mTemporaryOvershields.Tick(deltaTime, mShield, mMaxShield);
		if (decay > 0.f) mShield = std::max(0.f, mShield - decay);
		BroadcastShieldChanged(previous);
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

		const float previousTotalShield = GetShield();
		const float previousNormal = std::min(mShield, mMaxShield);
		mShield = std::min(mMaxShield, mShield + regeneration * regenerationTime);
		AdjustPolarNormalShield(previousNormal);
		BroadcastShieldChanged(previousTotalShield);
	}

	// The normal shield is full here, so Tick skips it and this call owns the recharge delay. While the delay runs, the reserve
	// fills at the damaged fill share (0 for the plain battery, so it waits); after the delay it fills at the full rate.
	void ShieldComponent::TickBatteryReserve(float deltaTime, float regenerationPerSecond, bool allowRecharge)
	{
		const float cap = GetBatteryCap();
		if (cap <= 0.f || deltaTime <= 0.f || !allowRecharge || IsPassiveRegenBlocked() || mShield < mMaxShield) return;
		const float delayed = std::min(mRechargeDelayRemaining, deltaTime);
		mRechargeDelayRemaining -= delayed;
		const float regeneration = std::max(0.f, regenerationPerSecond);
		const float fill = regeneration * (delayed * mModuleBehavior.batteryDamagedFillFraction + (deltaTime - delayed));
		if (fill <= 0.f || mBatteryReserve >= cap) return;
		mBatteryReserve = std::min(cap, mBatteryReserve + fill);
	}

	void ShieldComponent::BroadcastShieldChanged(float previousShield)
	{
		if (GetShield() != previousShield)
		{
			onShieldChanged.Broadcast(GetShield() - previousShield, GetShield(), mMaxShield);
		}
	}
}

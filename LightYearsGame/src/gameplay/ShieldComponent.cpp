#include "gameplay/ShieldComponent.h"
#include <framework/Actor.h>
#include <framework/reflection/Reflection.h>

#include <algorithm>

namespace ly
{
	VEGA_REFLECT(ShieldComponent, ActorComponent)
	{
		VEGA_ADDABLE();
		VEGA_CATEGORY("Shield");
		VEGA_READONLY("Shield", GetShield);
		VEGA_PROPERTY("Max Shield", GetMaxShield, SetMaxShield).Range(0.f, 100000.f);
		VEGA_PROPERTY("Recharge Delay", GetRechargeDelay, SetRechargeDelay).Range(0.f, 1000.f).Speed(0.1f);
		VEGA_READONLY("Recharge Delay Remaining", GetRechargeDelayRemaining);
		VEGA_READONLY("Module Temporary Shield", GetModuleTemporaryShield);
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
		if (mMaxShield == 0.f) { mModuleShieldLayers.Clear(); mModuleTemporaryShield = 0.f; }

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
		if (amount < 0.f)
		{
			const float consumed = std::min(mModuleTemporaryShield, -amount);
			mModuleTemporaryShield -= consumed; mModuleShieldLayers.Consume(consumed); amount += consumed;
			if (mModuleShieldLayers.IsEmpty()) mModuleTemporaryShield = 0.f;
		}
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
		if (sourceDamage <= 0.f || mMaxShield <= 0.f)
		{
			return 0.f;
		}

		mRechargeDelayRemaining = std::max(
			mRechargeDelayRemaining,
			mRechargeDelay + std::max(0.f, extraRechargeDelay)
		);

		const float multiplier = std::max(0.f, shieldDamageMultiplier);
		if (GetShield() <= 0.f || multiplier <= 0.f)
		{
			return 0.f;
		}

		const float previousShield = mShield;
		const float previousTotalShield = GetShield();
		const float previousOvershield = std::max(0.f, previousShield - mMaxShield);
		const float normal = std::min(mShield, mMaxShield);
		const float directional = !mModuleBehavior.frontRearSplit || side == ShieldImpactSide::Shared ? normal : side == ShieldImpactSide::Front ? GetFrontShield() : GetRearShield();
		const float absorbedShieldDamage = std::min(mModuleTemporaryShield + previousOvershield + directional, sourceDamage * multiplier);
		struct DamageScope
		{
			bool& flag;
			bool previous;
			~DamageScope() { flag = previous; }
		} scope{ mAbsorbingModuleDamage, mAbsorbingModuleDamage };
		mAbsorbingModuleDamage = moduleGenerated;
		const float layerDamage = std::min(mModuleTemporaryShield, absorbedShieldDamage);
		mModuleTemporaryShield -= layerDamage; mModuleShieldLayers.Consume(layerDamage);
		if (mModuleShieldLayers.IsEmpty()) mModuleTemporaryShield = 0.f;
		const float sharedDamage = std::min(previousOvershield, absorbedShieldDamage - layerDamage);
		const float normalDamage = absorbedShieldDamage - layerDamage - sharedDamage;
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
		const float previousTotalShield = GetShield();
		const float previousOvershield = std::max(0.f, previousShield - mMaxShield);
		mShield += safeAmount;
		AdjustPolarNormalShield(std::min(previousShield, mMaxShield));
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
		BroadcastShieldChanged(previousTotalShield);
		return safeAmount;
	}

	void ShieldComponent::GrantModuleTemporaryShield(const std::string& sourceId, float amount, float decaySeconds)
	{
		if (sourceId.empty() || amount <= 0.f || decaySeconds <= 0.f || mMaxShield <= 0.f) return;
		const float previous = GetShield();
		mModuleTemporaryShield = std::max(0.f, mModuleTemporaryShield - mModuleShieldLayers.RemoveSource(sourceId));
		mModuleTemporaryShield += amount;
		mModuleShieldLayers.Add({ sourceId, amount, 0.f, amount / decaySeconds });
		BroadcastShieldChanged(previous);
	}
	void ShieldComponent::RemoveModuleTemporaryShield(const std::string& sourceId)
	{
		const float previous = GetShield();
		mModuleTemporaryShield = std::max(0.f, mModuleTemporaryShield - mModuleShieldLayers.RemoveSource(sourceId));
		BroadcastShieldChanged(previous);
	}
	void ShieldComponent::TickTemporaryOvershields(float deltaTime)
	{
		const float previous = GetShield();
		const float decay = mTemporaryOvershields.Tick(deltaTime, mShield, mMaxShield);
		if (decay > 0.f) mShield = std::max(mMaxShield, mShield - decay);
		const float layerDecay = mModuleShieldLayers.Tick(deltaTime, mModuleTemporaryShield, 0.f);
		mModuleTemporaryShield = std::max(0.f, mModuleTemporaryShield - layerDecay);
		if (mModuleShieldLayers.IsEmpty()) mModuleTemporaryShield = 0.f;
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

	void ShieldComponent::BroadcastShieldChanged(float previousShield)
	{
		if (GetShield() != previousShield)
		{
			onShieldChanged.Broadcast(GetShield() - previousShield, GetShield(), mMaxShield);
		}
	}
}

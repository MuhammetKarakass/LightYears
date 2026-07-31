#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ship/ShipRuntime.h"

#include <algorithm>

namespace ly
{
	ShipRuntime::ShipRuntime(sas::AttributeSystem& ownerAttributes)
		: mOwnerAttributes{ &ownerAttributes }
	{
	}

	void ShipRuntime::InitializeFromShipDefinition(const ShipDefinition& shipDefinition)
	{
		mEnergyAttributes = shipDefinition.energyAttributes;
		mAttributeSystem.Clear();

		mAttributeSystem.RegisterAttribute(ShipAttributeIds::MaxShield, 0.f);
		mAttributeSystem.RegisterAttribute(ShipAttributeIds::ShieldRegen, 0.f);
		mAttributeSystem.RegisterAttribute(ShipAttributeIds::ShieldRechargeDelay, 0.f);
		mAttributeSystem.RegisterAttribute(ShipAttributeIds::AfterburnerCapacity, 0.f);
		mAttributeSystem.RegisterAttribute(ShipAttributeIds::AfterburnerRegen, 0.f);
		mAttributeSystem.RegisterAttribute(ShipAttributeIds::AfterburnerRechargeDelay, 0.f);
		mAttributeSystem.RegisterAttribute(
			ShipAttributeIds::AfterburnerEnergyDrainPerSecond,
			std::max(0.f, mEnergyAttributes.baseAfterburnerEnergyDrainPerSecond)
		);
		mAttributeSystem.RegisterAttribute(
			ShipAttributeIds::AfterburnerSpeedMultiplier,
			std::max(0.f, mEnergyAttributes.baseAfterburnerSpeedMultiplier)
		);
		mAttributeSystem.RegisterAttribute(
			ShipAttributeIds::AfterburnerAccelerationMultiplier,
			std::max(0.f, mEnergyAttributes.baseAfterburnerAccelerationMultiplier)
		);
		mAttributeSystem.RegisterAttribute(
			ShipAttributeIds::AfterburnerRampUpDuration,
			std::max(0.f, mEnergyAttributes.baseAfterburnerRampUpDuration)
		);
		mAttributeSystem.RegisterAttribute(
			ShipAttributeIds::AfterburnerRampDownDuration,
			std::max(0.f, mEnergyAttributes.baseAfterburnerRampDownDuration)
		);
		mAttributeSystem.RegisterAttribute(
			ShipAttributeIds::AfterburnerManeuverabilityMultiplier,
			std::clamp(mEnergyAttributes.baseAfterburnerManeuverabilityMultiplier, 0.f, 1.f)
		);

		if (!mOwnerAttributeCallbackBound && mOwnerAttributes)
		{
			mOwnerAttributes->onAttributeChanged.BindAction(this, &ShipRuntime::OnOwnerAttributeChanged);
			mOwnerAttributeCallbackBound = true;
		}

		RecalculateAttributes();
	}

	void ShipRuntime::RecalculateAttributes()
	{
		if (!mOwnerAttributes)
		{
			return;
		}

		const float maxHealth = std::max(0.f, mOwnerAttributes->GetCurrentValue(OwnerAttributeIds::MaxHealth));
		const float maxEnergy = mOwnerAttributes->GetCurrentValue(OwnerAttributeIds::EnergyMax);
		mOwnerAttributes->SetBaseValue(OwnerAttributeIds::HealthRegen, maxHealth / 1200.f);

		const float maxShield = std::max(0.f,
			mEnergyAttributes.baseMaxShield + maxEnergy * mEnergyAttributes.maxShieldPerMaxEnergy
		);
		const float afterburnerCapacity = std::max(0.f,
			mEnergyAttributes.baseAfterburnerCapacity +
				maxEnergy * mEnergyAttributes.afterburnerCapacityPerMaxEnergy
		);
		const float afterburnerRegen = afterburnerCapacity /
			std::max(0.001f, mEnergyAttributes.afterburnerFullRechargeDuration);

		// The owner-facing value is intentionally retained for UI and ability scaling.
		mOwnerAttributes->SetBaseValue(OwnerAttributeIds::EnergyRegen, afterburnerRegen);
		mAttributeSystem.SetBaseValue(ShipAttributeIds::MaxShield, maxShield);
		mAttributeSystem.SetBaseValue(
			ShipAttributeIds::ShieldRegen,
			maxShield / std::max(0.001f, mEnergyAttributes.shieldFullRechargeDuration)
		);
		mAttributeSystem.SetBaseValue(
			ShipAttributeIds::ShieldRechargeDelay,
			std::max(0.f, mEnergyAttributes.baseShieldRechargeDelay)
		);
		mAttributeSystem.SetBaseValue(ShipAttributeIds::AfterburnerCapacity, afterburnerCapacity);
		mAttributeSystem.SetBaseValue(ShipAttributeIds::AfterburnerRegen, afterburnerRegen);
		mAttributeSystem.SetBaseValue(
			ShipAttributeIds::AfterburnerRechargeDelay,
			std::max(0.f, mEnergyAttributes.baseAfterburnerRechargeDelay)
		);
	}

	void ShipRuntime::Clear()
	{
		mAttributeSystem.Clear();
		mEnergyAttributes = ShipEnergyAttributes{};
	}

	float ShipRuntime::GetAfterburnerCapacity() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerCapacity);
	}

	float ShipRuntime::GetAfterburnerRegenPerSecond() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerRegen);
	}

	float ShipRuntime::GetAfterburnerRechargeDelay() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerRechargeDelay);
	}

	float ShipRuntime::GetAfterburnerEnergyDrainPerSecond() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerEnergyDrainPerSecond);
	}

	float ShipRuntime::GetAfterburnerSpeedMultiplier() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerSpeedMultiplier);
	}

	float ShipRuntime::GetAfterburnerAccelerationMultiplier() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerAccelerationMultiplier);
	}

	float ShipRuntime::GetAfterburnerRampUpDuration() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerRampUpDuration);
	}

	float ShipRuntime::GetAfterburnerRampDownDuration() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerRampDownDuration);
	}

	float ShipRuntime::GetAfterburnerManeuverabilityMultiplier() const
	{
		return mAttributeSystem.GetCurrentValue(ShipAttributeIds::AfterburnerManeuverabilityMultiplier);
	}

	void ShipRuntime::OnOwnerAttributeChanged(GameplayTag attributeId, float previousValue, float currentValue)
	{
		(void)previousValue;
		(void)currentValue;
		if (attributeId == OwnerAttributeIds::MaxHealth || attributeId == OwnerAttributeIds::EnergyMax)
		{
			RecalculateAttributes();
		}
	}
}

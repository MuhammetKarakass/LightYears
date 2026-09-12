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
		if (mOwnerAttributes)
		{
			List<OwnerAttributeBaseEntry> profileAttributes = shipDefinition.baseOwnerAttributes;
			profileAttributes.push_back({
				OwnerAttributeIds::EnergyPower,
				std::max(0.f, mEnergyAttributes.baseEnergyPower)
			});
			RebindBaseOwnerAttributes(profileAttributes);
		}
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
		const float energyPower = std::max(
			0.f,
			mOwnerAttributes->GetCurrentValue(OwnerAttributeIds::EnergyPower)
		);
		RebindDerivedOwnerAttributes({ { OwnerAttributeIds::HealthRegen, maxHealth / 1200.f } });

		constexpr float ReactorBudgetPerEnergyPower = 2.f;
		const float reactorBudget = energyPower * ReactorBudgetPerEnergyPower;
		const float maxShield = std::max(0.f,
			mEnergyAttributes.baseMaxShield + reactorBudget * mEnergyAttributes.shieldAffinity
		);
		const float afterburnerCapacity = std::max(0.f,
			mEnergyAttributes.baseAfterburnerCapacity +
				reactorBudget * mEnergyAttributes.afterburnerAffinity
		);
		const float afterburnerRegen = afterburnerCapacity /
			std::max(0.001f, mEnergyAttributes.afterburnerFullRechargeDuration);

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
		RebindBaseOwnerAttributes({});
		RebindDerivedOwnerAttributes({});
		mAttributeSystem.Clear();
		mEnergyAttributes = ShipEnergyAttributes{};
	}

	void ShipRuntime::RebindOwnerAttributeContributions(const List<OwnerAttributeBaseEntry>& entries, List<OwnerAttributeBaseEntry>& appliedEntries)
	{
		if (!mOwnerAttributes)
		{
			return;
		}

		// Profile values are fixed base contributions; instant effects mutate the
		// resulting base value. Replace only our contribution, retaining those
		// mutations and all active modifier handles. Apply one write per attribute
		// so a temporary clamp between removing and adding cannot lose a debuff.
		List<OwnerAttributeBaseEntry> changes = entries;
		for (const OwnerAttributeBaseEntry& previous : appliedEntries)
		{
			auto replacement = std::find_if(changes.begin(), changes.end(),
				[&previous](const OwnerAttributeBaseEntry& entry) {
					return entry.attributeId == previous.attributeId;
				});
			if (replacement != changes.end())
			{
				replacement->baseValue -= previous.baseValue;
			}
			else
			{
				changes.push_back({ previous.attributeId, -previous.baseValue });
			}
		}
		for (const OwnerAttributeBaseEntry& change : changes)
		{
			mOwnerAttributes->SetBaseValue(change.attributeId,
				mOwnerAttributes->GetBaseValue(change.attributeId) + change.baseValue);
		}
		appliedEntries = entries;
	}

	void ShipRuntime::RebindBaseOwnerAttributes(const List<OwnerAttributeBaseEntry>& entries)
	{
		RebindOwnerAttributeContributions(entries, mAppliedBaseOwnerAttributes);
	}

	void ShipRuntime::RebindDerivedOwnerAttributes(const List<OwnerAttributeBaseEntry>& entries)
	{
		RebindOwnerAttributeContributions(entries, mAppliedDerivedOwnerAttributes);
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

	void ShipRuntime::OnOwnerAttributeChanged(sas::AttributeId attributeId, float previousValue, float currentValue)
	{
		(void)previousValue;
		(void)currentValue;
		if (attributeId == OwnerAttributeIds::MaxHealth || attributeId == OwnerAttributeIds::EnergyPower)
		{
			RecalculateAttributes();
		}
	}
}

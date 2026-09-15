#pragma once

#include "gameConfigs/ship/ShipStructs.h"
#include "attributes/AttributeSystem.h"

#include <unordered_map>

namespace ly
{
	// Resolves runtime values that belong to a ship body, independently of combat flow.
	// It deliberately depends only on owner attributes so future runtime owners can reuse it.
	class ShipRuntime
	{
	public:
		explicit ShipRuntime(sas::AttributeSystem& ownerAttributes);

		void InitializeFromShipDefinition(const ShipDefinition& shipDefinition);
		void RecalculateAttributes();
		bool SetBaseMaxShieldContribution(const std::string& sourceId, float value);
		bool RemoveBaseMaxShieldContribution(const std::string& sourceId);
		float GetAuthoredBaseMaxShield() const noexcept { return mEnergyAttributes.baseMaxShield; }
		void Clear();

		sas::AttributeSystem& GetAttributes() { return mAttributeSystem; }
		const sas::AttributeSystem& GetAttributes() const { return mAttributeSystem; }

		float GetAfterburnerCapacity() const;
		float GetAfterburnerRegenPerSecond() const;
		float GetAfterburnerRechargeDelay() const;
		float GetAfterburnerEnergyDrainPerSecond() const;
		float GetAfterburnerSpeedMultiplier() const;
		float GetAfterburnerAccelerationMultiplier() const;
		float GetAfterburnerRampUpDuration() const;
		float GetAfterburnerRampDownDuration() const;
		float GetAfterburnerManeuverabilityMultiplier() const;

	private:
		void RebindOwnerAttributeContributions(
			const List<OwnerAttributeBaseEntry>& entries,
			List<OwnerAttributeBaseEntry>& appliedEntries
		);
		void RebindBaseOwnerAttributes(const List<OwnerAttributeBaseEntry>& entries);
		void RebindDerivedOwnerAttributes(const List<OwnerAttributeBaseEntry>& entries);
		void OnOwnerAttributeChanged(sas::AttributeId attributeId, float previousValue, float currentValue);

		sas::AttributeSystem* mOwnerAttributes = nullptr;
		ShipEnergyAttributes mEnergyAttributes;
		std::unordered_map<std::string, float> mBaseMaxShieldContributions;
		List<OwnerAttributeBaseEntry> mAppliedBaseOwnerAttributes;
		List<OwnerAttributeBaseEntry> mAppliedDerivedOwnerAttributes;
		sas::AttributeSystem mAttributeSystem;
		bool mOwnerAttributeCallbackBound = false;
	};
}

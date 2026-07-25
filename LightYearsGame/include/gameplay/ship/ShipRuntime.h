#pragma once

#include "gameConfigs/ship/ShipStructs.h"
#include "gameplay/attributes/AttributeSystem.h"

namespace ly
{
	// Resolves runtime values that belong to a ship body, independently of combat flow.
	// It deliberately depends only on owner attributes so future runtime owners can reuse it.
	class ShipRuntime
	{
	public:
		explicit ShipRuntime(AttributeSystem& ownerAttributes);

		void InitializeFromShipDefinition(const ShipDefinition& shipDefinition);
		void RecalculateAttributes();
		void Clear();

		AttributeSystem& GetAttributes() { return mAttributeSystem; }
		const AttributeSystem& GetAttributes() const { return mAttributeSystem; }

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
		void OnOwnerAttributeChanged(GameplayTag attributeId, float previousValue, float currentValue);

		AttributeSystem* mOwnerAttributes = nullptr;
		ShipEnergyAttributes mEnergyAttributes;
		AttributeSystem mAttributeSystem;
		bool mOwnerAttributeCallbackBound = false;
	};
}

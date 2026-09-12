#pragma once

#include "framework/Delegate.h"
#include "gameConfigs/ship/ShipStructs.h"
#include "gameplay/attributes/AttributeIds.h"
#include "attributes/AttributeSystem.h"

#include <algorithm>
#include <array>

namespace ly
{
	class ShipProgression
	{
	public:
		void Configure(const ShipProgressionDefinition& definition);
		void BindAttributes(sas::AttributeSystem& attributes);
		// Removes progression modifiers from a live runtime before releasing it.
		void RemoveModifiersAndUnbind();
		// Call only after the bound ship starts destruction. It deliberately never
		// dereferences the previous runtime, whose lifetime may already be ending.
		void ForgetDestroyedAttributes();
		void AddXP(float amount);
		void ResetForNewRun();

		float GetXP() const { return mCurrentXP; }
		int GetLevel() const { return mCurrentLevel; }
		bool IsConfigured() const { return mIsConfigured; }
		float GetXPRequiredForNextLevel() const;

		Delegate<int, int> onLevelChanged;

	private:
		void RebuildLevelModifiers();
		void RemoveCurrentLevelModifiers();

		ShipProgressionDefinition mDefinition;
		sas::AttributeSystem* mBoundAttributes = nullptr;
		List<sas::AttributeModifierHandle> mLevelModifierHandles;
		float mCurrentXP = 0.f;
		int mCurrentLevel = 1;
		bool mIsConfigured = false;
	};

	inline bool IsNaturalGrowthAttribute(const sas::AttributeId& attributeId)
	{
		static const std::array<sas::AttributeId, 10> allowed{
			OwnerAttributeIds::MaxHealth, OwnerAttributeIds::AttackPower, OwnerAttributeIds::EnergyPower,
			OwnerAttributeIds::Armor, OwnerAttributeIds::Luck, OwnerAttributeIds::AttackSpeed,
			OwnerAttributeIds::CriticalChance, OwnerAttributeIds::AbilityHaste,
			OwnerAttributeIds::MoveSpeedHorizontal, OwnerAttributeIds::MoveSpeedVertical
		};
		return std::find(allowed.begin(), allowed.end(), attributeId) != allowed.end();
	}

	inline bool IsAllowedBaseOwnerCombatAttribute(const sas::AttributeId& attributeId)
	{
		static const std::array<sas::AttributeId, 7> allowed{
			OwnerAttributeIds::AttackPower, OwnerAttributeIds::Armor, OwnerAttributeIds::Luck,
			OwnerAttributeIds::AttackSpeed, OwnerAttributeIds::CriticalChance,
			OwnerAttributeIds::CriticalDamage, OwnerAttributeIds::AbilityHaste
		};
		return std::find(allowed.begin(), allowed.end(), attributeId) != allowed.end();
	}
}

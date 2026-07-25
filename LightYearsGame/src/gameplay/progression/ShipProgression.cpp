#include "gameplay/progression/ShipProgression.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace ly
{
	const List<AttributeGrowthEntry>& GetLevelGrowthBaseValues()
	{
		static const List<AttributeGrowthEntry> BaseValues{
			{ OwnerAttributeIds::MaxHealth, 8.f },
			{ OwnerAttributeIds::EnergyMax, 2.f },
			{ OwnerAttributeIds::AttackPower, 3.f },
			{ OwnerAttributeIds::AttackSpeed, 0.5f },
			{ OwnerAttributeIds::AbilityHaste, 0.5f },
			{ OwnerAttributeIds::MoveSpeedHorizontal, 0.2f },
			{ OwnerAttributeIds::MoveSpeedVertical, 0.2f },
			{ OwnerAttributeIds::Armor, 1.5f },
			{ OwnerAttributeIds::Luck, 0.3f },
			{ OwnerAttributeIds::CriticalChance, 0.35f }
		};
		return BaseValues;
	}

	bool IsLevelGrowthAttribute(const GameplayTag& attributeId)
	{
		for (const AttributeGrowthEntry& entry : GetLevelGrowthBaseValues())
		{
			if (entry.attributeId == attributeId)
			{
				return true;
			}
		}
		return false;
	}

	void ShipProgression::Configure(const ShipProgressionDefinition& definition)
	{
		mDefinition = definition;
		mIsConfigured = true;
		RebuildLevelModifiers();
	}

	void ShipProgression::BindAttributes(AttributeSystem& attributes)
	{
		if (mBoundAttributes == &attributes)
		{
			RebuildLevelModifiers();
			return;
		}

		// A respawn destroys the old runtime. Never dereference its old pointer;
		// bind the new runtime and rebuild the deterministic total bonus instead.
		mBoundAttributes = &attributes;
		mLevelModifierHandles.clear();
		RebuildLevelModifiers();
	}

	void ShipProgression::UnbindAttributes()
	{
		mBoundAttributes = nullptr;
		mLevelModifierHandles.clear();
	}

	void ShipProgression::AddXP(float amount)
	{
		if (amount <= 0.f)
		{
			return;
		}

		mCurrentXP += amount;
		const int previousLevel = mCurrentLevel;
		while (mCurrentXP >= GetXPRequiredForNextLevel())
		{
			mCurrentXP -= GetXPRequiredForNextLevel();
			++mCurrentLevel;
		}

		if (mCurrentLevel != previousLevel)
		{
			RebuildLevelModifiers();
			onLevelChanged.Broadcast(previousLevel, mCurrentLevel);
		}
	}

	void ShipProgression::ResetForNewRun()
	{
		mCurrentXP = 0.f;
		mCurrentLevel = 1;
		UnbindAttributes();
	}

	float ShipProgression::GetXPRequiredForNextLevel() const
	{
		return std::max(1.f, mDefinition.baseXP * std::pow(static_cast<float>(mCurrentLevel), mDefinition.xpExponent));
	}

	float ShipProgression::GetGrowthMultiplier(const GameplayTag& attributeId) const
	{
		for (const AttributeGrowthEntry& entry : mDefinition.growthOverrides)
		{
			if (entry.attributeId == attributeId)
			{
				return std::max(0.f, entry.multiplier);
			}
		}
		return 0.25f;
	}

	void ShipProgression::RebuildLevelModifiers()
	{
		if (!mBoundAttributes || !mIsConfigured)
		{
			return;
		}

		RemoveCurrentLevelModifiers();
		const float completedLevelCount = static_cast<float>(std::max(0, mCurrentLevel - 1));
		for (const AttributeGrowthEntry& baseGrowth : GetLevelGrowthBaseValues())
		{
			const float totalBonus = completedLevelCount * baseGrowth.multiplier * GetGrowthMultiplier(baseGrowth.attributeId);
			if (totalBonus <= 0.f)
			{
				continue;
			}
			mLevelModifierHandles.push_back(mBoundAttributes->AddModifier(
				AttributeModifier{ baseGrowth.attributeId, AttributeModifierOperation::Add, totalBonus }
			));
		}

		for (const AttributeGrowthEntry& overrideEntry : mDefinition.growthOverrides)
		{
			assert(IsLevelGrowthAttribute(overrideEntry.attributeId) && "Derived attributes cannot receive level growth.");
		}
	}

	void ShipProgression::RemoveCurrentLevelModifiers()
	{
		for (const AttributeModifierHandle handle : mLevelModifierHandles)
		{
			mBoundAttributes->RemoveModifier(handle);
		}
		mLevelModifierHandles.clear();
	}
}

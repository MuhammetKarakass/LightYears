#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/progression/ShipProgression.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace ly
{
	void ShipProgression::Configure(const ShipProgressionDefinition& definition)
	{
		mDefinition = definition;
		mIsConfigured = true;
		RebuildLevelModifiers();
	}

	void ShipProgression::BindAttributes(sas::AttributeSystem& attributes)
	{
		if (mBoundAttributes == &attributes)
		{
			RebuildLevelModifiers();
			return;
		}

		if (mBoundAttributes)
		{
			RemoveCurrentLevelModifiers();
		}
		mBoundAttributes = &attributes;
		mLevelModifierHandles.clear();
		RebuildLevelModifiers();
	}

	void ShipProgression::RemoveModifiersAndUnbind()
	{
		if (mBoundAttributes)
		{
			RemoveCurrentLevelModifiers();
		}
		mBoundAttributes = nullptr;
	}

	void ShipProgression::ForgetDestroyedAttributes()
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
		RemoveModifiersAndUnbind();
	}

	float ShipProgression::GetXPRequiredForNextLevel() const
	{
		return std::max(1.f, mDefinition.baseXP * std::pow(static_cast<float>(mCurrentLevel), mDefinition.xpExponent));
	}

	void ShipProgression::RebuildLevelModifiers()
	{
		if (!mBoundAttributes || !mIsConfigured)
		{
			return;
		}

		RemoveCurrentLevelModifiers();
		const float completedLevelCount = static_cast<float>(std::max(0, mCurrentLevel - 1));
		for (const AttributeGrowthEntry& growth : mDefinition.naturalGrowth)
		{
			assert(IsNaturalGrowthAttribute(growth.attributeId) &&
				"Only approved owner attributes may receive natural ship growth.");
			const float totalBonus = completedLevelCount * std::max(0.f, growth.perLevel);
			if (totalBonus <= 0.f)
			{
				continue;
			}
			mLevelModifierHandles.push_back(mBoundAttributes->AddModifier(
				sas::AttributeModifier{ growth.attributeId, sas::AttributeModifierOperation::Add, totalBonus }
			));
		}
	}

	void ShipProgression::RemoveCurrentLevelModifiers()
	{
		for (const sas::AttributeModifierHandle handle : mLevelModifierHandles)
		{
			mBoundAttributes->RemoveModifier(handle);
		}
		mLevelModifierHandles.clear();
	}
}

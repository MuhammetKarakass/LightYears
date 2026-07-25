#pragma once

#include "framework/Delegate.h"
#include "gameConfigs/ship/ShipStructs.h"
#include "gameplay/attributes/AttributeSystem.h"

namespace ly
{
	class ShipProgression
	{
	public:
		void Configure(const ShipProgressionDefinition& definition);
		void BindAttributes(AttributeSystem& attributes);
		// Call when the currently bound ship begins destruction. This deliberately
		// does not dereference the previous runtime, whose lifetime may already be ending.
		void UnbindAttributes();
		void AddXP(float amount);
		void ResetForNewRun();

		float GetXP() const { return mCurrentXP; }
		int GetLevel() const { return mCurrentLevel; }
		bool IsConfigured() const { return mIsConfigured; }
		float GetXPRequiredForNextLevel() const;
		float GetGrowthMultiplier(const GameplayTag& attributeId) const;

		Delegate<int, int> onLevelChanged;

	private:
		void RebuildLevelModifiers();
		void RemoveCurrentLevelModifiers();

		ShipProgressionDefinition mDefinition;
		AttributeSystem* mBoundAttributes = nullptr;
		List<AttributeModifierHandle> mLevelModifierHandles;
		float mCurrentXP = 0.f;
		int mCurrentLevel = 1;
		bool mIsConfigured = false;
	};

	const List<AttributeGrowthEntry>& GetLevelGrowthBaseValues();
	bool IsLevelGrowthAttribute(const GameplayTag& attributeId);
}

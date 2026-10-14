#pragma once

#include "abilities/AbilityTypes.h"
#include "gameplay/tags/GameplayTags.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ly
{
	struct CombatRuntimeModifier
	{
		float outgoingDamageMultiplier = 1.f;
		// Unscoped modifiers apply to every outgoing hit. A scoped modifier applies only to hits whose source ability sits in
		// sourceSlot (when set) and carries sourceFamily (when valid).
		sas::AbilitySlot sourceSlot = sas::AbilitySlot::None;
		GameplayTag sourceFamily;

		bool IsScoped() const { return sourceSlot != sas::AbilitySlot::None || sourceFamily.IsValid(); }
	};

	class CombatRuntimeModifiers final
	{
	public:
		CombatRuntimeModifiers() = default;

		bool Set(const std::string& sourceId, CombatRuntimeModifier modifier);
		bool Set(const std::string& sourceId, float multiplier);
		bool Remove(const std::string& sourceId);
		void Clear();

		float GetOutgoingDamageMultiplier() const noexcept
		{
			return mCachedMultiplier;
		}

		// Product of the scoped modifiers that match the source ability; 1 when none match.
		float GetScopedOutgoingDamageMultiplier(sas::AbilitySlot sourceSlot, const std::vector<GameplayTag>& sourceTags) const;

		bool HasModifier(const std::string& sourceId) const;
		size_t Size() const noexcept { return mModifiers.size() + mScopedModifiers.size(); }
		bool Empty() const noexcept { return mModifiers.empty() && mScopedModifiers.empty(); }

	private:
		struct ScopedModifier
		{
			float multiplier = 1.f;
			sas::AbilitySlot sourceSlot = sas::AbilitySlot::None;
			GameplayTag sourceFamily;
		};

		void RecomputeCachedMultiplier();

		std::unordered_map<std::string, float> mModifiers;
		std::unordered_map<std::string, ScopedModifier> mScopedModifiers;
		float mCachedMultiplier = 1.f;
	};
}

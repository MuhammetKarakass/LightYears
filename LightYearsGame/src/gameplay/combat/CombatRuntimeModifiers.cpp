#include "gameplay/combat/CombatRuntimeModifiers.h"
#include "gameplay/math/MultiplierMath.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ly
{
	bool CombatRuntimeModifiers::Set(const std::string& sourceId, float multiplier)
	{
		if (sourceId.empty() || !std::isfinite(multiplier) || multiplier < 0.f)
		{
			return false;
		}

		mScopedModifiers.erase(sourceId);
		mModifiers[sourceId] = multiplier;
		RecomputeCachedMultiplier();
		return true;
	}

	bool CombatRuntimeModifiers::Set(const std::string& sourceId, CombatRuntimeModifier modifier)
	{
		if (!modifier.IsScoped())
		{
			return Set(sourceId, modifier.outgoingDamageMultiplier);
		}
		if (sourceId.empty() || !std::isfinite(modifier.outgoingDamageMultiplier) || modifier.outgoingDamageMultiplier < 0.f)
		{
			return false;
		}

		mModifiers.erase(sourceId);
		mScopedModifiers[sourceId] = ScopedModifier{ modifier.outgoingDamageMultiplier, modifier.sourceSlot, modifier.sourceFamily };
		RecomputeCachedMultiplier();
		return true;
	}

	bool CombatRuntimeModifiers::Remove(const std::string& sourceId)
	{
		const bool scopedRemoved = mScopedModifiers.erase(sourceId) > 0;
		auto it = mModifiers.find(sourceId);
		if (it == mModifiers.end())
		{
			if (scopedRemoved) RecomputeCachedMultiplier();
			return scopedRemoved;
		}

		mModifiers.erase(it);
		RecomputeCachedMultiplier();
		return true;
	}

	void CombatRuntimeModifiers::Clear()
	{
		mModifiers.clear();
		mScopedModifiers.clear();
		mCachedMultiplier = 1.f;
	}

	float CombatRuntimeModifiers::GetScopedOutgoingDamageMultiplier(sas::AbilitySlot sourceSlot, const std::vector<GameplayTag>& sourceTags) const
	{
		float multiplier = 1.f;
		for (const auto& [_, scoped] : mScopedModifiers)
		{
			if (scoped.sourceSlot != sas::AbilitySlot::None && scoped.sourceSlot != sourceSlot) continue;
			if (scoped.sourceFamily.IsValid() && !std::any_of(sourceTags.begin(), sourceTags.end(),
				[&scoped](const GameplayTag& tag) { return tag.MatchesTag(scoped.sourceFamily); })) continue;
			multiplier *= scoped.multiplier;
		}
		return multiplier;
	}

	bool CombatRuntimeModifiers::HasModifier(const std::string& sourceId) const
	{
		return mModifiers.find(sourceId) != mModifiers.end() || mScopedModifiers.find(sourceId) != mScopedModifiers.end();
	}

	void CombatRuntimeModifiers::RecomputeCachedMultiplier()
	{
		if (mModifiers.empty())
		{
			mCachedMultiplier = 1.f;
			return;
		}

		std::vector<float> values;
		values.reserve(mModifiers.size());
		for (const auto& [_, val] : mModifiers)
		{
			values.push_back(val);
		}
		float resolvedMultiplier = 0.f;
		mCachedMultiplier = math::TryResolveMultiplierProduct(
			values.data(),
			values.size(),
			resolvedMultiplier
		) ? resolvedMultiplier : 0.f;
	}
}

#include "gameplay/combat/CombatRuntimeModifiers.h"
#include "gameplay/math/MultiplierMath.h"

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

		mModifiers[sourceId] = multiplier;
		RecomputeCachedMultiplier();
		return true;
	}

	bool CombatRuntimeModifiers::Set(const std::string& sourceId, CombatRuntimeModifier modifier)
	{
		return Set(sourceId, modifier.outgoingDamageMultiplier);
	}

	bool CombatRuntimeModifiers::Remove(const std::string& sourceId)
	{
		auto it = mModifiers.find(sourceId);
		if (it == mModifiers.end())
		{
			return false;
		}

		mModifiers.erase(it);
		RecomputeCachedMultiplier();
		return true;
	}

	void CombatRuntimeModifiers::Clear()
	{
		mModifiers.clear();
		mCachedMultiplier = 1.f;
	}

	bool CombatRuntimeModifiers::HasModifier(const std::string& sourceId) const
	{
		return mModifiers.find(sourceId) != mModifiers.end();
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

#pragma once

#include <string>
#include <unordered_map>

namespace ly
{
	struct CombatRuntimeModifier
	{
		float outgoingDamageMultiplier = 1.f;
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

		bool HasModifier(const std::string& sourceId) const;
		size_t Size() const noexcept { return mModifiers.size(); }
		bool Empty() const noexcept { return mModifiers.empty(); }

	private:
		void RecomputeCachedMultiplier();

		std::unordered_map<std::string, float> mModifiers;
		float mCachedMultiplier = 1.f;
	};
}

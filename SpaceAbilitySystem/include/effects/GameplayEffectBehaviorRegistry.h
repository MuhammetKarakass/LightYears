#pragma once

#include "effects/GameplayEffectBehaviorKey.h"

#include <unordered_map>
#include <utility>

namespace sas
{
	template <
		typename Hooks,
		typename Key = GameplayEffectBehaviorKey,
		typename Hash = std::hash<Key>
	>
	class GameplayEffectBehaviorRegistry
	{
	public:
		bool Register(const Key& behaviorKey, Hooks hooks)
		{
			return mHooks.emplace(behaviorKey, std::move(hooks)).second;
		}

		bool IsRegistered(const Key& behaviorKey) const
		{
			return mHooks.find(behaviorKey) != mHooks.end();
		}

		const Hooks* Find(const Key& behaviorKey) const
		{
			const auto found = mHooks.find(behaviorKey);
			return found != mHooks.end() ? &found->second : nullptr;
		}

		void Clear()
		{
			mHooks.clear();
		}

	private:
		std::unordered_map<Key, Hooks, Hash> mHooks;
	};
}

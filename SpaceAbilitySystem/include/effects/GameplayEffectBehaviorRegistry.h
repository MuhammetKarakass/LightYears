#pragma once

#include "framework/Core.h"

#include <unordered_map>
#include <utility>

namespace sas
{
	template <typename Hooks>
	class GameplayEffectBehaviorRegistry
	{
	public:
		bool Register(const ly::GameplayTag& behaviorTag, Hooks hooks)
		{
			return mHooks.emplace(behaviorTag, std::move(hooks)).second;
		}

		bool IsRegistered(const ly::GameplayTag& behaviorTag) const
		{
			return mHooks.find(behaviorTag) != mHooks.end();
		}

		const Hooks* Find(const ly::GameplayTag& behaviorTag) const
		{
			const auto found = mHooks.find(behaviorTag);
			return found != mHooks.end() ? &found->second : nullptr;
		}

		void Clear()
		{
			mHooks.clear();
		}

	private:
		std::unordered_map<ly::GameplayTag, Hooks, ly::GameplayTagHash> mHooks;
	};
}

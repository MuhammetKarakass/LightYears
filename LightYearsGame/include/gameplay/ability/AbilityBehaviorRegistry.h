#pragma once

#include "gameplay/ability/AbilityBehavior.h"

#include <functional>

namespace ly
{
	using AbilityBehaviorFactory = std::function<unique_ptr<AbilityBehavior>()>;

	class AbilityBehaviorRegistry
	{
	public:
		static bool Register(const GameplayTag& behaviorId, AbilityBehaviorFactory factory);
		static bool IsRegistered(const GameplayTag& behaviorId);
		static unique_ptr<AbilityBehavior> Create(const GameplayTag& behaviorId);
	};
}

#pragma once

#include "gameConfigs/ability/AbilityStructs.h"

#include <string>

namespace ly
{
	class AbilityInstance;
	class AbilitySystem;
	class Actor;

	struct AbilityBehaviorContext
	{
		AbilitySystem& abilitySystem;
		AbilityInstance& instance;
		Actor& owner;
		const AbilityDefinition& definition;
	};

	class AbilityBehavior
	{
	public:
		virtual ~AbilityBehavior() = default;

		virtual bool Validate(
			const AbilityDefinition& definition,
			std::string* failureReason = nullptr
		) const;
		virtual bool Activate(AbilityBehaviorContext& context);
		virtual void Tick(AbilityBehaviorContext& context, float deltaTime);
		virtual void End(AbilityBehaviorContext& context, AbilityEndReason reason);
	};
}

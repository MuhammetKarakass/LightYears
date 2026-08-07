#pragma once

#include "gameplay/ability/GameAbilityActionExecutor.h"

namespace ly
{
	// Owns the persistent fire lifecycle for FireWeaponAction while the generic
	// executor retains scheduling and dispatch for every action variant.
	class FireWeaponActionRuntime final
	{
	public:
		static void Execute(ActiveAbilityAction& action, AbilityExecutionContext& context);
		static void Tick(
			ActiveAbilityAction& action,
			AbilityExecutionContext& context,
			float deltaTime
		);
		static void End(ActiveAbilityAction& action, AbilityExecutionContext& context);
	};
}

#pragma once

#include "gameplay/ability/AbilityExecution.h"
#include "gameplay/ability/AbilityEvent.h"


namespace ly
{
	class AbilitySystem;

	struct AbilityExecutionContext
	{
		AbilitySystem* abilitySystem = nullptr;
		const AbilityDefinition* definition = nullptr;
		const AbilityEvent* event = nullptr;
	};

	class AbilityExecutor
	{
	public:
		static void BeginExecution(AbilityExecution& execution, AbilityExecutionContext& context);
		static void TickExecution(AbilityExecution& execution, AbilityExecutionContext& context, float deltaTime);
		static void EndExecution(AbilityExecution& execution, AbilityExecutionContext& context, AbilityEndReason reason);

	private:
		static void AddPhaseActions(AbilityExecution& execution, const List<AbilityActionSpec>& actions, AbilityActionPhase phase);
		static void TickAction(ActiveAbilityAction& action, AbilityExecutionContext& context, float deltaTime);
		static void ExecuteAction(ActiveAbilityAction& action, AbilityExecutionContext& context);
	};
}



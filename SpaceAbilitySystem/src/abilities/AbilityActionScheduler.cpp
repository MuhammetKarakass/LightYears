#include "abilities/AbilityActionScheduler.h"

namespace sas
{
	bool AbilityActionScheduler::IsExecutionDue(
		RepeatedAbilityActionState& state,
		float deltaTime,
		int maxExecutions
	)
	{
		state.intervalRemaining -= deltaTime;
		return state.intervalRemaining <= 0.f &&
			(maxExecutions <= 0 || state.executionCount < maxExecutions);
	}

	void AbilityActionScheduler::RecordExecution(
		RepeatedAbilityActionState& state,
		float nextInterval
	)
	{
		++state.executionCount;
		state.intervalRemaining = nextInterval;
	}
}

#pragma once

namespace sas
{
	struct RepeatedAbilityActionState
	{
		float intervalRemaining = 0.f;
		int executionCount = 0;
	};

	class AbilityActionScheduler
	{
	public:
		static bool IsExecutionDue(
			RepeatedAbilityActionState& state,
			float deltaTime,
			int maxExecutions
		);
		static void RecordExecution(
			RepeatedAbilityActionState& state,
			float nextInterval
		);
	};
}

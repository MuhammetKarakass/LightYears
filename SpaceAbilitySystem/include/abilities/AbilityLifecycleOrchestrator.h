#pragma once

#include "abilities/AbilityPolicies.h"
#include "abilities/AbilityRuntimeState.h"

namespace sas
{
	struct AbilityLifecycleDecision
	{
		bool requestActivation = false;
		bool requestEnd = false;
		AbilityEndReason endReason = AbilityEndReason::Completed;
		bool stateChanged = false;
	};

	class AbilityLifecycleOrchestrator
	{
	public:
		static AbilityLifecycleDecision EvaluateInput(
			AbilityActivationPolicy activationPolicy,
			AbilityLifetimePolicy lifetimePolicy,
			const AbilityRuntimeState& state
		);
		static AbilityLifecycleDecision TickActiveDuration(
			AbilityLifetimePolicy lifetimePolicy,
			AbilityRuntimeState& state,
			float deltaTime
		);
		static bool ShouldCompleteImmediately(AbilityLifetimePolicy lifetimePolicy);
	};
}

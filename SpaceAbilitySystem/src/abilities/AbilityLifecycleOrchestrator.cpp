#include "abilities/AbilityLifecycleOrchestrator.h"

#include <algorithm>

namespace sas
{
	AbilityLifecycleDecision AbilityLifecycleOrchestrator::EvaluateInput(
		AbilityActivationPolicy activationPolicy,
		AbilityLifetimePolicy lifetimePolicy,
		const AbilityRuntimeState& state,
		float minimumToggleActiveDuration
	)
	{
		AbilityLifecycleDecision decision;
		switch (activationPolicy)
		{
		case AbilityActivationPolicy::OnPressed:
			decision.requestActivation = state.IsPressedThisFrame();
			break;
		case AbilityActivationPolicy::WhileHeld:
			decision.requestActivation = state.IsInputHeld();
			decision.requestEnd =
				!state.IsInputHeld() &&
				state.IsActive() &&
				lifetimePolicy == AbilityLifetimePolicy::WhileInputHeld;
			decision.endReason = AbilityEndReason::InputReleased;
			break;
		case AbilityActivationPolicy::Toggle:
			if (state.IsPressedThisFrame())
			{
				if (!state.IsActive())
				{
					decision.requestActivation = true;
				}
				else if (state.GetActiveTimeElapsed() >=
					std::max(0.f, minimumToggleActiveDuration))
				{
					decision.requestEnd = true;
					decision.endReason = AbilityEndReason::Cancelled;
				}
			}
			break;
		case AbilityActivationPolicy::Passive:
			decision.requestActivation = true;
			break;
		case AbilityActivationPolicy::GameplayEvent:
			break;
		}
		return decision;
	}

	AbilityLifecycleDecision AbilityLifecycleOrchestrator::TickActiveDuration(
		AbilityLifetimePolicy lifetimePolicy,
		AbilityRuntimeState& state,
		float deltaTime
	)
	{
		AbilityLifecycleDecision decision;
		if (lifetimePolicy != AbilityLifetimePolicy::Duration ||
			state.IsActiveDurationDeferred() ||
			state.GetActiveTimeRemaining() <= 0.f)
		{
			return decision;
		}

		decision.stateChanged = state.TickActiveDuration(deltaTime);
		if (state.GetActiveTimeRemaining() <= 0.f)
		{
			decision.requestEnd = true;
			decision.endReason = AbilityEndReason::DurationExpired;
		}
		return decision;
	}

	bool AbilityLifecycleOrchestrator::ShouldCompleteImmediately(
		AbilityLifetimePolicy lifetimePolicy
	)
	{
		return lifetimePolicy == AbilityLifetimePolicy::Instant;
	}
}

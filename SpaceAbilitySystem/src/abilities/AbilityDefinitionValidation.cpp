#include "abilities/AbilityDefinitionValidation.h"

namespace sas
{
	bool ValidateAbilityDefinition(
		const AbilityDefinition& definition,
		std::string* failureReason
	)
	{
		if (definition.abilityId.empty())
		{
			if (failureReason)
			{
				*failureReason = "Ability definition requires an ID.";
			}
			return false;
		}
		if (definition.cooldown < 0.f || definition.duration < 0.f || definition.maxCharges < 0)
		{
			if (failureReason)
			{
				*failureReason = "Ability cooldown, duration, and charge count must be non-negative.";
			}
			return false;
		}
		if (definition.lifetimePolicy == AbilityLifetimePolicy::Duration && definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Duration abilities require a positive duration.";
			}
			return false;
		}

		const bool isPassive = definition.slot == AbilitySlot::None;
		if (isPassive &&
			definition.activationPolicy == AbilityActivationPolicy::Passive &&
			definition.lifetimePolicy != AbilityLifetimePolicy::UntilCancelled)
		{
			if (failureReason)
			{
				*failureReason = "Passive abilities must use UntilCancelled lifetime.";
			}
			return false;
		}

		// Slot responsibility guard during the AbilitySlot migration. The
		// definition slot still decides whether the runtime registers an ability
		// as passive (never reachable through slot input) or as an input-slot
		// ability. Reject definitions whose activation policy contradicts that
		// registration so the duplicated definition/runtime slot ownership cannot
		// silently disagree. Slot-less event-driven abilities stay valid: the two
		// models are not treated as fully equivalent yet.
		const bool isInputDriven =
			definition.activationPolicy == AbilityActivationPolicy::OnPressed ||
			definition.activationPolicy == AbilityActivationPolicy::WhileHeld ||
			definition.activationPolicy == AbilityActivationPolicy::Toggle;
		if (isPassive && isInputDriven)
		{
			if (failureReason)
			{
				*failureReason =
					"Abilities without an input slot must not use an input activation policy.";
			}
			return false;
		}
		if (definition.activationPolicy == AbilityActivationPolicy::Passive &&
			definition.slot != AbilitySlot::None)
		{
			if (failureReason)
			{
				*failureReason = "Passive abilities must not declare an input slot.";
			}
			return false;
		}

		// WhileInputHeld only terminates through the WhileHeld input path in
		// AbilityLifecycleOrchestrator, so any other activation policy declares a
		// lifetime the runtime can never fulfill.
		if (definition.lifetimePolicy == AbilityLifetimePolicy::WhileInputHeld &&
			definition.activationPolicy != AbilityActivationPolicy::WhileHeld)
		{
			if (failureReason)
			{
				*failureReason = "WhileInputHeld abilities must use WhileHeld activation.";
			}
			return false;
		}
		return true;
	}
}

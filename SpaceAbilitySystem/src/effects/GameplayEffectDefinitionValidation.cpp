#include "effects/GameplayEffectDefinitionValidation.h"

#include <cmath>
#include <unordered_set>

namespace sas
{
	namespace
	{
		bool Fail(std::string* failureReason, const std::string& reason)
		{
			if (failureReason)
			{
				*failureReason = reason;
			}
			return false;
		}
	}

	bool ValidateGameplayEffectDefinition(
		const GameplayEffectDefinition& definition,
		std::string* failureReason
	)
	{
		if (definition.effectId.empty())
		{
			return Fail(failureReason, "Gameplay effect ID cannot be empty.");
		}
		if (definition.durationPolicy == GameplayEffectDurationPolicy::Duration &&
			!definition.sourceParameterized &&
			(!std::isfinite(definition.duration) || definition.duration <= 0.f))
		{
			return Fail(
				failureReason,
				"Duration gameplay effect '" + definition.effectId +
					"' requires a positive finite duration."
			);
		}
		if (definition.maxStacks < 1)
		{
			return Fail(
				failureReason,
				"Gameplay effect '" + definition.effectId +
					"' requires maxStacks >= 1."
			);
		}
		for (const AttributeModifier& modifier : definition.modifiers)
		{
			if (!modifier.attributeId.IsValid() ||
				!std::isfinite(modifier.magnitude))
			{
				return Fail(
					failureReason,
					"Gameplay effect '" + definition.effectId +
						"' contains an invalid modifier."
				);
			}
		}
		for (const GameplayAttribute& attribute : definition.attributes)
		{
			if (!attribute.id.IsValid() ||
				!std::isfinite(attribute.baseValue) ||
				!std::isfinite(attribute.currentValue) ||
				!std::isfinite(attribute.minValue) ||
				!std::isfinite(attribute.maxValue) ||
				attribute.minValue > attribute.maxValue)
			{
				return Fail(
					failureReason,
					"Gameplay effect '" + definition.effectId +
						"' contains an invalid runtime attribute."
				);
			}
		}
		return true;
	}

	bool ValidateGameplayEffectDefinitionCatalog(
		const ly::List<const GameplayEffectDefinition*>& definitions,
		std::string* failureReason
	)
	{
		std::unordered_set<std::string> effectIds;
		for (const GameplayEffectDefinition* definition : definitions)
		{
			if (!definition)
			{
				return Fail(
					failureReason,
					"Gameplay-effect catalog contains a null definition."
				);
			}
			if (!ValidateGameplayEffectDefinition(*definition, failureReason))
			{
				return false;
			}
			if (!effectIds.emplace(definition->effectId).second)
			{
				return Fail(
					failureReason,
					"Gameplay-effect catalog contains duplicate effect ID '" +
						definition->effectId + "'."
				);
			}
		}
		return true;
	}
}

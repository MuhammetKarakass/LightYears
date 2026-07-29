#include "gameplay/effects/GameplayEffectValidation.h"

#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/effects/GameplayEffectBehavior.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"

#include <cmath>

namespace ly
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
		if (definition.behaviorTag.IsValid() &&
			!GameplayEffectBehavior::IsBehaviorRegistered(definition.behaviorTag))
		{
			return Fail(
				failureReason,
				"Gameplay effect '" + definition.effectId +
					"' references an unregistered behavior."
			);
		}
		if (!definition.activeVisualId.empty() &&
			!GameplayEffectVisualRegistry::IsRegistered(definition.activeVisualId))
		{
			return Fail(
				failureReason,
				"Gameplay effect '" + definition.effectId +
					"' references an unregistered visual."
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

	bool ValidateShippedGameplayEffectDefinitions(std::string* failureReason)
	{
		const List<const GameplayEffectDefinition*>& definitions =
			EffectData::GetShippedGameplayEffectDefinitions();
		for (size_t index = 0; index < definitions.size(); ++index)
		{
			const GameplayEffectDefinition* definition = definitions[index];
			if (!definition ||
				!ValidateGameplayEffectDefinition(*definition, failureReason))
			{
				return false;
			}
			for (size_t previousIndex = 0; previousIndex < index; ++previousIndex)
			{
				if (definitions[previousIndex] &&
					definitions[previousIndex]->effectId == definition->effectId)
				{
					return Fail(
						failureReason,
						"Shipped gameplay-effect catalog contains duplicate effect IDs."
					);
				}
			}
		}
		return true;
	}
}

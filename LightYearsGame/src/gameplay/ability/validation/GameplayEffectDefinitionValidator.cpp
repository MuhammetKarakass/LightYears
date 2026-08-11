#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"

#include "effects/GameplayEffectDefinitionValidation.h"
#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"

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

		bool ValidateAttributeList(
			const sas::GameplayAttributeList& attributes,
			std::string* failureReason
		)
		{
			for (const sas::GameplayAttribute& attribute : attributes)
			{
				if (!AttributeIdSchema::Validate(attribute.id, nullptr))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateAttributeModifiers(
			const List<sas::AttributeModifier>& modifiers,
			std::string* failureReason
		)
		{
			for (const sas::AttributeModifier& modifier : modifiers)
			{
				if (!AttributeIdSchema::Validate(modifier.attributeId, nullptr))
				{
					return false;
				}
			}
			return true;
		}
	}

	bool GameplayEffectDefinitionValidator::Validate(
		const sas::GameplayEffectDefinition& definition,
		const BehaviorRegistrationQuery& isBehaviorRegistered,
		std::string* failureReason
	)
	{
		if (!sas::ValidateGameplayEffectDefinition(definition, failureReason) ||
			!content::ContentIdSchema::ValidateEffectId(definition.effectId, failureReason) ||
			!ValidateAttributeModifiers(definition.modifiers, failureReason) ||
			!ValidateAttributeList(definition.attributes, failureReason))
		{
			return false;
		}
		for (const GameplayTag& tag : definition.grantedTags)
		{
			if (!GameplayTagSchema::ValidateEffectGrantedTag(tag, failureReason))
			{
				return false;
			}
			if (tag.name == definition.effectId)
			{
				return Fail(
					failureReason,
					"Gameplay effect ID must not also be used as a granted tag."
				);
			}
		}
		for (const GameplayTag& tag : definition.applicationRequiredTags)
		{
			if (!GameplayTagSchema::ValidateEffectApplicationTag(tag, failureReason))
			{
				return false;
			}
		}
		for (const GameplayTag& tag : definition.applicationBlockedTags)
		{
			if (!GameplayTagSchema::ValidateEffectApplicationTag(tag, failureReason))
			{
				return false;
			}
		}
		if (definition.behaviorKey.IsValid() &&
			(!isBehaviorRegistered || !isBehaviorRegistered(definition.behaviorKey)))
		{
			return Fail(
				failureReason,
				"Gameplay effect '" + definition.effectId +
					"' references an unregistered behavior."
			);
		}
		if (!definition.activeVisualId.empty() &&
			!content::ContentIdSchema::ValidateGameplayEffectVisualId(
				definition.activeVisualId,
				failureReason
			))
		{
			return false;
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
		return true;
	}

	bool GameplayEffectDefinitionValidator::ValidateCatalog(
		const List<const sas::GameplayEffectDefinition*>& definitions,
		const BehaviorRegistrationQuery& isBehaviorRegistered,
		std::string* failureReason
	)
	{
		if (!sas::ValidateGameplayEffectDefinitionCatalog(definitions, failureReason))
		{
			return false;
		}
		for (const sas::GameplayEffectDefinition* definition : definitions)
		{
			if (!definition || !Validate(*definition, isBehaviorRegistered, failureReason))
			{
				return false;
			}
		}
		return true;
	}
}

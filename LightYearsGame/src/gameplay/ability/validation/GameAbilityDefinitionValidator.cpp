#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"

#include "abilities/AbilityDefinitionValidation.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace ly
{
	// Keep this translation unit dependent on the complete behavior selector so
	// newly registered concrete families are reflected in runtime validation.
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

		bool ValidateAttributeId(const sas::AttributeId& id, std::string* failureReason)
		{
			return AttributeIdSchema::Validate(id, failureReason);
		}

		bool ValidateAttributeModifiers(
			const List<sas::AttributeModifier>& modifiers,
			std::string* failureReason
		)
		{
			for (const sas::AttributeModifier& modifier : modifiers)
			{
				if (!ValidateAttributeId(modifier.attributeId, failureReason))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateAbilityAttributeTarget(
			const GameAbilityDefinition& ability,
			std::string_view familyNamespace,
			const sas::AttributeId& targetAttributeId,
			std::string* failureReason
		)
		{
			if (!ValidateAttributeId(targetAttributeId, failureReason))
			{
				return false;
			}

			// Only Ability.* targets are owned by an ability definition. Other
			// domains (for example Owner.*, Effect.*, and AbilityActor.*) retain
			// their existing consumer-specific validation contracts.
			if (!AttributeIdSchema::IsInNamespace(targetAttributeId, "Ability"))
			{
				return true;
			}

			if (familyNamespace.empty() ||
				!AttributeIdSchema::IsInNamespace(targetAttributeId, familyNamespace))
			{
				return Fail(
					failureReason,
					"Ability attribute targets must belong to the family encoded in the ability ID."
				);
			}

			if (!sas::FindAttribute(ability.attributes, targetAttributeId))
			{
				return Fail(
					failureReason,
					"Ability attribute targets must be declared in the ability attributes list."
				);
			}
			return true;
		}

		bool ValidateAbilityAttributeModifiers(
			const GameAbilityDefinition& ability,
			std::string_view familyNamespace,
			const List<sas::AttributeModifier>& modifiers,
			std::string* failureReason
		)
		{
			for (const sas::AttributeModifier& modifier : modifiers)
			{
				if (!ValidateAbilityAttributeTarget(
					ability,
					familyNamespace,
					modifier.attributeId,
					failureReason
				))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateScalingRules(
			const GameAbilityDefinition& ability,
			std::string_view familyNamespace,
			const List<sas::AttributeScalingRule>& scalingRules,
			std::string* failureReason
		)
		{
			for (const sas::AttributeScalingRule& rule : scalingRules)
			{
				if (!ValidateAbilityAttributeTarget(
					ability,
					familyNamespace,
					rule.targetAttributeId,
					failureReason
				) ||
					!ValidateAttributeId(rule.sourceAttributeId, failureReason))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateAbilityScopedAttributes(
			const GameAbilityDefinition& ability,
			std::string_view familyNamespace,
			std::string* failureReason
		)
		{
			List<sas::AttributeId> declaredIds;
			for (const sas::GameplayAttribute& attribute : ability.attributes)
			{
				if (!ValidateAttributeId(attribute.id, failureReason))
				{
					return false;
				}
				const bool isCommonAttribute = AttributeIdSchema::IsInNamespace(
					attribute.id,
					"Common"
				);
				const bool isOwnedAbilityAttribute = !familyNamespace.empty() &&
					AttributeIdSchema::IsInNamespace(attribute.id, familyNamespace);
				if (!isCommonAttribute && !isOwnedAbilityAttribute)
				{
					return Fail(
						failureReason,
						"Ability-scoped attributes must belong to Common.* or the family encoded in the ability ID."
					);
				}
				if (attribute.id == CommonAttributeIds::ProjectileCount &&
					(attribute.baseValue < 1.f ||
						std::round(attribute.baseValue) != attribute.baseValue ||
						attribute.minValue < 1.f))
				{
					return Fail(
						failureReason,
						"Common.ProjectileCount must have an integer base value and a minimum of one."
					);
				}
				if (std::find(declaredIds.begin(), declaredIds.end(), attribute.id) != declaredIds.end())
				{
					return Fail(
						failureReason,
						"Ability-scoped attribute IDs must be unique."
					);
				}
				declaredIds.push_back(attribute.id);
			}
			return true;
		}

		bool ValidateOwnedEffectSpecs(
			const GameAbilityDefinition& ability,
			std::string* failureReason
		)
		{
			for (const AbilityEffectSpecDefinition& spec : ability.effectSpecs)
			{
				const sas::GameplayEffectDefinition* effect =
					EffectData::FindGameplayEffectDefinition(spec.effectId.ToString());
				if (!effect)
				{
					return Fail(failureReason, "Ability effectSpecs references an unknown effect.");
				}
				const float duration = spec.useAbilityDuration
					? ability.duration
					: spec.duration.value_or(effect->duration);
				if (effect->durationPolicy == sas::GameplayEffectDurationPolicy::Duration &&
					(!std::isfinite(duration) || duration <= 0.f))
				{
					return Fail(
						failureReason,
						"Ability-owned duration must be positive for effect '" +
							spec.effectId.ToString() + "'."
					);
				}
				if (spec.maxStacks.has_value() && *spec.maxStacks < 1)
				{
					return Fail(failureReason, "Ability-owned maxStacks must be at least one.");
				}
				if (!ValidateAttributeModifiers(spec.modifiers, failureReason))
				{
					return false;
				}
				for (const sas::GameplayAttribute& attribute : spec.attributes)
				{
					if (!ValidateAttributeId(attribute.id, failureReason))
					{
						return false;
					}
				}
			}
			return true;
		}

		bool ValidateSourceEffectSpecs(
			const GameAbilityDefinition& ability,
			const List<AbilityActionSpec>& actions,
			std::string* failureReason
		)
		{
			for (const AbilityActionSpec& action : actions)
			{
				const auto* applyEffect = std::get_if<ApplyEffectAction>(&action.action);
				if (!applyEffect)
				{
					continue;
				}
				const sas::GameplayEffectDefinition* effect =
					EffectData::FindGameplayEffectDefinition(applyEffect->effectId.ToString());
				if (effect && effect->sourceParameterized &&
					!ability.FindEffectSpec(applyEffect->effectId))
				{
					return Fail(
						failureReason,
						"Ability '" + ability.abilityId +
							"' must own an effectSpecs entry for '" +
							applyEffect->effectId.ToString() + "'."
					);
				}
			}
			return true;
		}

		bool ValidateAbilitySourceTags(
			const AbilityTriggerSpec& trigger,
			std::string* failureReason
		)
		{
			for (const GameplayTag& tag : trigger.requiredAbilityTags)
			{
				if (!GameplayTagSchema::Validate(
					tag,
					GameplayTagKind::Ability,
					failureReason
				))
				{
					return Fail(
						failureReason,
						"Ability trigger required source tags must belong to the Ability.* domain."
					);
				}
			}
			for (const GameplayTag& tag : trigger.blockedAbilityTags)
			{
				if (!GameplayTagSchema::Validate(
					tag,
					GameplayTagKind::Ability,
					failureReason
				))
				{
					return Fail(
						failureReason,
						"Ability trigger blocked source tags must belong to the Ability.* domain."
					);
				}
			}
			return true;
		}

		bool ValidateTriggerBudget(
			const AbilityTriggerSpec& trigger,
			std::string* failureReason
		)
		{
			if (trigger.maxMatches < 0)
			{
				return Fail(
					failureReason,
					"Ability trigger maxMatches cannot be negative."
				);
			}
			return true;
		}

		bool ValidateTriggerPayloadTags(
			const AbilityTriggerSpec& trigger,
			std::string* failureReason
		)
		{
			for (const GameplayTag& tag : trigger.requiredPayloadTags)
			{
				if (!GameplayTagSchema::Validate(tag, GameplayTagKind::Any, failureReason))
				{
					return Fail(failureReason, "Ability trigger required payload tag is invalid.");
				}
			}
			for (const GameplayTag& tag : trigger.blockedPayloadTags)
			{
				if (!GameplayTagSchema::Validate(tag, GameplayTagKind::Any, failureReason))
				{
					return Fail(failureReason, "Ability trigger blocked payload tag is invalid.");
				}
			}
			return true;
		}

		bool ValidateLevelUpgradeCosts(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		)
		{
			if (definition.levelUpgradeScrapCosts.empty())
			{
				return true;
			}
			if (definition.levelUpgradeScrapCosts.size() != definition.levelProgression.size())
			{
				return Fail(
					failureReason,
					"Ability upgrade scrap costs must match the number of level steps."
				);
			}
			for (const unsigned int cost : definition.levelUpgradeScrapCosts)
			{
				if (cost == 0)
				{
					return Fail(
						failureReason,
						"Ability upgrade scrap costs must be greater than zero."
					);
				}
			}
			return true;
		}

		bool ValidateAndRecordUpgradeIds(
			const List<std::string>& upgradeIds,
			List<std::string>& declaredUpgradeIds,
			std::string* failureReason
		)
		{
			for (const std::string& upgradeId : upgradeIds)
			{
				const bool alreadyDeclared = std::find(
					declaredUpgradeIds.begin(),
					declaredUpgradeIds.end(),
					upgradeId
				) != declaredUpgradeIds.end();
				if (!content::ContentIdSchema::ValidateContentId(upgradeId) || alreadyDeclared)
				{
					return Fail(failureReason, "Ability upgrade IDs must be valid and unique.");
				}
				declaredUpgradeIds.push_back(upgradeId);
			}
			return true;
		}

		bool ValidateAbilityLevelTrigger(
			const AbilityTriggerSpec& trigger,
			const AbilityActionValidator::EffectValidationFunction& validateEffect,
			std::string* failureReason
		)
		{
			if (!GameplayTagSchema::Validate(
				trigger.eventTag,
				GameplayTagKind::Event,
				failureReason
			))
			{
				return Fail(
					failureReason,
					"Ability level triggers require a valid event tag."
				);
			}
			if (!ValidateAbilitySourceTags(trigger, failureReason) ||
				!ValidateTriggerBudget(trigger, failureReason) ||
				!ValidateTriggerPayloadTags(trigger, failureReason))
			{
				return false;
			}
			return AbilityActionValidator::Validate(
				trigger.actions,
				validateEffect,
				failureReason
			);
		}

		bool ValidateAbilityLevelStep(
			const GameAbilityDefinition& definition,
			std::string_view familyNamespace,
			const AbilityLevelStep& step,
			const AbilityActionValidator::EffectValidationFunction& validateEffect,
			std::string* failureReason
		)
		{
			if (!ValidateAbilityAttributeModifiers(
				definition,
				familyNamespace,
				step.attributeModifiers,
				failureReason
			))
			{
				return false;
			}
			if (!AbilityActionValidator::Validate(
				step.addedActions,
				validateEffect,
				failureReason
			))
			{
				return false;
			}
			for (const AbilityTriggerSpec& trigger : step.addedTriggers)
			{
				if (!ValidateAbilityLevelTrigger(trigger, validateEffect, failureReason))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateLevelProgression(
			const GameAbilityDefinition& definition,
			std::string_view familyNamespace,
			const AbilityActionValidator::EffectValidationFunction& validateEffect,
			std::string* failureReason
		)
		{
			if (!ValidateLevelUpgradeCosts(definition, failureReason))
			{
				return false;
			}
			List<std::string> declaredUpgradeIds;
			if (!ValidateAndRecordUpgradeIds(
				definition.unlockedUpgradeIds,
				declaredUpgradeIds,
				failureReason
			))
			{
				return false;
			}
			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				if (!ValidateAndRecordUpgradeIds(
					step.unlockedUpgradeIds,
					declaredUpgradeIds,
					failureReason
				) ||
					!ValidateAbilityLevelStep(
						definition,
						familyNamespace,
						step,
						validateEffect,
						failureReason
					))
				{
					return false;
				}
			}
			return true;
		}
	}

	bool GameAbilityDefinitionValidator::Validate(
		const GameAbilityDefinition& definition,
		const AbilityActionValidator::EffectValidationFunction& validateEffect,
		std::string* failureReason
	)
	{
		if (!sas::ValidateAbilityDefinition(definition, failureReason))
		{
			return false;
		}
		content::ParsedAbilityId parsedId;
		std::string_view familyNamespace;
		if (definition.slot != sas::AbilitySlot::PrimaryFire)
		{
			if (!content::ContentIdSchema::ParseAbilityId(
				definition.abilityId,
				parsedId,
				failureReason
			))
			{
				return false;
			}
			familyNamespace = parsedId.family;
		}

		std::string expectedBehaviorFamilyName;
		// Content-only test/prototype abilities intentionally use the generic
		// configured behavior. Concrete shipped families must use their own
		// behavior selector and therefore match the family encoded in their content ID.
		if (definition.slot != sas::AbilitySlot::PrimaryFire &&
			definition.behaviorType != AbilityBehaviorType::Configured)
		{
			std::size_t categoryCount = 0;
			bool hasMatchingCategory = false;
			bool hasMatchingFamily = false;
			for (const GameplayTag& abilityTag : definition.abilityTags)
			{
				if (GameplayTagSchema::IsAbilityCategory(abilityTag))
				{
					++categoryCount;
					hasMatchingCategory = hasMatchingCategory ||
						abilityTag.name == parsedId.category;
				}
				hasMatchingFamily = hasMatchingFamily ||
					abilityTag.name == parsedId.family;
			}
			if (categoryCount != 1 || !hasMatchingCategory)
			{
				return Fail(
					failureReason,
					"Ability must have exactly one category tag matching its ID."
				);
			}
			if (!hasMatchingFamily)
			{
				return Fail(failureReason, "Ability must have a family tag matching its ID.");
			}
			const std::size_t familySeparator = parsedId.family.find_last_of('.');
			expectedBehaviorFamilyName = familySeparator == std::string::npos
				? parsedId.family
				: parsedId.family.substr(familySeparator + 1);
		}
		if (!ValidateAbilityScopedAttributes(definition, familyNamespace, failureReason) ||
			!ValidateAbilityAttributeModifiers(
				definition,
				familyNamespace,
				definition.attributeModifiers,
				failureReason
			) ||
			!ValidateScalingRules(
				definition,
				familyNamespace,
				definition.scalingRules,
				failureReason
			))
		{
			return false;
		}
		if (definition.slot != sas::AbilitySlot::PrimaryFire &&
			definition.behaviorType != AbilityBehaviorType::Configured)
		{
			// Concrete family selectors (including newly shipped families) must
			// remain textually aligned with their content-ID family segment.
			const std::string behaviorName = ToString(definition.behaviorType);
			if (expectedBehaviorFamilyName != behaviorName)
			{
				return Fail(
					failureReason,
					"Ability '" + definition.abilityId +
					"' behavior '" + behaviorName +
					"' does not match family '" + expectedBehaviorFamilyName + "'."
				);
			}
		}
		for (const GameplayTag& abilityTag : definition.abilityTags)
		{
			if (!GameplayTagSchema::Validate(abilityTag, GameplayTagKind::Any, failureReason))
			{
				return false;
			}
			if (!abilityTag.MatchesTag(GameplayTagSchema::AbilityRoot))
			{
				return Fail(
					failureReason,
					"Ability tags must belong to the Ability.* semantic domain."
				);
			}
		}
		for (const GameplayTag& tag : definition.requiredOwnerTags)
		{
			if (!GameplayTagSchema::ValidateAbilityOwnerConditionTag(tag, failureReason))
			{
				return false;
			}
		}
		for (const GameplayTag& tag : definition.blockedOwnerTags)
		{
			if (!GameplayTagSchema::ValidateAbilityOwnerConditionTag(tag, failureReason))
			{
				return false;
			}
		}
		for (const GameplayTag& damageTag : definition.damageTags)
		{
			if (!GameplayTagSchema::Validate(
				damageTag,
				GameplayTagKind::DamageType,
				failureReason
			))
			{
				return false;
			}
		}
		for (const GameplayTag& capability : definition.attachmentCapabilities)
		{
			if (!GameplayTagSchema::Validate(
				capability,
				GameplayTagKind::AttachmentCapability,
				failureReason
			))
			{
				return false;
			}
		}
		if (!AbilityActionValidator::Validate(definition.actions, validateEffect, failureReason) ||
			!ValidateOwnedEffectSpecs(definition, failureReason) ||
			!ValidateSourceEffectSpecs(definition, definition.actions, failureReason))
		{
			return false;
		}
		for (const AbilityTriggerSpec& trigger : definition.triggers)
		{
			if (!GameplayTagSchema::Validate(
				trigger.eventTag,
				GameplayTagKind::Event,
				failureReason
			) || !AbilityActionValidator::Validate(
				trigger.actions,
				validateEffect,
				failureReason
			))
			{
				if (failureReason && failureReason->empty())
				{
					*failureReason = "Ability triggers require a valid event tag.";
				}
				return false;
			}
			if (!ValidateAbilitySourceTags(trigger, failureReason))
			{
				return false;
			}
			if (!ValidateTriggerBudget(trigger, failureReason))
			{
				return false;
			}
			if (!ValidateTriggerPayloadTags(trigger, failureReason))
			{
				return false;
			}
			if (!ValidateSourceEffectSpecs(definition, trigger.actions, failureReason))
			{
				return false;
			}
		}
		if (!ValidateLevelProgression(
			definition,
			familyNamespace,
			validateEffect,
			failureReason
		))
		{
			return false;
		}

		unique_ptr<GameAbilityBehavior> behavior =
			GameAbilityBehaviorRegistry::Create(definition.behaviorType);
		if (!behavior)
		{
			return Fail(
				failureReason,
				"Ability definition references an unregistered behavior."
			);
		}
		if (!behavior->Validate(definition, failureReason))
		{
			if (failureReason && failureReason->empty())
			{
				*failureReason = "Ability behavior rejected its definition.";
			}
			return false;
		}
		return true;
	}

	bool GameAbilityDefinitionValidator::ValidateCatalog(
		const List<const GameAbilityDefinition*>& definitions,
		const AbilityActionValidator::EffectValidationFunction& validateEffect,
		std::string* failureReason
	)
	{
		return sas::ValidateAbilityDefinitionCatalog(
			definitions,
			[&validateEffect](
				const GameAbilityDefinition& definition,
				std::string* validationFailure
			)
			{
				return Validate(definition, validateEffect, validationFailure);
			},
			failureReason
		);
	}
}

#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"

#include "abilities/AbilityDefinitionValidation.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <algorithm>
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

		bool ValidateAttributeTag(const GameplayTag& tag, std::string* failureReason)
		{
			return GameplayTagSchema::Validate(
				tag,
				GameplayTagKind::Attribute,
				failureReason
			);
		}

		bool ValidateAttributeModifiers(
			const List<sas::AttributeModifier>& modifiers,
			std::string* failureReason
		)
		{
			for (const sas::AttributeModifier& modifier : modifiers)
			{
				if (!ValidateAttributeTag(modifier.attributeId, failureReason))
				{
					return false;
				}
			}
			return true;
		}

		bool ValidateScalingRules(
			const List<sas::AttributeScalingRule>& scalingRules,
			std::string* failureReason
		)
		{
			for (const sas::AttributeScalingRule& rule : scalingRules)
			{
				if (!ValidateAttributeTag(rule.targetAttributeId, failureReason) ||
					!ValidateAttributeTag(rule.sourceAttributeId, failureReason))
				{
					return false;
				}
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
					EffectData::FindGameplayEffectDefinition(spec.effectId);
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
							spec.effectId + "'."
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
					if (!ValidateAttributeTag(attribute.id, failureReason))
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
					EffectData::FindGameplayEffectDefinition(applyEffect->effectId);
				if (effect && effect->sourceParameterized &&
					!ability.FindEffectSpec(applyEffect->effectId))
				{
					return Fail(
						failureReason,
						"Ability '" + ability.abilityId +
							"' must own an effectSpecs entry for '" +
							applyEffect->effectId + "'."
					);
				}
			}
			return true;
		}

		bool ValidateLevelProgression(
			const GameAbilityDefinition& definition,
			const AbilityActionValidator::EffectValidationFunction& validateEffect,
			std::string* failureReason
		)
		{
			if (!definition.levelUpgradeScrapCosts.empty())
			{
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
			}

			List<GameplayTag> declaredUpgradeIds;
			for (const GameplayTag& upgradeId : definition.unlockedUpgradeIds)
			{
				const bool alreadyDeclared = std::find(
					declaredUpgradeIds.begin(),
					declaredUpgradeIds.end(),
					upgradeId
				) != declaredUpgradeIds.end();
				if (!upgradeId.IsValid() || alreadyDeclared)
				{
					return Fail(failureReason, "Ability upgrade IDs must be valid and unique.");
				}
				declaredUpgradeIds.push_back(upgradeId);
			}

			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				if (!ValidateAttributeModifiers(step.attributeModifiers, failureReason))
				{
					return Fail(
						failureReason,
						"Ability level modifiers require a valid attribute ID."
					);
				}
				for (const GameplayTag& upgradeId : step.unlockedUpgradeIds)
				{
					const bool alreadyDeclared = std::find(
						declaredUpgradeIds.begin(),
						declaredUpgradeIds.end(),
						upgradeId
					) != declaredUpgradeIds.end();
					if (!upgradeId.IsValid() || alreadyDeclared)
					{
						return Fail(
							failureReason,
							"Ability level upgrade IDs must be valid and unique."
						);
					}
					declaredUpgradeIds.push_back(upgradeId);
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
					if (!AbilityActionValidator::Validate(
						trigger.actions,
						validateEffect,
						failureReason
					))
					{
						return false;
					}
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
		std::string expectedBehaviorFamilyName;
		// Content-only test/prototype abilities intentionally use the generic
		// configured behavior. Concrete shipped families must use their own
		// behavior tag and therefore match the family encoded in their content ID.
		if (definition.slot != sas::AbilitySlot::PrimaryFire &&
			!definition.behaviorTag.MatchesTagExact(AbilityBehaviorSchema::Configured))
		{
			content::ParsedAbilityId parsedId;
			if (!content::ContentIdSchema::ParseAbilityId(
				definition.abilityId,
				parsedId,
				failureReason
			))
			{
				return false;
			}
			std::size_t categoryCount = 0;
			bool hasMatchingCategory = false;
			bool hasMatchingFamily = false;
			for (const GameplayTag& abilityTag : definition.abilityTags)
			{
				if (GameplayTagSchema::IsAbilityCategory(abilityTag))
				{
					++categoryCount;
					hasMatchingCategory = hasMatchingCategory ||
						abilityTag.name == parsedId.categoryTag.name;
				}
				hasMatchingFamily = hasMatchingFamily ||
					abilityTag.name == parsedId.familyTag.name;
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
			const std::size_t familySeparator = parsedId.familyTag.name.find_last_of('.');
			expectedBehaviorFamilyName = familySeparator == std::string::npos
				? parsedId.familyTag.name
				: parsedId.familyTag.name.substr(familySeparator + 1);
		}
		if (!ValidateAttributeModifiers(definition.attributeModifiers, failureReason) ||
			!ValidateScalingRules(definition.scalingRules, failureReason) ||
			!GameplayTagSchema::Validate(
				definition.behaviorTag,
				GameplayTagKind::AbilityBehavior,
				failureReason
			))
		{
			return false;
		}
		if (definition.slot != sas::AbilitySlot::PrimaryFire &&
			!definition.behaviorTag.MatchesTagExact(AbilityBehaviorSchema::Configured))
		{
			const std::size_t behaviorSeparator = definition.behaviorTag.name.find_last_of('.');
			const std::string behaviorName = behaviorSeparator == std::string::npos
				? definition.behaviorTag.name
				: definition.behaviorTag.name.substr(behaviorSeparator + 1);
			if (expectedBehaviorFamilyName != behaviorName)
			{
				return Fail(
					failureReason,
					"Ability behavior tag must end with the family segment from its ability ID."
				);
			}
		}
		for (const GameplayTag& abilityTag : definition.abilityTags)
		{
			if (!GameplayTagSchema::Validate(abilityTag, GameplayTagKind::Any, failureReason))
			{
				return false;
			}
			if (!abilityTag.MatchesTag(GameplayTagSchema::AbilityRoot) &&
				!GameplayTagSchema::Validate(
					abilityTag,
					GameplayTagKind::PrimaryWeaponType,
					nullptr
				))
			{
				return Fail(
					failureReason,
					"Ability tags must belong to Ability.* or name a concrete primary weapon type."
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
			if (!ValidateSourceEffectSpecs(definition, trigger.actions, failureReason))
			{
				return false;
			}
		}
		if (!ValidateLevelProgression(definition, validateEffect, failureReason))
		{
			return false;
		}

		unique_ptr<GameAbilityBehavior> behavior =
			GameAbilityBehaviorRegistry::Create(definition.behaviorTag);
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

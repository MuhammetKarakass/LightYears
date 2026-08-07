#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"

#include "attributes/AttributeMath.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>

namespace ly::AbilityActionAttributeResolver
{
	namespace
	{
		float CalculateDefinitionAttributeValue(
			const sas::GameplayAttribute& attribute,
			const GameAbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition,
			const GameAbility* instance
		)
		{
			float value = sas::CalculateModifiedAttributeValue(
				attribute,
				abilityDefinition.attributeModifiers
			);
			if (instance)
			{
				value = instance->ApplyAttachmentModifiers(
					AttachmentHostKind::Ability,
					sas::GameplayAttribute{ attribute.id, value, attribute.minValue, attribute.maxValue }
				).currentValue;
			}
			if (weaponDefinition)
			{
				value = sas::CalculateModifiedAttributeValue(
					sas::GameplayAttribute{ attribute.id, value, attribute.minValue, attribute.maxValue },
					weaponDefinition->attributeModifiers
				);
				if (instance)
				{
					value = instance->ApplyAttachmentModifiers(
						AttachmentHostKind::PrimaryWeapon,
						sas::GameplayAttribute{ attribute.id, value, attribute.minValue, attribute.maxValue }
					).currentValue;
				}
			}
			return value;
		}

		float ResolveAttributeValue(
			AbilityExecutionContext& context,
			const PrimaryWeaponDefinition* weaponDefinition,
			const sas::GameplayAttribute& attribute
		)
		{
			if (!context.abilitySystem || !context.definition)
			{
				return std::clamp(attribute.currentValue, attribute.minValue, attribute.maxValue);
			}

			float value = CalculateDefinitionAttributeValue(
				attribute,
				*context.definition,
				weaponDefinition,
				context.instance
			);
			value = sas::ApplyAttributeScalings(
				value,
				attribute.id,
				context.definition->scalingRules,
				context.abilitySystem->GetAttributes()
			);
			if (weaponDefinition)
			{
				value = sas::ApplyAttributeScalings(
					value,
					attribute.id,
					weaponDefinition->scalingRules,
					context.abilitySystem->GetAttributes()
				);
			}
			if (attribute.id == CommonAttributeIds::Interval && value > 0.f && !weaponDefinition)
			{
				value *= sas::AttributeMath::GetAbilityCooldownMultiplier(
					context.abilitySystem->GetAttributes().GetCurrentValue(
						OwnerAttributeIds::AbilityHaste
					)
				);
			}
			return std::clamp(value, attribute.minValue, attribute.maxValue);
		}
	}

	sas::GameplayAttributeList ResolveAttributes(
		AbilityExecutionContext& context,
		const PrimaryWeaponDefinition* weaponDefinition,
		const sas::GameplayAttributeList& attributes,
		const List<GameplayTag>& originalDamageTags
	)
	{
		sas::GameplayAttributeList sourceAttributes = attributes;
		if (context.instance)
		{
			sourceAttributes = context.instance->MergeAttachmentAttributes(
				AttachmentHostKind::Ability,
				sourceAttributes
			);
			if (weaponDefinition)
			{
				sourceAttributes = context.instance->MergeAttachmentAttributes(
					AttachmentHostKind::PrimaryWeapon,
					sourceAttributes
				);
			}
		}

		sas::GameplayAttributeList values;
		for (const sas::GameplayAttribute& attribute : sourceAttributes)
		{
			values.push_back(sas::GameplayAttribute{
				attribute.id,
				ResolveAttributeValue(context, weaponDefinition, attribute),
				attribute.minValue,
				attribute.maxValue
			});
		}
		if (context.instance)
		{
			values = context.instance->ApplyAttachmentConditions(
				AttachmentHostKind::Ability,
				values,
				originalDamageTags
			);
			if (weaponDefinition)
			{
				values = context.instance->ApplyAttachmentConditions(
					AttachmentHostKind::PrimaryWeapon,
					values,
					originalDamageTags
				);
			}
		}
		return values;
	}

	float ResolveEffectiveInterval(
		AbilityExecutionContext& context,
		float baseInterval,
		const PrimaryWeaponDefinition* weaponDefinition
	)
	{
		if (baseInterval <= 0.f)
		{
			return 0.f;
		}
		return ResolveAttributeValue(
			context,
			weaponDefinition,
			sas::GameplayAttribute{ CommonAttributeIds::Interval, baseInterval, 0.001f }
		);
	}

	List<GameplayTag> BuildBaseDamageTags(const GameAbilityDefinition* definition)
	{
		if (definition && !definition->damageTags.empty())
		{
			return definition->damageTags;
		}
		return { DamageTypeSchema::Photonic };
	}

	List<GameplayTag> ResolveDamageTags(
		const AbilityExecutionContext& context,
		AttachmentHostKind hostKind
	)
	{
		if (context.instance)
		{
			return context.instance->GetResolvedDamageTags(hostKind);
		}
		return BuildBaseDamageTags(context.definition);
	}
}

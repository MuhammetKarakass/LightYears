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
		bool IsInvocationOutputAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return std::find(
				definition.invocationOutputAttributes.begin(),
				definition.invocationOutputAttributes.end(),
				attributeId
			) != definition.invocationOutputAttributes.end();
		}

		float CalculateDefinitionAttributeValue(
			const sas::GameplayAttribute& attribute,
			const GameAbilityDefinition& abilityDefinition,
			const PrimaryWeaponDefinition* weaponDefinition
		)
		{
			float value = sas::CalculateModifiedAttributeValue(
				attribute,
				abilityDefinition.attributeModifiers
			);
			if (weaponDefinition)
			{
				value = sas::CalculateModifiedAttributeValue(
					sas::GameplayAttribute{ attribute.id, value, attribute.minValue, attribute.maxValue },
					weaponDefinition->attributeModifiers
				);
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

			float value = CalculateDefinitionAttributeValue(attribute, *context.definition, weaponDefinition);
			if (!weaponDefinition)
			{
				value = context.abilitySystem->ApplyScopedAbilityModifiers(
					*context.definition,
					sas::GameplayAttribute{
						attribute.id,
						value,
						attribute.minValue,
						attribute.maxValue
					}
				).currentValue;
			}
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
			value = sas::ApplyAttributeScalings(
				value,
				attribute.id,
				context.definition->levelScalingRules,
				context.abilitySystem->GetAttributes()
			);
			// Echo power applies only to declared output channels. Applying it to
			// every resolved number made fixed delivery data slower/shorter and
			// shrank Relay Prism even though Prism size is not level-scaled.
			if (IsInvocationOutputAttribute(*context.definition, attribute.id))
			{
				value *= context.definition->attributeOutputMultiplier;
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
		const sas::GameplayAttributeList& attributes
	)
	{
		sas::GameplayAttributeList values;
		for (const sas::GameplayAttribute& attribute : attributes)
		{
			values.push_back(sas::GameplayAttribute{
				attribute.id,
				ResolveAttributeValue(context, weaponDefinition, attribute),
				attribute.minValue,
				attribute.maxValue
			});
		}
		return values;
	}

	sas::GameplayAttributeList ResolveAbilityAttributes(
		AbilityExecutionContext& context
	)
	{
		if (!context.definition)
		{
			return {};
		}

		return ResolveAttributes(
			context,
			nullptr,
			context.definition->attributes
		);
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

	List<GameplayTag> ResolveDamageTags(const AbilityExecutionContext& context)
	{
		if (context.instance)
		{
			return context.instance->GetResolvedDamageTags();
		}
		return BuildBaseDamageTags(context.definition);
	}
}

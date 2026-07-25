#include "PrimaryWeaponDefinitionValidator.h"

#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"

#include <algorithm>
#include <array>

namespace ly::PrimaryWeaponDefinitionValidator
{
	namespace
	{
		struct ValidationContext
		{
			const PrimaryWeaponHandler& handler;
			List<const PrimaryWeaponFeatureHandler*> features;
			List<GameplayTag> featureTags;
			List<GameplayTag> upgradeIds;
			List<GameplayTag> attributeIds;
		};

		bool HasExactTag(
			const List<GameplayTag>& tags,
			const GameplayTag& expectedTag
		)
		{
			return std::any_of(tags.begin(), tags.end(), [&](const GameplayTag& tag)
			{
				return tag.MatchesTagExact(expectedTag);
			});
		}

		bool MatchesAnyRoot(
			const GameplayTag& tag,
			const List<GameplayTag>& roots
		)
		{
			return std::any_of(roots.begin(), roots.end(), [&](const GameplayTag& root)
			{
				return tag.MatchesTag(root);
			});
		}

		bool IsCommonWeaponAttribute(const GameplayTag& attributeId)
		{
			if (attributeId.MatchesTag(DamageAttributeIds::AttributeRoot))
			{
				return true;
			}
			static const std::array<GameplayTag, 6> supportedAttributes{
				CommonAttributeIds::Damage,
				CommonAttributeIds::FireRate,
				CommonAttributeIds::Interval,
				CommonAttributeIds::Range,
				CommonAttributeIds::CollisionRadius,
				CommonAttributeIds::AreaRadius
			};
			return std::any_of(
				supportedAttributes.begin(),
				supportedAttributes.end(),
				[&](const GameplayTag& supported)
				{
					return attributeId.MatchesTagExact(supported);
				}
			);
		}

		bool IsAllowedAttribute(
			const ValidationContext& context,
			const GameplayTag& attributeId
		)
		{
			if (IsCommonWeaponAttribute(attributeId) ||
				MatchesAnyRoot(attributeId, context.handler.GetOwnedAttributeRoots()) ||
				MatchesAnyRoot(attributeId, context.handler.GetInheritedAttributeRoots()))
			{
				return true;
			}
			return std::any_of(
				context.features.begin(),
				context.features.end(),
				[&](const PrimaryWeaponFeatureHandler* feature)
				{
					return feature &&
						MatchesAnyRoot(attributeId, feature->GetAttributeRoots());
				}
			);
		}

		PrimaryWeaponValidationResult ValidateAttributeId(
			const ValidationContext& context,
			const GameplayTag& attributeId,
			const char* usage
		)
		{
			return IsAllowedAttribute(context, attributeId)
				? PrimaryWeaponValidationResult{ true, {} }
				: PrimaryWeaponValidationResult{
					false,
					std::string{ usage } +
						" is not consumed by the selected weapon type or feature."
				};
		}

		PrimaryWeaponValidationResult DeclareFeature(
			const GameplayTag& featureTag,
			ValidationContext& context
		)
		{
			if (!featureTag.IsValid() ||
				HasExactTag(context.featureTags, featureTag))
			{
				return {
					false,
					"Primary weapon feature tags must be valid and unique."
				};
			}

			const PrimaryWeaponFeatureHandler* feature =
				PrimaryWeaponHandlerRegistry::FindFeature(featureTag);
			if (!feature)
			{
				return {
					false,
					"No primary weapon feature handler is registered for this feature."
				};
			}

			context.featureTags.push_back(featureTag);
			context.features.push_back(feature);
			return { true, {} };
		}

		PrimaryWeaponValidationResult ResolveDeclarations(
			const PrimaryWeaponDefinition& definition,
			ValidationContext& context
		)
		{
			for (const GameplayTag& featureTag : definition.featureTags)
			{
				const PrimaryWeaponValidationResult result =
					DeclareFeature(featureTag, context);
				if (!result.isValid)
				{
					return result;
				}
			}

			if (!definition.progressionProfile.IsWellFormed())
			{
				return { false, "Primary weapon progression profile has an invalid level range." };
			}

			for (const PrimaryWeaponLevelStep& step : definition.progressionProfile.ResolveLevelSteps())
			{
				for (const GameplayTag& upgradeId : step.unlockedUpgradeIds)
				{
					if (!upgradeId.IsValid() ||
						HasExactTag(context.upgradeIds, upgradeId) ||
						HasExactTag(context.featureTags, upgradeId))
					{
						return {
							false,
							"Primary weapon level upgrade IDs must be valid, unique, and distinct from feature tags."
						};
					}
					context.upgradeIds.push_back(upgradeId);
				}

				for (const GameplayTag& featureTag : step.unlockedFeatureTags)
				{
					if (HasExactTag(context.upgradeIds, featureTag))
					{
						return {
							false,
							"Primary weapon level feature tags must be unique and distinct from upgrade IDs."
						};
					}
					const PrimaryWeaponValidationResult result =
						DeclareFeature(featureTag, context);
					if (!result.isValid)
					{
						return result;
					}
					context.upgradeIds.push_back(featureTag);
				}
			}

			if (!definition.heatGainCurve.empty() &&
				!HasExactTag(
					context.featureTags,
					PrimaryWeaponSchema::Feature::Heat::FeatureId
				))
			{
				return { false, "Heat gain curve requires the heat feature." };
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult ValidateAttributes(
			const PrimaryWeaponDefinition& definition,
			ValidationContext& context
		)
		{
			for (const GameplayAttribute& attribute : definition.attributes)
			{
				if (!attribute.id.IsValid() ||
					HasExactTag(context.attributeIds, attribute.id))
				{
					return {
						false,
						"Primary weapon attributes must have valid, unique IDs."
					};
				}
				context.attributeIds.push_back(attribute.id);

				const PrimaryWeaponValidationResult result =
					ValidateAttributeId(
						context,
						attribute.id,
						"Weapon attribute"
					);
				if (!result.isValid)
				{
					return result;
				}
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult ValidateBaseModifiers(
			const PrimaryWeaponDefinition& definition,
			const ValidationContext& context
		)
		{
			for (const AttributeModifier& modifier : definition.attributeModifiers)
			{
				const PrimaryWeaponValidationResult result =
					ValidateAttributeId(
						context,
						modifier.attributeId,
						"Weapon modifier"
					);
				if (!result.isValid)
				{
					return result;
				}
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult ValidateLevelModifiers(
			const PrimaryWeaponDefinition& definition,
			const ValidationContext& context
		)
		{
			for (const PrimaryWeaponLevelStep& step : definition.progressionProfile.ResolveLevelSteps())
			{
				for (const AttributeModifier& modifier : step.attributeModifiers)
				{
					const PrimaryWeaponValidationResult result =
						ValidateAttributeId(
							context,
							modifier.attributeId,
							"Weapon level modifier"
						);
					if (!result.isValid)
					{
						return result;
					}
					if (!HasExactTag(context.attributeIds, modifier.attributeId))
					{
						return {
							false,
							"Primary weapon level modifiers must target a declared weapon attribute."
						};
					}
				}
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult ValidateScalingRules(
			const PrimaryWeaponDefinition& definition,
			const ValidationContext& context
		)
		{
			for (const AttributeScalingRule& scaling : definition.scalingRules)
			{
				const PrimaryWeaponValidationResult result =
					ValidateAttributeId(
						context,
						scaling.targetAttributeId,
						"Weapon scaling target"
					);
				if (!result.isValid)
				{
					return result;
				}
			}
			return { true, {} };
		}

		PrimaryWeaponValidationResult ValidateConsumers(
			const PrimaryWeaponDefinition& definition,
			const ValidationContext& context
		)
		{
			const PrimaryWeaponValidationResult handlerResult =
				context.handler.ValidateDefinition(definition);
			if (!handlerResult.isValid)
			{
				return handlerResult;
			}

			for (const PrimaryWeaponFeatureHandler* feature : context.features)
			{
				const PrimaryWeaponValidationResult featureResult =
					feature->ValidateDefinition(definition);
				if (!featureResult.isValid)
				{
					return featureResult;
				}
			}
			return { true, {} };
		}
	}

	PrimaryWeaponValidationResult Validate(
		const PrimaryWeaponDefinition& definition
	)
	{
		const PrimaryWeaponHandler* handler =
			PrimaryWeaponHandlerRegistry::FindHandler(definition.weaponTypeTag);
		if (!handler)
		{
			return {
				false,
				"No primary weapon handler is registered for this weapon type."
			};
		}

		ValidationContext context{ *handler };
		for (const auto validator : {
			ResolveDeclarations,
			ValidateAttributes
		})
		{
			const PrimaryWeaponValidationResult result =
				validator(definition, context);
			if (!result.isValid)
			{
				return result;
			}
		}

		for (const auto validator : {
			ValidateBaseModifiers,
			ValidateLevelModifiers,
			ValidateScalingRules,
			ValidateConsumers
		})
		{
			const PrimaryWeaponValidationResult result =
				validator(definition, context);
			if (!result.isValid)
			{
				return result;
			}
		}
		return { true, {} };
	}
}

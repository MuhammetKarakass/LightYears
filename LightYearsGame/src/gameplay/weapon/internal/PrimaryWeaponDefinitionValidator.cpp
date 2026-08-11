#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/attributes/AttributeIds.h"
#include "PrimaryWeaponDefinitionValidator.h"

#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace ly::PrimaryWeaponDefinitionValidator
{
	namespace
	{
		struct ValidationContext
		{
			const PrimaryWeaponHandler& handler;
			List<const PrimaryWeaponFeatureHandler*> features;
			List<PrimaryWeaponFeatureType> featureTypes;
			List<std::string> upgradeIds;
			List<sas::AttributeId> attributeIds;
		};

		bool HasExactId(
			const List<std::string>& ids,
			const std::string& expectedId
		)
		{
			return std::any_of(ids.begin(), ids.end(), [&](const std::string& id)
			{
				return id == expectedId;
			});
		}

		bool HasFeature(
			const List<PrimaryWeaponFeatureType>& featureTypes,
			PrimaryWeaponFeatureType expectedType
		)
		{
			return std::find(featureTypes.begin(), featureTypes.end(), expectedType) != featureTypes.end();
		}

		bool IsFeatureUpgradeId(
			const List<PrimaryWeaponFeatureType>& featureTypes,
			const std::string& upgradeId
		)
		{
			return std::any_of(featureTypes.begin(), featureTypes.end(), [&](PrimaryWeaponFeatureType type)
			{
				return upgradeId == PrimaryWeaponFeatureUpgradeId(type);
			});
		}

		bool IsInAttributeRoot(
			const sas::AttributeId& id,
			const sas::AttributeId& root
		)
		{
			const std::string_view name = id.GetName();
			const std::string_view prefix = root.GetName();
			return name.size() >= prefix.size() &&
				name.compare(0, prefix.size(), prefix) == 0 &&
				(name.size() == prefix.size() || name[prefix.size()] == '.');
		}

		bool MatchesAnyRoot(
			const sas::AttributeId& id,
			const List<sas::AttributeId>& roots
		)
		{
			return std::any_of(roots.begin(), roots.end(), [&](const sas::AttributeId& root)
			{
				return IsInAttributeRoot(id, root);
			});
		}

		bool IsCommonWeaponAttribute(const sas::AttributeId& attributeId)
		{
			if (IsInAttributeRoot(attributeId, DamageAttributeIds::Root))
			{
				return true;
			}
			static const std::array<sas::AttributeId, 6> supportedAttributes{
				CommonAttributeIds::Damage,
				CommonAttributeIds::FireRate,
				CommonAttributeIds::Interval,
				CommonAttributeIds::Range,
				CollisionAttributeIds::Radius,
				AreaAttributeIds::Radius
			};
			return std::any_of(
				supportedAttributes.begin(),
				supportedAttributes.end(),
				[&](const sas::AttributeId& supported)
				{
					return attributeId == supported;
				}
			);
		}

		bool IsAllowedAttribute(
			const ValidationContext& context,
			const sas::AttributeId& attributeId
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
			const sas::AttributeId& attributeId,
			const char* usage
		)
		{
			if (!AttributeIdSchema::Validate(attributeId, nullptr))
			{
				return { false, std::string{ usage } + " has an invalid AttributeId." };
			}
			return IsAllowedAttribute(context, attributeId)
				? PrimaryWeaponValidationResult{ true, {} }
				: PrimaryWeaponValidationResult{
					false,
					std::string{ usage } +
						" is not consumed by the selected weapon type or feature."
				};
		}

		PrimaryWeaponValidationResult DeclareFeature(
			PrimaryWeaponFeatureType featureType,
			ValidationContext& context
		)
		{
			if (HasFeature(context.featureTypes, featureType))
			{
				return {
					false,
				"Primary weapon feature types must be valid and unique."
				};
			}

			const PrimaryWeaponFeatureHandler* feature =
				PrimaryWeaponHandlerRegistry::FindFeature(featureType);
			if (!feature)
			{
				return {
					false,
					"No primary weapon feature handler is registered for this feature."
				};
			}

			context.featureTypes.push_back(featureType);
			context.features.push_back(feature);
			return { true, {} };
		}

		PrimaryWeaponValidationResult ResolveDeclarations(
			const PrimaryWeaponDefinition& definition,
			ValidationContext& context
		)
		{
			for (const PrimaryWeaponFeatureType featureType : definition.featureTypes)
			{
				const PrimaryWeaponValidationResult result =
					DeclareFeature(featureType, context);
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
				for (const std::string& upgradeId : step.unlockedUpgradeIds)
				{
					if (!content::ContentIdSchema::ValidateContentId(upgradeId) ||
						HasExactId(context.upgradeIds, upgradeId) ||
						IsFeatureUpgradeId(context.featureTypes, upgradeId))
					{
						return {
							false,
							"Primary weapon level upgrade IDs must be valid, unique, and distinct from feature upgrades."
						};
					}
					context.upgradeIds.push_back(upgradeId);
				}

				for (const PrimaryWeaponFeatureType featureType : step.unlockedFeatureTypes)
				{
					const std::string featureUpgradeId =
						PrimaryWeaponFeatureUpgradeId(featureType);
					if (HasExactId(context.upgradeIds, featureUpgradeId))
					{
						return {
							false,
							"Primary weapon level feature upgrades must be unique and distinct from upgrade IDs."
						};
					}
					const PrimaryWeaponValidationResult result =
						DeclareFeature(featureType, context);
					if (!result.isValid)
					{
						return result;
					}
					context.upgradeIds.push_back(featureUpgradeId);
				}
			}

			if (!definition.heatGainCurve.empty() &&
				!HasFeature(
					context.featureTypes,
					PrimaryWeaponFeatureType::Heat
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
			for (const sas::GameplayAttribute& attribute : definition.attributes)
			{
				if (!AttributeIdSchema::Validate(attribute.id, nullptr) ||
					std::find(context.attributeIds.begin(), context.attributeIds.end(), attribute.id) != context.attributeIds.end())
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
			for (const sas::AttributeModifier& modifier : definition.attributeModifiers)
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
				for (const sas::AttributeModifier& modifier : step.attributeModifiers)
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
					if (std::find(context.attributeIds.begin(), context.attributeIds.end(), modifier.attributeId) == context.attributeIds.end())
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
			for (const sas::AttributeScalingRule& scaling : definition.scalingRules)
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
				if (!AttributeIdSchema::Validate(scaling.sourceAttributeId, nullptr))
				{
					return {
						false,
						"Weapon scaling source has an invalid AttributeId."
					};
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
		std::string tagFailureReason;
		if (!definition.weaponId.empty())
		{
			content::ParsedWeaponId parsedWeaponId;
			if (!content::ContentIdSchema::ParseWeaponId(
				definition.weaponId,
				parsedWeaponId,
				&tagFailureReason
			))
			{
				return { false, "Primary weapon content ID is invalid: " + tagFailureReason };
			}
			const std::string typeFamily = PrimaryWeaponFamilyName(definition.weaponType);
			if (parsedWeaponId.family != typeFamily)
			{
				return {
					false,
					"Primary weapon content ID family must match its selected weapon type."
				};
			}
		}
		for (const GameplayTag& damageTag : definition.damageTags)
		{
			if (!GameplayTagSchema::Validate(
				damageTag,
				GameplayTagKind::DamageType,
				&tagFailureReason
			))
			{
				return { false, "Primary weapon damage tag is invalid: " + tagFailureReason };
			}
		}
		for (const GameplayTag& capability : definition.attachmentCapabilities)
		{
			if (!GameplayTagSchema::Validate(
				capability,
				GameplayTagKind::AttachmentCapability,
				&tagFailureReason
			))
			{
				return { false, "Primary weapon attachment capability tag is invalid: " + tagFailureReason };
			}
		}
		const PrimaryWeaponHandler* handler =
			PrimaryWeaponHandlerRegistry::FindHandler(definition.weaponType);
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

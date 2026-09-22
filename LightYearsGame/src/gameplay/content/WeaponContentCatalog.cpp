#include "gameplay/content/WeaponContentCatalog.h"

#include "gameplay/content/WeaponLoader.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "../weapon/internal/PrimaryWeaponDefinitionValidator.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ly::content
{
	namespace
	{
		List<PrimaryWeaponDefinition>& GetDefinitions()
		{
			static List<PrimaryWeaponDefinition> definitions;
			return definitions;
		}

		bool& GetLoadedState()
		{
			static bool loaded = false;
			return loaded;
		}

		bool Fail(std::string* failureReason, const std::string& message)
		{
			if (failureReason)
			{
				*failureReason = message;
			}
			return false;
		}

		bool HasDamageTag(const PrimaryWeaponDefinition& definition, const GameplayTag& tag)
		{
			for (const GameplayTag& damageTag : definition.damageTags)
			{
				if (damageTag.MatchesTag(tag))
				{
					return true;
				}
			}
			return false;
		}

		bool RequireDamageAttributes(
			const PrimaryWeaponDefinition& definition,
			const List<sas::AttributeId>& required,
			std::string* failureReason
		)
		{
			for (const sas::AttributeId& attributeId : required)
			{
				if (!sas::FindAttribute(definition.attributes, attributeId))
				{
					return Fail(
						failureReason,
						"Weapon '" + definition.weaponId + "' owns damage type values but is missing '" +
						std::string{ attributeId.GetName() } + "'."
					);
				}
			}
			return true;
		}

		bool ValidateDamageOwnership(
			const PrimaryWeaponDefinition& definition,
			std::string* failureReason
		)
		{
			if (HasDamageTag(definition, DamageTypeSchema::Energy) &&
				!RequireDamageAttributes(definition, { DamageAttributeIds::ShieldRegenerationDelay }, failureReason)) return false;
			if (HasDamageTag(definition, DamageTypeSchema::Thermal) &&
				!RequireDamageAttributes(definition, { DamageAttributeIds::IgniteStacks }, failureReason)) return false;
			if (HasDamageTag(definition, DamageTypeSchema::Cryo) &&
				!RequireDamageAttributes(definition, { DamageAttributeIds::CryoBuildupPerHit }, failureReason)) return false;
			if (HasDamageTag(definition, DamageTypeSchema::Electric) &&
				!RequireDamageAttributes(definition, { DamageAttributeIds::ElectricStacks }, failureReason)) return false;
			return true;
		}
	}

	bool WeaponContentCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		std::string* failureReason)
	{
		const WeaponLoader::Result loaded = WeaponLoader::LoadFromFile(filePath);
		if (!loaded.Succeeded())
		{
			return Fail(failureReason, loaded.error);
		}
		if (loaded.definitions.empty())
		{
			return Fail(failureReason, "Weapon catalog is empty");
		}

		for (const PrimaryWeaponDefinition& definition : loaded.definitions)
		{
			const PrimaryWeaponValidationResult validation =
				PrimaryWeaponDefinitionValidator::Validate(definition);
			if (!validation.isValid)
			{
				return Fail(
					failureReason,
					"Invalid weapon '" + definition.weaponId + "': " + validation.reason
				);
			}
			if (!ValidateDamageOwnership(definition, failureReason))
			{
				return false;
			}
		}

		GetDefinitions() = loaded.definitions;
		GetLoadedState() = true;
		return true;
	}

	const PrimaryWeaponDefinition* WeaponContentCatalog::FindById(
		const std::string& weaponId)
	{
		for (const PrimaryWeaponDefinition& definition : GetDefinitions())
		{
			if (definition.weaponId == weaponId)
			{
				return &definition;
			}
		}
		return nullptr;
	}

	std::optional<float> WeaponContentCatalog::ResolveAuthoredAttributeAtLevel(
		const std::string& weaponId,
		int weaponLevel,
		const sas::AttributeId& attributeId,
		std::string* failureReason)
	{
		const PrimaryWeaponDefinition* weapon = FindById(weaponId);
		if (!weapon)
		{
			Fail(failureReason, "Weapon '" + weaponId + "' was not found.");
			return std::nullopt;
		}
		if (!weapon->progressionProfile.IsWellFormed())
		{
			Fail(failureReason, "Weapon '" + weaponId + "' has an invalid progression profile.");
			return std::nullopt;
		}
		if (weaponLevel < 1 || weaponLevel > weapon->progressionProfile.maxLevel)
		{
			Fail(failureReason, "Weapon '" + weaponId + "' level " + std::to_string(weaponLevel) +
				" is outside the authored range 1.." + std::to_string(weapon->progressionProfile.maxLevel) + ".");
			return std::nullopt;
		}

		const sas::GameplayAttribute* base = sas::FindAttribute(weapon->attributes, attributeId);
		if (!base)
		{
			Fail(failureReason, "Weapon '" + weaponId + "' has no authored '" +
				std::string{ attributeId.GetName() } + "' attribute.");
			return std::nullopt;
		}

		if (attributeId == CommonAttributeIds::Range)
		{
			const auto hasDynamicRange = [](const List<sas::AttributeScalingRule>& rules)
			{
				return std::any_of(rules.begin(), rules.end(), [](const sas::AttributeScalingRule& rule)
				{
					return rule.targetAttributeId == CommonAttributeIds::Range;
				});
			};
			if (hasDynamicRange(weapon->scalingRules))
			{
				Fail(failureReason, "Weapon '" + weaponId + "' Common.Range depends on dynamic owner scaling.");
				return std::nullopt;
			}
		}

		List<sas::AttributeModifier> modifiers;
		for (const sas::AttributeModifier& modifier : weapon->attributeModifiers)
			if (modifier.attributeId == attributeId) modifiers.push_back(modifier);
		const List<PrimaryWeaponLevelStep> steps = weapon->progressionProfile.ResolveLevelSteps();
		for (int level = 1; level < weaponLevel; ++level)
		{
			const PrimaryWeaponLevelStep& step = steps[static_cast<size_t>(level - 1)];
			if (attributeId == CommonAttributeIds::Range)
			{
				for (const sas::AttributeScalingRule& rule : step.scalingRules)
				{
					if (rule.targetAttributeId == CommonAttributeIds::Range)
					{
						Fail(failureReason, "Weapon '" + weaponId + "' Common.Range depends on dynamic progression scaling.");
						return std::nullopt;
					}
				}
			}
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
				if (modifier.attributeId == attributeId) modifiers.push_back(modifier);
		}

		const float value = sas::CalculateModifiedAttributeValue(*base, modifiers);
		if (!std::isfinite(value))
		{
			Fail(failureReason, "Weapon '" + weaponId + "' authored '" +
				std::string{ attributeId.GetName() } + "' resolves to a non-finite value.");
			return std::nullopt;
		}
		return value;
	}

	bool WeaponContentCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}

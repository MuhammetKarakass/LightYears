#include "gameplay/content/WeaponContentCatalog.h"

#include "gameplay/content/WeaponLoader.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "../weapon/internal/PrimaryWeaponDefinitionValidator.h"

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
				!RequireDamageAttributes(definition, {
					DamageAttributeIds::ShieldDamageMultiplier,
					DamageAttributeIds::ShieldRegenerationDelay
				}, failureReason)) return false;
			if (HasDamageTag(definition, DamageTypeSchema::Kinetic) &&
				!RequireDamageAttributes(definition, { DamageAttributeIds::ArmorPenetration }, failureReason)) return false;
			if (HasDamageTag(definition, DamageTypeSchema::Thermal) &&
				!RequireDamageAttributes(definition, {
					DamageAttributeIds::IgniteStacks,
					DamageAttributeIds::BurnDamagePerSecond,
					DamageAttributeIds::BurnDuration,
					DamageAttributeIds::BurnMaxStacks
				}, failureReason)) return false;
			if (HasDamageTag(definition, DamageTypeSchema::Cryo) &&
				!RequireDamageAttributes(definition, {
					DamageAttributeIds::CryoBuildupPerHit,
					DamageAttributeIds::CryoBuildupRequired,
					DamageAttributeIds::CryoBuildupDuration,
					DamageAttributeIds::CryoSlowPercent,
					DamageAttributeIds::CryoSlowDuration
				}, failureReason)) return false;
			if (HasDamageTag(definition, DamageTypeSchema::Electric) &&
				!RequireDamageAttributes(definition, {
					DamageAttributeIds::ElectricStacks,
					DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
					DamageAttributeIds::ElectricDuration,
					DamageAttributeIds::ElectricMaxStacks
				}, failureReason)) return false;
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

	bool WeaponContentCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}

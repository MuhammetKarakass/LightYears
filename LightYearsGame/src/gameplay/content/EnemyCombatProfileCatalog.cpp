#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/EnemyCombatProfileLoader.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "abilities/AbilityGrantRules.h"

#include <set>
#include <cmath>
#include <algorithm>
#include <string>

namespace ly::content
{
	namespace
	{
		List<EnemyCombatProfile>& GetDefinitions()
		{
			static List<EnemyCombatProfile> definitions;
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

		bool UsesPowerScaling(const sas::AttributeScalingRule& rule)
		{
			return rule.sourceAttributeId == OwnerAttributeIds::AttackPower ||
				rule.sourceAttributeId == OwnerAttributeIds::EnergyPower;
		}

		bool WeaponHasPowerScaling(const PrimaryWeaponDefinition& weapon)
		{
			for (const auto& rule : weapon.scalingRules)
			{
				if (UsesPowerScaling(rule)) return true;
			}
			for (const auto& rule : weapon.progressionProfile.rules)
			{
				for (const auto& scalingRule : rule.reward.scalingRules)
				{
					if (UsesPowerScaling(scalingRule)) return true;
				}
			}
			return false;
		}

		bool AbilityHasPowerScaling(const GameAbilityDefinition& ability)
		{
			for (const auto& rule : ability.scalingRules)
			{
				if (UsesPowerScaling(rule)) return true;
			}
			for (const auto& rule : ability.levelScalingRules)
			{
				if (UsesPowerScaling(rule)) return true;
			}
			for (const auto& step : ability.levelProgression)
			{
				for (const auto& rule : step.scalingRules)
				{
					if (UsesPowerScaling(rule)) return true;
				}
			}
			return false;
		}

		bool IsEnemyNaturalGrowthAttribute(const sas::AttributeId& attributeId)
		{
			return attributeId == OwnerAttributeIds::MaxHealth || attributeId == OwnerAttributeIds::Armor;
		}

		bool IsValidVariationRange(const EnemyVariationRange& range)
		{
			return std::isfinite(range.minimumMultiplier) && std::isfinite(range.maximumMultiplier) &&
				range.minimumMultiplier >= 0.f && range.maximumMultiplier >= range.minimumMultiplier;
		}
	}

	bool EnemyCombatProfileCatalog::ValidateProfile(
		const EnemyCombatProfile& profile,
		std::string* failureReason
	)
	{
		std::string idFailure;
		if (!ContentIdSchema::ValidateEnemyCombatProfileId(profile.profileId, &idFailure))
		{
			return Fail(failureReason, "Invalid enemy combat profile ID '" + profile.profileId + "': " + idFailure);
		}

		List<sas::AttributeId> progressionAttributes;
		for (const AttributeGrowthEntry& growth : profile.progression.naturalGrowth)
		{
			if (!IsEnemyNaturalGrowthAttribute(growth.attributeId) || !std::isfinite(growth.perLevel) || growth.perLevel < 0.f)
				return Fail(failureReason, "Invalid enemy natural growth attribute in profile '" + profile.profileId + "'. Only Owner.MaxHealth and Owner.Armor are allowed.");
			if (std::find(progressionAttributes.begin(), progressionAttributes.end(), growth.attributeId) != progressionAttributes.end())
				return Fail(failureReason, "Duplicate enemy natural growth attribute in profile '" + profile.profileId + "'.");
			progressionAttributes.push_back(growth.attributeId);
		}
		if (!std::isfinite(profile.progression.maxShieldPerLevel) || !std::isfinite(profile.progression.outgoingDamagePerLevel) ||
			profile.progression.maxShieldPerLevel < 0.f || profile.progression.outgoingDamagePerLevel < 0.f ||
			!IsValidVariationRange(profile.progression.maxHealthVariation) || !IsValidVariationRange(profile.progression.armorVariation) ||
			!IsValidVariationRange(profile.progression.maxShieldVariation) || !IsValidVariationRange(profile.progression.outgoingDamageVariation))
			return Fail(failureReason, "Invalid progression values in profile '" + profile.profileId + "'.");

		std::set<sas::AbilitySlot> usedSlots;
		std::set<std::string> usedWeaponIds;
		std::set<std::string> usedAbilityIds;
		List<const PrimaryWeaponDefinition*> weaponDefinitions;
		for (const EnemyWeaponBinding& binding : profile.weapons)
		{
			if (!ContentIdSchema::ValidateWeaponId(binding.weaponId, &idFailure))
			{
				return Fail(failureReason, "Invalid weapon ID '" + binding.weaponId + "' in profile '" + profile.profileId + "': " + idFailure);
			}
			if (!usedWeaponIds.insert(binding.weaponId).second)
			{
				return Fail(failureReason, "Duplicate weapon ID '" + binding.weaponId + "' used in profile '" + profile.profileId + "'.");
			}
			if (binding.slot == sas::AbilitySlot::None || !usedSlots.insert(binding.slot).second)
			{
				return Fail(failureReason, "Weapon '" + binding.weaponId + "' uses an invalid or duplicate slot in profile '" + profile.profileId + "'.");
			}
			const PrimaryWeaponDefinition* weaponDef = WeaponContentCatalog::FindById(binding.weaponId);
			if (!weaponDef)
			{
				return Fail(failureReason, "Weapon '" + binding.weaponId + "' not found in WeaponContentCatalog for profile '" + profile.profileId + "'.");
			}
			const GameAbilityDefinition weaponAbility = AbilityData::MakePrimaryFireAbilityDefinition(*weaponDef);
			if (binding.level < 1 || binding.level > weaponAbility.GetMaxLevel())
			{
				return Fail(failureReason, "Weapon '" + binding.weaponId + "' level " + std::to_string(binding.level) + " out of range [1, " + std::to_string(weaponAbility.GetMaxLevel()) + "] for profile '" + profile.profileId + "'.");
			}
			weaponDefinitions.push_back(weaponDef);
		}

		for (const EnemyAbilityBinding& binding : profile.abilities)
		{
			const GameAbilityDefinition* abilityDef = AbilityData::FindShippedAbilityDefinition(binding.abilityId);
			if (!abilityDef)
			{
				return Fail(failureReason, "Ability '" + binding.abilityId + "' not found in AbilityContentCatalog for profile '" + profile.profileId + "'.");
			}
			const bool passive = sas::IsPassiveAbility(*abilityDef);
			if ((!passive && !sas::IsLoadoutAbilitySlot(binding.slot)) || (passive && binding.slot != sas::AbilitySlot::None))
			{
				return Fail(failureReason, "Ability '" + binding.abilityId + "' uses an invalid slot in profile '" + profile.profileId + "'.");
			}
			if (binding.slot != sas::AbilitySlot::None && !usedSlots.insert(binding.slot).second)
			{
				return Fail(failureReason, "Duplicate active loadout slot used in profile '" + profile.profileId + "'.");
			}
			if (!usedAbilityIds.insert(binding.abilityId).second)
			{
				return Fail(failureReason, "Duplicate ability ID '" + binding.abilityId + "' used in profile '" + profile.profileId + "'.");
			}
		}

		if (profile.weapons.empty() && profile.abilities.empty() && !profile.allowContactDamageOnly)
		{
			return Fail(failureReason, "Enemy combat profile '" + profile.profileId + "' has no attack method (no primary weapon or abilities) and allowContactDamageOnly is false.");
		}

		for (const EnemyAbilityBinding& binding : profile.abilities)
		{
			const GameAbilityDefinition* abilityDef = AbilityData::FindShippedAbilityDefinition(binding.abilityId);
			if (!abilityDef)
			{
				return Fail(failureReason, "Ability '" + binding.abilityId + "' not found in AbilityContentCatalog for profile '" + profile.profileId + "'.");
			}

			if (binding.level < 1 || binding.level > abilityDef->GetMaxLevel())
			{
				return Fail(failureReason, "Ability '" + binding.abilityId + "' level " + std::to_string(binding.level) + " out of range [1, " + std::to_string(abilityDef->GetMaxLevel()) + "] for profile '" + profile.profileId + "'.");
			}

			if (profile.powerScalingPolicy == EnemyPowerScalingPolicy::Disabled && AbilityHasPowerScaling(*abilityDef))
			{
				return Fail(failureReason, "Ability '" + binding.abilityId + "' uses AP/EP power scaling, which is disabled for profile '" + profile.profileId + "'.");
			}
		}

		for (const PrimaryWeaponDefinition* weaponDef : weaponDefinitions)
		{
			if (profile.powerScalingPolicy == EnemyPowerScalingPolicy::Disabled && WeaponHasPowerScaling(*weaponDef))
				return Fail(failureReason, "Weapon '" + weaponDef->weaponId + "' uses AP/EP power scaling, which is disabled for profile '" + profile.profileId + "'.");
		}

		return true;
	}

	bool EnemyCombatProfileCatalog::ValidateShippedProfiles(std::string* failureReason)
	{
		if (!IsLoaded() || GetDefinitions().empty())
		{
			return Fail(failureReason, "Enemy combat profile catalog is not loaded.");
		}
		return true;
	}

	bool EnemyCombatProfileCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		std::string* failureReason
	)
	{
		const EnemyCombatProfileLoader::Result loaded = EnemyCombatProfileLoader::LoadFromFile(filePath);
		if (!loaded.Succeeded())
		{
			return Fail(failureReason, loaded.error);
		}
		if (loaded.profiles.empty())
		{
			return Fail(failureReason, "Enemy combat profile catalog is empty");
		}

		std::set<std::string> profileIds;
		for (const EnemyCombatProfile& profile : loaded.profiles)
		{
			if (!profileIds.insert(profile.profileId).second)
			{
				return Fail(failureReason, "Duplicate enemy combat profile ID '" + profile.profileId + "' in catalog.");
			}
			if (!ValidateProfile(profile, failureReason))
			{
				return false;
			}
		}

		GetDefinitions() = loaded.profiles;
		GetLoadedState() = true;
		return true;
	}

	const EnemyCombatProfile* EnemyCombatProfileCatalog::FindById(const std::string& profileId)
	{
		for (const EnemyCombatProfile& profile : GetDefinitions())
		{
			if (profile.profileId == profileId)
			{
				return &profile;
			}
		}
		return nullptr;
	}

	const List<EnemyCombatProfile>& EnemyCombatProfileCatalog::GetProfiles()
	{
		return GetDefinitions();
	}

	bool EnemyCombatProfileCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}

	void EnemyCombatProfileCatalog::Clear()
	{
		GetDefinitions().clear();
		GetLoadedState() = false;
	}
}

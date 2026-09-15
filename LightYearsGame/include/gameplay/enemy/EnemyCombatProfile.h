#pragma once

#include "framework/Core.h"
#include "abilities/AbilityPolicies.h"
#include "gameConfigs/ship/ShipStructs.h"

#include <string>

namespace ly
{
	enum class EnemyPowerScalingPolicy
	{
		Disabled,
		Allowed
	};

	struct EnemyAbilityBinding
	{
		std::string abilityId;
		sas::AbilitySlot slot = sas::AbilitySlot::None;
		int level = 1;
	};

	struct EnemyWeaponBinding
	{
		std::string weaponId;
		sas::AbilitySlot slot = sas::AbilitySlot::PrimaryFire;
		int level = 1;
	};

	struct EnemyVariationRange
	{
		float minimumMultiplier = 1.f;
		float maximumMultiplier = 1.f;
	};

	struct EnemyProgressionDefinition
	{
		List<AttributeGrowthEntry> naturalGrowth;
		float maxShieldPerLevel = 0.f;
		float outgoingDamagePerLevel = 0.f;
		EnemyVariationRange maxHealthVariation;
		EnemyVariationRange armorVariation;
		EnemyVariationRange maxShieldVariation;
		EnemyVariationRange outgoingDamageVariation;
	};

	struct EnemyCombatProfile
	{
		std::string profileId;
		List<EnemyWeaponBinding> weapons;
		List<EnemyAbilityBinding> abilities;
		EnemyPowerScalingPolicy powerScalingPolicy = EnemyPowerScalingPolicy::Disabled;
		bool allowContactDamageOnly = false;
		EnemyProgressionDefinition progression;
	};
}

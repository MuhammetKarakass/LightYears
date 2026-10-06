#pragma once

#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"
#include "framework/Core.h"
#include "gameplay/damage/DamageContext.h"

#include <SFML/System/Vector2.hpp>

#include <string>

namespace ly
{
	// Cell offsets relative to the module's own origin; (0,0)-based.
	struct ModuleShape
	{
		List<sf::Vector2i> cells;
	};

	enum class ModuleAttributeTarget
	{
		Owner,
		Ship,
		PrimaryWeapon
	};

	enum class ModuleTriggerKind
	{
		None,
		AbilityActivated,
		ShieldBroken,
		DamageTaken,
		SourceDamageDealt
	};

	enum class ModuleDamageCondition
	{
		PrimaryShotOnly,
		TargetHealthAtMost,
		FullMagazineShot
	};

	enum class ModuleCooldownScope
	{
		FamilyOfZone,
		AllActiveNonPrimary
	};

	struct ModuleAttributeModifier
	{
		ModuleAttributeTarget target = ModuleAttributeTarget::Owner;
		sas::AttributeModifier modifier;
		bool affectedByZoneBonus = true;
	};

	// Resolved later into a ScopedAbilityRule for the module's family.
	struct ModuleAbilityModifier
	{
		sas::AttributeModifier modifier;
		bool affectedByZoneBonus = true;
	};

	struct ModuleTriggerBuff
	{
		ModuleTriggerKind trigger = ModuleTriggerKind::None;
		List<GameplayTag> requiredEventTags;
		float durationSeconds = 0.f;
		List<ModuleAttributeModifier> modifiers;
		float movementSpeedMultiplier = 1.f;
	};

	struct ModuleDamageRule
	{
		ModuleDamageCondition condition = ModuleDamageCondition::PrimaryShotOnly;
		float conditionValue = 0.f;
		// FullMagazineShot applies to this many successful shots at the start of each magazine.
		int openingShotCount = 1;
		float damageMultiplier = 1.f;
		float bonusDamageFraction = 0.f;
		GameplayTag bonusDamageType;
		DamageCriticalPolicy bonusCriticalPolicy = DamageCriticalPolicy::Disabled;
		ModuleTriggerKind activeWhileTrigger = ModuleTriggerKind::None;
		float activeDurationSeconds = 0.f;
	};

	struct ModuleZoneEffect
	{
		GameplayTag zone;
		ModuleCooldownScope scope = ModuleCooldownScope::FamilyOfZone;
		float multiplier = 1.f;
	};

	struct ModuleSizeVariant
	{
		ModuleShape shape;
		List<ModuleAttributeModifier> attributeModifiers;
		List<ModuleAbilityModifier> abilityModifiers;
		List<ModuleTriggerBuff> triggerBuffs;
		List<ModuleDamageRule> damageRules;
		List<ModuleZoneEffect> zoneEffects;
	};

	struct ModuleDefinition
	{
		sas::ContentId moduleId;
		std::string displayName;
		std::string description;
		// One of GameplayTags::Ability::{Primary,Offense,Defense,Movement,Utility}.
		GameplayTag family;
		// One of GameplayTags::Module::System::*; empty for a normal module.
		GameplayTag systemCategory;
		List<ModuleSizeVariant> sizeVariants;
	};

	struct ModuleZoneEntry
	{
		GameplayTag zone;
		// Family that receives the bonus; ignored when anyFamily is set ("*").
		GameplayTag family;
		bool anyFamily = false;
		float bonus = 0.f;
	};

	struct ModuleSystemZoneEntry
	{
		GameplayTag system;
		GameplayTag zone;
	};

	struct ModuleZoneTable
	{
		List<ModuleZoneEntry> zones;
		List<ModuleSystemZoneEntry> systemZones;
		float rebootDurationSeconds = 0.f;
		float combatWindowSeconds = 0.f;

		const ModuleZoneEntry* FindZone(const GameplayTag& zone) const
		{
			for (const ModuleZoneEntry& entry : zones)
			{
				if (entry.zone == zone) return &entry;
			}
			return nullptr;
		}

		const GameplayTag* FindSystemZone(const GameplayTag& system) const
		{
			for (const ModuleSystemZoneEntry& entry : systemZones)
			{
				if (entry.system == system) return &entry.zone;
			}
			return nullptr;
		}
	};
}

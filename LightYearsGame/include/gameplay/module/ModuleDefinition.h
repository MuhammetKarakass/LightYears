#pragma once

#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"
#include "framework/Core.h"

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

	struct ModuleSizeVariant
	{
		ModuleShape shape;
		List<ModuleAttributeModifier> attributeModifiers;
		List<ModuleAbilityModifier> abilityModifiers;
	};

	struct ModuleDefinition
	{
		sas::ContentId moduleId;
		std::string displayName;
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

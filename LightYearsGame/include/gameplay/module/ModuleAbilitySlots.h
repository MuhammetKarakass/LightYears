#pragma once
#include "abilities/AbilityTypes.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/module/ModuleLoadout.h"

namespace vega
{
	inline sas::AbilitySlot AbilitySlotForModuleSystem(const GameplayTag& system)
	{
		if (system == GameplayTags::Module::System::AbilitySlot1) return sas::AbilitySlot::Ability1;
		if (system == GameplayTags::Module::System::AbilitySlot2) return sas::AbilitySlot::Ability2;
		if (system == GameplayTags::Module::System::AbilitySlot3) return sas::AbilitySlot::Ability3;
		if (system == GameplayTags::Module::System::AbilitySlot4) return sas::AbilitySlot::Ability4;
		return sas::AbilitySlot::None;
	}

	inline GameplayTag ModuleSystemForAbilitySlot(sas::AbilitySlot slot)
	{
		if (slot == sas::AbilitySlot::Ability1) return GameplayTags::Module::System::AbilitySlot1;
		if (slot == sas::AbilitySlot::Ability2) return GameplayTags::Module::System::AbilitySlot2;
		if (slot == sas::AbilitySlot::Ability3) return GameplayTags::Module::System::AbilitySlot3;
		if (slot == sas::AbilitySlot::Ability4) return GameplayTags::Module::System::AbilitySlot4;
		return {};
	}

	inline sas::AbilitySlot AbilityModuleSlotAtCell(const ModuleRulesContext& rules, sf::Vector2i cell)
	{
		for (const auto& initial : rules.schematic.defaultModules)
		{
			const auto* module = rules.lookup ? rules.lookup(initial.moduleId) : nullptr;
			if (module && initial.origin == cell && AbilitySlotForModuleSystem(module->systemCategory) != sas::AbilitySlot::None)
				return AbilitySlotForModuleSystem(module->systemCategory);
		}
		return sas::AbilitySlot::None;
	}

	inline GameplayTag ModuleSystemForPlacement(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement)
	{
		if (!module.abilityModule) return module.systemCategory;
		return ModuleSystemForAbilitySlot(placement.abilitySlot == sas::AbilitySlot::None ? AbilityModuleSlotAtCell(rules, placement.origin) : placement.abilitySlot);
	}
}

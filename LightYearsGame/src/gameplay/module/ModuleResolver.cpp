#include "gameplay/module/ModuleResolver.h"

#include <utility>

namespace ly
{
	namespace ModuleResolver
	{
		namespace
		{
			GameplayTag GetPlacementZone(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement)
			{
				const List<sf::Vector2i> cells = ModulePlacementRules::GetCoveredCells(module, placement);
				if (cells.empty()) return {};

				const GameplayTag& zone = rules.schematic.GetZoneAt(cells.front().x, cells.front().y);
				if (!zone.IsValid()) return {};
				for (const sf::Vector2i& cell : cells)
					if (rules.schematic.GetZoneAt(cell.x, cell.y) != zone) return {};
				return zone;
			}

			sas::AttributeModifier Scale(sas::AttributeModifier modifier, float bonus)
			{
				switch (modifier.operation)
				{
				case sas::AttributeModifierOperation::Add:
					modifier.magnitude *= 1.f + bonus;
					break;
				case sas::AttributeModifierOperation::Multiply:
					modifier.magnitude = 1.f + (modifier.magnitude - 1.f) * (1.f + bonus);
					break;
				case sas::AttributeModifierOperation::Override:
					break;
				}
				return modifier;
			}

			sas::AttributeModifier Apply(const sas::AttributeModifier& modifier, bool scaled, float bonus)
			{
				return scaled ? Scale(modifier, bonus) : modifier;
			}
		}

		float GetZoneBonus(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement)
		{
			const GameplayTag zone = GetPlacementZone(rules, module, placement);
			if (!zone.IsValid()) return 0.f;
			const ModuleZoneEntry* entry = rules.zoneTable.FindZone(zone);
			if (!entry) return 0.f;
			return (entry->anyFamily || entry->family == module.family) ? entry->bonus : 0.f;
		}

		ResolvedModuleEffects Resolve(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements)
		{
			ResolvedModuleEffects effects;
			for (const ModulePlacement& placement : activePlacements)
			{
				const ModuleDefinition* module = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
				if (!module) continue;
				if (placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= module->sizeVariants.size()) continue;

				const ModuleSizeVariant& variant = module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)];
				const GameplayTag placementZone = GetPlacementZone(rules, *module, placement);
				const ModuleZoneEntry* placementZoneEntry = placementZone.IsValid() ? rules.zoneTable.FindZone(placementZone) : nullptr;
				const float bonus = placementZoneEntry && (placementZoneEntry->anyFamily || placementZoneEntry->family == module->family)
					? placementZoneEntry->bonus
					: 0.f;

				for (const ModuleAttributeModifier& entry : variant.attributeModifiers)
				{
					const sas::AttributeModifier resolved = Apply(entry.modifier, entry.affectedByZoneBonus, bonus);
					switch (entry.target)
					{
					case ModuleAttributeTarget::Owner: effects.ownerModifiers.push_back(resolved); break;
					case ModuleAttributeTarget::Ship: effects.shipModifiers.push_back(resolved); break;
					case ModuleAttributeTarget::PrimaryWeapon: effects.primaryWeaponModifiers.push_back(resolved); break;
					}
				}
				// Rolled side stats are never scaled and apply to the owner.
				for (const sas::AttributeModifier& sideStat : placement.rolledSideStats)
				{
					effects.ownerModifiers.push_back(sideStat);
				}

				if (!variant.abilityModifiers.empty())
				{
					ScopedAbilityRule rule;
					rule.requiredAbilityTags.push_back(module->family);
					for (const ModuleAbilityModifier& entry : variant.abilityModifiers)
					{
						rule.attributeModifiers.push_back(Apply(entry.modifier, entry.affectedByZoneBonus, bonus));
					}
					effects.abilityRules.push_back(std::move(rule));
				}

				for (std::size_t ruleIndex = 0; ruleIndex < variant.triggerBuffs.size(); ++ruleIndex)
				{
					ResolvedModuleTriggerBuff resolved{ placement.instanceId, ruleIndex, variant.triggerBuffs[ruleIndex] };
					for (ModuleAttributeModifier& modifier : resolved.rule.modifiers)
						modifier.modifier = Apply(modifier.modifier, modifier.affectedByZoneBonus, bonus);
					effects.triggerBuffs.push_back(std::move(resolved));
				}

				for (std::size_t ruleIndex = 0; ruleIndex < variant.damageRules.size(); ++ruleIndex)
					effects.damageRules.push_back({ placement.instanceId, ruleIndex, variant.damageRules[ruleIndex] });

				if (placementZone.IsValid())
				{
					for (std::size_t ruleIndex = 0; ruleIndex < variant.zoneEffects.size(); ++ruleIndex)
					{
						const ModuleZoneEffect& rule = variant.zoneEffects[ruleIndex];
						if (rule.zone != placementZone) continue;
						GameplayTag family;
						if (rule.scope == ModuleCooldownScope::FamilyOfZone)
						{
							if (!placementZoneEntry || placementZoneEntry->anyFamily || !placementZoneEntry->family.IsValid()) continue;
							family = placementZoneEntry->family;
						}
						effects.cooldownRules.push_back({ placement.instanceId, ruleIndex, rule.zone, rule.scope, family, rule.multiplier });
					}
				}

				if (module->systemCategory.IsValid())
				{
					effects.enabledSystems.push_back(module->systemCategory);
				}
			}
			return effects;
		}
	}
}

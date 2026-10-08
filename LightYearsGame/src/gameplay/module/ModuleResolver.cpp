#include "gameplay/module/ModuleResolver.h"

#include <limits>
#include <utility>

namespace ly
{
	namespace ModuleResolver
	{
		namespace
		{
			template<typename T>
			T ValueAtLevel(T baseValue, const ModuleLevelTable<T>& table, int level)
			{
				if (level < 1 || static_cast<std::size_t>(level) > table.values.size()) return baseValue;
				return table.values[static_cast<std::size_t>(level - 1)];
			}

			std::size_t NamespacedRuleIndex(std::size_t ruleIndex, bool sideFeature)
			{
				constexpr std::size_t sideNamespace = std::size_t{ 1 } << (std::numeric_limits<std::size_t>::digits - 1);
				return sideFeature ? sideNamespace | ruleIndex : ruleIndex;
			}

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

			void ResolvePackage(ResolvedModuleEffects& effects, const ModuleRulePackage& package, const ModuleDefinition& module, const ModulePlacement& placement, int level, float zoneBonus, const GameplayTag& placementZone, const ModuleZoneEntry* placementZoneEntry, bool sideFeature)
			{
				for (const ModuleAttributeModifier& entry : package.attributeModifiers)
				{
					sas::AttributeModifier modifier = entry.modifier;
					modifier.magnitude = ValueAtLevel(modifier.magnitude, entry.magnitudeByLevel, level);
					modifier = Apply(modifier, entry.affectedByZoneBonus, zoneBonus);
					switch (entry.target)
					{
					case ModuleAttributeTarget::Owner: effects.ownerModifiers.push_back(modifier); break;
					case ModuleAttributeTarget::Ship: effects.shipModifiers.push_back(modifier); break;
					case ModuleAttributeTarget::PrimaryWeapon: effects.primaryWeaponModifiers.push_back(modifier); break;
					}
				}
				if (!package.abilityModifiers.empty())
				{
					ScopedAbilityRule rule;
					rule.requiredAbilityTags.push_back(module.family);
					for (const ModuleAbilityModifier& entry : package.abilityModifiers)
					{
						sas::AttributeModifier modifier = entry.modifier;
						modifier.magnitude = ValueAtLevel(modifier.magnitude, entry.magnitudeByLevel, level);
						rule.attributeModifiers.push_back(Apply(modifier, entry.affectedByZoneBonus, zoneBonus));
					}
					effects.abilityRules.push_back(std::move(rule));
				}
				for (std::size_t ruleIndex = 0; ruleIndex < package.triggerBuffs.size(); ++ruleIndex)
				{
					ModuleTriggerBuff rule = package.triggerBuffs[ruleIndex];
					rule.durationSeconds = ValueAtLevel(rule.durationSeconds, rule.durationSecondsByLevel, level);
					rule.movementSpeedMultiplier = ValueAtLevel(rule.movementSpeedMultiplier,
						rule.movementSpeedMultiplierByLevel, level);
					for (ModuleAttributeModifier& modifier : rule.modifiers)
					{
						modifier.modifier.magnitude = ValueAtLevel(modifier.modifier.magnitude, modifier.magnitudeByLevel, level);
						modifier.modifier = Apply(modifier.modifier, modifier.affectedByZoneBonus, zoneBonus);
					}
					effects.triggerBuffs.push_back({ placement.instanceId, NamespacedRuleIndex(ruleIndex, sideFeature), std::move(rule) });
				}
				for (std::size_t ruleIndex = 0; ruleIndex < package.damageRules.size(); ++ruleIndex)
				{
					ModuleDamageRule rule = package.damageRules[ruleIndex];
					rule.conditionValue = ValueAtLevel(rule.conditionValue, rule.conditionValueByLevel, level);
					rule.openingShotCount = ValueAtLevel(rule.openingShotCount, rule.openingShotCountByLevel, level);
					rule.damageMultiplier = ValueAtLevel(rule.damageMultiplier, rule.damageMultiplierByLevel, level);
					rule.bonusDamageFraction = ValueAtLevel(rule.bonusDamageFraction, rule.bonusDamageFractionByLevel, level);
					rule.activeDurationSeconds = ValueAtLevel(rule.activeDurationSeconds, rule.activeDurationSecondsByLevel, level);
					rule.maxHealthDamageFraction = ValueAtLevel(rule.maxHealthDamageFraction, rule.maxHealthDamageFractionByLevel, level);
					effects.damageRules.push_back({ placement.instanceId, NamespacedRuleIndex(ruleIndex, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.eventActions.size(); ++index)
				{
					ModuleEventAction rule = package.eventActions[index];
					rule.amount = ValueAtLevel(rule.amount, rule.amountByLevel, level);
					rule.chancePerLuck = ValueAtLevel(rule.chancePerLuck, rule.chancePerLuckByLevel, level);
					rule.targetCount = ValueAtLevel(rule.targetCount, rule.targetCountByLevel, level);
					effects.eventActions.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.ownerAttributeRules.size(); ++index)
				{
					ModuleOwnerAttributeRule rule = package.ownerAttributeRules[index];
					rule.amount = ValueAtLevel(rule.amount, rule.amountByLevel, level);
					effects.ownerAttributeRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.shieldReflections.size(); ++index)
				{
					auto rule = package.shieldReflections[index];
					rule.fraction = ValueAtLevel(rule.fraction, rule.fractionByLevel, level);
					effects.shieldReflections.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				if (!placementZone.IsValid()) return;
				for (std::size_t ruleIndex = 0; ruleIndex < package.zoneEffects.size(); ++ruleIndex)
				{
					const ModuleZoneEffect& source = package.zoneEffects[ruleIndex];
					if (source.zone != placementZone) continue;
					GameplayTag family;
					if (source.scope == ModuleCooldownScope::FamilyOfZone)
					{
						if (!placementZoneEntry || placementZoneEntry->anyFamily || !placementZoneEntry->family.IsValid()) continue;
						family = placementZoneEntry->family;
					}
					const float multiplier = ValueAtLevel(source.multiplier, source.multiplierByLevel, level);
					effects.cooldownRules.push_back({ placement.instanceId, NamespacedRuleIndex(ruleIndex, sideFeature),
						source.zone, source.scope, family, multiplier });
				}
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
				if (placement.level < 1 || placement.level > module->levelCount ||
					!HasModuleRulePackageLevelCoverage(variant, placement.level)) continue;
				const GameplayTag placementZone = GetPlacementZone(rules, *module, placement);
				const ModuleZoneEntry* placementZoneEntry = placementZone.IsValid() ? rules.zoneTable.FindZone(placementZone) : nullptr;
				const float bonus = placementZoneEntry && (placementZoneEntry->anyFamily || placementZoneEntry->family == module->family)
					? placementZoneEntry->bonus
					: 0.f;

				ResolvePackage(effects, variant, *module, placement, placement.level, bonus, placementZone, placementZoneEntry, false);
				// Rolled side stats are never scaled and apply to the owner.
				for (const sas::AttributeModifier& sideStat : placement.rolledSideStats)
				{
					effects.ownerModifiers.push_back(sideStat);
				}

				if (placement.sideFeatureId.IsValid() && rules.sideFeatureLookup && IsLargestCellCountVariant(*module, placement.variantIndex))
				{
					const ModuleSideFeatureDefinition* feature = rules.sideFeatureLookup(placement.sideFeatureId);
					if (IsSideFeatureUsableAtLevel(*module, feature, placement.level))
						ResolvePackage(effects, feature->rules, *module, placement, placement.level, bonus, placementZone, placementZoneEntry, true);
				}

				if (module->systemCategory.IsValid())
				{
					effects.enabledSystems.push_back(module->systemCategory);
				}
			}
			for (auto& damage : effects.damageRules)
				if (damage.rule.condition == ModuleDamageCondition::AdjacentAbilityDirectHit)
					for (const auto& placement : activePlacements)
						if (placement.instanceId == damage.instanceId)
							damage.adjacentAbilitySystems = ModulePlacementRules::GetAdjacentAbilitySlotSystems(rules, activePlacements, placement);
			return effects;
		}
	}
}

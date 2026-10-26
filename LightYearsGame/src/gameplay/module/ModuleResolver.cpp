#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/rules/EpicStatRuleResolution.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <variant>
#include <vector>

namespace vega
{
	namespace ModuleResolver
	{
		namespace
		{
			template<typename T>
			T ValueAtLevel(T baseValue, const ModuleLevelTable<T>& table, int level)
			{
				return table.ValueAtLevel(baseValue, level);
			}

			std::size_t NamespacedRuleIndex(std::size_t ruleIndex, bool sideFeature)
			{
				constexpr std::size_t sideNamespace = std::size_t{ 1 } << (std::numeric_limits<std::size_t>::digits - 1);
				return sideFeature ? sideNamespace | ruleIndex : ruleIndex;
			}

			// Adjacent Q/E/F/R abilities of a placement, in geometric order: covered cells in shape order, each probed up, right,
			// down, left. Stops at count. Unlike GetAdjacentAbilitySlotSystems this keeps no alphabetical order.
			List<sas::AbilitySlot> GeometricAbilitySlots(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements, const ModulePlacement& placement, std::size_t count)
			{
				List<sas::AbilitySlot> result;
				const auto* owner = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
				if (!owner || count == 0) return result;
				const sf::Vector2i directions[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
				for (const sf::Vector2i& cell : ModulePlacementRules::GetCoveredCells(*owner, placement))
				{
					for (const sf::Vector2i& direction : directions)
					{
						const sf::Vector2i probe = cell + direction;
						for (const ModulePlacement& candidate : activePlacements)
						{
							if (candidate.instanceId == placement.instanceId) continue;
							const auto* module = rules.lookup(candidate.moduleId);
							if (!module || !GameplayTagSchema::IsModuleAbilitySlotSystem(ModuleSystemForPlacement(rules, *module, candidate))) continue;
							const sas::AbilitySlot slot = AbilitySlotForModuleSystem(ModuleSystemForPlacement(rules, *module, candidate));
							if (slot == sas::AbilitySlot::None || std::find(result.begin(), result.end(), slot) != result.end()) continue;
							const List<sf::Vector2i> candidateCells = ModulePlacementRules::GetCoveredCells(*module, candidate);
							if (std::find(candidateCells.begin(), candidateCells.end(), probe) == candidateCells.end()) continue;
							result.push_back(slot);
							if (result.size() >= count) return result;
						}
					}
				}
				return result;
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

			struct InstalledRules
			{
				const ModuleDefinition* module = nullptr;
				const ModuleSizeVariant* variant = nullptr;
			};

			// Same validity checks Resolve applies before it reads a placement's rules.
			InstalledRules FindInstalledRules(const ModuleRulesContext& rules, const ModulePlacement& placement)
			{
				InstalledRules found;
				found.module = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
				if (!found.module) return {};
				if (placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= found.module->sizeVariants.size()) return {};
				found.variant = &found.module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)];
				if (placement.level < 1 || placement.level > found.module->levelCount || !HasModuleRulePackageLevelCoverage(*found.variant, placement.level)) return {};
				return found;
			}

				// Ability-slot shells (Module.System.AbilitySlot1..4) that no installed placement provides.
				int CountEmptyAbilitySlots(const ModuleRulesContext& rules, const List<ModulePlacement>& placements)
				{
					bool filled[4] = { false, false, false, false };
					for (const ModulePlacement& placement : placements)
					{
						const InstalledRules installed = FindInstalledRules(rules, placement);
						if (!installed.module) continue;
						const sas::AbilitySlot slot = AbilitySlotForModuleSystem(ModuleSystemForPlacement(rules, *installed.module, placement));
						if (slot != sas::AbilitySlot::None) filled[static_cast<int>(slot) - static_cast<int>(sas::AbilitySlot::Ability1)] = true;
					}
					return static_cast<int>(std::count(filled, filled + 4, false));
				}

			// Ability shells a fragmented charge module may reach: fragments are not capped (F1 keeps adjacency uncapped), and the four
			// Q/E/F/R shells are the only ones that exist.
			constexpr int kAbilityShellCount = 4;

			// Status rule spans of one placement's main package. A side modifier merges only into the entries that package produced.
			struct StatusRuleCounts { std::size_t thermal = 0; std::size_t kinetic = 0; std::size_t sustained = 0; };
			StatusRuleCounts CountStatusRules(const ResolvedModuleEffects& effects)
			{
				return StatusRuleCounts{ effects.statusRules.thermalBursts.size(), effects.statusRules.kineticConsumes.size(), effects.sustainedSpeedRules.size() };
			}
			// Side modifiers never add a burst, Armor Rend or sonic rule of their own; they widen the placement's main entries.
			void MergeThermalModifier(ModuleThermalBurstRule& target, const ModuleThermalBurstRule& modifier)
			{
				if (modifier.areaRadius > 0.f) { target.areaRadius = modifier.areaRadius; target.areaDamageFraction = modifier.areaDamageFraction; }
				if (modifier.smolderSeconds > 0.f) { target.smolderSeconds = modifier.smolderSeconds; target.smolderRadius = modifier.smolderRadius; target.smolderStacksPerSecond = modifier.smolderStacksPerSecond; }
			}
			void MergeKineticModifier(ModuleKineticConsumeRule& target, const ModuleKineticConsumeRule& modifier)
			{
				if (modifier.normalTargetThreshold > 0) target.normalTargetThreshold = modifier.normalTargetThreshold;
				if (modifier.rendFraction > 0.f) { target.rendFraction = modifier.rendFraction; target.rendSeconds = modifier.rendSeconds; }
			}
			// Side modifiers extend this placement's sonic duration or turn capability.
			void MergeSustainedModifier(ModuleSustainedSpeedRule& target, const ModuleSustainedSpeedRule& modifier)
			{
				if (modifier.lingerSeconds > 0.f) target.lingerSeconds = modifier.lingerSeconds;
				target.turnCapabilityMultiplier = std::max(target.turnCapabilityMultiplier, modifier.turnCapabilityMultiplier);
			}
			void MergeSideStatusModifiers(ResolvedModuleEffects& effects, const ModuleRulePackage& side, const StatusRuleCounts& mainBegin, const StatusRuleCounts& mainEnd)
			{
				for (const ModuleThermalBurstRule& modifier : side.statusRules.thermalBursts)
					for (std::size_t index = mainBegin.thermal; index < mainEnd.thermal; ++index) MergeThermalModifier(effects.statusRules.thermalBursts[index], modifier);
				for (const ModuleKineticConsumeRule& modifier : side.statusRules.kineticConsumes)
					for (std::size_t index = mainBegin.kinetic; index < mainEnd.kinetic; ++index) MergeKineticModifier(effects.statusRules.kineticConsumes[index], modifier);
				for (const ModuleSustainedSpeedRule& modifier : side.sustainedSpeedRules)
					for (std::size_t index = mainBegin.sustained; index < mainEnd.sustained; ++index) MergeSustainedModifier(effects.sustainedSpeedRules[index].rule, modifier);
			}
			void ResolvePackage(ResolvedModuleEffects& effects, const ModuleRulePackage& package, const ModuleDefinition& module, const ModulePlacement& placement, int level, int baseLevel, const GameplayTag& placementZone, const ModuleZoneEntry* placementZoneEntry, bool sideFeature)
			{
				for (const ModuleAttributeModifier& entry : package.attributeModifiers)
				{
					sas::AttributeModifier modifier = entry.modifier;
					modifier.magnitude = ValueAtLevel(modifier.magnitude, entry.magnitudeByLevel, entry.affectedByZoneBonus ? level : baseLevel);
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
						modifier.magnitude = ValueAtLevel(modifier.magnitude, entry.magnitudeByLevel, entry.affectedByZoneBonus ? level : baseLevel);
						rule.attributeModifiers.push_back(modifier);
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
						modifier.modifier.magnitude = ValueAtLevel(modifier.modifier.magnitude, modifier.magnitudeByLevel, modifier.affectedByZoneBonus ? level : baseLevel);
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
					rule.perEmptyCellMultiplier = ValueAtLevel(rule.perEmptyCellMultiplier, rule.perEmptyCellMultiplierByLevel, level);
					effects.damageRules.push_back({ placement.instanceId, NamespacedRuleIndex(ruleIndex, sideFeature), std::move(rule) });
				}
				for (std::size_t ruleIndex = 0; ruleIndex < package.targetSlowRules.size(); ++ruleIndex)
					effects.targetSlowRules.push_back({ placement.instanceId, NamespacedRuleIndex(ruleIndex, sideFeature), package.targetSlowRules[ruleIndex] });
				for (std::size_t index = 0; index < package.eventActions.size(); ++index)
				{
					ModuleEventAction rule = package.eventActions[index];
					if (sideFeature && rule.kind == ModuleEventActionKind::ReduceLongestCooldown)
						for (const ModuleSizeVariant& variant : module.sizeVariants)
							for (const ModuleEventAction& main : variant.eventActions)
								if (main.kind == ModuleEventActionKind::ReduceFamilyCooldowns) rule.abilityFamily = main.abilityFamily;
					rule.amount = ValueAtLevel(rule.amount, rule.amountByLevel, level);
					rule.chancePerLuck = ValueAtLevel(rule.chancePerLuck, rule.chancePerLuckByLevel, level);
					rule.targetCount = ValueAtLevel(rule.targetCount, rule.targetCountByLevel, level);
					if (rule.kind == ModuleEventActionKind::ReduceFamilyCooldowns || rule.kind == ModuleEventActionKind::ReduceLongestCooldown || rule.kind == ModuleEventActionKind::RefundAllCooldownsFraction) rule.amount = std::min(1.f, rule.amount);
					effects.eventActions.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.ownerAttributeRules.size(); ++index)
				{
					ModuleOwnerAttributeRule rule = package.ownerAttributeRules[index];
					rule.amount = ValueAtLevel(rule.amount, rule.amountByLevel, level);
					rule.maximumAmount = ValueAtLevel(rule.maximumAmount, rule.maximumAmountByLevel, level);
					if (sideFeature && rule.source == ModuleOwnerAttributeSource::ArmorLowerPower)
					{
						float conversion = 0.f;
						for (const auto& main : effects.ownerAttributeRules) if (main.instanceId == placement.instanceId && main.rule.source == ModuleOwnerAttributeSource::ArmorHigherPower) conversion += main.rule.amount;
						rule.amount *= conversion;
					}
					effects.ownerAttributeRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.conditionalMovementRules.size(); ++index)
				{
					auto rule = package.conditionalMovementRules[index]; rule.multiplier = ValueAtLevel(rule.multiplier, rule.multiplierByLevel, level);
					effects.conditionalMovementRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), rule });
				}
				for (const auto& rule : package.movementFadeRules) effects.movementFadeRules.push_back({ placement.instanceId, rule });
				for (std::size_t index = 0; index < package.shieldReflections.size(); ++index)
				{
					auto rule = package.shieldReflections[index];
					rule.fraction = ValueAtLevel(rule.fraction, rule.fractionByLevel, level);
					effects.shieldReflections.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.shieldRules.size(); ++index)
				{
					auto rule = package.shieldRules[index];
					rule.speedMultiplierAtMax = ValueAtLevel(rule.speedMultiplierAtMax, rule.speedMultiplierAtMaxByLevel, level);
					rule.continuousRateFraction = ValueAtLevel(rule.continuousRateFraction, rule.continuousRateFractionByLevel, level);
					rule.breakRearmFraction = ValueAtLevel(rule.breakRearmFraction, rule.breakRearmFractionByLevel, level);
					rule.armorShare = ValueAtLevel(rule.armorShare, rule.armorShareByLevel, level);
					effects.shieldRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.shieldBatteryRules.size(); ++index)
				{
					auto rule = package.shieldBatteryRules[index];
					rule.reserveFraction = ValueAtLevel(rule.reserveFraction, rule.reserveFractionByLevel, level);
					effects.shieldBatteryRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.shieldRuptureRules.size(); ++index)
				{
					auto rule = package.shieldRuptureRules[index];
					rule.damageFraction = ValueAtLevel(rule.damageFraction, rule.damageFractionByLevel, level);
					effects.shieldRuptureRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.movementRules.size(); ++index)
				{
					auto rule = package.movementRules[index];
					rule.thrustMultiplier = ValueAtLevel(rule.thrustMultiplier, rule.thrustMultiplierByLevel, level);
					effects.movementRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				if (!sideFeature) for (std::size_t index = 0; index < package.sustainedSpeedRules.size(); ++index)
				{
					auto rule = package.sustainedSpeedRules[index];
					rule.primaryDamageMultiplier = ValueAtLevel(rule.primaryDamageMultiplier, rule.primaryDamageMultiplierByLevel, level);
					effects.sustainedSpeedRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (const ModulePrimaryMountRule& mount : package.primaryMountRules)
				{
					if (mount.mode == ModulePrimaryMountMode::Turret) effects.primaryMount.turret = true;
					else
					{
						const float multiplier = ValueAtLevel(mount.damageMultiplier, mount.damageMultiplierByLevel, level);
						// Several Broadside placements still fire one volley per shot; the strongest coefficient applies.
						effects.primaryMount.broadsideDamageMultiplier = effects.primaryMount.broadside ? std::max(effects.primaryMount.broadsideDamageMultiplier, multiplier) : multiplier;
						effects.primaryMount.broadside = true;
					}
				}
				if (sideFeature && package.primaryExecutionPriority) effects.primaryMount.executionPriority = true;
				for (auto rule : package.statusRules.electricOverrides)
				{
					for (std::size_t index = 0; index < rule.damageTakenByStack.size() && index < rule.damageTakenByStackByLevel.size(); ++index)
						rule.damageTakenByStack[index] = ValueAtLevel(rule.damageTakenByStack[index], rule.damageTakenByStackByLevel[index], level);
					effects.statusRules.electricOverrides.push_back(std::move(rule));
				}
				for (std::size_t index = 0; index < package.statusRules.primaryStatusPrimers.size(); ++index)
				{
					auto rule = package.statusRules.primaryStatusPrimers[index];
					rule.cooldownSeconds = ValueAtLevel(rule.cooldownSeconds, rule.cooldownSecondsByLevel, level); rule.instanceId = placement.instanceId; rule.ruleIndex = NamespacedRuleIndex(index, sideFeature);
					effects.statusRules.primaryStatusPrimers.push_back(rule);
				}
				for (std::size_t index = 0; index < package.statusRules.electricSpreads.size(); ++index)
				{
					auto rule = package.statusRules.electricSpreads[index];
					rule.instanceId = placement.instanceId;
					rule.ruleIndex = NamespacedRuleIndex(index, sideFeature);
					effects.statusRules.electricSpreads.push_back(rule);
				}
				for (auto rule : package.statusRules.cryoFreezes)
				{
					rule.durationSeconds = ValueAtLevel(rule.durationSeconds, rule.durationSecondsByLevel, level);
					rule.cooldownSeconds = ValueAtLevel(rule.cooldownSeconds, rule.cooldownSecondsByLevel, level);
					effects.statusRules.cryoFreezes.push_back(std::move(rule));
				}
				for (auto rule : package.statusRules.thermalCryoDurationBonuses)
				{
					rule.bonusSeconds = ValueAtLevel(rule.bonusSeconds, rule.bonusSecondsByLevel, level);
					effects.statusRules.thermalCryoDurationBonuses.push_back(std::move(rule));
				}
				for (auto rule : package.statusRules.durationBonuses)
				{
					rule.bonusSeconds = ValueAtLevel(rule.bonusSeconds, rule.bonusSecondsByLevel, level);
					rule.durationMultiplier = ValueAtLevel(rule.durationMultiplier, rule.durationMultiplierByLevel, level);
					effects.statusRules.durationBonuses.push_back(std::move(rule));
				}
				for (const auto& rule : package.statusRules.freezeSpreads) effects.statusRules.freezeSpreads.push_back(rule);
				if (!sideFeature) for (auto rule : package.statusRules.thermalBursts)
				{
					rule.damageFraction = ValueAtLevel(rule.damageFraction, rule.damageFractionByLevel, level);
					effects.statusRules.thermalBursts.push_back(std::move(rule));
				}
				if (!sideFeature) for (auto rule : package.statusRules.kineticConsumes)
				{
					rule.damageMultiplier = ValueAtLevel(rule.damageMultiplier, rule.damageMultiplierByLevel, level);
					effects.statusRules.kineticConsumes.push_back(std::move(rule));
				}
				for (std::size_t index = 0; index < package.generatedRules.ricochets.size(); ++index)
				{
					auto rule = package.generatedRules.ricochets[index];
					rule.damageFraction = ValueAtLevel(rule.damageFraction, rule.damageFractionByLevel, level);
					effects.generatedRules.ricochets.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.generatedRules.guidedSalvos.size(); ++index)
				{
					auto rule = package.generatedRules.guidedSalvos[index];
					rule.damageFraction = ValueAtLevel(rule.damageFraction, rule.damageFractionByLevel, level);
					effects.generatedRules.guidedSalvos.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.generatedRules.delayedExplosions.size(); ++index)
				{
					auto rule = package.generatedRules.delayedExplosions[index];
					rule.damageFraction = ValueAtLevel(rule.damageFraction, rule.damageFractionByLevel, level);
					effects.generatedRules.delayedExplosions.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.generatedRules.trailMines.size(); ++index)
				{
					auto rule = package.generatedRules.trailMines[index];
					rule.damage = ValueAtLevel(rule.damage, rule.damageByLevel, level);
					rule.attackPowerCoefficient = ValueAtLevel(rule.attackPowerCoefficient, rule.attackPowerCoefficientByLevel, level);
					rule.maxActive = ValueAtLevel(rule.maxActive, rule.maxActiveByLevel, level);
					effects.generatedRules.trailMines.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.generatedRules.killExplosions.size(); ++index)
				{
					auto rule = package.generatedRules.killExplosions[index];
					rule.maxHealthFraction = ValueAtLevel(rule.maxHealthFraction, rule.maxHealthFractionByLevel, level);
					effects.generatedRules.killExplosions.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.generatedRules.annularBursts.size(); ++index)
				{
					auto rule = package.generatedRules.annularBursts[index];
					rule.baseDamage = ValueAtLevel(rule.baseDamage, rule.baseDamageByLevel, level);
					rule.energyPowerCoefficient = ValueAtLevel(rule.energyPowerCoefficient, rule.energyPowerCoefficientByLevel, level);
					effects.generatedRules.annularBursts.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.generatedRules.speedLances.size(); ++index)
				{
					auto rule = package.generatedRules.speedLances[index];
					rule.speedDamageCoefficient = ValueAtLevel(rule.speedDamageCoefficient, rule.speedDamageCoefficientByLevel, level);
					effects.generatedRules.speedLances.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule) });
				}
				for (std::size_t index = 0; index < package.ownerBurstRules.size(); ++index)
				{
					auto rule = package.ownerBurstRules[index];
					rule.damageFractionOfMaxHealth = ValueAtLevel(rule.damageFractionOfMaxHealth, rule.damageFractionOfMaxHealthByLevel, level);
					rule.damage = ValueAtLevel(rule.damage, rule.damageByLevel, level);
					float armor = 0.f;
					for (const auto& counter : module.sizeVariants[static_cast<std::size_t>(placement.variantIndex)].runCounterRules) if (counter.source == ModuleRunCounterSource::AbilityUses)
						armor += std::min(counter.armorCapByLevel.ValueAtLevel(counter.armorCap, level), counter.armorPerStack * GetModuleRunCounterStacks(counter, placement.runCounter));
					effects.ownerBurstRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), std::move(rule), armor });
				}
				// Modifiers patch only the main rules (side bit clear) of this placement; they never add a rule or change another placement.
				for (const ModuleOwnerBurstModifier& modifier : package.ownerBurstModifiers)
					for (ResolvedModuleOwnerBurstRule& resolved : effects.ownerBurstRules)
						if (resolved.instanceId == placement.instanceId && (resolved.ruleIndex >> (std::numeric_limits<std::size_t>::digits - 1)) == 0) ApplyModuleOwnerBurstModifier(resolved.rule, modifier);
				for (std::size_t index = 0; index < package.runCounterRules.size(); ++index)
				{
					const ModuleRunCounterRule& source = package.runCounterRules[index];
					if (source.source == ModuleRunCounterSource::AbilityUses)
					{
						const float armor = std::min(source.armorCapByLevel.ValueAtLevel(source.armorCap, level), source.armorPerStack * GetModuleRunCounterStacks(source, placement.runCounter));
						if (armor > 0.f) effects.ownerModifiers.push_back({ OwnerAttributeIds::Armor, sas::AttributeModifierOperation::Add, armor });
						continue;
					}
					if (source.source != ModuleRunCounterSource::Kill) continue;
					const float perStack = ValueAtLevel(source.damagePerStack, source.damagePerStackByLevel, level);
					const std::optional<float> elite = source.eliteDamagePerKillByLevel.values.empty() ? std::nullopt : std::optional<float>{ source.eliteDamagePerKillByLevel.ValueAtLevel(0.f, level) };
					const std::optional<float> boss = source.bossDamagePerKillByLevel.values.empty() ? std::nullopt : std::optional<float>{ source.bossDamagePerKillByLevel.ValueAtLevel(0.f, level) };
					effects.killRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), perStack, source.killWeights, source.adjacentAbilityLimit, {}, elite, boss });
				}
				for (std::size_t index = 0; index < package.killStackCooldownRules.size(); ++index)
					effects.killCooldownRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), package.killStackCooldownRules[index], {} });
				for (std::size_t index = 0; index < package.killStackDamageRules.size(); ++index)
					effects.killDamageRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), package.killStackDamageRules[index] });
				for (std::size_t index = 0; index < package.chargeRules.size(); ++index)
				{
					const ModuleChargeRule& rule = package.chargeRules[index];
					const int neighbourCount = placement.fragmented ? kAbilityShellCount : static_cast<int>(ModulePlacementRules::GetCoveredCells(module, placement).size());
					effects.chargeRules.push_back({ placement.instanceId, NamespacedRuleIndex(index, sideFeature), rule.chargePerLevel.ValueAtLevel(0, level), rule.overflowTransfer, rule.trickleSeconds, rule.trickleCharges, neighbourCount, {} });
				}
				MergeEpicOffenseRules(effects.epicOffense, package.epicOffense, level);
				ResolveEpicUtilityMovementRules(effects, package, placement.instanceId, level, sideFeature);
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
					if (source.kind == ModuleZoneEffectKind::PrimaryHitCooldownSeconds)
					{
						ModuleEventAction action;
						action.trigger = ModuleTriggerKind::PrimaryHit;
						action.kind = ModuleEventActionKind::ReduceAllCooldowns;
						action.amount = ValueAtLevel(source.seconds, source.secondsByLevel, level);
						action.abilityFamily = family;
						effects.eventActions.push_back({ placement.instanceId, NamespacedRuleIndex(package.eventActions.size() + ruleIndex, sideFeature), std::move(action) });
						continue;
					}
					const float multiplier = ValueAtLevel(source.multiplier, source.multiplierByLevel, level);
					effects.cooldownRules.push_back({ placement.instanceId, NamespacedRuleIndex(ruleIndex, sideFeature),
						source.zone, source.scope, family, multiplier });
				}
			}
		}

		GameplayTag GetPlacementFamily(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement)
		{
			if (placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= module.sizeVariants.size()) return module.family;
			const auto& effects = module.sizeVariants[static_cast<std::size_t>(placement.variantIndex)].zoneEffects;
			if (std::none_of(effects.begin(), effects.end(), [](const ModuleZoneEffect& effect) { return effect.kind == ModuleZoneEffectKind::PrimaryHitCooldownSeconds; })) return module.family;
			const auto* zone = rules.zoneTable.FindZone(GetPlacementZone(rules, module, placement));
			return zone && !zone->anyFamily ? zone->family : GameplayTag{};
		}
		namespace
		{
			int UnderdogRarityWeight(const ResolvedModuleLegendaryUnderdogCoreRule& rule, ModuleRarity rarity)
			{
				switch (rarity)
				{
				case ModuleRarity::Common: return rule.commonWeight;
				case ModuleRarity::Uncommon: return rule.uncommonWeight;
				case ModuleRarity::Rare: return rule.rareWeight;
				case ModuleRarity::Epic: return rule.epicWeight;
				case ModuleRarity::Legendary: return rule.legendaryWeight;
				case ModuleRarity::None: break;
				}
				return 0;
			}

			bool IsUnderdogCore(const ModuleDefinition& module)
			{
				return module.legendary && std::holds_alternative<ModuleLegendaryUnderdogCoreRule>(*module.legendary);
			}

			void ComputeUnderdogWeights(ResolvedModuleLegendaryUnderdogCoreRule& resolved, const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements)
			{
				for (const ModulePlacement& placement : activePlacements)
				{
					const ModuleDefinition* module = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
					if (!module || IsUnderdogCore(*module) || module->abilityModule || module->systemCategory.IsValid() || module->rarity == ModuleRarity::None) continue;
					const int weight = UnderdogRarityWeight(resolved, module->rarity);
					if (weight <= 0) continue;
					const GameplayTag family = GetPlacementFamily(rules, *module, placement);
					if (family == GameplayTags::Ability::Offense) resolved.offenseWeight += weight;
					else if (family == GameplayTags::Ability::Defense) resolved.defenseWeight += weight;
					else if (family == GameplayTags::Ability::Primary) resolved.primaryWeight += weight;
					else if (family == GameplayTags::Ability::Movement) resolved.movementWeight += weight;
					else if (family == GameplayTags::Ability::Utility) resolved.utilityWeight += weight;
				}
			}
		}


		int GetZoneLevelBonus(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement)
		{
			if (module.abilityModule) return 0;
			const auto zoneLevelBonus = [&rules, &module, &placement](const GameplayTag& zone) -> int
			{
				const ModuleZoneEntry* entry = zone.IsValid() ? rules.zoneTable.FindZone(zone) : nullptr;
				return entry && (entry->anyFamily || entry->family == GetPlacementFamily(rules, module, placement)) ? entry->levelBonus : 0;
			};
			int bonus = 0;
			if (placement.fragmented)
			{
				const List<sf::Vector2i> cells = ModulePlacementRules::GetCoveredCells(module, placement);
				if (cells.empty()) return 0;
				bonus = kMaxZoneLevelBonus;
				for (const sf::Vector2i& cell : cells) bonus = std::min(bonus, zoneLevelBonus(rules.schematic.GetZoneAt(cell.x, cell.y)));
			}
			else bonus = zoneLevelBonus(GetPlacementZone(rules, module, placement));
			if (rules.sideFeatureLookup && IsLargestCellCountVariant(module, placement.variantIndex))
			{
				const auto* feature = rules.sideFeatureLookup(placement.sideFeatureId);
				if (IsSideFeatureUsableAtLevel(module, feature, placement.level)) bonus = std::max(bonus, feature->rules.minimumZoneLevelBonus);
			}
			return std::clamp(bonus, 0, kMaxZoneLevelBonus);
		}

		int FindZoneLevelBonus(const ResolvedModuleEffects& effects, std::uint32_t instanceId)
		{
			for (const ResolvedModuleEffects::ZoneBonus& zone : effects.zoneBonuses) if (zone.instanceId == instanceId) return zone.levelBonus;
			return 0;
		}

		int CountEmptyCells(const ModuleRulesContext& rules, const List<ModulePlacement>& placements)
		{
			const ShipSchematic& schematic = rules.schematic;
			std::vector<bool> covered(static_cast<std::size_t>(schematic.width * schematic.height), false);
			for (const ModulePlacement& placement : placements)
			{
				const ModuleDefinition* module = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
				if (!module) continue;
				for (const sf::Vector2i& cell : ModulePlacementRules::GetCoveredCells(*module, placement))
				{
					if (schematic.IsInBounds(cell.x, cell.y)) covered[static_cast<std::size_t>(cell.y * schematic.width + cell.x)] = true;
				}
			}
			int empty = 0;
			for (int y = 0; y < schematic.height; ++y)
			{
				for (int x = 0; x < schematic.width; ++x)
				{
					if (!schematic.GetZoneAt(x, y).IsValid() || schematic.IsLockedAt(x, y)) continue;
					if (!covered[static_cast<std::size_t>(y * schematic.width + x)]) ++empty;
				}
			}
			return empty;
		}

		ResolvedModuleEffects Resolve(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements)
		{
			return Resolve(rules, activePlacements, activePlacements);
		}

		ResolvedModuleEffects Resolve(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements, const List<ModulePlacement>& layoutPlacements)
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
				const int zoneLevel = GetZoneLevelBonus(rules, *module, placement);
				effects.zoneBonuses.push_back({ placement.instanceId, zoneLevel });

				const StatusRuleCounts mainBegin = CountStatusRules(effects);
					ResolvePackage(effects, variant, *module, placement, placement.level + zoneLevel, placement.level, placementZone, placementZoneEntry, false);
				if (module->legendary) ResolveLegendaryRules(effects.legendaryRules, *module->legendary, placement.instanceId, placement.level + zoneLevel);
					const StatusRuleCounts mainEnd = CountStatusRules(effects);
				// Rolled side stats are never scaled and apply to the owner.
				for (const sas::AttributeModifier& sideStat : placement.rolledSideStats)
				{
					effects.ownerModifiers.push_back(sideStat);
				}

				if (placement.sideFeatureId.IsValid() && rules.sideFeatureLookup && IsLargestCellCountVariant(*module, placement.variantIndex))
				{
					const ModuleSideFeatureDefinition* feature = rules.sideFeatureLookup(placement.sideFeatureId);
					if (IsSideFeatureUsableAtLevel(*module, feature, placement.level))
						{
							ResolvePackage(effects, feature->rules, *module, placement, placement.level, placement.level, placementZone, placementZoneEntry, true);
							MergeSideStatusModifiers(effects, feature->rules, mainBegin, mainEnd);
							for (auto recovery : feature->rules.cooldownRecoveryRules)
							{
								if (recovery.perEmptyAbilitySlot) recovery.bonus *= static_cast<float>(CountEmptyAbilitySlots(rules, layoutPlacements));
								effects.cooldownRecoveryRules.push_back(recovery);
							}
							if (feature->rules.deferredDamage && feature->rules.deferredDamage->fraction > effects.deferredDamage.fraction) effects.deferredDamage = *feature->rules.deferredDamage;
							if (feature->rules.primaryShareOfAbilityBonus > 0.f)
							{
								for (const auto& main : variant.damageRules)
								{
									if (main.requiredEmptyAbilitySlots <= 0) continue;
									ModuleDamageRule primary = main;
									primary.condition = ModuleDamageCondition::PrimaryShotOnly;
									primary.damageMultiplier = 1.f + (main.damageMultiplierByLevel.ValueAtLevel(main.damageMultiplier, placement.level + zoneLevel) - 1.f) * feature->rules.primaryShareOfAbilityBonus;
									primary.damageMultiplierByLevel = {};
									effects.damageRules.push_back({ placement.instanceId, NamespacedRuleIndex(0, true), primary, {} });
								}
							}
							if (feature->rules.primarySplitFire) effects.primaryMount.splitFire = true;
						}
				}

				const auto system = ModuleSystemForPlacement(rules, *module, placement);
				if (system.IsValid()) effects.enabledSystems.push_back(system);
				if (module->abilityModule && system.IsValid())
				{
					const auto slot = AbilitySlotForModuleSystem(system);
					effects.abilityModules.push_back({ slot, *module->abilityModule });
				}
			}
			// Layout-dependent damage rules read the installed layout once: empty-cell scaling and the ability-slot gate.
				// The layout includes rebooting placements, which still occupy their cells and ability slots.
			const bool layoutDependent = std::any_of(effects.damageRules.begin(), effects.damageRules.end(),
				[](const ResolvedModuleDamageRule& damage) { return damage.rule.perEmptyCellMultiplier > 0.f || damage.rule.requiredEmptyAbilitySlots > 0; });
			if (layoutDependent)
			{
				const int emptyCells = CountEmptyCells(rules, layoutPlacements);
				const int emptyAbilitySlots = CountEmptyAbilitySlots(rules, layoutPlacements);
				for (ResolvedModuleDamageRule& damage : effects.damageRules)
				{
					if (damage.rule.perEmptyCellMultiplier > 0.f) damage.rule.damageMultiplier = 1.f + damage.rule.perEmptyCellMultiplier * static_cast<float>(emptyCells);
				}
				effects.damageRules.erase(std::remove_if(effects.damageRules.begin(), effects.damageRules.end(),
					[emptyAbilitySlots](const ResolvedModuleDamageRule& damage) { return damage.rule.requiredEmptyAbilitySlots > 0 && damage.rule.requiredEmptyAbilitySlots > emptyAbilitySlots; }),
					effects.damageRules.end());
			}
			for (auto& damage : effects.damageRules)
				if (damage.rule.condition == ModuleDamageCondition::AdjacentAbilityDirectHit)
					for (const auto& placement : activePlacements)
						if (placement.instanceId == damage.instanceId)
							damage.adjacentAbilitySystems = ModulePlacementRules::GetAdjacentAbilitySlotSystems(rules, activePlacements, placement);
			// Kill-derived effects read each placement's kill counters and its active neighbours, as AdjacentAbilityDirectHit does.
			const auto findActive = [&activePlacements](std::uint32_t instanceId) -> const ModulePlacement*
			{
				for (const ModulePlacement& placement : activePlacements) if (placement.instanceId == instanceId) return &placement;
				return nullptr;
			};
			for (ResolvedModuleKillRule& kill : effects.killRules)
			{
				const ModulePlacement* placement = findActive(kill.instanceId);
				if (!placement) continue;
				kill.adjacentAbilitySystems = ModulePlacementRules::GetAdjacentAbilitySlotSystems(rules, activePlacements, *placement, static_cast<std::size_t>(kill.adjacentAbilityLimit));
				if (!(placement->killDamageBonus > 0.0)) continue;
				for (const GameplayTag& system : kill.adjacentAbilitySystems)
				{
					const sas::AbilitySlot slot = AbilitySlotForModuleSystem(system);
					if (slot == sas::AbilitySlot::None) continue;
					const std::string id = "ModuleKill." + std::to_string(kill.instanceId) + "." + std::to_string(kill.ruleIndex) + "." + std::to_string(static_cast<int>(slot));
					effects.killSources.push_back({ id, slot, GameplayTag{}, static_cast<float>(1.0 + placement->killDamageBonus) });
				}
			}
			for (ResolvedModuleKillCooldownRule& cooldown : effects.killCooldownRules)
			{
				const ModulePlacement* placement = findActive(cooldown.instanceId);
				if (!placement) continue;
				cooldown.adjacentAbilitySystems = ModulePlacementRules::GetAdjacentAbilitySlotSystems(rules, activePlacements, *placement, static_cast<std::size_t>(cooldown.rule.adjacentAbilityLimit));
				const float multiplier = std::max(0.f, 1.f - GetKillStackCooldownReduction(cooldown.rule, placement->killStacks));
				for (const GameplayTag& system : cooldown.adjacentAbilitySystems)
				{
					const sas::AbilitySlot slot = AbilitySlotForModuleSystem(system);
					if (slot == sas::AbilitySlot::None) continue;
					effects.cooldownRules.push_back({ cooldown.instanceId, cooldown.ruleIndex, GameplayTag{}, ModuleCooldownScope::FamilyOfZone, GameplayTag{}, multiplier, slot, cooldown.rule.minimumSeconds });
				}
			}
			for (const ResolvedModuleKillDamageRule& wall : effects.killDamageRules)
			{
				const ModulePlacement* placement = findActive(wall.instanceId);
				if (!placement || !(placement->killDamageBonus > 0.0)) continue;
				const std::string id = "ModuleKillWall." + std::to_string(wall.instanceId) + "." + std::to_string(wall.ruleIndex);
				effects.killSources.push_back({ id, sas::AbilitySlot::None, wall.rule.family, static_cast<float>(1.0 + wall.rule.fraction * placement->killDamageBonus) });
			}
			// Charge rules read the active neighbours once the layout is known; a rebooting placement neither grants nor receives.
			for (ResolvedModuleChargeRule& charge : effects.chargeRules)
			{
				const ModulePlacement* placement = findActive(charge.instanceId);
				if (!placement) continue;
				charge.slots = GeometricAbilitySlots(rules, activePlacements, *placement, static_cast<std::size_t>(charge.neighbourCount));
			}
			ApplyEpicFamilyStatRules(effects, rules, activePlacements);
			for (const auto& placement : activePlacements)
			{
				const auto* module = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
				const auto* feature = rules.sideFeatureLookup ? rules.sideFeatureLookup(placement.sideFeatureId) : nullptr;
				if (!module || !IsLargestCellCountVariant(*module, placement.variantIndex) || !IsSideFeatureUsableAtLevel(*module, feature, placement.level) || !feature->rules.momentumExtension) continue;
				for (auto& buff : effects.triggerBuffs)
				{
					if (buff.instanceId != placement.instanceId) continue;
					buff.rule.extendSeconds = feature->rules.momentumExtension->seconds;
					buff.rule.maximumDurationSeconds = feature->rules.momentumExtension->maximumSeconds;
				}
			}
			for (ResolvedModuleLegendaryUnderdogCoreRule& underdog : effects.legendaryRules.underdogCores) ComputeUnderdogWeights(underdog, rules, activePlacements);
			return effects;
		}
	}
}

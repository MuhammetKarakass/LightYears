#include "gameplay/module/rules/legendary/LegendaryRules.h"

#include <type_traits>

namespace vega
{
	void ResolveLegendaryRules(ResolvedModuleLegendaryRules& effects, const ModuleLegendaryRule& rule, std::uint32_t instanceId, int effectiveLevel)
	{
		std::visit([&](const auto& authored)
		{
			using Rule = std::decay_t<decltype(authored)>;
			if constexpr (std::is_same_v<Rule, ModuleLegendarySecondHardpointRule>)
			{
				ResolvedModuleLegendarySecondHardpointRule resolved;
				resolved.instanceId = instanceId;
				resolved.markDamageBonus = authored.markDamageBonusByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.markSeconds = authored.markSeconds;
				resolved.switchSeconds = authored.switchSeconds;
				effects.secondHardpoints.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryMeltdownBarrelRule>)
			{
				ResolvedModuleLegendaryMeltdownBarrelRule resolved;
				resolved.instanceId = instanceId;
				resolved.novaBudgetFraction = authored.novaBudgetFractionByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.focusSeconds = authored.focusSeconds;
				resolved.novaRadius = authored.novaRadius;
				resolved.igniteStacks = authored.igniteStacks;
				resolved.heatCapacity = authored.heatCapacity;
				effects.meltdownBarrels.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryPrimedArsenalRule>)
			{
				ResolvedModuleLegendaryPrimedArsenalRule resolved;
				resolved.instanceId = instanceId;
				resolved.powerPerReadySecond = authored.powerPerReadySecondByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.readyWindowSeconds = authored.readyWindowSeconds;
				effects.primedArsenals.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryReactionCoreRule>)
			{
				ResolvedModuleLegendaryReactionCoreRule resolved;
				resolved.instanceId = instanceId;
				resolved.steamSeconds = authored.steamSecondsByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.steamDeviationDegrees = authored.steamDeviationDegrees;
				resolved.magnetSeconds = authored.magnetSeconds;
				resolved.magnetSpeed = authored.magnetSpeedByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.magnetRadius = authored.magnetRadius;
				resolved.plasmaDamage = authored.plasmaDamageByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.plasmaEnergyPowerCoefficient = authored.plasmaEnergyPowerCoefficientByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.plasmaRadius = authored.plasmaRadius;
				resolved.fractureSeconds = authored.fractureSecondsByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.bossArmorReductionFraction = authored.bossArmorReductionFraction;
				resolved.stunSeconds = authored.stunSecondsByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.shrapnelCount = authored.shrapnelCount;
				resolved.shrapnelDamage = authored.shrapnelDamageByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.shrapnelAttackPowerCoefficient = authored.shrapnelAttackPowerCoefficientByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.shrapnelEnergyPowerCoefficient = authored.shrapnelEnergyPowerCoefficientByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.shrapnelMaxHitsPerTarget = authored.shrapnelMaxHitsPerTarget;
				resolved.shrapnelRadius = authored.shrapnelRadius;
				resolved.targetCooldownSeconds = authored.targetCooldownSeconds;
				effects.reactionCores.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryNeuralHijackerRule>)
			{
				ResolvedModuleLegendaryNeuralHijackerRule resolved;
				resolved.instanceId = instanceId;
				resolved.allySeconds = authored.allySecondsByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.allyHealthFraction = authored.allyHealthFraction;
				resolved.allyDamageMultiplier = authored.allyDamageMultiplier;
				effects.neuralHijackers.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryHardlightHullRule>)
			{
				ResolvedModuleLegendaryHardlightHullRule resolved;
				resolved.instanceId = instanceId;
				resolved.healthToShieldFraction = authored.healthToShieldFractionByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.maxHealth = authored.maxHealth;
				effects.hardlightHulls.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryBulwarkPulseRule>)
			{
				ResolvedModuleLegendaryBulwarkPulseRule resolved;
				resolved.instanceId = instanceId;
				resolved.baseDamage = authored.baseDamageByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.healthCoefficient = authored.healthCoefficientByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.healthThreshold = authored.healthThreshold;
				resolved.radius = authored.radius;
				resolved.cooldownSeconds = authored.cooldownSeconds;
				effects.bulwarkPulses.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryParasiteHullRule>)
			{
				ResolvedModuleLegendaryParasiteHullRule resolved;
				resolved.instanceId = instanceId;
				resolved.borrowedHealthBonus = authored.borrowedHealthBonusByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.borrowedDamageBonus = authored.borrowedDamageBonusByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.maximumSeconds = authored.maximumSeconds;
				resolved.repairMaxHealthFractionPerSecond = authored.repairMaxHealthFractionPerSecond;
				resolved.minimumReturnHealthFraction = authored.minimumReturnHealthFraction;
				resolved.cooldownSeconds = authored.cooldownSeconds;
				resolved.possessionRange = authored.possessionRange;
				effects.parasiteHulls.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryBlinkDriveRule>)
			{
				ResolvedModuleLegendaryBlinkDriveRule resolved;
				resolved.instanceId = instanceId;
				resolved.rechargeSeconds = authored.rechargeSecondsByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.movementSpeedBonus = authored.movementSpeedBonusByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.charges = authored.charges;
				resolved.useCooldownSeconds = authored.useCooldownSeconds;
				resolved.boostSeconds = authored.boostSeconds;
				resolved.distance = authored.distance;
				effects.blinkDrives.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryIonCometRule>)
			{
				ResolvedModuleLegendaryIonCometRule resolved;
				resolved.instanceId = instanceId;
				resolved.tickDamage = authored.tickDamageByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.tickEnergyPowerCoefficient = authored.tickEnergyPowerCoefficientByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.burstDamage = authored.burstDamageByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.burstEnergyPowerCoefficient = authored.burstEnergyPowerCoefficientByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.minimumSpeed = authored.minimumSpeed;
				resolved.baseRadius = authored.baseRadius;
				resolved.radiusPerSpeed = authored.radiusPerSpeed;
				resolved.damageSpeedDivisor = authored.damageSpeedDivisor;
				resolved.diminishingSpeedThreshold = authored.diminishingSpeedThreshold;
				resolved.diminishingSpeedInterval = authored.diminishingSpeedInterval;
				resolved.diminishingSpeedFactor = authored.diminishingSpeedFactor;
				resolved.maximumCharge = authored.maximumCharge;
				resolved.chargePerSecond = authored.chargePerSecond;
				resolved.chargeSpeedSpan = authored.chargeSpeedSpan;
				resolved.brakeWindowSeconds = authored.brakeWindowSeconds;
				resolved.brakeSpeedLossFraction = authored.brakeSpeedLossFraction;
				resolved.tickElectricStacks = authored.tickElectricStacks;
				resolved.burstElectricStacks = authored.burstElectricStacks;
				effects.ionComets.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryTwinRackRule>)
			{
				ResolvedModuleLegendaryTwinRackRule resolved;
				resolved.instanceId = instanceId;
				resolved.pairedRefundFraction = authored.pairedRefundFractionByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.cooldownMultiplier = authored.cooldownMultiplier;
				resolved.abilitiesPerSlot = authored.abilitiesPerSlot;
				effects.twinRacks.push_back(resolved);
			}
			else if constexpr (std::is_same_v<Rule, ModuleLegendaryUnderdogCoreRule>)
			{
				ResolvedModuleLegendaryUnderdogCoreRule resolved;
				resolved.instanceId = instanceId;
				resolved.offensePowerPerWeight = authored.offensePowerPerWeightByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.defenseHealthPerWeight = authored.defenseHealthPerWeightByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.primaryAttackSpeedPercentPerWeight = authored.primaryAttackSpeedPercentPerWeightByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.movementRatingPerWeight = authored.movementRatingPerWeightByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.utilityHastePercentPerWeight = authored.utilityHastePercentPerWeightByLevel.ValueAtLevel(0.f, effectiveLevel);
				resolved.commonWeight = authored.commonWeight;
				resolved.uncommonWeight = authored.uncommonWeight;
				resolved.rareWeight = authored.rareWeight;
				resolved.epicWeight = authored.epicWeight;
				resolved.legendaryWeight = authored.legendaryWeight;
				effects.underdogCores.push_back(resolved);
			}
		}, rule);
	}
}

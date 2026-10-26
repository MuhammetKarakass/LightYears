#include "ModuleLoaderParsing.h"

#include <set>

namespace vega::content::module_loader_detail
{
	namespace
	{
		void ValidateFields(const Json& object, const std::string& context, const std::set<std::string>& allowed)
		{
			for (const auto& field : object.items())
				if (!allowed.count(field.key())) throw std::runtime_error(context + " unknown Legendary field: " + field.key());
		}

		ModuleLevelTable<float> LevelNumber(const Json& object, const char* key, const std::string& context, int levelCount)
		{
			ModuleLevelTable<float> table;
			const float base = ParseLevelFloat(object, key, context + "." + key, levelCount, table);
			if (table.values.empty()) table.values.push_back(base);
			for (int level = 1; level <= levelCount + kMaxZoneLevelBonus; ++level)
			{
				const float value = table.ValueAtLevel(base, level);
				if (!std::isfinite(value) || value <= 0.f) throw std::runtime_error(context + " Legendary level value must be finite and positive: " + key);
			}
			return table;
		}

		float FixedNumber(const Json& object, const char* key, const std::string& context)
		{
			const float value = ParseFiniteFloat(object.at(key), context + "." + key);
			if (value <= 0.f) throw std::runtime_error(context + " Legendary value must be positive: " + key);
			return value;
		}
	}

	ModuleLegendaryRule ParseLegendaryRule(const Json& object, const GameplayTag& family, const std::string& context, int levelCount)
	{
		if (!object.is_object()) throw std::runtime_error(context + " legendary must be an object");
		const std::string kind = object.at("kind").get<std::string>();
		if (kind == "SecondHardpoint")
		{
			if (family != GameplayTags::Ability::Primary) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "markDamageBonus", "markDamageBonusGrowth", "markSeconds", "switchSeconds" });
			ModuleLegendarySecondHardpointRule rule;
			rule.markDamageBonusByLevel = LevelNumber(object, "markDamageBonus", context, levelCount);
			rule.markSeconds = FixedNumber(object, "markSeconds", context);
			rule.switchSeconds = FixedNumber(object, "switchSeconds", context);
			return rule;
		}
		if (kind == "MeltdownBarrel")
		{
			if (family != GameplayTags::Ability::Primary) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "novaBudgetFraction", "novaBudgetFractionGrowth", "focusSeconds", "novaRadius", "igniteStacks", "heatCapacity" });
			ModuleLegendaryMeltdownBarrelRule rule;
			rule.novaBudgetFractionByLevel = LevelNumber(object, "novaBudgetFraction", context, levelCount);
			rule.focusSeconds = FixedNumber(object, "focusSeconds", context);
			rule.novaRadius = FixedNumber(object, "novaRadius", context);
			rule.igniteStacks = ParsePositiveInt(object.at("igniteStacks"), context + ".igniteStacks");
			rule.heatCapacity = FixedNumber(object, "heatCapacity", context);
			return rule;
		}
		if (kind == "PrimedArsenal")
		{
			if (family != GameplayTags::Ability::Offense) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "powerPerReadySecond", "powerPerReadySecondGrowth", "readyWindowSeconds" });
			ModuleLegendaryPrimedArsenalRule rule;
			rule.powerPerReadySecondByLevel = LevelNumber(object, "powerPerReadySecond", context, levelCount);
			rule.readyWindowSeconds = FixedNumber(object, "readyWindowSeconds", context);
			return rule;
		}
		if (kind == "ReactionCore")
		{
			if (family != GameplayTags::Ability::Offense) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "steamSeconds", "steamSecondsGrowth", "steamDeviationDegrees", "magnetSeconds", "magnetSpeed", "magnetSpeedGrowth", "magnetRadius", "plasmaDamage", "plasmaDamageGrowth", "plasmaEnergyPowerCoefficient", "plasmaEnergyPowerCoefficientGrowth", "plasmaRadius", "fractureSeconds", "fractureSecondsGrowth", "bossArmorReductionFraction", "stunSeconds", "stunSecondsGrowth", "shrapnelCount", "shrapnelDamage", "shrapnelDamageGrowth", "shrapnelAttackPowerCoefficient", "shrapnelAttackPowerCoefficientGrowth", "shrapnelEnergyPowerCoefficient", "shrapnelEnergyPowerCoefficientGrowth", "shrapnelMaxHitsPerTarget", "shrapnelRadius", "targetCooldownSeconds" });
			ModuleLegendaryReactionCoreRule rule;
			rule.steamSecondsByLevel = LevelNumber(object, "steamSeconds", context, levelCount);
			rule.steamDeviationDegrees = FixedNumber(object, "steamDeviationDegrees", context);
			rule.magnetSeconds = FixedNumber(object, "magnetSeconds", context);
			rule.magnetSpeedByLevel = LevelNumber(object, "magnetSpeed", context, levelCount);
			rule.magnetRadius = FixedNumber(object, "magnetRadius", context);
			rule.plasmaDamageByLevel = LevelNumber(object, "plasmaDamage", context, levelCount);
			rule.plasmaEnergyPowerCoefficientByLevel = LevelNumber(object, "plasmaEnergyPowerCoefficient", context, levelCount);
			rule.plasmaRadius = FixedNumber(object, "plasmaRadius", context);
			rule.fractureSecondsByLevel = LevelNumber(object, "fractureSeconds", context, levelCount);
			rule.bossArmorReductionFraction = FixedNumber(object, "bossArmorReductionFraction", context);
			rule.stunSecondsByLevel = LevelNumber(object, "stunSeconds", context, levelCount);
			rule.shrapnelCount = ParsePositiveInt(object.at("shrapnelCount"), context + ".shrapnelCount");
			rule.shrapnelDamageByLevel = LevelNumber(object, "shrapnelDamage", context, levelCount);
			rule.shrapnelAttackPowerCoefficientByLevel = LevelNumber(object, "shrapnelAttackPowerCoefficient", context, levelCount);
			rule.shrapnelEnergyPowerCoefficientByLevel = LevelNumber(object, "shrapnelEnergyPowerCoefficient", context, levelCount);
			rule.shrapnelMaxHitsPerTarget = ParsePositiveInt(object.at("shrapnelMaxHitsPerTarget"), context + ".shrapnelMaxHitsPerTarget");
			rule.shrapnelRadius = FixedNumber(object, "shrapnelRadius", context);
			rule.targetCooldownSeconds = FixedNumber(object, "targetCooldownSeconds", context);
			return rule;
		}
		if (kind == "NeuralHijacker")
		{
			if (family != GameplayTags::Ability::Offense) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "allySeconds", "allySecondsGrowth", "allyHealthFraction", "allyDamageMultiplier" });
			ModuleLegendaryNeuralHijackerRule rule;
			rule.allySecondsByLevel = LevelNumber(object, "allySeconds", context, levelCount);
			rule.allyHealthFraction = FixedNumber(object, "allyHealthFraction", context);
			rule.allyDamageMultiplier = FixedNumber(object, "allyDamageMultiplier", context);
			return rule;
		}
		if (kind == "HardlightHull")
		{
			if (family != GameplayTags::Ability::Defense) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "healthToShieldFraction", "healthToShieldFractionGrowth", "maxHealth" });
			ModuleLegendaryHardlightHullRule rule;
			rule.healthToShieldFractionByLevel = LevelNumber(object, "healthToShieldFraction", context, levelCount);
			rule.maxHealth = FixedNumber(object, "maxHealth", context);
			return rule;
		}
		if (kind == "BulwarkPulse")
		{
			if (family != GameplayTags::Ability::Defense) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "baseDamage", "baseDamageGrowth", "healthCoefficient", "healthCoefficientGrowth", "healthThreshold", "radius", "cooldownSeconds" });
			ModuleLegendaryBulwarkPulseRule rule;
			rule.baseDamageByLevel = LevelNumber(object, "baseDamage", context, levelCount);
			rule.healthCoefficientByLevel = LevelNumber(object, "healthCoefficient", context, levelCount);
			rule.healthThreshold = FixedNumber(object, "healthThreshold", context);
			rule.radius = FixedNumber(object, "radius", context);
			rule.cooldownSeconds = FixedNumber(object, "cooldownSeconds", context);
			return rule;
		}
		if (kind == "ParasiteHull")
		{
			if (family != GameplayTags::Ability::Defense) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "borrowedHealthBonus", "borrowedHealthBonusGrowth", "borrowedDamageBonus", "borrowedDamageBonusGrowth", "maximumSeconds", "repairMaxHealthFractionPerSecond", "minimumReturnHealthFraction", "cooldownSeconds", "possessionRange" });
			ModuleLegendaryParasiteHullRule rule;
			rule.borrowedHealthBonusByLevel = LevelNumber(object, "borrowedHealthBonus", context, levelCount);
			rule.borrowedDamageBonusByLevel = LevelNumber(object, "borrowedDamageBonus", context, levelCount);
			rule.maximumSeconds = FixedNumber(object, "maximumSeconds", context);
			rule.repairMaxHealthFractionPerSecond = FixedNumber(object, "repairMaxHealthFractionPerSecond", context);
			rule.minimumReturnHealthFraction = FixedNumber(object, "minimumReturnHealthFraction", context);
			rule.cooldownSeconds = FixedNumber(object, "cooldownSeconds", context);
			rule.possessionRange = FixedNumber(object, "possessionRange", context);
			return rule;
		}
		if (kind == "BlinkDrive")
		{
			if (family != GameplayTags::Ability::Movement) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "rechargeSeconds", "rechargeSecondsGrowth", "movementSpeedBonus", "movementSpeedBonusGrowth", "charges", "useCooldownSeconds", "boostSeconds", "distance" });
			ModuleLegendaryBlinkDriveRule rule;
			rule.rechargeSecondsByLevel = LevelNumber(object, "rechargeSeconds", context, levelCount);
			rule.movementSpeedBonusByLevel = LevelNumber(object, "movementSpeedBonus", context, levelCount);
			rule.charges = ParsePositiveInt(object.at("charges"), context + ".charges");
			rule.useCooldownSeconds = FixedNumber(object, "useCooldownSeconds", context);
			rule.boostSeconds = FixedNumber(object, "boostSeconds", context);
			rule.distance = FixedNumber(object, "distance", context);
			return rule;
		}
		if (kind == "IonComet")
		{
			if (family != GameplayTags::Ability::Movement) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "tickDamage", "tickDamageGrowth", "tickEnergyPowerCoefficient", "tickEnergyPowerCoefficientGrowth", "burstDamage", "burstDamageGrowth", "burstEnergyPowerCoefficient", "burstEnergyPowerCoefficientGrowth", "minimumSpeed", "baseRadius", "radiusPerSpeed", "damageSpeedDivisor", "diminishingSpeedThreshold", "diminishingSpeedInterval", "diminishingSpeedFactor", "maximumCharge", "chargePerSecond", "chargeSpeedSpan", "brakeWindowSeconds", "brakeSpeedLossFraction", "tickElectricStacks", "burstElectricStacks" });
			ModuleLegendaryIonCometRule rule;
			rule.tickDamageByLevel = LevelNumber(object, "tickDamage", context, levelCount);
			rule.tickEnergyPowerCoefficientByLevel = LevelNumber(object, "tickEnergyPowerCoefficient", context, levelCount);
			rule.burstDamageByLevel = LevelNumber(object, "burstDamage", context, levelCount);
			rule.burstEnergyPowerCoefficientByLevel = LevelNumber(object, "burstEnergyPowerCoefficient", context, levelCount);
			rule.minimumSpeed = FixedNumber(object, "minimumSpeed", context);
			rule.baseRadius = FixedNumber(object, "baseRadius", context);
			rule.radiusPerSpeed = FixedNumber(object, "radiusPerSpeed", context);
			rule.damageSpeedDivisor = FixedNumber(object, "damageSpeedDivisor", context);
			rule.diminishingSpeedThreshold = FixedNumber(object, "diminishingSpeedThreshold", context);
			rule.diminishingSpeedInterval = FixedNumber(object, "diminishingSpeedInterval", context);
			rule.diminishingSpeedFactor = FixedNumber(object, "diminishingSpeedFactor", context);
			rule.maximumCharge = FixedNumber(object, "maximumCharge", context);
			rule.chargePerSecond = FixedNumber(object, "chargePerSecond", context);
			rule.chargeSpeedSpan = FixedNumber(object, "chargeSpeedSpan", context);
			rule.brakeWindowSeconds = FixedNumber(object, "brakeWindowSeconds", context);
			rule.brakeSpeedLossFraction = FixedNumber(object, "brakeSpeedLossFraction", context);
			rule.tickElectricStacks = ParsePositiveInt(object.at("tickElectricStacks"), context + ".tickElectricStacks");
			rule.burstElectricStacks = ParsePositiveInt(object.at("burstElectricStacks"), context + ".burstElectricStacks");
			if (rule.diminishingSpeedFactor >= 1.f) throw std::runtime_error(context + ".diminishingSpeedFactor must be less than one");
			if (rule.brakeSpeedLossFraction > 1.f) throw std::runtime_error(context + ".brakeSpeedLossFraction must not exceed one");
			return rule;
		}
		if (kind == "TwinRack")
		{
			if (family != GameplayTags::Ability::Utility) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "pairedRefundFraction", "pairedRefundFractionGrowth", "cooldownMultiplier", "abilitiesPerSlot" });
			ModuleLegendaryTwinRackRule rule;
			rule.pairedRefundFractionByLevel = LevelNumber(object, "pairedRefundFraction", context, levelCount);
			rule.cooldownMultiplier = FixedNumber(object, "cooldownMultiplier", context);
			rule.abilitiesPerSlot = ParsePositiveInt(object.at("abilitiesPerSlot"), context + ".abilitiesPerSlot");
			return rule;
		}
		if (kind == "UnderdogCore")
		{
			if (family != GameplayTags::Ability::Utility) throw std::runtime_error(context + " Legendary kind does not match module family");
			ValidateFields(object, context, { "kind", "offensePowerPerWeight", "offensePowerPerWeightGrowth", "defenseHealthPerWeight", "defenseHealthPerWeightGrowth", "primaryAttackSpeedPercentPerWeight", "primaryAttackSpeedPercentPerWeightGrowth", "movementRatingPerWeight", "movementRatingPerWeightGrowth", "utilityHastePercentPerWeight", "utilityHastePercentPerWeightGrowth", "commonWeight", "uncommonWeight", "rareWeight", "epicWeight", "legendaryWeight" });
			ModuleLegendaryUnderdogCoreRule rule;
			rule.offensePowerPerWeightByLevel = LevelNumber(object, "offensePowerPerWeight", context, levelCount);
			rule.defenseHealthPerWeightByLevel = LevelNumber(object, "defenseHealthPerWeight", context, levelCount);
			rule.primaryAttackSpeedPercentPerWeightByLevel = LevelNumber(object, "primaryAttackSpeedPercentPerWeight", context, levelCount);
			rule.movementRatingPerWeightByLevel = LevelNumber(object, "movementRatingPerWeight", context, levelCount);
			rule.utilityHastePercentPerWeightByLevel = LevelNumber(object, "utilityHastePercentPerWeight", context, levelCount);
			rule.commonWeight = ParsePositiveInt(object.at("commonWeight"), context + ".commonWeight");
			rule.uncommonWeight = ParsePositiveInt(object.at("uncommonWeight"), context + ".uncommonWeight");
			rule.rareWeight = ParsePositiveInt(object.at("rareWeight"), context + ".rareWeight");
			rule.epicWeight = ParsePositiveInt(object.at("epicWeight"), context + ".epicWeight");
			rule.legendaryWeight = ParsePositiveInt(object.at("legendaryWeight"), context + ".legendaryWeight");
			return rule;
		}
		throw std::runtime_error(context + " unknown Legendary kind: " + kind);
	}
}

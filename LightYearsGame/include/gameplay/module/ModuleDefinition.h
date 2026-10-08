#pragma once

#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"
#include "framework/Core.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/module/ModuleRulePredicates.h"

#include <SFML/System/Vector2.hpp>

#include <string>
#include <utility>
#include <cstddef>

namespace ly
{
	enum class ModuleRarity
	{
		Common,
		Uncommon,
		Rare,
		Epic,
		Legendary
	};

	template<typename T>
	struct ModuleLevelTable
	{
		List<T> values;
		T ValueAtLevel(T fallback, int level) const { return level > 0 && static_cast<std::size_t>(level) <= values.size() ? values[static_cast<std::size_t>(level - 1)] : fallback; }
	};

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
		ModuleLevelTable<float> magnitudeByLevel;
	};

	// Resolved later into a ScopedAbilityRule for the module's family.
	struct ModuleAbilityModifier
	{
		sas::AttributeModifier modifier;
		bool affectedByZoneBonus = true;
		ModuleLevelTable<float> magnitudeByLevel;
	};

	struct ModuleTriggerBuff
	{
		ModuleTriggerKind trigger = ModuleTriggerKind::None;
		List<GameplayTag> requiredEventTags;
		float durationSeconds = 0.f;
		List<ModuleAttributeModifier> modifiers;
		float movementSpeedMultiplier = 1.f;
		ModuleLevelTable<float> durationSecondsByLevel;
		ModuleLevelTable<float> movementSpeedMultiplierByLevel;
	};

	struct ModuleDamageRule
	{
		bool incoming = false;
		ModuleDamageCondition condition = ModuleDamageCondition::PrimaryShotOnly;
		float conditionValue = 0.f;
		// FullMagazineShot applies to this many successful shots at the start of each magazine.
		int openingShotCount = 1;
		float damageMultiplier = 1.f;
		float bonusDamageFraction = 0.f;
		GameplayTag bonusDamageType;
		DamageCriticalPolicy bonusCriticalPolicy = DamageCriticalPolicy::Disabled;
		ModuleTriggerKind activeWhileTrigger = ModuleTriggerKind::None;
		// Same matching as ModuleTriggerBuff::requiredEventTags.
		List<GameplayTag> activeWhileEventTags;
		float activeDurationSeconds = 0.f;
		ModuleLevelTable<float> conditionValueByLevel;
		ModuleLevelTable<int> openingShotCountByLevel;
		ModuleLevelTable<float> damageMultiplierByLevel;
		ModuleLevelTable<float> bonusDamageFractionByLevel;
		ModuleLevelTable<float> activeDurationSecondsByLevel;
		float maxHealthDamageFraction = 0.f;
		ModuleLevelTable<float> maxHealthDamageFractionByLevel;
	};

	struct ModuleZoneEffect
	{
		GameplayTag zone;
		ModuleCooldownScope scope = ModuleCooldownScope::FamilyOfZone;
		float multiplier = 1.f;
		ModuleLevelTable<float> multiplierByLevel;
	};

	enum class ModuleEventActionKind { RestoreShield, ShieldOverflowToHealth, RestoreHealth, RestoreAfterburner, ReduceAllCooldowns, ReduceLongestCooldown, RefundUsedCooldown, ApplyNearbyCryo, AwardGold };
	struct ModuleEventAction
	{
		ModuleTriggerKind trigger = ModuleTriggerKind::KillConfirmed;
		ModuleEventActionKind kind = ModuleEventActionKind::RestoreShield;
		float amount = 0.f;
		float overflowHealthFraction = 0.f;
		float chancePerLuck = 0.f;
		int everyCount = 1;
		ModuleLevelTable<float> amountByLevel;
		ModuleLevelTable<float> chancePerLuckByLevel;
		float radius = 0.f;
		float cooldownSeconds = 0.f;
		int targetCount = 1;
		ModuleLevelTable<int> targetCountByLevel;
	};
	enum class ModuleOwnerAttributeSource { HealthThreshold, MovementSpeed, HigherPower, Luck };
	struct ModuleOwnerAttributeRule
	{
		ModuleOwnerAttributeSource source = ModuleOwnerAttributeSource::HealthThreshold;
		sas::AttributeId attributeId;
		float healthAtMost = 1.f;
		float amount = 0.f;
		float fadeSeconds = 0.f;
		ModuleLevelTable<float> amountByLevel;
		ModuleAttributeTarget target = ModuleAttributeTarget::Owner;
		sas::AttributeModifierOperation operation = sas::AttributeModifierOperation::Add;
	};
	struct ModuleMovementFadeRule { float fadeSeconds = 0.f; };
	struct ModuleElectricStatusRule
	{
		List<float> damageTakenByStack;
		List<ModuleLevelTable<float>> damageTakenByStackByLevel;
	};
	struct ModuleCryoFreezeRule
	{
		int threshold = 0;
		float durationSeconds = 0.f;
		float cooldownSeconds = 0.f;
		bool normalTargetsOnly = true;
		ModuleLevelTable<float> durationSecondsByLevel;
		ModuleLevelTable<float> cooldownSecondsByLevel;
	};
	struct ModuleStatusDurationRule
	{
		float bonusSeconds = 0.f;
		ModuleLevelTable<float> bonusSecondsByLevel;
	};
	struct ModuleFreezeSpreadRule
	{
		float radius = 0.f;
		int stacks = 1;
	};
	// Value snapshots owned by one ship's module applier; no global registry.
	struct ModuleStatusRules
	{
		List<ModuleElectricStatusRule> electricOverrides;
		List<ModuleCryoFreezeRule> cryoFreezes;
		List<ModuleStatusDurationRule> thermalCryoDurationBonuses;
		List<ModuleFreezeSpreadRule> freezeSpreads;
		List<ModuleStatusDurationRule> durationBonuses;
		bool Empty() const { return electricOverrides.empty() && cryoFreezes.empty() && thermalCryoDurationBonuses.empty() && freezeSpreads.empty() && durationBonuses.empty(); }
	};

	struct ModuleRicochetRule
	{
		float damageFraction = 0.f;
		float range = 0.f;
		float secondaryFraction = 0.f;
		bool transferStatus = false;
		ModuleLevelTable<float> damageFractionByLevel;
	};
	struct ModuleGuidedSalvoRule
	{
		float damageFraction = 0.f;
		int projectileCount = 0;
		float speed = 0.f;
		float lifetime = 0.f;
		float targetRange = 0.f;
		int maxActive = 0;
		ModuleLevelTable<float> damageFractionByLevel;
	};
	struct ModuleDelayedExplosionRule
	{
		float damageFraction = 0.f;
		float delaySeconds = 0.f;
		float radius = 0.f;
		int maxActive = 0;
		ModuleLevelTable<float> damageFractionByLevel;
	};
	struct ModuleTrailMineRule
	{
		float damage = 0.f;
		float spacing = 0.f;
		float windowSeconds = 0.f;
		float fuseSeconds = 0.f;
		float radius = 0.f;
		int maxActive = 0;
		int additionalLimit = 0;
		int cryoStacks = 0;
		ModuleLevelTable<float> damageByLevel;
		ModuleLevelTable<int> maxActiveByLevel;
	};
	struct ModuleKillExplosionRule
	{
		float maxHealthFraction = 0.f;
		float radius = 0.f;
		int maxActive = 0;
		List<GameplayTag> requiredTargetStatuses;
		ModuleLevelTable<float> maxHealthFractionByLevel;
	};
	struct ModuleGeneratedRules
	{
		List<ModuleRicochetRule> ricochets;
		List<ModuleGuidedSalvoRule> guidedSalvos;
		List<ModuleDelayedExplosionRule> delayedExplosions;
		List<ModuleTrailMineRule> trailMines;
		List<ModuleKillExplosionRule> killExplosions;
		bool Empty() const { return ricochets.empty() && guidedSalvos.empty() && delayedExplosions.empty() && trailMines.empty() && killExplosions.empty(); }
	};
	struct ModuleRulePackage
	{
		List<ModuleAttributeModifier> attributeModifiers;
		List<ModuleAbilityModifier> abilityModifiers;
		List<ModuleTriggerBuff> triggerBuffs;
		List<ModuleDamageRule> damageRules;
		List<ModuleZoneEffect> zoneEffects;
		List<ModuleEventAction> eventActions;
		List<ModuleOwnerAttributeRule> ownerAttributeRules;
		struct ShieldReflection
		{
			float fraction = 0.f;
			float durationSeconds = 0.f;
			ModuleLevelTable<float> fractionByLevel;
		};
		List<ShieldReflection> shieldReflections;
		ModuleStatusRules statusRules;
		ModuleGeneratedRules generatedRules;
		List<ModuleMovementFadeRule> movementFadeRules;
	};

	struct ModuleSizeVariant : ModuleRulePackage
	{
		ModuleShape shape;

		ModuleSizeVariant() = default;
		ModuleSizeVariant(ModuleShape shapeValue, List<ModuleAttributeModifier> attributeModifiersValue = {}, List<ModuleAbilityModifier> abilityModifiersValue = {}, List<ModuleTriggerBuff> triggerBuffsValue = {}, List<ModuleDamageRule> damageRulesValue = {}, List<ModuleZoneEffect> zoneEffectsValue = {})
			: ModuleRulePackage{ std::move(attributeModifiersValue), std::move(abilityModifiersValue),
				std::move(triggerBuffsValue), std::move(damageRulesValue), std::move(zoneEffectsValue) },
			  shape{ std::move(shapeValue) }
		{
		}
	};

	inline bool HasModuleRulePackageLevelCoverage(const ModuleRulePackage& rules, int levelCount)
	{
		if (levelCount <= 0) return false;
		const auto covers = [levelCount](const auto& table)
		{
			return table.values.empty() || table.values.size() >= static_cast<std::size_t>(levelCount);
		};
		for (const ModuleAttributeModifier& entry : rules.attributeModifiers) if (!covers(entry.magnitudeByLevel)) return false;
		for (const ModuleAbilityModifier& entry : rules.abilityModifiers) if (!covers(entry.magnitudeByLevel)) return false;
		for (const ModuleTriggerBuff& buff : rules.triggerBuffs)
		{
			if (!covers(buff.durationSecondsByLevel) || !covers(buff.movementSpeedMultiplierByLevel)) return false;
			for (const ModuleAttributeModifier& modifier : buff.modifiers) if (!covers(modifier.magnitudeByLevel)) return false;
		}
		for (const ModuleDamageRule& rule : rules.damageRules)
			if (!covers(rule.conditionValueByLevel) || !covers(rule.openingShotCountByLevel) ||
				!covers(rule.damageMultiplierByLevel) || !covers(rule.bonusDamageFractionByLevel) ||
				!covers(rule.activeDurationSecondsByLevel) || !covers(rule.maxHealthDamageFractionByLevel)) return false;
		for (const ModuleEventAction& action : rules.eventActions)
			if (!covers(action.amountByLevel) || !covers(action.chancePerLuckByLevel) || !covers(action.targetCountByLevel)) return false;
		for (const ModuleOwnerAttributeRule& rule : rules.ownerAttributeRules) if (!covers(rule.amountByLevel)) return false;
		for (const auto& rule : rules.shieldReflections) if (!covers(rule.fractionByLevel)) return false;
		for (const auto& rule : rules.statusRules.electricOverrides)
			for (const auto& table : rule.damageTakenByStackByLevel) if (!covers(table)) return false;
		for (const auto& rule : rules.statusRules.cryoFreezes)
			if (!covers(rule.durationSecondsByLevel) || !covers(rule.cooldownSecondsByLevel)) return false;
		for (const auto& rule : rules.statusRules.thermalCryoDurationBonuses) if (!covers(rule.bonusSecondsByLevel)) return false;
		for (const auto& rule : rules.statusRules.durationBonuses) if (!covers(rule.bonusSecondsByLevel)) return false;
		for (const ModuleZoneEffect& effect : rules.zoneEffects) if (!covers(effect.multiplierByLevel)) return false;
		for (const auto& rule : rules.generatedRules.ricochets) if (!covers(rule.damageFractionByLevel)) return false;
		for (const auto& rule : rules.generatedRules.guidedSalvos) if (!covers(rule.damageFractionByLevel)) return false;
		for (const auto& rule : rules.generatedRules.delayedExplosions) if (!covers(rule.damageFractionByLevel)) return false;
		for (const auto& rule : rules.generatedRules.trailMines) if (!covers(rule.damageByLevel) || !covers(rule.maxActiveByLevel)) return false;
		for (const auto& rule : rules.generatedRules.killExplosions) if (!covers(rule.maxHealthFractionByLevel)) return false;
		return true;
	}

	struct ModuleSharedPoolPermission
	{
		sas::ContentId poolId;
		List<sas::ContentId> allowedFeatureIds;
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
		ModuleRarity rarity = ModuleRarity::Common;
		int levelCount = 3;
		List<sas::ContentId> sideFeatures;
		List<ModuleSharedPoolPermission> sharedPools;
		List<int> sizeLadderCellCounts;
	};

	inline int GetLargestCellCountVariantIndex(const ModuleDefinition& module)
	{
		int largestIndex = -1;
		std::size_t largestCellCount = 0;
		for (std::size_t index = 0; index < module.sizeVariants.size(); ++index)
		{
			const std::size_t cellCount = module.sizeVariants[index].shape.cells.size();
			if (cellCount > largestCellCount)
			{
				largestIndex = static_cast<int>(index);
				largestCellCount = cellCount;
			}
		}
		return largestIndex;
	}

	inline bool AllowsPrivateSideFeature(const ModuleDefinition& module, const sas::ContentId& sideFeatureId)
	{
		for (const sas::ContentId& featureId : module.sideFeatures)
			if (featureId == sideFeatureId) return true;
		return false;
	}

	inline bool AllowsSharedSideFeature(const ModuleDefinition& module, const sas::ContentId& poolId, const sas::ContentId& sideFeatureId)
	{
		for (const ModuleSharedPoolPermission& permission : module.sharedPools)
		{
			if (permission.poolId != poolId) continue;
			for (const sas::ContentId& featureId : permission.allowedFeatureIds)
				if (featureId == sideFeatureId) return true;
		}
		return false;
	}

	inline bool AllowsSideFeature(const ModuleDefinition& module, const sas::ContentId& sideFeatureId)
	{
		if (!sideFeatureId.IsValid()) return true;
		if (AllowsPrivateSideFeature(module, sideFeatureId)) return true;
		for (const ModuleSharedPoolPermission& permission : module.sharedPools)
			for (const sas::ContentId& featureId : permission.allowedFeatureIds)
				if (featureId == sideFeatureId) return true;
		return false;
	}

	inline bool IsLargestCellCountVariant(const ModuleDefinition& module, int variantIndex)
	{
		if (variantIndex < 0 || static_cast<std::size_t>(variantIndex) >= module.sizeVariants.size()) return false;
		const int largestIndex = GetLargestCellCountVariantIndex(module);
		if (largestIndex < 0) return false;
		return module.sizeVariants[static_cast<std::size_t>(variantIndex)].shape.cells.size() ==
			module.sizeVariants[static_cast<std::size_t>(largestIndex)].shape.cells.size();
	}

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

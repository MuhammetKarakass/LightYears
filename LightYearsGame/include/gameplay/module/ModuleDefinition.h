#pragma once

#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"
#include "framework/Core.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/module/ModuleRulePredicates.h"

#include <SFML/System/Vector2.hpp>

#include <string>
#include <tuple>
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
		GameplayTag damageType;
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
		GameplayTag damageType;
		ModuleLevelTable<float> damageByLevel;
		ModuleLevelTable<int> maxActiveByLevel;
	};
	struct ModuleKillExplosionRule
	{
		float maxHealthFraction = 0.f;
		float radius = 0.f;
		int maxActive = 0;
		GameplayTag damageType;
		sas::ContentId presentationId;
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
	// Rule equality decides whether a runtime keeps its state across a reapply. Each operator binds every
	// field of the rule structurally, so adding a field without extending the comparison fails to compile.
	template<typename T> bool operator==(const ModuleLevelTable<T>& left, const ModuleLevelTable<T>& right) { return left.values == right.values; }
	inline bool operator==(const ModuleAttributeModifier& left, const ModuleAttributeModifier& right) { const auto& [a0, a1, a2, a3] = left; const auto& [b0, b1, b2, b3] = right; const auto& [am0, am1, am2, am3] = a1; const auto& [bm0, bm1, bm2, bm3] = b1; return std::tie(a0, a2, a3) == std::tie(b0, b2, b3) && std::tie(am0, am1, am2, am3) == std::tie(bm0, bm1, bm2, bm3); }
	inline bool operator==(const ModuleTriggerBuff& left, const ModuleTriggerBuff& right) { const auto& [a0, a1, a2, a3, a4, a5, a6] = left; const auto& [b0, b1, b2, b3, b4, b5, b6] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6) == std::tie(b0, b1, b2, b3, b4, b5, b6); }
	inline bool operator==(const ModuleDamageRule& left, const ModuleDamageRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16, b17] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16, b17); }
	inline bool operator==(const ModuleRicochetRule& left, const ModuleRicochetRule& right) { const auto& [a0, a1, a2, a3, a4] = left; const auto& [b0, b1, b2, b3, b4] = right; return std::tie(a0, a1, a2, a3, a4) == std::tie(b0, b1, b2, b3, b4); }
	inline bool operator==(const ModuleGuidedSalvoRule& left, const ModuleGuidedSalvoRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7); }
	inline bool operator==(const ModuleDelayedExplosionRule& left, const ModuleDelayedExplosionRule& right) { const auto& [a0, a1, a2, a3, a4] = left; const auto& [b0, b1, b2, b3, b4] = right; return std::tie(a0, a1, a2, a3, a4) == std::tie(b0, b1, b2, b3, b4); }
	inline bool operator==(const ModuleTrailMineRule& left, const ModuleTrailMineRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10); }
	inline bool operator==(const ModuleKillExplosionRule& left, const ModuleKillExplosionRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6] = left; const auto& [b0, b1, b2, b3, b4, b5, b6] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6) == std::tie(b0, b1, b2, b3, b4, b5, b6); }
	inline bool operator==(const ModuleEventAction& left, const ModuleEventAction& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11); }
	inline bool operator==(const ModuleOwnerAttributeRule& left, const ModuleOwnerAttributeRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7); }
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
			GameplayTag damageType;
			ModuleLevelTable<float> fractionByLevel;
		};
		List<ShieldReflection> shieldReflections;
		ModuleStatusRules statusRules;
		ModuleGeneratedRules generatedRules;
		List<ModuleMovementFadeRule> movementFadeRules;
	};
	inline bool operator==(const ModuleRulePackage::ShieldReflection& left, const ModuleRulePackage::ShieldReflection& right) { const auto& [a0, a1, a2, a3] = left; const auto& [b0, b1, b2, b3] = right; return std::tie(a0, a1, a2, a3) == std::tie(b0, b1, b2, b3); }

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

	// The one traversal of every per-level table in a rule package. The visitor receives the table's field
	// path, the rule's index and the table, and returns false to stop. A new table is added here only.
	template<typename Visitor>
	bool VisitModuleRuleLevelTables(const ModuleRulePackage& rules, Visitor&& visit)
	{
		for (std::size_t i = 0; i < rules.attributeModifiers.size(); ++i) if (!visit("attributeModifiers.magnitude", i, rules.attributeModifiers[i].magnitudeByLevel)) return false;
		for (std::size_t i = 0; i < rules.abilityModifiers.size(); ++i) if (!visit("abilityModifiers.magnitude", i, rules.abilityModifiers[i].magnitudeByLevel)) return false;
		for (std::size_t i = 0; i < rules.triggerBuffs.size(); ++i)
		{
			const ModuleTriggerBuff& buff = rules.triggerBuffs[i];
			if (!visit("triggerBuffs.durationSeconds", i, buff.durationSecondsByLevel) || !visit("triggerBuffs.movementSpeedMultiplier", i, buff.movementSpeedMultiplierByLevel)) return false;
			for (const ModuleAttributeModifier& modifier : buff.modifiers) if (!visit("triggerBuffs.modifiers.magnitude", i, modifier.magnitudeByLevel)) return false;
		}
		for (std::size_t i = 0; i < rules.damageRules.size(); ++i)
		{
			const ModuleDamageRule& rule = rules.damageRules[i];
			if (!visit("damageRules.conditionValue", i, rule.conditionValueByLevel) || !visit("damageRules.openingShotCount", i, rule.openingShotCountByLevel) ||
				!visit("damageRules.damageMultiplier", i, rule.damageMultiplierByLevel) || !visit("damageRules.bonusDamageFraction", i, rule.bonusDamageFractionByLevel) ||
				!visit("damageRules.activeDurationSeconds", i, rule.activeDurationSecondsByLevel) || !visit("damageRules.maxHealthDamageFraction", i, rule.maxHealthDamageFractionByLevel)) return false;
		}
		for (std::size_t i = 0; i < rules.eventActions.size(); ++i)
		{
			const ModuleEventAction& action = rules.eventActions[i];
			if (!visit("eventActions.amount", i, action.amountByLevel) || !visit("eventActions.chancePerLuck", i, action.chancePerLuckByLevel) || !visit("eventActions.targetCount", i, action.targetCountByLevel)) return false;
		}
		for (std::size_t i = 0; i < rules.ownerAttributeRules.size(); ++i) if (!visit("ownerAttributeRules.amount", i, rules.ownerAttributeRules[i].amountByLevel)) return false;
		for (std::size_t i = 0; i < rules.shieldReflections.size(); ++i) if (!visit("shieldReflections.fraction", i, rules.shieldReflections[i].fractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.statusRules.electricOverrides.size(); ++i)
			for (const auto& table : rules.statusRules.electricOverrides[i].damageTakenByStackByLevel) if (!visit("statusRules.electricOverrides.damageTakenByStack", i, table)) return false;
		for (std::size_t i = 0; i < rules.statusRules.cryoFreezes.size(); ++i)
			if (!visit("statusRules.cryoFreezes.durationSeconds", i, rules.statusRules.cryoFreezes[i].durationSecondsByLevel) || !visit("statusRules.cryoFreezes.cooldownSeconds", i, rules.statusRules.cryoFreezes[i].cooldownSecondsByLevel)) return false;
		for (std::size_t i = 0; i < rules.statusRules.thermalCryoDurationBonuses.size(); ++i) if (!visit("statusRules.thermalCryoDurationBonuses.bonusSeconds", i, rules.statusRules.thermalCryoDurationBonuses[i].bonusSecondsByLevel)) return false;
		for (std::size_t i = 0; i < rules.statusRules.durationBonuses.size(); ++i) if (!visit("statusRules.durationBonuses.bonusSeconds", i, rules.statusRules.durationBonuses[i].bonusSecondsByLevel)) return false;
		for (std::size_t i = 0; i < rules.zoneEffects.size(); ++i) if (!visit("zoneEffects.multiplier", i, rules.zoneEffects[i].multiplierByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.ricochets.size(); ++i) if (!visit("generatedRules.ricochets.damageFraction", i, rules.generatedRules.ricochets[i].damageFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.guidedSalvos.size(); ++i) if (!visit("generatedRules.guidedSalvos.damageFraction", i, rules.generatedRules.guidedSalvos[i].damageFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.delayedExplosions.size(); ++i) if (!visit("generatedRules.delayedExplosions.damageFraction", i, rules.generatedRules.delayedExplosions[i].damageFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.trailMines.size(); ++i)
			if (!visit("generatedRules.trailMines.damage", i, rules.generatedRules.trailMines[i].damageByLevel) || !visit("generatedRules.trailMines.maxActive", i, rules.generatedRules.trailMines[i].maxActiveByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.killExplosions.size(); ++i) if (!visit("generatedRules.killExplosions.maxHealthFraction", i, rules.generatedRules.killExplosions[i].maxHealthFractionByLevel)) return false;
		return true;
	}

	inline bool HasModuleRulePackageLevelCoverage(const ModuleRulePackage& rules, int levelCount)
	{
		if (levelCount <= 0) return false;
		const std::size_t required = static_cast<std::size_t>(levelCount);
		return VisitModuleRuleLevelTables(rules, [required](const char*, std::size_t, const auto& table) { return table.values.empty() || table.values.size() >= required; });
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

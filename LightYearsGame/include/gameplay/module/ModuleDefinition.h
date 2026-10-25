#pragma once

#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"
#include "framework/Core.h"
#include "gameplay/module/rules/ModuleSideRules.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/module/ModuleLevelTable.h"
#include "gameplay/module/ModuleRulePredicates.h"
#include "gameplay/module/rules/EpicStatRules.h"
#include "gameplay/module/rules/EpicDefenseRules.h"
#include "gameplay/module/rules/EpicOffenseRules.h"
#include "gameplay/module/rules/EpicUtilityMovementRules.h"

#include <SFML/System/Vector2.hpp>

#include <string>
#include <optional>
#include <tuple>
#include <utility>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace vega
{
	enum class ModuleRarity
	{
		Common,
		Uncommon,
		Rare,
		Epic,
		Legendary
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
		float extendSeconds = 0.f;
		float maximumDurationSeconds = 0.f;
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
		// Stripped Hull: the resolved damageMultiplier becomes 1 + this value x usable empty cells. 0 = off.
		float perEmptyCellMultiplier = 0.f;
		ModuleLevelTable<float> perEmptyCellMultiplierByLevel;
		// Triple Core: the rule is kept while at least this many ability-slot shells are missing. 0 = off.
		int requiredEmptyAbilitySlots = 0;
		// TargetMaxHealthRatioAtLeast only: the multiplier grows by ratioSlope per unit of ratio above conditionValue. 0 = flat.
		float ratioSlope = 0.f;
	};
	// Giant Slayer-style Toppler: after an owner hit that lands on a target whose MaxHealth is at least minRatio x owner MaxHealth,
	// the target moves slower for seconds (bossSlowFraction for Boss-class targets). Not level-dependent.
	struct ModuleTargetSlowRule
	{
		float minRatio = 1.f;
		float slowFraction = 0.f;
		float bossSlowFraction = 0.f;
		float seconds = 0.f;
	};

	enum class ModuleZoneEffectKind { CooldownMultiplier, PrimaryHitCooldownSeconds };
	struct ModuleZoneEffect
	{
		GameplayTag zone;
		ModuleCooldownScope scope = ModuleCooldownScope::FamilyOfZone;
		float multiplier = 1.f;
		ModuleLevelTable<float> multiplierByLevel;
		ModuleZoneEffectKind kind = ModuleZoneEffectKind::CooldownMultiplier;
		float seconds = 0.f;
		ModuleLevelTable<float> secondsByLevel;
	};

	enum class ModuleEventActionKind { RestoreShield, GrantTemporaryShield, ShieldOverflowToHealth, RestoreHealth, RestoreHealthOverTime, RestoreAfterburner, ReduceAllCooldowns, ReduceFamilyCooldowns, ReduceLongestCooldown, ReduceLongestCooldownSeconds, RefundAllCooldownsFraction, RefundUsedCooldown, ApplyNearbyCryo, AwardGold };
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
		GameplayTag abilityFamily;
		List<GameplayTag> requiredEventTags;
		float durationSeconds = 0.f;
		float healthAtMost = 1.f;
	};
	// GoldArmorBonus (side features only): the placement's own Gold Armor bonus times amount; it reads the main Gold rule of the same placement.
	enum class ModuleOwnerAttributeSource { HealthThreshold, MovementSpeed, HigherPower, Luck, Armor, ArmorHigherPower, ArmorLowerPower, Gold, GoldArmorBonus, MaxHealthFraction, PowerEqualize, ShieldReserve };
	struct ModuleOwnerAttributeRule
	{
		ModuleOwnerAttributeSource source = ModuleOwnerAttributeSource::HealthThreshold;
		sas::AttributeId attributeId;
		float healthAtMost = 1.f;
		float amount = 0.f;
		float fadeSeconds = 0.f;
		float minimumSpeed = 0.f;
		float speedStep = 0.f;
		ModuleLevelTable<float> amountByLevel;
		float maximumAmount = 0.f;
		ModuleLevelTable<float> maximumAmountByLevel;
		ModuleAttributeTarget target = ModuleAttributeTarget::Owner;
		sas::AttributeModifierOperation operation = sas::AttributeModifierOperation::Add;
	};
	struct ModuleConditionalMovementRule
	{
		ModuleDamageCondition condition = ModuleDamageCondition::OwnerShieldOverMax;
		float multiplier = 1.f;
		ModuleLevelTable<float> multiplierByLevel;
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
	struct ModuleElectricSpreadRule
	{
		int threshold = 5;
		float radius = 0.f;
		int stacks = 2;
		int targetCount = 2;
		float cooldownSeconds = 4.f;
		// Resolver-owned placement identity; never read from content JSON.
		std::uint32_t instanceId = 0;
		std::size_t ruleIndex = 0;
	};
	struct ModulePrimaryStatusPrimerRule
	{
		float cooldownSeconds = 1.f;
		ModuleLevelTable<float> cooldownSecondsByLevel;
		std::uint32_t instanceId = 0;
		std::size_t ruleIndex = 0;
	};
	struct ModuleStatusDurationRule
	{
		float bonusSeconds = 0.f;
		ModuleLevelTable<float> bonusSecondsByLevel;
		float durationMultiplier = 1.f;
		ModuleLevelTable<float> durationMultiplierByLevel;
	};
	struct ModuleFreezeSpreadRule
	{
		float radius = 0.f;
		int stacks = 1;
	};
	// Ember Accumulation: when the owner's Thermal hit brings its Ignite stacks on a target to stackThreshold, one burst of
	// damageFraction x that stack's full DoT (ThermalDamagePerSecond x thermal duration) replaces the DoT. Optional area:
	// other opposing combatants within areaRadius take areaDamageFraction of the burst; they get no stacks or DoT.
	// Side features (Area Blast, Smoldering Remains) are modifier-only entries with damageFraction 0; the resolver merges their
	// area and smolder fields into the placement's own burst rule. smolderSeconds > 0 leaves a Thermal field at the burst point
	// that adds smolderStacksPerSecond stacks once per second to combatants within smolderRadius. 0 = off.
	struct ModuleThermalBurstRule
	{
		int stackThreshold = 4;
		float damageFraction = 0.f;
		ModuleLevelTable<float> damageFractionByLevel;
		float areaRadius = 0.f;
		float areaDamageFraction = 0.f;
		float smolderSeconds = 0.f;
		float smolderRadius = 0.f;
		int smolderStacksPerSecond = 0;
	};
	inline bool operator==(const ModuleThermalBurstRule& left, const ModuleThermalBurstRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7); }
	// Kinetic Breaker: the owner's next direct hit on a target with at least stackThreshold Kinetic stacks spends them, ignores
	// the target's Armor and multiplies the hit. normalTargetThreshold (0 = unset) replaces stackThreshold for Normal-class targets
	// only. Optional Armor Rend lowers the target's Armor by rendFraction for rendSeconds after the spend.
	struct ModuleKineticConsumeRule
	{
		int stackThreshold = 4;
		int normalTargetThreshold = 0;
		float damageMultiplier = 1.f;
		ModuleLevelTable<float> damageMultiplierByLevel;
		float rendFraction = 0.f;
		float rendSeconds = 0.f;
	};
	inline bool operator==(const ModuleKineticConsumeRule& left, const ModuleKineticConsumeRule& right) { const auto& [a0, a1, a2, a3, a4, a5] = left; const auto& [b0, b1, b2, b3, b4, b5] = right; return std::tie(a0, a1, a2, a3, a4, a5) == std::tie(b0, b1, b2, b3, b4, b5); }
	// Value snapshots owned by one ship's module applier; no global registry.
	struct ModuleStatusRules
	{
		List<ModuleElectricStatusRule> electricOverrides;
		List<ModuleElectricSpreadRule> electricSpreads;
		List<ModulePrimaryStatusPrimerRule> primaryStatusPrimers;
		List<ModuleCryoFreezeRule> cryoFreezes;
		List<ModuleStatusDurationRule> thermalCryoDurationBonuses;
		List<ModuleFreezeSpreadRule> freezeSpreads;
		List<ModuleStatusDurationRule> durationBonuses;
		List<ModuleThermalBurstRule> thermalBursts;
		List<ModuleKineticConsumeRule> kineticConsumes;
		bool Empty() const { return electricOverrides.empty() && electricSpreads.empty() && primaryStatusPrimers.empty() && cryoFreezes.empty() && thermalCryoDurationBonuses.empty() && freezeSpreads.empty() && durationBonuses.empty() && thermalBursts.empty() && kineticConsumes.empty(); }
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
		float attackPowerCoefficient = 0.f;
		float spacing = 0.f;
		float windowSeconds = 0.f;
		float fuseSeconds = 0.f;
		float radius = 0.f;
		int maxActive = 0;
		int additionalLimit = 0;
		int cryoStacks = 0;
		GameplayTag damageType;
		ModuleLevelTable<float> damageByLevel;
		ModuleLevelTable<float> attackPowerCoefficientByLevel;
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
	// Release Valve: damage = baseDamage + energyPowerCoefficient x Energy Power. Both are level tables.
	struct ModuleAnnularBurstRule
	{
		float innerRadius = 0.f;
		float outerRadius = 0.f;
		float baseDamage = 0.f;
		float energyPowerCoefficient = 0.f;
		ModuleLevelTable<float> baseDamageByLevel;
		ModuleLevelTable<float> energyPowerCoefficientByLevel;
	};
	inline bool operator==(const ModuleAnnularBurstRule& a, const ModuleAnnularBurstRule& b) { return std::tie(a.innerRadius, a.outerRadius, a.baseDamage, a.energyPowerCoefficient, a.baseDamageByLevel, a.energyPowerCoefficientByLevel) == std::tie(b.innerRadius, b.outerRadius, b.baseDamage, b.energyPowerCoefficient, b.baseDamageByLevel, b.energyPowerCoefficientByLevel); }
	struct ModuleSpeedLanceRule
	{
		float speedDamageCoefficient = 0.f;
		float sampleSeconds = 0.f;
		float projectileSpeed = 0.f;
		float range = 0.f;
		float echoDelaySeconds = 0.f;
		float echoFraction = 0.f;
		int firstTargetElectricStacks = 0;
		ModuleLevelTable<float> speedDamageCoefficientByLevel;
	};
	inline bool operator==(const ModuleSpeedLanceRule& a, const ModuleSpeedLanceRule& b) { return std::tie(a.speedDamageCoefficient, a.sampleSeconds, a.projectileSpeed, a.range, a.echoDelaySeconds, a.echoFraction, a.firstTargetElectricStacks, a.speedDamageCoefficientByLevel) == std::tie(b.speedDamageCoefficient, b.sampleSeconds, b.projectileSpeed, b.range, b.echoDelaySeconds, b.echoFraction, b.firstTargetElectricStacks, b.speedDamageCoefficientByLevel); }
	struct ModuleGeneratedRules
	{
		List<ModuleRicochetRule> ricochets;
		List<ModuleGuidedSalvoRule> guidedSalvos;
		List<ModuleDelayedExplosionRule> delayedExplosions;
		List<ModuleTrailMineRule> trailMines;
		List<ModuleKillExplosionRule> killExplosions;
		List<ModuleSpeedLanceRule> speedLances;
		List<ModuleAnnularBurstRule> annularBursts;
		bool Empty() const { return ricochets.empty() && guidedSalvos.empty() && delayedExplosions.empty() && trailMines.empty() && killExplosions.empty() && speedLances.empty() && annularBursts.empty(); }
	};
	enum class ModuleShieldRegenMode { Normal, SpeedScaled, Continuous };
	// Shield shaping for one placement. Every field is optional and the defaults change nothing.
	struct ModuleShieldRule
	{
		ModuleShieldRegenMode regenMode = ModuleShieldRegenMode::Normal;
		float speedMultiplierAtMax = 1.f;
		float minimumSpeed = 0.f;
		float continuousRateFraction = 1.f;
		float breakRearmFraction = 0.f;
		float armorShare = 0.f;
		ModuleLevelTable<float> speedMultiplierAtMaxByLevel;
		ModuleLevelTable<float> continuousRateFractionByLevel;
		ModuleLevelTable<float> breakRearmFractionByLevel;
		ModuleLevelTable<float> armorShareByLevel;
		bool frontRearSplit = false;
	};
	// Ship-wide thrust shaping. thrustMultiplier scales thrust and the normal speed cap (0, 1]. When
	// equalizeDirections is set, reverse and strafe thrust take the forward thrust value.
	struct ModuleMovementRule
	{
		float thrustMultiplier = 1.f;
		bool equalizeDirections = false;
		ModuleLevelTable<float> thrustMultiplierByLevel;
	};
	// Rule equality decides whether a runtime keeps its state across a reapply. Each operator binds every
	// field of the rule structurally, so adding a field without extending the comparison fails to compile.
	inline bool operator==(const ModuleAttributeModifier& left, const ModuleAttributeModifier& right) { const auto& [a0, a1, a2, a3] = left; const auto& [b0, b1, b2, b3] = right; const auto& [am0, am1, am2, am3] = a1; const auto& [bm0, bm1, bm2, bm3] = b1; return std::tie(a0, a2, a3) == std::tie(b0, b2, b3) && std::tie(am0, am1, am2, am3) == std::tie(bm0, bm1, bm2, bm3); }
	inline bool operator==(const ModuleTriggerBuff& left, const ModuleTriggerBuff& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8); }
	inline bool operator==(const ModuleDamageRule& left, const ModuleDamageRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16, b17, b18, b19, b20, b21] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16, b17, b18, b19, b20, b21); }
	inline bool operator==(const ModuleRicochetRule& left, const ModuleRicochetRule& right) { const auto& [a0, a1, a2, a3, a4] = left; const auto& [b0, b1, b2, b3, b4] = right; return std::tie(a0, a1, a2, a3, a4) == std::tie(b0, b1, b2, b3, b4); }
	inline bool operator==(const ModuleGuidedSalvoRule& left, const ModuleGuidedSalvoRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7); }
	inline bool operator==(const ModuleDelayedExplosionRule& left, const ModuleDelayedExplosionRule& right) { const auto& [a0, a1, a2, a3, a4] = left; const auto& [b0, b1, b2, b3, b4] = right; return std::tie(a0, a1, a2, a3, a4) == std::tie(b0, b1, b2, b3, b4); }
	inline bool operator==(const ModuleTrailMineRule& left, const ModuleTrailMineRule& right) { return std::tie(left.damage, left.attackPowerCoefficient, left.spacing, left.windowSeconds, left.fuseSeconds, left.radius, left.maxActive, left.additionalLimit, left.cryoStacks, left.damageType, left.damageByLevel, left.attackPowerCoefficientByLevel, left.maxActiveByLevel) == std::tie(right.damage, right.attackPowerCoefficient, right.spacing, right.windowSeconds, right.fuseSeconds, right.radius, right.maxActive, right.additionalLimit, right.cryoStacks, right.damageType, right.damageByLevel, right.attackPowerCoefficientByLevel, right.maxActiveByLevel); }
	inline bool operator==(const ModuleKillExplosionRule& left, const ModuleKillExplosionRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6] = left; const auto& [b0, b1, b2, b3, b4, b5, b6] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6) == std::tie(b0, b1, b2, b3, b4, b5, b6); }
	inline bool operator==(const ModuleEventAction& left, const ModuleEventAction& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15); }
	inline bool operator==(const ModuleShieldRule& left, const ModuleShieldRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10); }
	inline bool operator==(const ModuleMovementRule& left, const ModuleMovementRule& right) { const auto& [a0, a1, a2] = left; const auto& [b0, b1, b2] = right; return std::tie(a0, a1, a2) == std::tie(b0, b1, b2); }
	inline bool operator==(const ModuleOwnerAttributeRule& left, const ModuleOwnerAttributeRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11); }
	// Sonic state (Escape Velocity). While the ship's speed stays at or above speedFraction of its current cap for holdSeconds,
	// Primary shots fly faster, pierce more and deal more damage. lingerSeconds keeps the state that long after the speed drops.
	struct ModuleSustainedSpeedRule
	{
		float speedFraction = 1.f;
		float holdSeconds = 0.f;
		float primarySpeedMultiplier = 1.f;
		float primaryPierceBonus = 0.f;
		float primaryDamageMultiplier = 1.f;
		float lingerSeconds = 0.f;
		float turnCapabilityMultiplier = 1.f;
		ModuleLevelTable<float> primaryDamageMultiplierByLevel;
	};
	inline bool operator==(const ModuleSustainedSpeedRule& left, const ModuleSustainedSpeedRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7); }
	// Run counter rule (source Distance). The counter lives on the placement: it grows while the module is installed and
	// resets with the run. Each completed unitsPerStack adds one stack; a stack adds damagePerStack to the outgoing damage
	// multiplier and moveSpeedPerStack to the movement speed multiplier.
	// Run counter rule (source Kill). Each enemy the adjacent Q/E/F/R ability kills adds killWeights[class] stacks to the placement
	// and damagePerStack x that weight to its permanent kill damage bonus (a ratio, not a stack count). Distance rules ignore it.
	enum class ModuleRunCounterSource { Distance, Kill, AbilityUses };
	struct ModuleKillWeights
	{
		float normal = 1.f;
		float elite = 1.f;
		float miniBoss = 1.f;
		float boss = 1.f;
		float ForClass(ControlTargetClass targetClass) const { return targetClass == ControlTargetClass::Boss ? boss : targetClass == ControlTargetClass::MiniBoss ? miniBoss : targetClass == ControlTargetClass::Elite ? elite : normal; }
	};
	inline bool operator==(const ModuleKillWeights& left, const ModuleKillWeights& right) { const auto& [a0, a1, a2, a3] = left; const auto& [b0, b1, b2, b3] = right; return std::tie(a0, a1, a2, a3) == std::tie(b0, b1, b2, b3); }
	struct ModuleRunCounterRule
	{
		ModuleRunCounterSource source = ModuleRunCounterSource::Distance;
		float unitsPerStack = 0.f;
		float damagePerStack = 0.f;
		float moveSpeedPerStack = 0.f;
		ModuleLevelTable<float> damagePerStackByLevel;
		ModuleLevelTable<float> moveSpeedPerStackByLevel;
		// Kill source only: per-class stack weights and how many adjacent ability slots the rule may credit (default 2).
		ModuleKillWeights killWeights;
		int adjacentAbilityLimit = 2;
		GameplayTag requiredFamily;
		float armorPerStack = 0.f;
		float armorCap = 0.f;
		ModuleLevelTable<float> armorCapByLevel;
		ModuleLevelTable<float> eliteDamagePerKillByLevel;
		ModuleLevelTable<float> bossDamagePerKillByLevel;
	};
	inline bool operator==(const ModuleRunCounterRule& left, const ModuleRunCounterRule& right) { const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = left; const auto& [b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13] = right; return std::tie(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) == std::tie(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13); }
	// Bounty Mark: every stacksPerStep kill stacks of the placement reduce the cooldown of each adjacent ability by the next
	// step's reduction (reductionPerStep in order, tailReduction after the list). Reductions add up; the cooldown never falls below
	// minimumSeconds through this rule.
	struct ModuleKillStackCooldownRule
	{
		float stacksPerStep = 0.f;
		List<float> reductionPerStep;
		float tailReduction = 0.f;
		float minimumSeconds = 0.f;
		int adjacentAbilityLimit = 2;
	};
	inline bool operator==(const ModuleKillStackCooldownRule& left, const ModuleKillStackCooldownRule& right) { const auto& [a0, a1, a2, a3, a4] = left; const auto& [b0, b1, b2, b3, b4] = right; return std::tie(a0, a1, a2, a3, a4) == std::tie(b0, b1, b2, b3, b4); }
	// Trophy Wall: the placement's kill damage bonus also applies to every ability of family, at fraction of its value.
	struct ModuleKillStackDamageRule
	{
		GameplayTag family;
		float fraction = 0.f;
	};
	inline bool operator==(const ModuleKillStackDamageRule& left, const ModuleKillStackDamageRule& right) { const auto& [a0, a1] = left; const auto& [b0, b1] = right; return std::tie(a0, a1) == std::tie(b0, b1); }
	// Cooldown reduction (a fraction) for kill stacks. Steps are whole multiples of stacksPerStep; the list is consumed in order.
	inline float GetKillStackCooldownReduction(const ModuleKillStackCooldownRule& rule, double stacks)
	{
		if (!(rule.stacksPerStep > 0.f) || !(stacks > 0.0)) return 0.f;
		const double steps = std::floor(stacks / static_cast<double>(rule.stacksPerStep));
		const std::size_t listed = std::min(static_cast<std::size_t>(steps), rule.reductionPerStep.size());
		float reduction = 0.f;
		for (std::size_t step = 0; step < listed; ++step) reduction += rule.reductionPerStep[step];
		return reduction + static_cast<float>(steps - static_cast<double>(listed)) * rule.tailReduction;
	}
	// Completed stacks for a run counter value. The counter is never negative, so truncation is floor.
	inline int GetModuleRunCounterStacks(const ModuleRunCounterRule& rule, double counter) { return rule.unitsPerStack > 0.f ? static_cast<int>(counter / rule.unitsPerStack) : 0; }
	// Owner-centred burst. A ShieldBroken rule fires on a shield break. A KillConfirmed rule is charge-gated: each enemy the
	// owner kills within chargeRadius adds one charge, and chargeKills charges fire it and reset the count. Each enemy in radius
	// takes damageFractionOfMaxHealth of the owner's MaxHealth or the flat damage table (exactly one), plus igniteStacks Ignite.
	// pushImpulse moves hit enemies away, stunSeconds stuns them unless their class is in stunImmuneClasses. echoDelaySeconds > 0
	// schedules a second wave at echoFraction of the damage and echoPushFraction of the push. pullImpulse > 0 pulls enemies in
	// radius toward the owner pullLeadSeconds before the detonation. cooldownSeconds is per rule and per owner. Zero push, stun,
	// echo or pull turns that part off.
	struct ModuleOwnerBurstRule
	{
		ModuleTriggerKind trigger = ModuleTriggerKind::ShieldBroken;
		float radius = 0.f;
		float damageFractionOfMaxHealth = 0.f;
		ModuleLevelTable<float> damageFractionOfMaxHealthByLevel;
		float damage = 0.f;
		ModuleLevelTable<float> damageByLevel;
		GameplayTag damageType;
		int igniteStacks = 0;
		int chargeKills = 0;
		float chargeRadius = 0.f;
		float cooldownSeconds = 0.f;
		float pushImpulse = 0.f;
		float stunSeconds = 0.f;
		List<ControlTargetClass> stunImmuneClasses{ ControlTargetClass::Elite, ControlTargetClass::MiniBoss, ControlTargetClass::Boss };
		float echoDelaySeconds = 0.f;
		float echoFraction = 0.f;
		float echoPushFraction = 1.f;
		float pullImpulse = 0.f;
		float pullLeadSeconds = 0.f;
		List<ControlTargetClass> pullImmuneClasses{ ControlTargetClass::Boss };
		float energyPowerCoefficient = 0.f;
		float accumulatedArmorCoefficient = 0.f;
		List<GameplayTag> requiredEventTags;
	};
	inline bool operator==(const ModuleOwnerBurstRule& left, const ModuleOwnerBurstRule& right) { return std::tie(left.trigger, left.radius, left.damageFractionOfMaxHealth, left.damageFractionOfMaxHealthByLevel, left.damage, left.damageByLevel, left.damageType, left.igniteStacks, left.chargeKills, left.chargeRadius, left.cooldownSeconds, left.pushImpulse, left.stunSeconds, left.stunImmuneClasses, left.echoDelaySeconds, left.echoFraction, left.echoPushFraction, left.pullImpulse, left.pullLeadSeconds, left.pullImmuneClasses, left.energyPowerCoefficient, left.accumulatedArmorCoefficient, left.requiredEventTags) == std::tie(right.trigger, right.radius, right.damageFractionOfMaxHealth, right.damageFractionOfMaxHealthByLevel, right.damage, right.damageByLevel, right.damageType, right.igniteStacks, right.chargeKills, right.chargeRadius, right.cooldownSeconds, right.pushImpulse, right.stunSeconds, right.stunImmuneClasses, right.echoDelaySeconds, right.echoFraction, right.echoPushFraction, right.pullImpulse, right.pullLeadSeconds, right.pullImmuneClasses, right.energyPowerCoefficient, right.accumulatedArmorCoefficient, right.requiredEventTags); }
	// Side-feature patch on the placement's main ownerBurst rules. It never adds a rule, so it adds no damage source of its own.
	// Zero numeric fields keep the main value. The echo group is set together when echoDelaySeconds is present, the pull group
	// when pullImpulse is present.
	struct ModuleOwnerBurstModifier
	{
		float chargeRadius = 0.f;
		float stunSeconds = 0.f;
		float echoDelaySeconds = 0.f;
		float echoFraction = 0.f;
		float echoPushFraction = 1.f;
		float pullImpulse = 0.f;
		float pullLeadSeconds = 0.f;
		List<ControlTargetClass> pullImmuneClasses{ ControlTargetClass::Boss };
	};
	inline void ApplyModuleOwnerBurstModifier(ModuleOwnerBurstRule& rule, const ModuleOwnerBurstModifier& modifier)
	{
		if (modifier.chargeRadius > 0.f) rule.chargeRadius = modifier.chargeRadius;
		if (modifier.stunSeconds > rule.stunSeconds) rule.stunSeconds = modifier.stunSeconds;
		if (modifier.echoDelaySeconds > 0.f)
		{
			rule.echoDelaySeconds = modifier.echoDelaySeconds;
			rule.echoFraction = modifier.echoFraction;
			rule.echoPushFraction = modifier.echoPushFraction;
		}
		if (modifier.pullImpulse > 0.f)
		{
			rule.pullImpulse = modifier.pullImpulse;
			rule.pullLeadSeconds = modifier.pullLeadSeconds;
			rule.pullImmuneClasses = modifier.pullImmuneClasses;
		}
	}
	// Spare Capacitor charge rule. A main-package entry gives each adjacent Q/E/F/R ability chargePerLevel[level] extra
	// charge capacity; the number of adjacent abilities is the placement's cell count. A side-feature entry carries no
	// chargePerLevel: overflowTransfer moves a grant that does not fit to the next adjacent ability, and trickleSeconds > 0
	// gives trickleCharges to the adjacent ability with the fewest charges on each interval.
	struct ModuleChargeRule
	{
		ModuleLevelTable<int> chargePerLevel;
		bool overflowTransfer = false;
		float trickleSeconds = 0.f;
		int trickleCharges = 0;
	};
	#include "gameplay/module/rules/ShieldBatteryRuptureRules.h"

	enum class ModulePrimaryMountMode { Turret, Broadside };
	// Epic Primary mounts. Turret aims the Primary at the nearest valid enemy in range, whichever way the ship faces; its damage is an
	// ordinary PrimaryShotOnly damage rule. Broadside fires each volley from both sides instead of forward; damageMultiplier is the
	// coefficient of each side volley (level table, positive). The fire behaviour lives in the Primary fire path, not in this rule.
	struct ModulePrimaryMountRule
	{
		ModulePrimaryMountMode mode = ModulePrimaryMountMode::Turret;
		float damageMultiplier = 1.f;
		ModuleLevelTable<float> damageMultiplierByLevel;
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
			GameplayTag damageType;
			ModuleLevelTable<float> fractionByLevel;
			// Electric reflections only: Electric stacks each reflected hit applies to its target. 0 = none.
			int electricStacks = 0;
		};
		List<ShieldReflection> shieldReflections;
		ModuleStatusRules statusRules;
		ModuleGeneratedRules generatedRules;
		List<ModuleMovementFadeRule> movementFadeRules;
		List<ModuleConditionalMovementRule> conditionalMovementRules;
		List<ModuleShieldRule> shieldRules;
		List<ModuleShieldBatteryRule> shieldBatteryRules;
		List<ModuleShieldRuptureRule> shieldRuptureRules;
		List<ModuleMovementRule> movementRules;
		List<ModuleRunCounterRule> runCounterRules;
		List<ModuleSustainedSpeedRule> sustainedSpeedRules;
		List<ModuleOwnerBurstRule> ownerBurstRules;
		List<ModuleOwnerBurstModifier> ownerBurstModifiers;
		List<ModuleTargetSlowRule> targetSlowRules;
		List<ModuleKillStackCooldownRule> killStackCooldownRules;
		List<ModuleKillStackDamageRule> killStackDamageRules;
		List<ModuleChargeRule> chargeRules;
		List<EpicFamilyStatRule> familyStatRules;
		List<ModulePrimaryMountRule> primaryMountRules;
		// Execution Priority (Turret side feature only): the Turret prefers the valid target with the lowest health fraction.
		bool primaryExecutionPriority = false;
		// Epic Offense rules (Prism Core, Blood Contract and their side features).
		ModuleEpicOffenseRules epicOffense;
		// Epic Utility/Movement rules (rules/EpicUtilityMovementRules.h).
		List<ModuleCascadeRule> cascadeRules;
		List<ModuleFieldMasteryRule> fieldMasteryRules;
		List<ModuleAssaultVectorRule> assaultVectorRules;
		List<ModulePhaseSlipRule> phaseSlipRules;
		// F1 Parçalama, side features only: the placement may split into one-cell fragments at the largest size. Layout only.
		// Epic Defense (rules/EpicDefenseRules.h): Last Stand revive is a main package rule; hull-threshold rules are main (cooldown) or side (damage, movement).
		List<ModuleReviveRule> reviveRules;
		List<ModuleHealthThresholdRule> healthThresholdRules;
		bool fragmentation = false;
		int minimumZoneLevelBonus = 0;
		List<ModuleCooldownRecoveryRule> cooldownRecoveryRules;
		std::optional<ModuleDeferredDamageRule> deferredDamage;
		std::optional<ModuleMomentumExtension> momentumExtension;
		float primaryShareOfAbilityBonus = 0.f;
		bool primarySplitFire = false;
	};
	inline bool operator==(const ModuleRulePackage::ShieldReflection& left, const ModuleRulePackage::ShieldReflection& right) { const auto& [a0, a1, a2, a3, a4] = left; const auto& [b0, b1, b2, b3, b4] = right; return std::tie(a0, a1, a2, a3, a4) == std::tie(b0, b1, b2, b3, b4); }

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
		for (std::size_t i = 0; i < rules.runCounterRules.size(); ++i) if (!visit("runCounterRules.eliteDamagePerKill", i, rules.runCounterRules[i].eliteDamagePerKillByLevel) || !visit("runCounterRules.bossDamagePerKill", i, rules.runCounterRules[i].bossDamagePerKillByLevel)) return false;
		for (std::size_t i = 0; i < rules.reviveRules.size(); ++i) if (!visit("reviveRules.healthFraction", i, rules.reviveRules[i].healthFractionByLevel) || !visit("reviveRules.charges", i, rules.reviveRules[i].chargesByLevel)) return false;
		for (std::size_t i = 0; i < rules.healthThresholdRules.size(); ++i) if (!visit("healthThresholdRules.cooldownRecoveryMultiplier", i, rules.healthThresholdRules[i].cooldownRecoveryMultiplierByLevel)) return false;
		for (std::size_t i = 0; i < rules.statusRules.primaryStatusPrimers.size(); ++i) if (!visit("statusRules.primaryStatusPrimers.cooldownSeconds", i, rules.statusRules.primaryStatusPrimers[i].cooldownSecondsByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.speedLances.size(); ++i) if (!visit("generatedRules.speedLances.speedDamageCoefficient", i, rules.generatedRules.speedLances[i].speedDamageCoefficientByLevel)) return false;
		for (std::size_t i = 0; i < rules.phaseSlipRules.size(); ++i) if (!visit("phaseSlipRules.explosionRadius", i, rules.phaseSlipRules[i].explosionRadiusByLevel)) return false;
		for (std::size_t i = 0; i < rules.runCounterRules.size(); ++i) if (!visit("runCounterRules.armorCap", i, rules.runCounterRules[i].armorCapByLevel)) return false;
		for (std::size_t i = 0; i < rules.conditionalMovementRules.size(); ++i) if (!visit("conditionalMovementRules.multiplier", i, rules.conditionalMovementRules[i].multiplierByLevel)) return false;
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
				!visit("damageRules.activeDurationSeconds", i, rule.activeDurationSecondsByLevel) || !visit("damageRules.maxHealthDamageFraction", i, rule.maxHealthDamageFractionByLevel) || !visit("damageRules.perEmptyCellMultiplier", i, rule.perEmptyCellMultiplierByLevel)) return false;
		}
		for (std::size_t i = 0; i < rules.eventActions.size(); ++i)
		{
			const ModuleEventAction& action = rules.eventActions[i];
			if (!visit("eventActions.amount", i, action.amountByLevel) || !visit("eventActions.chancePerLuck", i, action.chancePerLuckByLevel) || !visit("eventActions.targetCount", i, action.targetCountByLevel)) return false;
		}
		for (std::size_t i = 0; i < rules.ownerAttributeRules.size(); ++i) if (!visit("ownerAttributeRules.amount", i, rules.ownerAttributeRules[i].amountByLevel) || !visit("ownerAttributeRules.maximumAmount", i, rules.ownerAttributeRules[i].maximumAmountByLevel)) return false;
		for (std::size_t i = 0; i < rules.shieldReflections.size(); ++i) if (!visit("shieldReflections.fraction", i, rules.shieldReflections[i].fractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.shieldRules.size(); ++i)
		{
			const ModuleShieldRule& rule = rules.shieldRules[i];
			if (!visit("shieldRules.speedMultiplierAtMax", i, rule.speedMultiplierAtMaxByLevel) || !visit("shieldRules.continuousRateFraction", i, rule.continuousRateFractionByLevel) ||
				!visit("shieldRules.breakRearmFraction", i, rule.breakRearmFractionByLevel) || !visit("shieldRules.armorShare", i, rule.armorShareByLevel)) return false;
		}
		for (std::size_t i = 0; i < rules.statusRules.electricOverrides.size(); ++i)
			for (const auto& table : rules.statusRules.electricOverrides[i].damageTakenByStackByLevel) if (!visit("statusRules.electricOverrides.damageTakenByStack", i, table)) return false;
		for (std::size_t i = 0; i < rules.statusRules.cryoFreezes.size(); ++i)
			if (!visit("statusRules.cryoFreezes.durationSeconds", i, rules.statusRules.cryoFreezes[i].durationSecondsByLevel) || !visit("statusRules.cryoFreezes.cooldownSeconds", i, rules.statusRules.cryoFreezes[i].cooldownSecondsByLevel)) return false;
		for (std::size_t i = 0; i < rules.statusRules.thermalCryoDurationBonuses.size(); ++i) if (!visit("statusRules.thermalCryoDurationBonuses.bonusSeconds", i, rules.statusRules.thermalCryoDurationBonuses[i].bonusSecondsByLevel)) return false;
		for (std::size_t i = 0; i < rules.statusRules.durationBonuses.size(); ++i)
		{
			if (!visit("statusRules.durationBonuses.bonusSeconds", i, rules.statusRules.durationBonuses[i].bonusSecondsByLevel)) return false;
			if (!visit("statusRules.durationBonuses.durationMultiplier", i, rules.statusRules.durationBonuses[i].durationMultiplierByLevel)) return false;
		}
		for (std::size_t i = 0; i < rules.statusRules.thermalBursts.size(); ++i) if (!visit("statusRules.thermalBursts.damageFraction", i, rules.statusRules.thermalBursts[i].damageFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.statusRules.kineticConsumes.size(); ++i) if (!visit("statusRules.kineticConsumes.damageMultiplier", i, rules.statusRules.kineticConsumes[i].damageMultiplierByLevel)) return false;
		for (std::size_t i = 0; i < rules.zoneEffects.size(); ++i) if (!visit("zoneEffects.multiplier", i, rules.zoneEffects[i].multiplierByLevel) || !visit("zoneEffects.seconds", i, rules.zoneEffects[i].secondsByLevel)) return false;
		for (std::size_t i = 0; i < rules.epicOffense.monochromeCores.size(); ++i) if (!visit("epicOffense.monochromeCores.pairMultiplier", i, rules.epicOffense.monochromeCores[i].pairMultiplierByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.ricochets.size(); ++i) if (!visit("generatedRules.ricochets.damageFraction", i, rules.generatedRules.ricochets[i].damageFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.guidedSalvos.size(); ++i) if (!visit("generatedRules.guidedSalvos.damageFraction", i, rules.generatedRules.guidedSalvos[i].damageFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.delayedExplosions.size(); ++i) if (!visit("generatedRules.delayedExplosions.damageFraction", i, rules.generatedRules.delayedExplosions[i].damageFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.trailMines.size(); ++i)
			if (!visit("generatedRules.trailMines.damage", i, rules.generatedRules.trailMines[i].damageByLevel) || !visit("generatedRules.trailMines.attackPowerCoefficient", i, rules.generatedRules.trailMines[i].attackPowerCoefficientByLevel) || !visit("generatedRules.trailMines.maxActive", i, rules.generatedRules.trailMines[i].maxActiveByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.killExplosions.size(); ++i) if (!visit("generatedRules.killExplosions.maxHealthFraction", i, rules.generatedRules.killExplosions[i].maxHealthFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.generatedRules.annularBursts.size(); ++i) if (!visit("generatedRules.annularBursts.baseDamage", i, rules.generatedRules.annularBursts[i].baseDamageByLevel) || !visit("generatedRules.annularBursts.energyPowerCoefficient", i, rules.generatedRules.annularBursts[i].energyPowerCoefficientByLevel)) return false;
		for (std::size_t i = 0; i < rules.shieldBatteryRules.size(); ++i) if (!visit("shieldBatteryRules.reserveFraction", i, rules.shieldBatteryRules[i].reserveFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.shieldRuptureRules.size(); ++i) if (!visit("shieldRuptureRules.damageFraction", i, rules.shieldRuptureRules[i].damageFractionByLevel)) return false;
		for (std::size_t i = 0; i < rules.movementRules.size(); ++i) if (!visit("movementRules.thrustMultiplier", i, rules.movementRules[i].thrustMultiplierByLevel)) return false;
			for (std::size_t i = 0; i < rules.runCounterRules.size(); ++i)
				if (!visit("runCounterRules.damagePerStack", i, rules.runCounterRules[i].damagePerStackByLevel) || !visit("runCounterRules.moveSpeedPerStack", i, rules.runCounterRules[i].moveSpeedPerStackByLevel)) return false;
			for (std::size_t i = 0; i < rules.sustainedSpeedRules.size(); ++i) if (!visit("sustainedSpeedRules.primaryDamageMultiplier", i, rules.sustainedSpeedRules[i].primaryDamageMultiplierByLevel)) return false;
			for (std::size_t i = 0; i < rules.ownerBurstRules.size(); ++i) if (!visit("ownerBurstRules.damageFractionOfMaxHealth", i, rules.ownerBurstRules[i].damageFractionOfMaxHealthByLevel) || !visit("ownerBurstRules.damage", i, rules.ownerBurstRules[i].damageByLevel)) return false;
			for (std::size_t i = 0; i < rules.chargeRules.size(); ++i) if (!visit("chargeRules.chargePerLevel", i, rules.chargeRules[i].chargePerLevel)) return false;
			for (std::size_t i = 0; i < rules.familyStatRules.size(); ++i) if (!visit("familyStatRules.perModule", i, rules.familyStatRules[i].perModuleByLevel)) return false;
			for (std::size_t i = 0; i < rules.primaryMountRules.size(); ++i) if (!visit("primaryMountRules.damageMultiplier", i, rules.primaryMountRules[i].damageMultiplierByLevel)) return false;
		for (std::size_t i = 0; i < rules.epicOffense.prismCores.size(); ++i) if (!visit("epicOffense.prismCores.statusDurationMultiplier", i, rules.epicOffense.prismCores[i].statusDurationMultiplierByLevel)) return false;
		for (std::size_t i = 0; i < rules.epicOffense.bloodContracts.size(); ++i)
			if (!visit("epicOffense.bloodContracts.fullHealthMultiplier", i, rules.epicOffense.bloodContracts[i].fullHealthMultiplierByLevel) || !visit("epicOffense.bloodContracts.emptyHealthMultiplier", i, rules.epicOffense.bloodContracts[i].emptyHealthMultiplierByLevel)) return false;
		for (std::size_t i = 0; i < rules.cascadeRules.size(); ++i) if (!visit("cascadeRules.stepBonus", i, rules.cascadeRules[i].stepBonusByLevel)) return false;
		for (std::size_t i = 0; i < rules.fieldMasteryRules.size(); ++i) if (!visit("fieldMasteryRules.usesPerLevelUp", i, rules.fieldMasteryRules[i].usesPerLevelUpByLevel)) return false;
		for (std::size_t i = 0; i < rules.assaultVectorRules.size(); ++i) if (!visit("assaultVectorRules.damageBonus", i, rules.assaultVectorRules[i].damageBonusByLevel) || !visit("assaultVectorRules.armorBonus", i, rules.assaultVectorRules[i].armorBonusByLevel)) return false;
		for (std::size_t i = 0; i < rules.phaseSlipRules.size(); ++i) if (!visit("phaseSlipRules.primaryDamageMultiplier", i, rules.phaseSlipRules[i].primaryDamageMultiplierByLevel)) return false;
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
		// Module IDs that cannot be installed together with this one. Optional; the relation is symmetric.
		List<sas::ContentId> conflictsWith;
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

	// Largest level bonus a zone entry can add to a module; the resolver clamps the zone level bonus to it.
	inline constexpr int kMaxZoneLevelBonus = 2;

	struct ModuleZoneEntry
	{
		GameplayTag zone;
		// Family that receives the bonus; ignored when anyFamily is set ("*").
		GameplayTag family;
		bool anyFamily = false;
		// Levels added to a matching module's level.
		int levelBonus = 0;
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

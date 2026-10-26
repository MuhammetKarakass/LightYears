#pragma once

#include "framework/Core.h"
#include "gameplay/ability/ScopedAbilityRule.h"
#include "gameplay/module/ModuleDefinition.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/weapon/PrimaryWeaponMountProfile.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace vega
{
	struct ResolvedModuleTriggerBuff
	{
		std::uint32_t instanceId{ 0 };
		std::size_t ruleIndex{ 0 };
		ModuleTriggerBuff rule;
		bool preserveActiveDurationOnChange = false;
	};

	struct ResolvedModuleDamageRule
	{
		std::uint32_t instanceId{ 0 };
		std::size_t ruleIndex{ 0 };
		ModuleDamageRule rule;
		List<GameplayTag> adjacentAbilitySystems;
	};

	struct ResolvedModuleCooldownRule
	{
		std::uint32_t instanceId{ 0 };
		std::size_t ruleIndex{ 0 };
		GameplayTag zone;
		ModuleCooldownScope scope = ModuleCooldownScope::FamilyOfZone;
		GameplayTag family;
		float multiplier = 1.f;
		// Bounty Mark: a loadout slot (None = scope rule) and an absolute cooldown floor in seconds (0 = none).
		sas::AbilitySlot slot = sas::AbilitySlot::None;
		float minimumSeconds = 0.f;
	};

	// Kill credit (Trophy Core): the per-kill damage ratio at the placement's level, the per-class stack weights and the
	// adjacent ability systems the placement credits.
	struct ResolvedModuleKillRule
	{
		std::uint32_t instanceId{ 0 };
		std::size_t ruleIndex{ 0 };
		float damagePerKill{ 0.f };
		ModuleKillWeights weights;
		int adjacentAbilityLimit{ 2 };
		List<GameplayTag> adjacentAbilitySystems;
		std::optional<float> eliteDamagePerKill;
		std::optional<float> bossDamagePerKill;
	};
	struct ResolvedModuleKillCooldownRule
	{
		std::uint32_t instanceId{ 0 };
		std::size_t ruleIndex{ 0 };
		ModuleKillStackCooldownRule rule;
		List<GameplayTag> adjacentAbilitySystems;
	};
	struct ResolvedModuleKillDamageRule
	{
		std::uint32_t instanceId{ 0 };
		std::size_t ruleIndex{ 0 };
		ModuleKillStackDamageRule rule;
	};
	// One scoped outgoing damage multiplier from kill damage. The id is stable per placement, rule and target slot or family.
	struct ResolvedModuleKillSource
	{
		std::string id;
		sas::AbilitySlot slot = sas::AbilitySlot::None;
		GameplayTag family;
		float multiplier = 1.f;
	};

	// Spare Capacitor charge rule of one placement. chargeBonus is the capacity added to each adjacent ability (0 for a side-feature
	// entry). slots are the adjacent Q/E/F/R abilities in geometric order, at most neighbourCount (the placement's cell count).
	struct ResolvedModuleChargeRule
	{
		std::uint32_t instanceId = 0;
		std::size_t ruleIndex = 0;
		int chargeBonus = 0;
		bool overflowTransfer = false;
		float trickleSeconds = 0.f;
		int trickleCharges = 0;
		int neighbourCount = 0;
		List<sas::AbilitySlot> slots;
	};

	struct ResolvedModuleEventAction
	{
		std::uint32_t instanceId = 0;
		std::size_t ruleIndex = 0;
		ModuleEventAction rule;
	};
	struct ResolvedModuleOwnerAttributeRule
	{
		std::uint32_t instanceId = 0;
		std::size_t ruleIndex = 0;
		ModuleOwnerAttributeRule rule;
	};
	template<typename Rule> struct ResolvedModuleGeneratedRule { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; Rule rule; };
	struct ResolvedModuleGeneratedRules
	{
		List<ResolvedModuleGeneratedRule<ModuleRicochetRule>> ricochets;
		List<ResolvedModuleGeneratedRule<ModuleGuidedSalvoRule>> guidedSalvos;
		List<ResolvedModuleGeneratedRule<ModuleDelayedExplosionRule>> delayedExplosions;
		List<ResolvedModuleGeneratedRule<ModuleTrailMineRule>> trailMines;
		List<ResolvedModuleGeneratedRule<ModuleKillExplosionRule>> killExplosions;
		List<ResolvedModuleGeneratedRule<ModuleSpeedLanceRule>> speedLances;
		List<ResolvedModuleGeneratedRule<ModuleAnnularBurstRule>> annularBursts;
	};
	struct ResolvedModuleTargetSlowRule { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleTargetSlowRule rule; };
	struct ResolvedModuleOwnerBurstRule { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleOwnerBurstRule rule; float accumulatedArmor = 0.f; };
	struct ResolvedModuleEffects
	{
		struct ZoneBonus { std::uint32_t instanceId; int levelBonus; };
		List<ZoneBonus> zoneBonuses;
		// True when a resolved modifier carries a ship-level term, so a ship level change must re-resolve.
		bool usesShipLevel = false;
		struct AbilityModule { sas::AbilitySlot slot; AbilityModuleRules rules; };
		List<AbilityModule> abilityModules;
		List<sas::AttributeModifier> ownerModifiers;
		List<sas::AttributeModifier> shipModifiers;
		List<sas::AttributeModifier> primaryWeaponModifiers;
		// requiredAbilityTags = { module family }, one rule per placement.
		List<ScopedAbilityRule> abilityRules;
		List<ResolvedModuleTriggerBuff> triggerBuffs;
		List<ResolvedModuleDamageRule> damageRules;
		List<ResolvedModuleCooldownRule> cooldownRules;
		List<ModuleCooldownRecoveryRule> cooldownRecoveryRules;
		ModuleDeferredDamageRule deferredDamage;
		List<GameplayTag> enabledSystems;
		List<ResolvedModuleEventAction> eventActions;
		List<ResolvedModuleOwnerAttributeRule> ownerAttributeRules;
		struct ConditionalMovement { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleConditionalMovementRule rule; };
		List<ConditionalMovement> conditionalMovementRules;
		struct ShieldReflection { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleRulePackage::ShieldReflection rule; };
		List<ShieldReflection> shieldReflections;
		ModuleStatusRules statusRules;
		ResolvedModuleGeneratedRules generatedRules;
		List<ResolvedModuleTargetSlowRule> targetSlowRules;
		struct MovementFade { std::uint32_t instanceId = 0; ModuleMovementFadeRule rule; };
		List<MovementFade> movementFadeRules;
		struct ShieldRule { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleShieldRule rule; };
		List<ShieldRule> shieldRules;
		List<ResolvedModuleShieldBatteryRule> shieldBatteryRules;
		List<ResolvedModuleShieldRuptureRule> shieldRuptureRules;
		struct MovementRule { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleMovementRule rule; };
		List<MovementRule> movementRules;
		struct SustainedSpeedRule { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleSustainedSpeedRule rule; };
		List<SustainedSpeedRule> sustainedSpeedRules;
		List<ResolvedModuleOwnerBurstRule> ownerBurstRules;
		// Kill-derived lists. killCooldownRules and killDamageRules are expanded into cooldownRules and killSources after the loop.
		List<ResolvedModuleKillRule> killRules;
		List<ResolvedModuleKillCooldownRule> killCooldownRules;
		List<ResolvedModuleKillDamageRule> killDamageRules;
		List<ResolvedModuleKillSource> killSources;
		List<ResolvedModuleChargeRule> chargeRules;
		// Ship-wide Primary mount shape (Turret Mount, Broadside Mount), read by the Primary fire path through the ability component.
		PrimaryWeaponMountProfile primaryMount;
		// Epic Offense aggregate of every placed Prism Core, Blood Contract and side feature rule.
		EpicOffenseEffects epicOffense;
		ResolvedModuleEpicRules epicRules;
		ResolvedModuleLegendaryRules legendaryRules;
	};

	namespace ModuleResolver
	{
		// Zone-driven hit refunds adopt the placement's family; an all-family zone has no single family.
		GameplayTag GetPlacementFamily(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement);
		// Level bonus for one placement from its zone: a whole placement takes its single zone (0 when the cells are mixed),
		// a fragmented placement takes the lowest cell bonus, so its worst fragment decides. Clamped to [0, kMaxZoneLevelBonus].
		int GetZoneLevelBonus(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement);

		// Level bonus recorded for one placement in effects.zoneBonuses; 0 when the placement has no entry.
		int FindZoneLevelBonus(const ResolvedModuleEffects& effects, std::uint32_t instanceId);

		// Usable schematic cells (zone set, not locked) that no placement covers. Derived from the schematic.
		int CountEmptyCells(const ModuleRulesContext& rules, const List<ModulePlacement>& placements);

		// Output follows the order of activePlacements.
		ResolvedModuleEffects Resolve(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements);

			// Empty-cell and ability-slot counts read layoutPlacements (installed, rebooting included); effects follow activePlacements.
			ResolvedModuleEffects Resolve(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements, const List<ModulePlacement>& layoutPlacements);

		// Epic Utility/Movement rules of one package at the placement's effective level (rules/EpicUtilityMovementResolve.cpp).
		void ResolveEpicUtilityMovementRules(ResolvedModuleEffects& effects, const ModuleRulePackage& package, std::uint32_t instanceId, int level, bool sideFeature);
	}
}

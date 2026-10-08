#pragma once

#include "framework/Core.h"
#include "gameplay/ability/ScopedAbilityRule.h"
#include "gameplay/module/ModuleDefinition.h"
#include "gameplay/module/ModuleLoadout.h"

#include <cstddef>
#include <cstdint>

namespace ly
{
	struct ResolvedModuleTriggerBuff
	{
		std::uint32_t instanceId{ 0 };
		std::size_t ruleIndex{ 0 };
		ModuleTriggerBuff rule;
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
	};
	struct ResolvedModuleEffects
	{
		List<sas::AttributeModifier> ownerModifiers;
		List<sas::AttributeModifier> shipModifiers;
		List<sas::AttributeModifier> primaryWeaponModifiers;
		// requiredAbilityTags = { module family }, one rule per placement.
		List<ScopedAbilityRule> abilityRules;
		List<ResolvedModuleTriggerBuff> triggerBuffs;
		List<ResolvedModuleDamageRule> damageRules;
		List<ResolvedModuleCooldownRule> cooldownRules;
		List<GameplayTag> enabledSystems;
		List<ResolvedModuleEventAction> eventActions;
		List<ResolvedModuleOwnerAttributeRule> ownerAttributeRules;
		struct ShieldReflection { std::uint32_t instanceId = 0; std::size_t ruleIndex = 0; ModuleRulePackage::ShieldReflection rule; };
		List<ShieldReflection> shieldReflections;
		ModuleStatusRules statusRules;
		ResolvedModuleGeneratedRules generatedRules;
		struct MovementFade { std::uint32_t instanceId = 0; ModuleMovementFadeRule rule; };
		List<MovementFade> movementFadeRules;
	};

	namespace ModuleResolver
	{
		// Bonus for one placement: the zone bonus when every covered cell lies in
		// one zone that rewards the module's family (or any family), else 0.
		float GetZoneBonus(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement);

		// Output follows the order of activePlacements.
		ResolvedModuleEffects Resolve(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements);
	}
}

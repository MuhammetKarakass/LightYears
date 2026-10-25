#include "gameplay/module/rules/EpicStatRuleResolution.h"

#include "gameplay/module/ModulePlacementRules.h"

#include <cstddef>
#include <utility>

namespace vega
{
	namespace
	{
		// Owner trigger buffs created from family rules get rule indices above any authored triggerBuffs index.
		constexpr std::size_t kFamilyTriggerBuffRuleBase = std::size_t{ 1 } << 20;

		// Number of installed main modules of `family`, excluding the owner placement. System shells never count and fragments
		// are one placement each, because each placement is one module.
		int CountOtherMainModules(const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements, std::uint32_t ownerInstanceId, const GameplayTag& family)
		{
			int count = 0;
			for (const ModulePlacement& other : activePlacements)
			{
				if (other.instanceId == ownerInstanceId) continue;
				const ModuleDefinition* module = rules.lookup ? rules.lookup(other.moduleId) : nullptr;
				if (module && ModuleResolver::GetPlacementFamily(rules, *module, other) == family && !module->systemCategory.IsValid()) ++count;
			}
			return count;
		}
	}

	void ApplyEpicFamilyStatRules(ResolvedModuleEffects& effects, const ModuleRulesContext& rules, const List<ModulePlacement>& activePlacements)
	{
		for (const ModulePlacement& placement : activePlacements)
		{
			const ModuleDefinition* module = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
			if (!module) continue;
			if (placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= module->sizeVariants.size()) continue;
			if (placement.level < 1 || placement.level > module->levelCount) continue;
			const ModuleSizeVariant& variant = module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)];
			const int level = placement.level + ModuleResolver::FindZoneLevelBonus(effects, placement.instanceId);

			for (std::size_t index = 0; index < variant.familyStatRules.size(); ++index)
			{
				const EpicFamilyStatRule& rule = variant.familyStatRules[index];
				const int count = CountOtherMainModules(rules, activePlacements, placement.instanceId, rule.family);
				const float perModule = rule.perModuleByLevel.ValueAtLevel(0.f, level);
				const float abilityBonus = rules.countLoadoutAbilities ? rule.perAbility * static_cast<float>(rules.countLoadoutAbilities(rule.family)) : 0.f;
				// Keep a zero contribution's active window so clearing and rebinding the last Movement slot cannot lose the doubling.
				if (!(perModule * static_cast<float>(count) + abilityBonus > 0.f) && !(rule.perAbility > 0.f && rule.doubleOnAbilityUse)) continue;

				sas::AttributeModifier modifier;
				modifier.attributeId = rule.attributeId;
				modifier.operation = rule.operation;
				modifier.magnitude = perModule * static_cast<float>(count) + abilityBonus;
				if (rule.operation == sas::AttributeModifierOperation::Multiply) modifier.magnitude = rule.reduce ? 1.f - modifier.magnitude : 1.f + modifier.magnitude;

				switch (rule.target)
				{
				case EpicFamilyStatTarget::Owner: effects.ownerModifiers.push_back(modifier); break;
				case EpicFamilyStatTarget::PrimaryWeapon: effects.primaryWeaponModifiers.push_back(modifier); break;
				case EpicFamilyStatTarget::Ability:
				{
					ScopedAbilityRule abilityRule;
					abilityRule.requiredAbilityTags.push_back(rule.family);
					abilityRule.attributeModifiers.push_back(modifier);
					effects.abilityRules.push_back(std::move(abilityRule));
					break;
				}
				}

				if (rule.doubleOnAbilityUse && rule.durationSeconds > 0.f)
				{
					ModuleAttributeModifier entry;
					entry.target = ModuleAttributeTarget::Owner;
					entry.modifier = modifier;
					entry.affectedByZoneBonus = false;

					ModuleTriggerBuff buff;
					buff.trigger = ModuleTriggerKind::AbilityActivated;
					buff.requiredEventTags.push_back(rule.family);
					buff.durationSeconds = rule.durationSeconds;
					buff.modifiers.push_back(std::move(entry));
					effects.triggerBuffs.push_back({ placement.instanceId, kFamilyTriggerBuffRuleBase + index, std::move(buff), rule.perAbility > 0.f });
				}
			}
		}
	}
}

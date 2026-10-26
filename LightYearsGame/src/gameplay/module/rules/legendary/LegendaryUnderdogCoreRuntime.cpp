#include "gameplay/module/rules/legendary/LegendaryUnderdogCoreRuntime.h"
#include "gameplay/ability/AbilityHaste.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/ScopedAbilityRule.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>

namespace vega
{
	LegendaryUnderdogCoreRuntime::LegendaryUnderdogCoreRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryUnderdogCoreRuntime::~LegendaryUnderdogCoreRuntime() { Clear(); }

	void LegendaryUnderdogCoreRuntime::Replace(const List<ResolvedModuleLegendaryUnderdogCoreRule>& rules)
	{
		Clear();
		mRules = rules;
		if (mRules.empty()) return;

		auto& attributes = mAbilities.GetAttributes();
		{
			LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
			const auto addOwner = [this, &attributes](const sas::AttributeId& attributeId, float amount)
			{
				if (!(amount > 0.f)) return;
				mOwnerHandles.push_back(attributes.AddModifier({ attributeId, sas::AttributeModifierOperation::Add, amount }));
			};
			for (const ResolvedModuleLegendaryUnderdogCoreRule& rule : mRules)
			{
				const float offense = rule.offensePowerPerWeight * static_cast<float>(rule.offenseWeight);
				const float defense = rule.defenseHealthPerWeight * static_cast<float>(rule.defenseWeight);
				const float movement = rule.movementRatingPerWeight * static_cast<float>(rule.movementWeight);
				addOwner(OwnerAttributeIds::AttackPower, offense);
				addOwner(OwnerAttributeIds::EnergyPower, offense);
				addOwner(OwnerAttributeIds::MaxHealth, defense);
				addOwner(OwnerAttributeIds::MoveSpeedHorizontal, movement);
				addOwner(OwnerAttributeIds::MoveSpeedVertical, movement);
			}
			if (auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner())) ship->RefreshMovementAttributesFromRuntime();
		}

		for (const ResolvedModuleLegendaryUnderdogCoreRule& rule : mRules)
		{
			const float primaryPercent = rule.primaryAttackSpeedPercentPerWeight * static_cast<float>(rule.primaryWeight);
			if (primaryPercent > 0.f)
				mPrimaryHandles.push_back(mAbilities.AddPrimaryWeaponModifier({ CommonAttributeIds::FireRate, sas::AttributeModifierOperation::Multiply, 1.f + primaryPercent * .01f }));

			const float utilityPercent = rule.utilityHastePercentPerWeight * static_cast<float>(rule.utilityWeight);
			if (utilityPercent > 0.f)
			{
				ScopedAbilityCooldownRule cooldownRule;
				cooldownRule.requiredAbilityTags.push_back(GameplayTags::Ability::Utility);
				// Utility Haste uses the shared Haste formula: cooldown / (1 + Haste/100).
				cooldownRule.multiplier = GetAbilityHasteCooldownMultiplier(utilityPercent);
				mCooldownHandles.push_back(mAbilities.AddScopedAbilityCooldownRule(std::move(cooldownRule)));
			}
		}
	}

	void LegendaryUnderdogCoreRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
	}

	void LegendaryUnderdogCoreRuntime::Clear()
	{
		for (const std::size_t handle : mPrimaryHandles) mAbilities.RemovePrimaryWeaponModifier(handle);
		mPrimaryHandles.clear();
		for (const std::size_t handle : mCooldownHandles) mAbilities.RemoveScopedAbilityCooldownRule(handle);
		mCooldownHandles.clear();

		auto& attributes = mAbilities.GetAttributes();
		{
			LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
			for (const sas::AttributeModifierHandle& handle : mOwnerHandles)
			{
				try { attributes.RemoveModifier(handle); }
				catch (...) {}
			}
			if (auto* ship = dynamic_cast<SpaceShip*>(&mAbilities.GetOwner())) ship->RefreshMovementAttributesFromRuntime();
		}
		mOwnerHandles.clear();
		mRules.clear();
	}
}

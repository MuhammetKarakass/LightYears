#include "gameplay/module/rules/EpicDefenseRuntime.h"

#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/module/ModuleSideFeature.h"

#include <algorithm>
#include <string>
#include <utility>

namespace vega
{
	namespace
	{
		constexpr const char* kEmergencyMovementSourceId = "ModuleEmergency.Movement";

		std::string EmergencyDamageSourceId(sas::AbilitySlot slot)
		{
			return "ModuleEmergency.Damage." + std::to_string(static_cast<int>(slot));
		}

		ModuleEmergencyEntry MakeEmergencyEntry(const ModuleHealthThresholdRule& rule, int level)
		{
			ModuleEmergencyEntry entry;
			entry.healthAtMost = rule.healthAtMost;
			entry.cooldownRecoveryMultiplier = ResolveEpicLevelValue(rule.cooldownRecoveryMultiplierByLevel, 1.f, level);
			entry.abilityDamageMultiplier = rule.abilityDamageMultiplier;
			entry.movementSpeedMultiplier = rule.movementSpeedMultiplier;
			return entry;
		}
	}

	void ModuleReviveBonusRuntime::Start(float seconds, float multiplier)
	{
		if (!(seconds > 0.f) || !(multiplier > 1.f)) return;
		RemoveModifiers();
		LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
		const sas::AttributeId stats[] = { OwnerAttributeIds::AttackPower, OwnerAttributeIds::EnergyPower, OwnerAttributeIds::AttackSpeed, OwnerAttributeIds::Armor };
		for (const sas::AttributeId& stat : stats)
		{
			sas::AttributeModifier modifier;
			modifier.attributeId = stat;
			modifier.operation = sas::AttributeModifierOperation::Multiply;
			modifier.magnitude = multiplier;
			const sas::AttributeModifierHandle handle = mAbilities.GetAttributes().AddModifier(modifier);
			if (handle.IsValid()) mHandles.push_back(handle);
		}
		mRemainingSeconds = seconds;
	}

	void ModuleReviveBonusRuntime::Tick(float deltaSeconds)
	{
		if (mRemainingSeconds <= 0.f || !(deltaSeconds > 0.f)) return;
		mRemainingSeconds -= deltaSeconds;
		if (mRemainingSeconds > 0.f) return;
		mRemainingSeconds = 0.f;
		RemoveModifiers();
	}

	void ModuleReviveBonusRuntime::Reset()
	{
		mRemainingSeconds = 0.f;
		RemoveModifiers();
	}

	void ModuleReviveBonusRuntime::RemoveModifiers()
	{
		if (mHandles.empty()) return;
		LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
		List<sas::AttributeModifierHandle> handles = std::move(mHandles);
		mHandles.clear();
		for (const sas::AttributeModifierHandle& handle : handles) mAbilities.GetAttributes().RemoveModifier(handle);
	}

	void ModuleEmergencyProtocolRuntime::Replace(const List<ModulePlacement>& activePlacements, const ModuleRulesContext& rules, const ResolvedModuleEffects& effects, ShipRuntimeModifiers* movementModifiers)
	{
		mMovementModifiers = movementModifiers;
		mEntries.clear();
		for (const ModulePlacement& placement : activePlacements)
		{
			const ModuleDefinition* module = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
			if (!module || placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= module->sizeVariants.size()) continue;
			const ModuleSizeVariant& variant = module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)];
			const int level = placement.level + ModuleResolver::FindZoneLevelBonus(effects, placement.instanceId);
			for (const ModuleHealthThresholdRule& rule : variant.healthThresholdRules) mEntries.push_back(MakeEmergencyEntry(rule, level));

			// Side features follow the resolver: only the largest variant takes one, and only when it is usable at the placement level.
			if (!placement.sideFeatureId.IsValid() || !rules.sideFeatureLookup || !IsLargestCellCountVariant(*module, placement.variantIndex)) continue;
			const ModuleSideFeatureDefinition* feature = rules.sideFeatureLookup(placement.sideFeatureId);
			if (!IsSideFeatureUsableAtLevel(*module, feature, placement.level)) continue;
			for (const ModuleHealthThresholdRule& rule : feature->rules.healthThresholdRules) mEntries.push_back(MakeEmergencyEntry(rule, level));
		}
	}

	void ModuleEmergencyProtocolRuntime::Tick(float deltaSeconds, const HealthComponent* health)
	{
		const float maxHealth = health ? health->GetMaxHealth() : 0.f;
		const float healthFraction = maxHealth > 0.f ? health->GetHealth() / maxHealth : 1.f;
		float cooldownRecovery = 1.f;
		float abilityDamage = 1.f;
		float movementSpeed = 1.f;
		for (const ModuleEmergencyEntry& entry : mEntries)
		{
			if (healthFraction > entry.healthAtMost) continue;
			cooldownRecovery = std::max(cooldownRecovery, entry.cooldownRecoveryMultiplier);
			abilityDamage = std::max(abilityDamage, entry.abilityDamageMultiplier);
			movementSpeed = std::max(movementSpeed, entry.movementSpeedMultiplier);
		}
		if (cooldownRecovery > 1.f && deltaSeconds > 0.f) RecoverCooldowns(deltaSeconds * (cooldownRecovery - 1.f));
		SyncDamage(abilityDamage);
		SyncMovement(movementSpeed);
	}

	void ModuleEmergencyProtocolRuntime::Clear()
	{
		mEntries.clear();
		SyncDamage(1.f);
		SyncMovement(1.f);
		mMovementModifiers = nullptr;
	}

	// The ability's own tick already recovers its cooldown by one second per second; this adds the rest of the multiplier.
	void ModuleEmergencyProtocolRuntime::RecoverCooldowns(float seconds)
	{
		const sas::AbilitySlot slots[] = { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };
		for (const sas::AbilitySlot slot : slots)
		{
			GameAbility* ability = mAbilities.GetAbility(slot);
			if (ability && ability->IsOnCooldown()) ability->ReduceCooldownRemaining(seconds);
		}
	}

	void ModuleEmergencyProtocolRuntime::SyncDamage(float multiplier)
	{
		if (multiplier == mDamageMultiplier) return;
		mDamageMultiplier = multiplier;
		const sas::AbilitySlot slots[] = { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };
		for (const sas::AbilitySlot slot : slots)
		{
			const std::string id = EmergencyDamageSourceId(slot);
			if (multiplier != 1.f) mCombatRuntime.SetRuntimeModifier(id, CombatRuntimeModifier{ multiplier, slot, GameplayTag{} });
			else mCombatRuntime.RemoveRuntimeModifier(id);
		}
	}

	void ModuleEmergencyProtocolRuntime::SyncMovement(float multiplier)
	{
		if (multiplier == mMovementMultiplier) return;
		mMovementMultiplier = multiplier;
		if (!mMovementModifiers) return;
		if (multiplier == 1.f)
		{
			mMovementModifiers->Remove(kEmergencyMovementSourceId);
			return;
		}
		ShipRuntimeModifier modifier;
		modifier.movementSpeedMultiplier = multiplier;
		mMovementModifiers->Set(kEmergencyMovementSourceId, modifier);
	}
}

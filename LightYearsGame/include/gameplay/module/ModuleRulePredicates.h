#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>

namespace ly
{
	enum class ModuleTriggerKind
	{
		None,
		AbilityActivated,
		ShieldBroken,
		DamageTaken,
		SourceDamageDealt,
		KillConfirmed,
		CriticalHit
	};

	enum class ModuleDamageCondition
	{
		PrimaryShotOnly,
		TargetHealthAtMost,
		FullMagazineShot,
		OwnerHealthAtMost,
		TargetWithoutStatus,
		PrimaryTargetWithoutStatus,
		TargetThermalAndCryo,
		SourceHasStatus,
		AdjacentAbilityDirectHit,
		FirstTargetHit,
		Always,
		OwnerShieldFull,
		OwnerShieldOverMax,
		TargetIsBossClass,
		TargetStatusKindsAtLeast,
		AbilityDirectHit,
		TargetMaxHealthRatioAtLeast,
		SourceMaxHealthGreaterThanOwner
	};

	// Fire conditions are snapshotted when a primary shot leaves the weapon; impact
	// conditions are evaluated against the target when the hit lands.
	enum class ModuleDamageRulePhase
	{
		Fire,
		Impact
	};

	inline ModuleDamageRulePhase GetModuleDamageConditionPhase(ModuleDamageCondition condition)
	{
		return condition == ModuleDamageCondition::FullMagazineShot ? ModuleDamageRulePhase::Fire : ModuleDamageRulePhase::Impact;
	}

	struct ModuleTriggerEventContext
	{
		ModuleTriggerKind trigger;
		float magnitude;
		const List<GameplayTag>& sourceAbilityTags;
		const List<GameplayTag>& payloadTags;
		// Damage produced by a module rule itself (bonus hits); it never opens a damage window.
		bool moduleGenerated = false;
		bool suppressModuleTriggers = false;
	};

	inline ModuleTriggerKind ModuleTriggerFromEventTag(const GameplayTag& eventTag)
	{
		if (eventTag == GameplayTags::Event::Combat::KillConfirmed) return ModuleTriggerKind::KillConfirmed;
		if (eventTag == GameplayTags::Event::Ability::Activated) return ModuleTriggerKind::AbilityActivated;
		if (eventTag == GameplayTags::Event::Owner::DamageTaken) return ModuleTriggerKind::DamageTaken;
		if (eventTag == GameplayTags::Event::Source::DamageDealt) return ModuleTriggerKind::SourceDamageDealt;
		return ModuleTriggerKind::None;
	}

	inline bool ModuleTriggerEventTagsMatch(const List<GameplayTag>& requiredTags, const List<GameplayTag>& actualTags)
	{
		return std::all_of(requiredTags.begin(), requiredTags.end(), [&actualTags](const GameplayTag& requiredTag)
		{
			return std::any_of(actualTags.begin(), actualTags.end(), [&requiredTag](const GameplayTag& actualTag)
				{ return actualTag.MatchesTag(requiredTag); });
		});
	}

	inline bool IsModuleTriggerEventEligible(ModuleTriggerKind trigger, float magnitude)
	{
		switch (trigger)
		{
		case ModuleTriggerKind::AbilityActivated:
		case ModuleTriggerKind::ShieldBroken:
		case ModuleTriggerKind::KillConfirmed:
		case ModuleTriggerKind::CriticalHit:
			return true;
		case ModuleTriggerKind::DamageTaken:
		case ModuleTriggerKind::SourceDamageDealt:
			return magnitude > 0.f;
		case ModuleTriggerKind::None:
			return false;
		}
		return false;
	}

	inline bool MatchesModuleTrigger(ModuleTriggerKind trigger, const List<GameplayTag>& requiredEventTags, float durationSeconds, const ModuleTriggerEventContext& event)
	{
		if (event.suppressModuleTriggers || trigger != event.trigger || !IsModuleTriggerEventEligible(event.trigger, event.magnitude) || durationSeconds <= 0.f) return false;
		switch (trigger)
		{
		case ModuleTriggerKind::AbilityActivated:
			if (std::find(requiredEventTags.begin(), requiredEventTags.end(), GameplayTags::Ability::Offense) != requiredEventTags.end() &&
				std::any_of(event.sourceAbilityTags.begin(), event.sourceAbilityTags.end(),
					[](const GameplayTag& tag) { return tag.MatchesTag(GameplayTags::Ability::Primary); })) return false;
			return ModuleTriggerEventTagsMatch(requiredEventTags, event.sourceAbilityTags);
		case ModuleTriggerKind::DamageTaken:
		case ModuleTriggerKind::SourceDamageDealt:
		case ModuleTriggerKind::CriticalHit:
			return ModuleTriggerEventTagsMatch(requiredEventTags, event.payloadTags);
		case ModuleTriggerKind::ShieldBroken:
		case ModuleTriggerKind::KillConfirmed:
			return ModuleTriggerEventTagsMatch(requiredEventTags, event.payloadTags);
		case ModuleTriggerKind::None:
			return false;
		}
		return false;
	}

	struct ModuleDamageConditionContext
	{
		bool isPrimaryShot = false;
		float targetHealthFraction = 1.f;
		float ownerHealthFraction = 1.f;
		int magazineShotsFired = 0;
		bool targetHasStatus = false;
		bool targetHasThermalAndCryo = false;
		bool sourceHasStatus = false;
		bool isAdjacentAbilityDirectHit = false;
		bool isFirstTargetHit = false;
		bool ownerShieldFull = false;
		bool ownerShieldOverMax = false;
		bool targetIsBossClass = false;
		int targetStatusKindCount = 0;
		// Outgoing hit from a Q/E/F/R ability slot that is not Primary and not periodic.
		bool isAbilityDirectHit = false;
		// Outgoing: target MaxHealth / owner MaxHealth; 0 when the owner MaxHealth is not positive.
		float targetMaxHealthRatio = 0.f;
		// Incoming: attacker MaxHealth and the owner (the hit target) MaxHealth at resolution.
		float sourceMaxHealth = 0.f;
		float ownerMaxHealth = 0.f;
	};

	inline bool MatchesModuleDamageCondition(ModuleDamageCondition condition, float conditionValue, int openingShotCount, const ModuleDamageConditionContext& context)
	{
		switch (condition)
		{
		case ModuleDamageCondition::TargetWithoutStatus: return !context.targetHasStatus;
		case ModuleDamageCondition::PrimaryTargetWithoutStatus: return context.isPrimaryShot && !context.targetHasStatus;
		case ModuleDamageCondition::TargetThermalAndCryo: return context.targetHasThermalAndCryo;
		case ModuleDamageCondition::SourceHasStatus: return context.sourceHasStatus;
		case ModuleDamageCondition::AdjacentAbilityDirectHit: return context.isAdjacentAbilityDirectHit;
		case ModuleDamageCondition::AbilityDirectHit: return context.isAbilityDirectHit;
		case ModuleDamageCondition::FirstTargetHit: return context.isFirstTargetHit;
		case ModuleDamageCondition::Always: return true;
		case ModuleDamageCondition::OwnerShieldFull: return context.ownerShieldFull;
		case ModuleDamageCondition::OwnerShieldOverMax: return context.ownerShieldOverMax;
		case ModuleDamageCondition::TargetIsBossClass: return context.targetIsBossClass;
		case ModuleDamageCondition::TargetStatusKindsAtLeast: return static_cast<float>(context.targetStatusKindCount) >= conditionValue;
		case ModuleDamageCondition::TargetMaxHealthRatioAtLeast: return context.targetMaxHealthRatio >= conditionValue;
		case ModuleDamageCondition::SourceMaxHealthGreaterThanOwner: return context.ownerMaxHealth > 0.f && context.sourceMaxHealth > context.ownerMaxHealth;
		case ModuleDamageCondition::OwnerHealthAtMost:
			return context.ownerHealthFraction > 0.f && context.ownerHealthFraction <= conditionValue;
		case ModuleDamageCondition::PrimaryShotOnly:
			return context.isPrimaryShot;
		case ModuleDamageCondition::TargetHealthAtMost:
			return context.targetHealthFraction <= conditionValue;
		case ModuleDamageCondition::FullMagazineShot:
			return context.magazineShotsFired < openingShotCount;
		}
		return false;
	}
}

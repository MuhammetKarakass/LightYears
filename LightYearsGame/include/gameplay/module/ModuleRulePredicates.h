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
		SourceDamageDealt
	};

	enum class ModuleDamageCondition
	{
		PrimaryShotOnly,
		TargetHealthAtMost,
		FullMagazineShot
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
	};

	inline ModuleTriggerKind ModuleTriggerFromEventTag(const GameplayTag& eventTag)
	{
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
			return true;
		case ModuleTriggerKind::DamageTaken:
		case ModuleTriggerKind::SourceDamageDealt:
			return magnitude > 0.f;
		case ModuleTriggerKind::None:
			return false;
		}
		return false;
	}

	inline bool MatchesModuleTrigger(
		ModuleTriggerKind trigger,
		const List<GameplayTag>& requiredEventTags,
		float durationSeconds,
		const ModuleTriggerEventContext& event
	)
	{
		if (trigger != event.trigger || !IsModuleTriggerEventEligible(event.trigger, event.magnitude) || durationSeconds <= 0.f) return false;
		switch (trigger)
		{
		case ModuleTriggerKind::AbilityActivated:
			if (std::find(requiredEventTags.begin(), requiredEventTags.end(), GameplayTags::Ability::Offense) != requiredEventTags.end() &&
				std::any_of(event.sourceAbilityTags.begin(), event.sourceAbilityTags.end(),
					[](const GameplayTag& tag) { return tag.MatchesTag(GameplayTags::Ability::Primary); })) return false;
			return ModuleTriggerEventTagsMatch(requiredEventTags, event.sourceAbilityTags);
		case ModuleTriggerKind::DamageTaken:
		case ModuleTriggerKind::SourceDamageDealt:
			return ModuleTriggerEventTagsMatch(requiredEventTags, event.payloadTags);
		case ModuleTriggerKind::ShieldBroken:
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
		int magazineShotsFired = 0;
	};

	inline bool MatchesModuleDamageCondition(
		ModuleDamageCondition condition,
		float conditionValue,
		int openingShotCount,
		const ModuleDamageConditionContext& context
	)
	{
		switch (condition)
		{
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

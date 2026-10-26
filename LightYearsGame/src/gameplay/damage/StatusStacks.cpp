#include "gameplay/damage/StatusStacks.h"
#include "attributes/GameplayAttribute.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"

#include <algorithm>

namespace vega
{
	namespace
	{
		struct StatusEffect { const GameplayTag* status; const char* effectId; };

		const List<StatusEffect>& StatusEffects()
		{
			static const List<StatusEffect> effects{
				{ &DamageStatusSchema::Ignite, DamageStatusEffectIds::IgniteEffectId },
				{ &DamageStatusSchema::CryoSlowed, DamageStatusEffectIds::CryoSlowedEffectId },
				{ &DamageStatusSchema::Electric, DamageStatusEffectIds::ElectricEffectId },
				{ &DamageStatusSchema::Kinetic, DamageStatusEffectIds::KineticEffectId }
			};
			return effects;
		}

		const char* EffectIdOf(const GameplayTag& status)
		{
			for (const StatusEffect& entry : StatusEffects())
				if (*entry.status == status) return entry.effectId;
			return nullptr;
		}
	}

	const List<GameplayTag>& GetStackingDamageStatuses()
	{
		static const List<GameplayTag> statuses{ DamageStatusSchema::Ignite, DamageStatusSchema::CryoSlowed, DamageStatusSchema::Electric, DamageStatusSchema::Kinetic };
		return statuses;
	}

	List<DamageStatusStackState> CaptureDamageStatusStacks(const sas::AbilitySystemComponent& target)
	{
		List<DamageStatusStackState> states;
		for (const StatusEffect& entry : StatusEffects())
			if (const sas::ActiveGameplayEffect* effect = target.FindGameplayEffectById(entry.effectId); effect && effect->stackCount > 0)
				states.push_back({ *entry.status, effect->stackCount, effect->spec.maxStacks, effect->remainingDuration });
		return states;
	}

	const DamageStatusStackState* FindDamageStatusStack(const List<DamageStatusStackState>& states, const GameplayTag& status)
	{
		const auto found = std::find_if(states.begin(), states.end(), [&status](const DamageStatusStackState& state) { return state.status == status; });
		return found != states.end() ? &*found : nullptr;
	}

	bool ConsumeDamageStatus(sas::AbilitySystemComponent& target, const GameplayTag& status)
	{
		const char* effectId = EffectIdOf(status);
		sas::ActiveGameplayEffect* effect = effectId ? target.FindGameplayEffectById(effectId) : nullptr;
		if (!effect) return false;
		if (sas::GameplayAttribute* pending = sas::FindAttribute(effect->runtimeAttributes, DamageAttributeIds::IgnitePendingDamage)) pending->currentValue = 0.f;
		target.RemoveGameplayEffect(effect->handle);
		return true;
	}

	List<GameplayTag> ResolveAddedDamageStatuses(const List<GameplayTag>& appliedStatuses, const List<DamageStatusStackState>& before, const List<DamageStatusStackState>& after)
	{
		List<GameplayTag> added;
		for (const GameplayTag& status : GetStackingDamageStatuses())
		{
			if (std::find(appliedStatuses.begin(), appliedStatuses.end(), status) == appliedStatuses.end()) continue;
			const DamageStatusStackState* previous = FindDamageStatusStack(before, status);
			const DamageStatusStackState* current = FindDamageStatusStack(after, status);
			const int previousStacks = previous ? previous->stacks : 0;
			const int currentStacks = current ? current->stacks : 0;
			// Below the cap an application always adds a stack, even when the same hit spends the stacks again; at the cap
			// it is only a refresh unless the stacks were spent and rebuilt.
			if (!previous || previousStacks < previous->maxStacks || currentStacks < previousStacks) added.push_back(status);
		}
		return added;
	}
}

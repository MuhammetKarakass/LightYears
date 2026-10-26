#include "gameplay/control/ControlEffects.h"
#include "effects/GameplayEffectSpec.h"
#include "framework/Actor.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/tags/ability/NullPulseTags.h"

#include <algorithm>

namespace vega
{
	namespace
	{
		// The catalog stun shared with Blastback, NullPulse and module bursts.
		constexpr char kBasicStunEffectId[] = "Effect.Control.Stun.Basic";
	}

	float ResolveControlDuration(const Actor& target, const GameplayTag& controlTag, float seconds)
	{
		if (!(seconds > 0.f)) return 0.f;
		const auto* combatant = dynamic_cast<const Combatant*>(&target);
		if (!combatant) return seconds;
		const ControlResponse response = combatant->ResolveControlResponse(controlTag);
		if (response.mode == ControlResponseMode::Immune || response.mode == ControlResponseMode::InterruptOnly) return 0.f;
		return seconds * std::max(0.f, response.durationMultiplier);
	}

	float ApplyControlStun(Actor& target, float seconds, Actor* source)
	{
		auto* combatant = dynamic_cast<Combatant*>(&target);
		const sas::GameplayEffectDefinition* definition = EffectData::FindGameplayEffectDefinition(kBasicStunEffectId);
		const float duration = ResolveControlDuration(target, GameplayTags::State::Effect::Control::Stunned, seconds);
		if (!combatant || !definition || !(duration > 0.f)) return 0.f;
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*definition);
		spec.duration = duration;
		spec.maxStacks = 1;
		return combatant->GetAbilitySystemComponent().ApplyGameplayEffect(spec, sas::GameplayEffectSourceContext{ source }).IsValid() ? duration : 0.f;
	}
}

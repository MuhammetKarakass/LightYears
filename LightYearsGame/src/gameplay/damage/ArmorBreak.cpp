#include "gameplay/damage/ArmorBreak.h"
#include "effects/GameplayEffectSpec.h"
#include "framework/Actor.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>

namespace vega
{
	namespace
	{
		const sas::AttributeId& FractionId()
		{
			static const sas::AttributeId id{ ArmorBreakIds::FractionAttributeId };
			return id;
		}
	}

	float ApplyArmorBreak(Actor& target, float fraction, float seconds, float bossMaxFraction, Actor* source)
	{
		auto* combatant = dynamic_cast<Combatant*>(&target);
		const sas::GameplayEffectDefinition* definition = EffectData::FindGameplayEffectDefinition(ArmorBreakIds::EffectId);
		if (!combatant || !definition || !(fraction > 0.f) || !(seconds > 0.f)) return 0.f;
		fraction = std::clamp(fraction, 0.f, 1.f);
		if (const auto* ship = dynamic_cast<const SpaceShip*>(&target))
		{
			const ControlTargetClass targetClass = ship->GetControlTargetClass();
			if (targetClass == ControlTargetClass::Boss || targetClass == ControlTargetClass::MiniBoss) fraction = std::min(fraction, std::clamp(bossMaxFraction, 0.f, 1.f));
		}
		if (!(fraction > 0.f)) return 0.f;
		auto& abilities = combatant->GetAbilitySystemComponent();
		if (const sas::ActiveGameplayEffect* existing = abilities.FindGameplayEffectById(ArmorBreakIds::EffectId))
		{
			fraction = std::max(fraction, sas::FindAttributeValue(existing->spec.attributes, FractionId(), 0.f));
			seconds = std::max(seconds, existing->remainingDuration);
			abilities.RemoveGameplayEffect(existing->handle);
		}
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*definition);
		spec.duration = seconds;
		spec.maxStacks = 1;
		spec.modifiers = { sas::AttributeModifier{ OwnerAttributeIds::Armor, sas::AttributeModifierOperation::Multiply, 1.f - fraction } };
		spec.attributes.emplace_back(FractionId(), fraction, fraction);
		return abilities.ApplyGameplayEffect(spec, sas::GameplayEffectSourceContext{ source }).IsValid() ? fraction : 0.f;
	}
}

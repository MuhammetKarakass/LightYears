#include "gameplay/weapon/ShotDeviation.h"
#include "effects/GameplayEffectSpec.h"
#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/combat/Combatant.h"

#include <algorithm>

namespace vega
{
	namespace
	{
		const sas::AttributeId& MaxDegreesId()
		{
			static const sas::AttributeId id{ ShotDeviationIds::MaxDegreesAttributeId };
			return id;
		}
	}

	bool ApplyShotDeviation(Actor& shooter, float maxDegrees, float seconds, Actor* source)
	{
		auto* combatant = dynamic_cast<Combatant*>(&shooter);
		const sas::GameplayEffectDefinition* definition = EffectData::FindGameplayEffectDefinition(ShotDeviationIds::EffectId);
		if (!combatant || !definition || !(maxDegrees > 0.f) || !(seconds > 0.f)) return false;
		auto& abilities = combatant->GetAbilitySystemComponent();
		if (const sas::ActiveGameplayEffect* existing = abilities.FindGameplayEffectById(ShotDeviationIds::EffectId))
		{
			maxDegrees = std::max(maxDegrees, sas::FindAttributeValue(existing->spec.attributes, MaxDegreesId(), 0.f));
			seconds = std::max(seconds, existing->remainingDuration);
			abilities.RemoveGameplayEffect(existing->handle);
		}
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*definition);
		spec.duration = seconds;
		spec.maxStacks = 1;
		spec.attributes.emplace_back(MaxDegreesId(), maxDegrees, maxDegrees);
		return abilities.ApplyGameplayEffect(spec, sas::GameplayEffectSourceContext{ source }).IsValid();
	}

	float GetShotDeviationDegrees(const Actor& shooter)
	{
		const auto* combatant = dynamic_cast<const Combatant*>(&shooter);
		const sas::ActiveGameplayEffect* effect = combatant ? combatant->GetAbilitySystemComponent().FindGameplayEffectById(ShotDeviationIds::EffectId) : nullptr;
		return effect ? std::max(0.f, sas::FindAttributeValue(effect->spec.attributes, MaxDegreesId(), 0.f)) : 0.f;
	}

	float SampleShotDeviationDegrees(const Actor& shooter)
	{
		const float maxDegrees = GetShotDeviationDegrees(shooter);
		return maxDegrees > 0.f ? RandRange(-maxDegrees, maxDegrees) : 0.f;
	}
}

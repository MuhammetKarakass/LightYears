#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "framework/GameplayTag.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include <algorithm>

namespace vega
{
	DamagePayload MakeModuleGeneratedDamagePayload(int cryoStacks, DamageSourceScalingPolicy scaling, int igniteStacks)
	{
		DamagePayload payload;
		payload.criticalPolicy = DamageCriticalPolicy::Disabled;
		payload.kineticStacks = 0;
		payload.cryoBuildupPerHit = cryoStacks;
		payload.igniteStacks = igniteStacks;
		payload.skipModuleDamageRules = true;
		payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
		payload.sourceScalingPolicy = scaling;
		return payload;
	}
	void ApplyModuleGeneratedAreaDamage(Actor& delivery, Actor& source, const std::shared_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& origin, float radius, float damage, const List<GameplayTag>& tags, int cryoStacks, DamageSourceScalingPolicy scaling, int igniteStacks)
	{
		if (!source.GetWorld() || !lease || !lease->active) return;
		const auto targets = targeting::FindOpposingCombatants(*source.GetWorld(), source, origin, radius, false);
		auto damageTags = tags;
		if (cryoStacks > 0 && std::find(damageTags.begin(), damageTags.end(), DamageTypeSchema::Cryo) == damageTags.end()) damageTags.push_back(DamageTypeSchema::Cryo);
		for (const auto& target : targets)
		{
			if (!lease->active || source.GetIsPendingDestroy()) break;
			if (target->GetIsPendingDestroy()) continue;
			ApplyCombatDamage(*target, damage, &source, damageTags, MakeModuleGeneratedDamagePayload(cryoStacks, scaling, igniteStacks), {}, {}, DamageDeliveryType::Area, &delivery);
		}
	}
}

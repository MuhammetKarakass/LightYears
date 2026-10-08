#pragma once
#include "gameplay/damage/DamageContext.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include <SFML/System/Vector2.hpp>

namespace ly
{
	DamagePayload MakeModuleGeneratedDamagePayload(int cryoStacks = 0, DamageSourceScalingPolicy scaling = DamageSourceScalingPolicy::AlreadyResolved);
	void ApplyModuleGeneratedAreaDamage(Actor& delivery, Actor& source, const std::shared_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& origin, float radius, float damage, const List<GameplayTag>& tags, int cryoStacks = 0, DamageSourceScalingPolicy scaling = DamageSourceScalingPolicy::AlreadyResolved);
}

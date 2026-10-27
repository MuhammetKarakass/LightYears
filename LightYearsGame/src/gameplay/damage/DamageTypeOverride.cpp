#include "gameplay/damage/DamageTypeOverride.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include <algorithm>

namespace vega
{
	void ApplyDamageTypeOverride(List<GameplayTag>& damageTags, DamagePayload& payload, const GameplayTag& type)
	{
		if (!type.IsValid()) return;
		const auto has = [&damageTags](const GameplayTag& tag) { return std::find(damageTags.begin(), damageTags.end(), tag) != damageTags.end(); };
		const int stacks = std::max(0, (has(DamageTypeSchema::Thermal) ? payload.igniteStacks : 0) + (has(DamageTypeSchema::Cryo) ? payload.cryoBuildupPerHit : 0) + (has(DamageTypeSchema::Electric) ? payload.electricStacks : 0) + (has(DamageTypeSchema::Kinetic) ? payload.kineticStacks : 0));
		damageTags.erase(std::remove_if(damageTags.begin(), damageTags.end(), [](const GameplayTag& tag) { return tag.MatchesTag(DamageTypeSchema::Root); }), damageTags.end());
		damageTags.push_back(type);
		const auto defaults = DamageTypeSystem::BuildPayload({ type });
		payload.shieldDamageMultiplier = defaults.shieldDamageMultiplier; payload.shieldRegenerationDelay = defaults.shieldRegenerationDelay; payload.armorPenetration = defaults.armorPenetration;
		payload.igniteStacks = type == DamageTypeSchema::Thermal ? stacks : 0;
		payload.cryoBuildupPerHit = type == DamageTypeSchema::Cryo ? stacks : 0;
		payload.electricStacks = type == DamageTypeSchema::Electric ? stacks : 0;
		payload.kineticStacks = type == DamageTypeSchema::Kinetic ? stacks : 0;
	}
}

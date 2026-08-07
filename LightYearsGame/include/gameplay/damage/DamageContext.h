#pragma once

#include "framework/Core.h"

namespace ly
{
	class Actor;

	// CombatRuntime emits this shared event after damage has been resolved. It
	// belongs to the combat pipeline rather than to an ability-family contract.
	struct CombatEventSchema
	{
		inline static const GameplayTag OwnerDamageTaken{ "Event.Owner.DamageTaken" };
	};

	// Numeric damage-type behavior is carried with each hit. Values are resolved
	// once by DamageTypeSystem, so the combat pipeline does not need to know the
	// weapon or ability that produced the hit.
	struct DamagePayload
	{
		float shieldDamageMultiplier = 1.f;
		float shieldRegenerationDelay = 0.f;
		float armorPenetration = 0.f;
		int igniteStacks = 0;
		float burnDamagePerSecond = 0.f;
		float burnDuration = 0.f;
		int burnMaxStacks = 1;
		int cryoBuildupPerHit = 0;
		int cryoBuildupRequired = 1;
		float cryoBuildupDuration = 0.f;
		float cryoSlowPercent = 0.f;
		float cryoSlowDuration = 0.f;
		int electricStacks = 0;
		float electricDamageTakenMultiplierPerStack = 0.f;
		float electricDuration = 0.f;
		int electricMaxStacks = 1;
		bool canCrit = true;
		float criticalDamageMultiplier = 2.f;
	};

	struct DamageContext
	{
		Actor* source = nullptr;
		Actor* target = nullptr;
		float originalDamage = 0.f;
		float remainingDamage = 0.f;
		float absorbedDamage = 0.f;
		float mitigatedDamage = 0.f;
		float modifiedDamage = 0.f;
		float appliedDamage = 0.f;
		bool wasCritical = false;
		List<GameplayTag> damageTags;
		DamagePayload payload;
	};
}



#pragma once

#include "framework/Core.h"

namespace ly
{
	class Actor;

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
		List<GameplayTag> damageTags;
	};
}



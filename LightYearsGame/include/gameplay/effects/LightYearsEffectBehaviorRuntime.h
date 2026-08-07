#pragma once

#include "effects/ActiveGameplayEffect.h"
#include "effects/GameplayEffectBehaviorRuntime.h"
#include "framework/Actor.h"
#include "gameplay/damage/DamageContext.h"

namespace ly
{
	// Damage processing has two stable phases. It belongs to the game combat
	// pipeline, rather than to the ability-system component that hosts abilities.
	enum class IncomingDamagePhase
	{
		PreMitigation,
		Standard
	};

	// All game effect behaviors share one typed runtime. Keeping this alias here
	// lets damage, combat, and effect families register behavior without depending
	// on the component that happens to own an ability-system instance.
	using LightYearsEffectBehaviorRuntime =
		sas::GameplayEffectBehaviorRuntime<
			sas::ActiveGameplayEffect,
			Actor,
			DamageContext,
			IncomingDamagePhase
		>;

	// The runtime is process-wide because behavior registrations describe shipped
	// game content; individual actors only own active effect instances.
	LightYearsEffectBehaviorRuntime& GetEffectBehaviorRuntime();
}

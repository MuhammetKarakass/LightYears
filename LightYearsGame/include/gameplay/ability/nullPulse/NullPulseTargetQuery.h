#pragma once

#include "framework/Core.h"

namespace ly
{
	class AbilityWorldActor;
	class Actor;
	class World;

	namespace NullPulseTargetQuery
	{
		// Projectile classification is owned by the actor family. This query only
		// applies the pulse geometry and never knows Rocket/Gravity/Overdrive types.
		List<shared_ptr<AbilityWorldActor>> FindClearableProjectiles(
			World& world,
			const Actor& source,
			float radius
		);

		List<shared_ptr<Actor>> FindEnemyTargets(
			World& world,
			const Actor& source,
			float radius
		);
	}
}

#pragma once

#include "gameplay/projectile/ProjectileRelayPayload.h"

namespace ly
{
	class AbilityWorldActor;

	// A projectile family opts into Relay conversion through this capability.
	// Relay Prism depends on this contract instead of knowing Rocket, weapon,
	// or another ability's concrete projectile class.
	class ProjectileRelayParticipant
	{
	public:
		virtual ~ProjectileRelayParticipant() = default;

		virtual bool CanBeCapturedByRelay() const = 0;
		virtual bool BuildRelaySnapshot(ProjectileRelaySnapshot& snapshot) const = 0;
		virtual weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const = 0;
	};
}


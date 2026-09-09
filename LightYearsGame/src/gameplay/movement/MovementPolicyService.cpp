#include "gameplay/movement/MovementPolicyService.h"

#include "framework/Actor.h"
#include "gameplay/MovementComponent.h"
#include "spaceShip/SpaceShip.h"

namespace ly::movement
{
	bool MovementPolicyService::SetPolicy(
		Actor& target,
		const MovementPolicyRequest& request
	)
	{
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&target))
		{
			return ship->GetMovementComponent().ApplyMovementPolicy(request);
		}
		return false;
	}

	bool MovementPolicyService::ReleasePolicy(
		Actor& target,
		const MovementPolicySourceId& sourceId,
		MovementPolicyReleaseMode releaseMode,
		float normalizationDuration
	)
	{
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&target))
		{
			return ship->GetMovementComponent().ReleaseMovementPolicy(
				sourceId,
				releaseMode,
				normalizationDuration
			);
		}
		return false;
	}

	void MovementPolicyService::ClearPolicies(Actor& target)
	{
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&target))
		{
			ship->GetMovementComponent().ClearMovementPolicies();
		}
	}

	bool MovementPolicyService::SupportsPolicies(const Actor& target)
	{
		if (const SpaceShip* ship = dynamic_cast<const SpaceShip*>(&target))
		{
			return ship->GetMovementComponent().SupportsMovementPolicies();
		}
		return false;
	}
}

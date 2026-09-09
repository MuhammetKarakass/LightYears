#include "gameplay/movement/MovementInfluenceService.h"

#include "framework/Actor.h"
#include "gameplay/MovementComponent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

#include <cmath>

namespace ly::movement
{
	bool MovementInfluenceService::ApplyImpulse(
		Actor& target,
		const ImpulseRequest& request
	)
	{
		if (!CanReceiveExternalMovement(target) ||
			!std::isfinite(request.velocity.x) || !std::isfinite(request.velocity.y))
		{
			return false;
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&target))
		{
			ship->GetMovementComponent().ApplyInfluenceImpulse(request);
			return true;
		}

		// Compatibility path for movable combatants that have not opted into the
		// ship movement component yet. New ship abilities never use this path.
		target.SetVelocity(target.GetVelocity() + request.velocity);
		return true;
	}

	bool MovementInfluenceService::ApplyInstantAcceleration(
		Actor& target,
		const sf::Vector2f& acceleration,
		float deltaTime
	)
	{
		if (!CanReceiveExternalMovement(target) || !std::isfinite(acceleration.x) ||
			!std::isfinite(acceleration.y) || deltaTime <= 0.f)
		{
			return false;
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&target))
		{
			ship->GetMovementComponent().ApplyInfluenceAcceleration(
				acceleration,
				deltaTime
			);
			return true;
		}

		target.SetVelocity(target.GetVelocity() + acceleration * deltaTime);
		return true;
	}

	bool MovementInfluenceService::SetAccelerationSource(
		Actor& target,
		const AccelerationSourceRequest& request
	)
	{
		if (!CanReceiveExternalMovement(target) || request.sourceId == 0 ||
			!std::isfinite(request.acceleration.x) || !std::isfinite(request.acceleration.y))
		{
			return false;
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&target))
		{
			ship->GetMovementComponent().SetInfluenceAccelerationSource(request);
			return true;
		}
		return false;
	}

	bool MovementInfluenceService::SetForcedMovement(
		Actor& target,
		const ForcedMovementRequest& request
	)
	{
		if (!CanReceiveExternalMovement(target) || request.sourceId == 0 ||
			!std::isfinite(request.velocity.x) || !std::isfinite(request.velocity.y))
		{
			return false;
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&target))
		{
			ship->GetMovementComponent().SetInfluenceForcedMovement(request);
			return true;
		}
		return false;
	}

	void MovementInfluenceService::RemoveSource(
		Actor& target,
		MovementInfluenceSourceId sourceId
	)
	{
		if (sourceId == 0)
		{
			return;
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&target))
		{
			ship->GetMovementComponent().RemoveInfluenceSource(sourceId);
		}
	}

	bool MovementInfluenceService::CanReceiveExternalMovement(const Actor& target)
	{
		const Combatant* combatant = dynamic_cast<const Combatant*>(&target);
		return !combatant || !combatant->GetAbilitySystemComponent().HasOwnedTag(
			GameplayTags::State::ActionLock::ExternalMovement
		);
	}
}

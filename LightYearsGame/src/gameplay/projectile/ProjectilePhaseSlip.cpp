#include "gameplay/projectile/ProjectilePhaseSlip.h"
#include "player/PlayerSpaceShip.h"

namespace vega
{
	bool TryPassPhaseSlipContact(Actor& projectile, Actor* target)
	{
		auto* ship = dynamic_cast<PlayerSpaceShip*>(target);
		return ship && !ship->GetIsPendingDestroy() && ship->GetModuleApplier().GetEpicUtilityMovementRuntime().TryPassProjectile(projectile);
	}
}

#include "weapon/Shooter.h"

namespace ly
{
	Shooter::Shooter(Actor* owner)
		: mOwner{ owner }
	{
	}
	void Shooter::Shoot()
	{
		if (CanShoot() && !IsOnCooldown())
		{
			ShootImp();
		}
	}
}
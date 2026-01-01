#pragma once

#include "enemy/EnemySpaceShip.h"

namespace ly
{
	class BulletShooter;
	class TwinBlade : public EnemySpaceShip
	{
	public:
		TwinBlade(World* owningWorld, const ShipDefinition& shipDef,float weaponSpreadWidth=40.f);

		~TwinBlade();

		virtual void Tick(float deltaTime) override;

		virtual void Shoot() override;


	private:
		unique_ptr<BulletShooter> mShooterLeft;
		unique_ptr<BulletShooter> mShooterRight;
	};
}
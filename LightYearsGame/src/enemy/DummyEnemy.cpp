#include "enemy/DummyEnemy.h"

namespace ly
{
	DummyEnemy::DummyEnemy(World* owningWorld, const ShipDefinition& shipDef)
		: EnemySpaceShip(owningWorld, shipDef)
	{
		SetMovementMode(ShipMovementMode::ThrustDrift);
		SetVelocity({ 0.f, 0.f });
	}

	void DummyEnemy::Tick(float deltaTime)
	{
		SpaceShip::Tick(deltaTime);
		Shoot();
	}

	void DummyEnemy::Shoot()
	{
		GetAbilitySystemComponent().SetAbilitySlotInput(
			sas::AbilitySlot::PrimaryFire,
			true
		);
	}
}

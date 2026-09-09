#include "enemy/DummyEnemy.h"

namespace ly
{
	namespace
	{
		constexpr float DummyMaxShield = 50.f;

		ShipDefinition MakeDummyShipDefinition(const ShipDefinition& source)
		{
			ShipDefinition dummyDefinition = source;
			dummyDefinition.energyAttributes.baseMaxShield = DummyMaxShield;
			// The training target should have a deterministic shield value even if
			// its source definition later gains an energy-based shield contribution.
			dummyDefinition.energyAttributes.maxShieldPerMaxEnergy = 0.f;
			return dummyDefinition;
		}
	}

	DummyEnemy::DummyEnemy(World* owningWorld, const ShipDefinition& shipDef)
		: EnemySpaceShip(owningWorld, MakeDummyShipDefinition(shipDef))
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

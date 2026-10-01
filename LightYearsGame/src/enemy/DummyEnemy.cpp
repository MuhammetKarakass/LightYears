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
			// Keep the training target's shield fixed regardless of source ship data.
			return dummyDefinition;
		}
	}

	DummyEnemy::DummyEnemy(World* owningWorld, const ShipDefinition& shipDef)
		: SpaceShip(owningWorld, MakeDummyShipDefinition(shipDef))
	{
		SetMovementMode(ShipMovementMode::ThrustDrift);
		SetVelocity({ 0.f, 0.f });
	}

	void DummyEnemy::Tick(float deltaTime)
	{
		Shoot();
		SpaceShip::Tick(deltaTime);
		GetAbilitySystemComponent().SetAbilitySlotInput(
			sas::AbilitySlot::PrimaryFire,
			false
		);
	}

	void DummyEnemy::SetupCollisionLayers()
	{
		SetCollisionLayer(CollisionLayer::Enemy);
		SetCollisionMask(
			CollisionLayer::Player |
			CollisionLayer::FriendlySummon |
			CollisionLayer::PlayerBullet |
			CollisionLayer::RelayProjectile |
			CollisionLayer::Environment
		);
	}

	void DummyEnemy::Shoot()
	{
		GetAbilitySystemComponent().SetAbilitySlotInput(
			sas::AbilitySlot::PrimaryFire,
			true
		);
	}
}

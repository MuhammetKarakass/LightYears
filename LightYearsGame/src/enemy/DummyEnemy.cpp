#include "gameConfigs/CollisionLayers.h"
#include "enemy/DummyEnemy.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <framework/World.h>
#include <framework/reflection/Reflection.h>
#include <stdexcept>

namespace vega
{
	VEGA_REFLECT(DummyEnemy, SpaceShip)
	{
		VEGA_SPAWNABLE([](World& world)
		{
			const auto* ship = content::ShipContentCatalog::FindById("Ship.Enemy.RangeKeeper.Basic");
			if (!ship) throw std::runtime_error("Default dummy ship is unavailable");
			return world.SpawnActor<DummyEnemy>(*ship).lock();
		});
		VEGA_CATEGORY("Dummy");
		VEGA_PROPERTY("Ship", GetShip, SetShip).Tooltip("Construction only: changing Ship is refused; place a prefab with the desired catalog Ship instead.").Construction();
	}

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
		, mShip(shipDef.catalogId)
	{
		SetMovementMode(ShipMovementMode::ThrustDrift);
		SetVelocity({ 0.f, 0.f });
	}

	void DummyEnemy::SetShip(const std::string& ship)
	{
		if (ship != mShip) throw std::runtime_error("Ship is construction-only; place a prefab with the desired catalog Ship instead of changing an existing DummyEnemy.");
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
		SetCollisionLayer(GameCollision::Enemy);
		SetCollisionMask(
			GameCollision::Player |
			GameCollision::FriendlySummon |
			GameCollision::PlayerBullet |
			GameCollision::RelayProjectile |
			GameCollision::Environment
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

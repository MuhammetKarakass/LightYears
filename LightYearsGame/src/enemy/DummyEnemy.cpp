#include "gameConfigs/CollisionLayers.h"
#include "enemy/DummyEnemy.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <framework/World.h>
#include <framework/reflection/Reflection.h>
#include <framework/scene/SceneEntityComponent.h>
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
		type.Property("Ship", [](const Self& actor)
		{
			const auto* identity = actor.FindComponent<SceneEntityComponent>();
			return identity ? identity->spawnData.value("Ship", std::string{ "Ship.Enemy.RangeKeeper.Basic" })
				: std::string{ "Ship.Enemy.RangeKeeper.Basic" };
		}, [](Self& actor, const std::string& ship)
		{
			if (!content::ShipContentCatalog::FindById(ship)) throw std::runtime_error("Unknown dummy ship: " + ship);
			actor.GetOrAddComponent<SceneEntityComponent>().spawnData["Ship"] = ship;
		}).Category("Dummy").Tooltip("Construction only: reload the scene to apply a changed ship.");
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

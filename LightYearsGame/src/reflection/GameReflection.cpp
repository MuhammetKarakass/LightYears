#include "reflection/GameReflection.h"
#include <framework/reflection/Reflection.h>
#include "spaceShip/SpaceShip.h"
#include "environment/Asteroid.h"
#include "level/PlayerStart.h"
#include "level/SpriteActor.h"
#include "enemy/DummyEnemy.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/EnergyComponent.h"
#include "gameplay/MovementComponent.h"
#include "player/PlayerMovementComponent.h"
#include "player/PlayerSpaceShip.h"
#include "enemy/EnemyActor.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"
#include <framework/scene/Scene.h>

namespace vega
{
	void RegisterGameTypes()
	{
		auto& context = EngineContext::Current();
		reflect::RegisterEngineTypes(context.Types());
		if (context.SceneSerialization().spawnFactories.count("PlayerSpaceShip")) return;
		VEGA_REGISTER(SpaceShip);
		VEGA_REGISTER(Asteroid);
		VEGA_REGISTER(PlayerStart);
		VEGA_REGISTER(SpriteActor);
		VEGA_REGISTER(DummyEnemy);
		SceneSerializer::RegisterSpawnFactory("DummyEnemy", [](World& world, const nlohmann::json& properties)
		{
			const auto id = properties.value("Ship", std::string{ "Ship.Enemy.RangeKeeper.Basic" });
			const auto* ship = content::ShipContentCatalog::FindById(id);
			if (!ship) throw std::runtime_error("Unknown dummy ship: " + id);
			return world.SpawnActor<DummyEnemy>(*ship).lock();
		}, context);
		VEGA_REGISTER(HealthComponent);
		VEGA_REGISTER(ShieldComponent);
		VEGA_REGISTER(EnergyComponent);
		VEGA_REGISTER(MovementComponent);
		VEGA_REGISTER(PlayerMovementComponent);
		VEGA_REGISTER(PlayerSpaceShip);
		SceneSerializer::RegisterSpawnFactory("PlayerSpaceShip", [](World& world, const nlohmann::json& properties)
		{
			const auto id = properties.value("Ship", std::string{ DefaultPlayerShipId });
			const auto* ship = content::ShipContentCatalog::FindById(id);
			if (!ship || id.rfind("Ship.Player.", 0) != 0) throw std::runtime_error("Unknown player ship: " + id);
			return world.SpawnActor<PlayerSpaceShip>(*ship).lock();
		}, context);
		VEGA_REGISTER(EnemyActor);
		SceneSerializer::RegisterSpawnFactory("EnemyActor", [](World& world, const nlohmann::json& properties)
		{
			const auto id = properties.value("Enemy ID", std::string{ EnemyIds::ApproachGunnerBasic });
			auto actor = content::ConstructEnemy(world, id).lock();
			if (!actor) throw std::runtime_error("Cannot construct catalog enemy: " + id);
			return actor;
		}, context);
	}
}

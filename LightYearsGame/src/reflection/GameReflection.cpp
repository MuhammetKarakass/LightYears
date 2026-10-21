#include "reflection/GameReflection.h"
#include <framework/reflection/Reflection.h>
#include "spaceShip/SpaceShip.h"
#include "environment/Asteroid.h"
#include "level/PlayerStart.h"
#include "level/SpriteActor.h"
#include "level/authoring/ArenaSetupActor.h"
#include "level/authoring/EncounterSetupActor.h"
#include "level/authoring/CampaignSetupActor.h"
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
	void RegisterGameTypes(reflect::TypeRegistry& registry)
	{
		reflect::RegisterEngineTypes(registry);
		// The last type registered below; finding it means this registry already holds the whole set.
		if (registry.Find("CampaignSetupActor")) return;
		VEGA_REGISTER(SpaceShip, registry);
		VEGA_REGISTER(Asteroid, registry);
		VEGA_REGISTER(PlayerStart, registry);
		VEGA_REGISTER(SpriteActor, registry);
		VEGA_REGISTER(DummyEnemy, registry);
		VEGA_REGISTER(HealthComponent, registry);
		VEGA_REGISTER(ShieldComponent, registry);
		VEGA_REGISTER(EnergyComponent, registry);
		VEGA_REGISTER(MovementComponent, registry);
		VEGA_REGISTER(PlayerMovementComponent, registry);
		VEGA_REGISTER(PlayerSpaceShip, registry);
		VEGA_REGISTER(EnemyActor, registry);
		VEGA_REGISTER(ArenaBoundarySettings, registry);
		VEGA_REGISTER(ArenaRespawnSettings, registry);
		VEGA_REGISTER(ArenaMusicSettings, registry);
		VEGA_REGISTER(ArenaEncounterProgression, registry);
		VEGA_REGISTER(ArenaWaveTemplate, registry);
		VEGA_REGISTER(ArenaWaveEntry, registry);
		VEGA_REGISTER(ArenaSetupActor, registry);
		VEGA_REGISTER(EncounterSetupActor, registry);
		VEGA_REGISTER(CampaignStageEntry, registry);
		VEGA_REGISTER(VanguardStageSettings, registry);
		VEGA_REGISTER(TwinBladeStageSettings, registry);
		VEGA_REGISTER(HexagonStageSettings, registry);
		VEGA_REGISTER(ChaosStageSettings, registry);
		VEGA_REGISTER(CampaignSetupActor, registry);
	}

	void RegisterGameSpawnFactories(EngineContext& context)
	{
		if (context.SceneSerialization().spawnFactories.count("PlayerSpaceShip")) return;
		SceneSerializer::RegisterSpawnFactory("DummyEnemy", [](World& world, const nlohmann::json& properties)
		{
			const auto id = properties.value("Ship", std::string{ "Ship.Enemy.RangeKeeper.Basic" });
			const auto* ship = content::ShipContentCatalog::FindById(id);
			if (!ship) throw std::runtime_error("Unknown dummy ship: " + id);
			return world.SpawnActor<DummyEnemy>(*ship).lock();
		}, context);
		SceneSerializer::RegisterSpawnFactory("PlayerSpaceShip", [](World& world, const nlohmann::json& properties)
		{
			const auto id = properties.value("Ship", std::string{ DefaultPlayerShipId });
			const auto* ship = content::ShipContentCatalog::FindById(id);
			if (!ship || id.rfind("Ship.Player.", 0) != 0) throw std::runtime_error("Unknown player ship: " + id);
			return world.SpawnActor<PlayerSpaceShip>(*ship).lock();
		}, context);
		SceneSerializer::RegisterSpawnFactory("EnemyActor", [](World& world, const nlohmann::json& properties)
		{
			const auto id = properties.value("Enemy ID", std::string{ EnemyIds::ApproachGunnerBasic });
			auto actor = content::ConstructEnemy(world, id).lock();
			if (!actor) throw std::runtime_error("Cannot construct catalog enemy: " + id);
			return actor;
		}, context);
	}

	void RegisterGameSceneTypes(EngineContext& context)
	{
		RegisterGameTypes(context.Types());
		RegisterGameSpawnFactories(context);
	}
}

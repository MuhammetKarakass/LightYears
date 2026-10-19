#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace vega
{
	class EngineContext;
	class World;
	class PlayerSpaceShip;
	class EnemyActor;
	class DummyEnemy;

	// Validates authored data without constructing an actor during catalog bootstrap.
	bool ValidateShipPrefab(EngineContext& context, const std::filesystem::path& assetRoot, const std::string& prefabPath, const std::string& shipId, std::string* failureReason = nullptr);
	std::weak_ptr<PlayerSpaceShip> SpawnShipPrefab(World& world, const std::string& prefabPath, const std::string& shipId, std::string* failureReason = nullptr);
	bool ValidateEnemyPrefab(EngineContext& context, const std::filesystem::path& assetRoot, const std::string& prefabPath, const std::string& enemyId, std::string* failureReason = nullptr);
	std::weak_ptr<EnemyActor> SpawnEnemyPrefab(World& world, const std::string& prefabPath, const std::string& enemyId, std::string* failureReason = nullptr);
	bool ValidateDummyPrefab(EngineContext& context, const std::filesystem::path& assetRoot, const std::string& prefabPath, const std::string& shipId, std::string* failureReason = nullptr);
	std::weak_ptr<DummyEnemy> SpawnDummyPrefab(World& world, const std::string& prefabPath, const std::string& shipId, std::string* failureReason = nullptr);
}

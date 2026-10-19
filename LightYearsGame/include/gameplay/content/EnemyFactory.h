#pragma once

#include <SFML/System/Vector2.hpp>
#include <string>

#include "framework/Core.h"
#include "gameplay/enemy/EnemySpawnContext.h"

namespace vega
{
	class EnemyActor;
	class World;

	namespace content
	{
		// Direct construction for scene factories and definitions without a prefab. Does not begin play.
		weak_ptr<EnemyActor> ConstructEnemy(World& world, const std::string& enemyId, float encounterDamageMultiplier = 1.f, const EnemySpawnContext& spawnContext = {});
		weak_ptr<EnemyActor> SpawnEnemy(World& world, const std::string& enemyId, const sf::Vector2f& location, float encounterDamageMultiplier = 1.f);
		weak_ptr<EnemyActor> SpawnEnemy(World& world, const std::string& enemyId, const sf::Vector2f& location, float encounterDamageMultiplier, const EnemySpawnContext& spawnContext);
	}
}

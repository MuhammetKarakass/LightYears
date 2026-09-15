#pragma once

#include <SFML/System/Vector2.hpp>
#include <string>

#include "framework/Core.h"
#include "gameplay/enemy/EnemySpawnContext.h"

namespace ly
{
	class EnemyActor;
	class World;

	namespace content
	{
		weak_ptr<EnemyActor> SpawnEnemy(World& world, const std::string& enemyId, const sf::Vector2f& location, float encounterDamageMultiplier = 1.f);
		weak_ptr<EnemyActor> SpawnEnemy(World& world, const std::string& enemyId, const sf::Vector2f& location, float encounterDamageMultiplier, const EnemySpawnContext& spawnContext);
	}
}

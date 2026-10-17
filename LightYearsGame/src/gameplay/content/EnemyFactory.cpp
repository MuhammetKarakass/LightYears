#include "gameplay/content/EnemyFactory.h"
#include "framework/debug/Profiler.h"

#include "enemy/EnemyActor.h"
#include "framework/World.h"
#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/EnemyContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"

#include <cmath>

namespace vega::content
{
	weak_ptr<EnemyActor> SpawnEnemy(World& world, const std::string& enemyId, const sf::Vector2f& location, float encounterDamageMultiplier)
	{
		return SpawnEnemy(world, enemyId, location, encounterDamageMultiplier, {});
	}

	weak_ptr<EnemyActor> SpawnEnemy(
		World& world,
		const std::string& enemyId,
		const sf::Vector2f& location,
		float encounterDamageMultiplier,
		const EnemySpawnContext& spawnContext)
	{
		VEGA_PROFILE_SCOPE("Enemy.Spawn");
		if (spawnContext.level < 1)
		{
			VEGA_GAME_ERROR("Cannot spawn enemy '%s': level must be at least one.", enemyId.c_str());
			return {};
		}
		if (!std::isfinite(encounterDamageMultiplier) || encounterDamageMultiplier < 0.f)
		{
			VEGA_GAME_ERROR("Cannot spawn enemy '%s': invalid encounter damage multiplier.", enemyId.c_str());
			return {};
		}
		const EnemyDefinition* enemy = EnemyContentCatalog::FindById(enemyId);
		if (!enemy)
		{
			VEGA_GAME_ERROR("Cannot spawn unknown enemy definition '%s'.", enemyId.c_str());
			return {};
		}
		const ShipDefinition* ship = ShipContentCatalog::FindById(enemy->shipId);
		const EnemyCombatProfile* combat = EnemyCombatProfileCatalog::FindById(enemy->combatProfileId);
		const EnemyBehaviorProfile* behavior = EnemyContentCatalog::FindBehaviorById(enemy->behaviorProfileId);
		if (!ship || !combat || !behavior)
		{
			VEGA_GAME_ERROR("Cannot spawn enemy '%s': referenced content is unavailable.", enemyId.c_str());
			return {};
		}
		weak_ptr<EnemyActor> spawned = world.SpawnActor<EnemyActor>(
			*ship,
			enemy->id,
			*combat,
			*behavior,
			encounterDamageMultiplier,
			spawnContext
		);
		if (auto actor = spawned.lock()) actor->SetActorLocation(location);
		return spawned;
	}
}

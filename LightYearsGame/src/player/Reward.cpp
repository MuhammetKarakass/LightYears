#include "player/Reward.h"
#include "player/PlayerSpaceShip.h"
#include "weapon/ThreeWayShooter.h"
#include "weapon/FrontalWiper.h"
#include <framework/World.h>
#include "player/PlayerManager.h"
#include "gameConfigs/GameplayConfig.h"


namespace ly
{
	Reward::Reward(World* owningWorld, const std::string& texturePath, RewardFunc rewardFunc, float speed) :
		Actor{ owningWorld, texturePath },
		mSpeed{ speed },
		mRewardFunc{ rewardFunc }
	{
	}

	void Reward::BeginPlay()
	{
		Actor::BeginPlay();
		SetEnablePhysics(true);
		SetupCollisionProperties();
	}

	void Reward::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);
		AddActorLocalLocationOffset({ 0.f, deltaTime * mSpeed });
	}
	void Reward::SetupCollisionProperties()
	{
		SetCollisionLayer(CollisionLayer::Powerup);
		SetCollisionMask(CollisionLayer::Player);
	}

	void Reward::OnActorBeginOverlap(Actor* other)
	{
		if (!other || other->GetIsPendingDestroy())
			return;
		if (!PlayerManager::GetPlayerManager().GetPlayer())
			return;
		weak_ptr<PlayerSpaceShip> playerSpaceShip = PlayerManager::GetPlayerManager().GetPlayer()->GetCurrentSpaceShip();
		if (playerSpaceShip.expired() || playerSpaceShip.lock()->GetIsPendingDestroy())
			return;

		if (playerSpaceShip.lock()->GetUniqueID() == other->GetUniqueID())
		{
			mRewardFunc(playerSpaceShip.lock().get());
			Destroy();
		}
	}
	weak_ptr<Reward> CreateRewardHealth(World* world)
	{
		return CreateReward(world, "SpaceShooterRedux/PNG/pickups/pill_green.png", RewardHealth);
	}
	weak_ptr<Reward> CreateRewardThreeWayShooter(World* world)
	{
		return CreateReward(world, "SpaceShooterRedux/PNG/pickups/three_shooter_pickup.png", RewardThreeWayShooter);
	}
	weak_ptr<Reward> CreateRewardFrontalWiper(World* world)
	{
		return CreateReward(world, "SpaceShooterRedux/PNG/pickups/front_row_shooter_pickup.png", RewardFrontalWiper);
	}
	weak_ptr<Reward> CreateRewardLife(World* world)
	{
		return CreateReward(world, "SpaceShooterRedux/PNG/pickups/playerLife1_blue.png", RewardLife);
	}
	weak_ptr<Reward> CreateReward(World* world, const std::string& texturePath, RewardFunc rewardFunc, float speed)
	{
		weak_ptr<Reward> reward = world->SpawnActor<Reward>(texturePath, rewardFunc);
		return reward;
	}
	void RewardHealth(PlayerSpaceShip* player)
	{
		static float healAmount = 25.f;

		if (player && !player->GetIsPendingDestroy())
		{
			player->GetHealthComponent().ChangeHealth(healAmount);
		}
	}
	void RewardThreeWayShooter(PlayerSpaceShip* player)
	{
		if (player && !player->GetIsPendingDestroy())
		{
			player->SetShooter(std::make_unique<ThreeWayShooter>(player, GameData::Laser_Blue_BulletDef, 0.4f, sf::Vector2f{ 0.f,50.f }));
		}
	}
	void RewardFrontalWiper(PlayerSpaceShip* player)
	{
		if (player && !player->GetIsPendingDestroy())
		{
			player->SetShooter(std::make_unique<FrontalWiper>(player, GameData::Laser_Blue_BulletDef, 0.5f, sf::Vector2f{ 0.f,50.f }));
		}
	}
	void RewardLife(PlayerSpaceShip* player)
	{
		if (!PlayerManager::GetPlayerManager().GetPlayer())
			return;
		PlayerManager::GetPlayerManager().GetPlayer()->AddLifeCount(1);
	}
}
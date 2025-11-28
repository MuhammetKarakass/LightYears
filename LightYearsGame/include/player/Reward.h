#pragma once 

#include <functional>
#include <framework/Actor.h>

namespace ly
{
	class PlayerSpaceShip;
	class World;
	class Reward;

	using RewardFunc = std::function<void(PlayerSpaceShip*)>;
	using RewardFactoryFunc = std::function<weak_ptr<Reward>(World*)>;

	// Weighted reward entry (factory + spawn weight)
	struct WeightedReward
	{
		RewardFactoryFunc factory;
		float weight;  // Spawn probability weight (0.0 - 1.0)
	};

	class Reward : public Actor
	{
	public:
		Reward(World* owningWorld, const std::string& texturePath, RewardFunc rewardFunc, float speed=-200.f);

		virtual void BeginPlay() override;
		virtual void Tick(float deltaTime) override;
		virtual void SetupCollisionProperties();

	private:
		virtual void OnActorBeginOverlap(Actor* other) override;

		float mSpeed;
		RewardFunc mRewardFunc;
	};

	weak_ptr<Reward> CreateRewardHealth(World* world);
	weak_ptr<Reward> CreateRewardThreeWayShooter(World* world);
	weak_ptr<Reward> CreateRewardFrontalWiper(World* world);
	weak_ptr<Reward> CreateRewardLife(World* world);

	weak_ptr<Reward> CreateReward(World* world, const std::string& texturePath, RewardFunc rewardFunc, float speed=200.f);

	void RewardHealth(PlayerSpaceShip* player);
	void RewardThreeWayShooter(PlayerSpaceShip* player);
	void RewardFrontalWiper(PlayerSpaceShip* player);
	void RewardLife(PlayerSpaceShip* player);
}
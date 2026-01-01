#include "environment/Asteroid.h"

namespace ly
{
	Asteroid::Asteroid(World* owningWorld, const sf::Vector2f& velocity, float health, float damage):
		Actor{owningWorld},
		mVelocity{ velocity },
		mHealthComponent{ health, health },
		mDamage{ damage },
		mBlinkColor{ 255, 0, 0, 255 },
		mBlinkTime{ 0.f },
		mBlinkDuration{ .1f },
		mRotationSpeed{ 10.f }
	{
		mTexturePaths = { "SpaceShooterRedux/PNG/Meteors/meteorBrown_big1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_big2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_med1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_big1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_med1.png" };

		if (!mTexturePaths.empty())
		{
			auto randIndex = RandRange(0, (int)mTexturePaths.size() - 1);
			SetTexture(mTexturePaths[randIndex]);
		}
		float randomScale = RandRange(0.7f, 1.3f);
		if (auto& sprite = GetSprite())
		{
			sprite->setScale({ randomScale,randomScale });
		}

		mRewards = 
		{
			{CreateRewardHealth, 0.35f},
			{CreateRewardThreeWayShooter, 0.2f},
			{CreateRewardFrontalWiper, 0.15f},
			{CreateRewardLife, 0.1f}
		};
	}
	void Asteroid::BeginPlay()
	{
		Actor::BeginPlay();

		mHealthComponent.onHealthChanged.BindAction(GetWeakPtr(), &Asteroid::OnHealthChanged);
		mHealthComponent.onTakenDamage.BindAction(GetWeakPtr(), &Asteroid::OnTakenDamage);
		mHealthComponent.onHealthEmpty.BindAction(GetWeakPtr(), &Asteroid::Blow);

		SetupCollisionLayers();

		SetEnablePhysics(true);
		
		float speedMultiplier = RandRange(0.8f, 1.2f);
		mVelocity *= speedMultiplier;
		SetEnablePhysics(true);
		float radius = GetActorGlobalBounds().size.x * 0.5f;
		SetCollisionRadius(radius);
	}
	void Asteroid::SetupCollisionLayers()
	{
		SetCollisionLayer(CollisionLayer::Enemy);
		SetCollisionMask(CollisionLayer::PlayerBullet | CollisionLayer::Player);
	}
	void Asteroid::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);
		AddActorLocationOffset(mVelocity * deltaTime);
		AddActorRotationOffset(mRotationSpeed * deltaTime);
		UpdateBlink(deltaTime);

		if ((IsActorOutOfWindow(150.f)))
		{
			Destroy();
		}
	}
	void Asteroid::ApplyDamage(float amt)
	{
		mHealthComponent.ChangeHealth(-amt);
		LOG("Health: %f", mHealthComponent.GetHealth());
	}
	void Asteroid::OnActorBeginOverlap(Actor* otherActor)
	{
		Actor::OnActorBeginOverlap(otherActor);
		if (!otherActor) return;
		if (otherActor->GetCollisionLayer() == CollisionLayer::Player)
		{
			otherActor->ApplyDamage(mDamage);
		}
	}

	void Asteroid::Blink()
	{
		mBlinkTime = mBlinkDuration;
	}

	void Asteroid::UpdateBlink(float deltaTime)
	{
		if (mBlinkTime > 0)
		{
			mBlinkTime -= deltaTime;
			mBlinkTime = mBlinkTime > 0 ? mBlinkTime : 0.f;
			GetSprite().value().setColor(LerpColor(sf::Color::White, mBlinkColor, mBlinkTime/mBlinkDuration));
		}
	}

	void Asteroid::OnHealthChanged(float amt, float health, float maxHealth)
	{

	}

	void Asteroid::OnTakenDamage(float amt, float health, float maxHealth)
	{
		Blink();
	}

	void Asteroid::Blow()
	{
		Explosion::SpawnExplosion(GetWorld(), GetActorLocation(), GetExplosionType());
		SpawnReward();
		Destroy();
	}

	void Asteroid::SpawnReward()
	{
		float totalWeight = 0.0f;
		for(const auto& reward : mRewards)
		{
			totalWeight += reward.weight;
		}

		float randValue = RandRange(0.0f, 1.0f);

		float currentWeight = 0.0f;
		for (const auto& reward : mRewards)
		{
			currentWeight += reward.weight;
			if (randValue <= currentWeight)
			{
				// Spawn this reward
				weak_ptr<Reward> spawnedReward = reward.factory(GetWorld());
				if (auto rewardPtr = spawnedReward.lock())
				{
					rewardPtr->SetActorLocation(GetActorLocation());
				}
				return;
			}
		}
	}
}
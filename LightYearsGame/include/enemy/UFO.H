#pragma once
#include "enemy/EnemySpaceShip.h"

namespace ly
{
	class BulletShooter;
	class UFO : public EnemySpaceShip
	{
	public:
		UFO(World* owningWorld, const sf::Vector2f& velocity, const std::string& texturePath = "SpaceShooterRedux/PNG/Enemies/ufoBlack.png", float rotationSpeed = 40.f);
		virtual ~UFO();

		virtual void Tick(float deltaTime) override;
		
		// UFO'lar özel collision setup'ý kullanýr
		virtual void SetupCollisionLayers() override;
		
		// UFO'lar birbirleriyle çarpýþabilir ve sekebilir
		virtual void OnActorBeginOverlap(Actor* otherActor) override;

	protected:
		static List<WeightedReward> GetDefaultRewards();

	private:
		virtual void Shoot() override;
		void CheckBounce(); // X ekseninde sekme kontrolü

		unique_ptr<BulletShooter> mShooter1;
		unique_ptr<BulletShooter> mShooter2;
		unique_ptr<BulletShooter> mShooter3;

		float mRotationSpeed;
	};
}
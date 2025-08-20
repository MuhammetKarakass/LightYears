#include "enemy/EnemySpaceShip.h"

namespace ly
{
	EnemySpaceShip::EnemySpaceShip(World* owningWorld, const std::string& texturePath, float collisionDamage):
		SpaceShip(owningWorld, texturePath),
		mCollisionDamage(collisionDamage)
	{
	}
	void EnemySpaceShip::Tick(float deltaTime)
	{
		SpaceShip::Tick(deltaTime);  // Base class tick'i çaðýr

		if (IsActorOutOfWindow(GetActorGlobalBounds().size.x))
		{
			Destroy();
		}
	}

	void EnemySpaceShip::SetupCollisionLayers()
	{
		LOG("EnemySpaceShip::OnActorBeginOverlap called with actor: ");
		SetCollisionLayer(CollisionLayer::Enemy);  
		SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
	}

	void EnemySpaceShip::OnActorBeginOverlap(Actor* other)
	{
		SpaceShip::OnActorBeginOverlap(other);  // Base class'ý çaðýr
		if (other == nullptr) return;
		if (GetCanCollide())
		{
			other->ApplyDamage(mCollisionDamage);
		}
	}
}
#pragma once
#include <spaceShip/SpaceShip.h>

namespace ly
{
	class EnemySpaceShip : public SpaceShip
	{
	public:
		EnemySpaceShip(World* owningWorld, const std::string& texturePath, float collisionDamage=75.f);


		virtual void Tick(float deltaTime) override;
		virtual void SetupCollisionLayers() override;

	private:
		virtual void OnActorBeginOverlap(Actor* other) override;
		float mCollisionDamage;
	};
}
#pragma once
#include "enemy/EnemySpaceShip.h"
#include "gameplay/ability/AbilitySystem.h"

namespace ly
{
	class UFO : public EnemySpaceShip
	{
	public:
		UFO(World* owningWorld, const ShipDefinition& shipDef, const sf::Vector2f& velocity, float rotationSpeed = 40.f);
		virtual ~UFO();

		virtual void Tick(float deltaTime) override;

		// UFOs use custom collision setup.
		virtual void SetupCollisionLayers() override;

		// UFOs can collide with each other and bounce.
		virtual void OnActorBeginOverlap(Actor* otherActor) override;

	protected:
		static List<WeightedReward> GetDefaultRewards();

	private:
		virtual void Shoot() override;
		void CheckBounce(); // Checks bounce on the X axis.

		AbilitySystem mAbilitySystem;

		float mRotationSpeed;
	};
}

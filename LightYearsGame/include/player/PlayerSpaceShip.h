#pragma once 
#include "spaceShip/SpaceShip.h"
#include "framework/Core.h"

namespace ly
{
	class World;
	class Shooter;
	class PlayerSpaceShip : public SpaceShip
	{
	public:

		PlayerSpaceShip(World* owningWorld,const std::string& texturePath = "SpaceShooterRedux/PNG/playerShip1_blue.png");

		virtual void Tick(float deltaTime) override;
		void SetSpeed(float speed) { mSpeed = speed; }
		float GetSpeed()const { return mSpeed; }

		void SetShooter(unique_ptr<Shooter>&& shooter);

		// Player gemisi için katman/mask ayarý
		virtual void SetupCollisionLayers() override;
		
		// Çarpýþma olayýný yakala
		virtual void OnActorBeginOverlap(Actor* otherActor) override;

	private:
		void SetInput();
		void ConsumeInput();
		void NormalizeInput();
		void ClampInputOnEdge();
		virtual void Shoot() override;


		float mSpeed;
		sf::Vector2f mMoveInput;

		unique_ptr<Shooter> mShooter;
	};
}
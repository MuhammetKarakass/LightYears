#pragma once 
#include "spaceShip/SpaceShip.h"

namespace ly
{
	class World;
	class BulletShooter;
	class PlayerSpaceShip : public SpaceShip
	{
	public:

		PlayerSpaceShip(World* owningWorld,const std::string& texturePath = "SpaceShooterRedux/PNG/playerShip1_blue.png");

		virtual void Tick(float deltaTime) override;
		void SetSpeed(float speed) { mSpeed = speed; }
		float GetSpeed()const { return mSpeed; }

	private:
		void SetInput();
		void ConsumeInput();
		void NormalizeInput();
		void ClampInputOnEdge();
		virtual void Shoot() override;


		float mSpeed;
		sf::Vector2f mMoveInput;

		unique_ptr<BulletShooter> mShooter;
	};
}
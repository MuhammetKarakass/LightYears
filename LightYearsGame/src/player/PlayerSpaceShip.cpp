#include "player/PlayerSpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "weapon/BulletShooter.h"

namespace ly
{
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld,const std::string& texturePath)
		:SpaceShip(owningWorld, texturePath), mSpeed(200.f), mMoveInput{ 0.f, 0.f },
		mShooter{new BulletShooter(this, 0.2f)}
	{

	}
	void PlayerSpaceShip::Tick(float deltaTime)
	{
		SpaceShip::Tick(deltaTime);
		SetInput();
		ConsumeInput();
	}
	void PlayerSpaceShip::SetInput()
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)|| sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
		{
			mMoveInput.y = -1.f;
		}

		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
		{
			mMoveInput.y = 1.f;
		}

		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
		{
			mMoveInput.x = -1.f;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
		{
			mMoveInput.x = 1.f;
		}
		ClampInputOnEdge();
		NormalizeInput();
	}
	void PlayerSpaceShip::ConsumeInput()
	{
		SetVelocity(mMoveInput * mSpeed);
		mMoveInput.x = 0.f;
		mMoveInput.y = 0.f;
	}
	void PlayerSpaceShip::NormalizeInput()
	{
		NormalizeVector(mMoveInput);
	}

	void PlayerSpaceShip::ClampInputOnEdge()
	{
		if(GetActorLocation().x-40.f<=0.f && mMoveInput.x==-1.f)
		{
			mMoveInput.x = 0.f;
		}
		else if (GetActorLocation().x + 40.f >= GetWindowSize().x && mMoveInput.x == 1.f)
		{
			mMoveInput.x = 0.f;
		}

		if (GetActorLocation().y - 40.f <= 0.f && mMoveInput.y == -1.f)
		{
			mMoveInput.y = 0.f;
		}
		else if (GetActorLocation().y + 40.f >= GetWindowSize().y && mMoveInput.y == 1.f)
		{
			mMoveInput.y = 0.f;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
		{
			Shoot();
		}
	}

	void PlayerSpaceShip::Shoot()
	{
		if(mShooter)
		{
			mShooter->Shoot();
		}
	}

}
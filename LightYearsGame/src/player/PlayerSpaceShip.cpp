#include "player/PlayerSpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>

namespace ly
{
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld,const std::string& texturePath)
		:SpaceShip(owningWorld, texturePath), mSpeed(200.f), mMoveInput{ 0.f, 0.f }
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
		LOG("NormalizeInput: %f , mMoveInput: %f ", mMoveInput.x,mMoveInput.y);
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
	}

}
#include "framework/BackGroundActor.h"
#include "framework/World.h"

namespace ly
{
	BackGroundActor::BackGroundActor(World* owningWorld, const std::string& texturePath, const sf::Vector2f& velocity):
		Actor(owningWorld, texturePath),
		mOriginalVelocity{ velocity },
		mLeftShift{ 0.f },
		mTopShift{ 0.f },
		mSlowMotionScale{ 0.5f },
		mPaused{ false }
	{
		SetEnablePhysics(false);
		auto windowSize = owningWorld->GetWindowSize();
		GetSprite().value().setOrigin({0.f,0.f});
		GetSprite().value().setTextureRect(sf::IntRect{ sf::Vector2i{0,0},sf::Vector2i{(int)windowSize.x,(int)windowSize.y}});
		SetTextureRepeated(true);
		SetTickWhenPaused(true);
		SetVelocity(velocity);
	}
	void BackGroundActor::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);


		mLeftShift += mVelocity.x * deltaTime;
		mTopShift += mVelocity.y * deltaTime;

		sf::IntRect currentRect = GetSprite().value().getTextureRect();
		currentRect.position.x = mLeftShift;
		currentRect.position.y = mTopShift;

		GetSprite().value().setTextureRect(currentRect);
	}

	void BackGroundActor::SetPaused(bool paused)
	{
		if(paused== mPaused)
			return;

		mPaused = paused;

		if (paused)
		{
			mOriginalVelocity = mVelocity;
			mVelocity = mVelocity * mSlowMotionScale;
		}
		else
		{
			mVelocity = mOriginalVelocity;
		}
	}
}
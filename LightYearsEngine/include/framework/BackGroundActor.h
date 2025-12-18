#pragma once

#include "framework/Actor.h"

namespace ly
{
	class BackGroundActor : public Actor
	{
	public:
		BackGroundActor(World* owningWorld, const std::string& texturePath, const sf::Vector2f& velocity = sf::Vector2f(0.f, -75.f));
		virtual void Tick(float deltaTime) override;

		void SetVelocity(const sf::Vector2f& velocity) { mVelocity = velocity; }
		sf::Vector2f GetVelocity() const { return mVelocity; }

		void SetPaused(bool paused);
		bool IsPaused() const { return mPaused; };

	private:
		sf::Vector2f mVelocity;
		sf::Vector2f mOriginalVelocity;
		float mLeftShift;
		float mTopShift;
		float mSlowMotionScale;
		bool mPaused;
	};
}
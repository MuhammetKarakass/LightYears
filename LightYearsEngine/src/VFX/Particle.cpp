#include "VFX/Particle.h"
#include "framework/MathUtility.h"
#include "framework/PerfMonitor.h"

namespace ly
{
	Particle::Particle(World* owningWorld, const std::string& texturePath)
		: Actor{ owningWorld, texturePath }, 
		mLifeTime{4.f }, mTimer{}
	{
		ly::perf::IncParticles();
	}

	void Particle::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);

		Move(deltaTime);
		Fade(deltaTime);

		if (mTimer.getElapsedTime().asSeconds() >= mLifeTime)
		{
			Destroy();
		}
	}

	void Particle::RandomVelocity(float minSpeed, float maxSpeed)
	{
		mVelocity = RandomUnitVector() * RandRange(minSpeed, maxSpeed);
	}

	void Particle::RandomSize(float min, float max)
	{
		float randScale = RandRange(min, max);
		GetSprite().value().setScale({ randScale, randScale });
	}

	void Particle::RandomLifeTime(float min, float max)
	{
		mLifeTime = RandRange(min, max);
	}

	void Particle::Move(float deltaTime)
	{
		AddActorLocationOffset(mVelocity * deltaTime);
	}

	void Particle::Fade(float deltaTime)
	{
		float elapsedTime = mTimer.getElapsedTime().asSeconds();
		GetSprite().value().setColor(LerpColor(GetSprite().value().getColor(), sf::Color{255,255,255,0}, elapsedTime / mLifeTime));
		GetSprite().value().setScale(LerpVector(GetSprite().value().getScale(), sf::Vector2f{0.f,0.f}, elapsedTime / mLifeTime));
	}

	void Particle::Destroy()
	{
		ly::perf::DecParticles();
		Actor::Destroy();
	}

}
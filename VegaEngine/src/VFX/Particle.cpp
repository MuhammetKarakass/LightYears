#include "VFX/Particle.h"
#include "framework/MathUtility.h"
#include "framework/SpriteComponent.h"
#include "framework/PerfMonitor.h"

namespace vega
{
	Particle::Particle(World* owningWorld, const std::string& texturePath)
		: Actor{ owningWorld, texturePath }, 
		mLifeTime{4.f }, mTimer{}
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		vega::perf::IncParticles();
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
		if (SpriteComponent* spriteComponent = FindComponent<SpriteComponent>())
		{
			spriteComponent->GetSprite().value().setScale({ randScale, randScale });
		}
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
		SpriteComponent* spriteComponent = FindComponent<SpriteComponent>();
		if (!spriteComponent) return;
		sf::Sprite& sprite = spriteComponent->GetSprite().value();
		sprite.setColor(LerpColor(sprite.getColor(), sf::Color{255,255,255,0}, elapsedTime / mLifeTime));
		sprite.setScale(LerpVector(sprite.getScale(), sf::Vector2f{0.f,0.f}, elapsedTime / mLifeTime));
	}

	void Particle::Destroy()
	{
		vega::perf::DecParticles();
		Actor::Destroy();
	}

}

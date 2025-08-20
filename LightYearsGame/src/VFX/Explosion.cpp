#include "framework/MathUtility.h"
#include "framework/World.h"
#include "VFX/Explosion.h"
#include "VFX/Particle.h"

namespace ly
{
	void Explosion::SpawnExplosion(World* world, const sf::Vector2f& location, ExplosionType type)
	{
		ExplosionParams params = GetPreset(type);
		SpawnExplosionWithParams(world, location, params);
	}

	void Explosion::SpawnExplosion(World* world, const sf::Vector2f& location, const ExplosionParams& params)
	{
		SpawnExplosionWithParams(world, location, params);
	}

	ExplosionParams Explosion::GetPreset(ExplosionType type)
	{
		ExplosionParams params;
		
		switch (type)
		{
		case ExplosionType::Tiny:
			params.particleCount = RandRange(8, 15);
			params.lifeTimeMin = 0.3f; params.lifeTimeMax = 0.8f;
			params.sizeMin = 0.5f; params.sizeMax = 1.2f;
			params.speedMin = 150.0f; params.speedMax = 250.0f;
			params.redRange = ColorRange{255, 255};   // Pure red
			params.greenRange = ColorRange{175, 255}; // Yellow-orange
			params.blueRange = ColorRange{0, 100};    // Little blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Small:
			params.particleCount = RandRange(15, 25);
			params.lifeTimeMin = 0.5f; params.lifeTimeMax = 1.2f;
			params.sizeMin = 0.8f; params.sizeMax = 1.8f;
			params.speedMin = 180.0f; params.speedMax = 320.0f;
			params.redRange = ColorRange{255, 255};   // Pure red
			params.greenRange = ColorRange{125, 225}; // Orange
			params.blueRange = ColorRange{0, 50};     // Minimal blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Medium:
			params.particleCount = RandRange(25, 35);
			params.lifeTimeMin = 0.8f; params.lifeTimeMax = 1.8f;
			params.sizeMin = 1.2f; params.sizeMax = 2.5f;
			params.speedMin = 200.0f; params.speedMax = 400.0f;
			params.redRange = ColorRange{255, 255};   // Pure red
			params.greenRange = ColorRange{50, 155};  // Deep orange
			params.blueRange = ColorRange{0, 25};     // Almost no blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Heavy:
			params.particleCount = RandRange(35, 50);
			params.lifeTimeMin = 1.0f; params.lifeTimeMax = 2.5f;
			params.sizeMin = 1.8f; params.sizeMax = 3.5f;
			params.speedMin = 250.0f; params.speedMax = 450.0f;
			params.redRange = ColorRange{255, 255};   // Pure red
			params.greenRange = ColorRange{25, 125};  // Variable orange
			params.blueRange = ColorRange{0, 50};     // Little blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};

			break;

		case ExplosionType::Plasma:
			params.particleCount = RandRange(30, 40);
			params.lifeTimeMin = 0.6f; params.lifeTimeMax = 1.5f;
			params.sizeMin = 1.0f; params.sizeMax = 2.2f;
			params.speedMin = 220.0f; params.speedMax = 380.0f;
			params.redRange = ColorRange{100, 200};   // Less red
			params.greenRange = ColorRange{150, 255}; // More green
			params.blueRange = ColorRange{200, 255};  // High blue = plasma
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Boss:
			params.particleCount = RandRange(60, 80);
			params.lifeTimeMin = 1.5f; params.lifeTimeMax = 3.0f;
			params.sizeMin = 2.5f; params.sizeMax = 4.5f;
			params.speedMin = 300.0f; params.speedMax = 600.0f;
			params.redRange = ColorRange{255, 255};   // Pure red
			params.greenRange = ColorRange{0, 100};   // Dark orange/red
			params.blueRange = ColorRange{0, 50};     // Minimal blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Environmental:
			params.particleCount = RandRange(20, 30);
			params.lifeTimeMin = 0.8f; params.lifeTimeMax = 2.0f;
			params.sizeMin = 1.0f; params.sizeMax = 2.0f;
			params.speedMin = 150.0f; params.speedMax = 300.0f;
			params.redRange = ColorRange{180, 220};   // Gray-ish
			params.greenRange = ColorRange{180, 220}; // Gray-ish
			params.blueRange = ColorRange{180, 220};  // Gray-ish
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;
		}
		
		return params;
	}

	void Explosion::SpawnExplosionWithParams(World* world, const sf::Vector2f& location, const ExplosionParams& params)
	{
		if (!world) return;

		for (int i = 0; i < params.particleCount; ++i)
		{
			// Safe array indexing
			if (params.particleTextures.empty()) continue;
			
			int maxIndex = static_cast<int>(params.particleTextures.size()) - 1;
			int imageIndex = RandRange(0, maxIndex);
			const std::string& particleTexture = params.particleTextures[imageIndex];
			
			weak_ptr<Particle> newParticle = world->SpawnActor<Particle>(particleTexture);

			// Safety check for weak_ptr
			if (auto particle = newParticle.lock())
			{
				particle->RandomLifeTime(params.lifeTimeMin, params.lifeTimeMax);
				particle->SetActorLocation(location);
				particle->RandomSize(params.sizeMin, params.sizeMax);
				particle->RandomVelocity(params.speedMin, params.speedMax);
				
				particle->GetSprite().setColor(params.GenerateRandomColor());
			}
		}
	}
}


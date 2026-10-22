#include "framework/MathUtility.h"
#include "framework/World.h"
#include "VFX/Explosion.h"
#include "VFX/ExplosionLights.h"
#include "VFX/Particle.h"
#include "framework/SpriteComponent.h"
#include "framework/PerfMonitor.h"

namespace vega
{
	namespace
	{
		struct FlashProfile
		{
			float radius;
			float intensity;
			float lifetime;
		};

		// Radius and strength of an explosion's flash, by the explosion type; the params-only path uses the Small values.
		FlashProfile FlashProfileFor(ExplosionType type)
		{
			switch (type)
			{
			case ExplosionType::Tiny: return { 60.f, 1.0f, 0.35f };
			case ExplosionType::Small: return { 120.f, 1.6f, 0.35f };
			case ExplosionType::Medium: return { 180.f, 2.2f, 0.35f };
			case ExplosionType::Heavy: return { 240.f, 2.8f, 0.35f };
			case ExplosionType::Plasma: return { 160.f, 2.2f, 0.35f };
			case ExplosionType::Boss: return { 400.f, 3.5f, 0.6f };
			case ExplosionType::Environmental: return { 140.f, 1.6f, 0.35f };
			}
			return { 120.f, 1.6f, 0.35f };
		}

		void AddExplosionFlash(const sf::Vector2f& location, const ExplosionParams& params, ExplosionType type)
		{
			const FlashProfile profile = FlashProfileFor(type);
			ExplosionFlash flash;
			flash.position = location;
			flash.radius = profile.radius;
			flash.intensity = profile.intensity;
			flash.lifetime = profile.lifetime;
			// The middle of the particle colour ranges is the flash's colour.
			flash.color = sf::Color(
				static_cast<std::uint8_t>((params.redRange.min + params.redRange.max) / 2),
				static_cast<std::uint8_t>((params.greenRange.min + params.greenRange.max) / 2),
				static_cast<std::uint8_t>((params.blueRange.min + params.blueRange.max) / 2));
			ExplosionLights::Add(flash);
		}
	}

	void Explosion::SpawnExplosion(World* world, const sf::Vector2f& location, ExplosionType type)
	{
		ExplosionParams params = GetPreset(type);
		AddExplosionFlash(location, params, type);
		SpawnExplosionWithParams(world, location, params);
	}

	void Explosion::SpawnExplosion(World* world, const sf::Vector2f& location, const ExplosionParams& params)
	{
		AddExplosionFlash(location, params, ExplosionType::Small);
		SpawnExplosionWithParams(world, location, params);
	}

	ExplosionParams Explosion::GetPreset(ExplosionType type)
	{
		ExplosionParams params;
		
		switch (type)
		{
		case ExplosionType::Tiny:
			params.particleCount = RandRange(8,15);
			params.lifeTimeMin =0.3f; params.lifeTimeMax =0.8f;
			params.sizeMin =0.5f; params.sizeMax =1.2f;
			params.speedMin =150.0f; params.speedMax =250.0f;
			params.redRange = ColorRange{255,255}; // Pure red
			params.greenRange = ColorRange{175,255}; // Yellow-orange
			params.blueRange = ColorRange{0,100}; // Little blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Small:
			params.particleCount = RandRange(15,25);
			params.lifeTimeMin =0.5f; params.lifeTimeMax =1.2f;
			params.sizeMin =0.8f; params.sizeMax =1.8f;
			params.speedMin =180.0f; params.speedMax =320.0f;
			params.redRange = ColorRange{255,255}; // Pure red
			params.greenRange = ColorRange{125,225}; // Orange
			params.blueRange = ColorRange{0,50}; // Minimal blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Medium:
			params.particleCount = RandRange(25,35);
			params.lifeTimeMin =0.8f; params.lifeTimeMax =1.8f;
			params.sizeMin =1.2f; params.sizeMax =2.5f;
			params.speedMin =200.0f; params.speedMax =400.0f;
			params.redRange = ColorRange{255,255}; // Pure red
			params.greenRange = ColorRange{50,155}; // Deep orange
			params.blueRange = ColorRange{0,25}; // Almost no blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Heavy:
			params.particleCount = RandRange(35,50);
			params.lifeTimeMin =1.0f; params.lifeTimeMax =2.5f;
			params.sizeMin =1.8f; params.sizeMax =3.5f;
			params.speedMin =250.0f; params.speedMax =450.0f;
			params.redRange = ColorRange{255,255}; // Pure red
			params.greenRange = ColorRange{25,125}; // Variable orange
			params.blueRange = ColorRange{0,50}; // Little blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};

			break;

		case ExplosionType::Plasma:
			params.particleCount = RandRange(30,40);
			params.lifeTimeMin =0.6f; params.lifeTimeMax =1.5f;
			params.sizeMin =1.0f; params.sizeMax =2.2f;
			params.speedMin =220.0f; params.speedMax =380.0f;
			params.redRange = ColorRange{100,200}; // Less red
			params.greenRange = ColorRange{150,255}; // More green
			params.blueRange = ColorRange{200,255}; // High blue = plasma
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Boss:
			params.particleCount = RandRange(60,80);
			params.lifeTimeMin =1.5f; params.lifeTimeMax =3.0f;
			params.sizeMin =2.5f; params.sizeMax =4.5f;
			params.speedMin =300.0f; params.speedMax =600.0f;
			params.redRange = ColorRange{255,255}; // Pure red
			params.greenRange = ColorRange{0,100}; // Dark orange/red
			params.blueRange = ColorRange{0,50}; // Minimal blue
			params.particleTextures = {
				"SpaceShooterRedux/PNG/Effects/star1.png",
				"SpaceShooterRedux/PNG/Effects/star2.png",
				"SpaceShooterRedux/PNG/Effects/star3.png"
			};
			break;

		case ExplosionType::Environmental:
			params.particleCount = RandRange(20,30);
			params.lifeTimeMin =0.8f; params.lifeTimeMax =2.0f;
			params.sizeMin =1.0f; params.sizeMax =2.0f;
			params.speedMin =150.0f; params.speedMax =300.0f;
			params.redRange = ColorRange{180,220}; // Gray-ish
			params.greenRange = ColorRange{180,220}; // Gray-ish
			params.blueRange = ColorRange{180,220}; // Gray-ish
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
		VEGA_PROFILE_SCOPE("VFX.SpawnExplosion");
		if (!world) return;

		// throttle particle count when many actors are active to avoid spikes
		int spawnCount = params.particleCount;
		int activeActors = vega::perf::g_activeActors.load();
		if (activeActors >400)
		{
			spawnCount = std::max(2, spawnCount /10);
		}
		else if (activeActors >200)
		{
			spawnCount = std::max(4, spawnCount /8);
		}
		else if (activeActors >100)
		{
			spawnCount = std::max(6, spawnCount /4);
		}
		// global cap
		spawnCount = std::min(spawnCount,60);

		for (int i =0; i < spawnCount; ++i)
		{
			if (params.particleTextures.empty()) continue;
			
			int maxIndex = static_cast<int>(params.particleTextures.size()) -1;
			int imageIndex = RandRange(0, maxIndex);
			const std::string& particleTexture = params.particleTextures[imageIndex];
			
			weak_ptr<Particle> newParticle = world->SpawnActor<Particle>(particleTexture);

			if (auto particle = newParticle.lock())
			{
				SpriteComponent* spriteComponent = particle->FindComponent<SpriteComponent>();
				if (!spriteComponent || !spriteComponent->HasSprite())
				{
					particle->Destroy();
					continue;
				}

				particle->RandomLifeTime(params.lifeTimeMin, params.lifeTimeMax);
				particle->SetActorLocation(location);
				particle->RandomSize(params.sizeMin, params.sizeMax);
				particle->RandomVelocity(params.speedMin, params.speedMax);

				spriteComponent->GetSprite()->setColor(params.GenerateRandomColor());
				// Sparks and flames give off light: the emissive layer adds them to the scene so they bloom.
				particle->SetRenderLayer(RenderLayer::WorldEmissive);
			}
		}
	}
}




#include "gameplay/ability/rocket/RocketVisualActor.h"

#include "VFX/Explosion.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		float Saturate(float value)
		{
			return std::clamp(value, 0.f, 1.f);
		}

		sf::Color WithIntensity(const sf::Color& color, float intensity)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * Saturate(intensity),
				0.f,
				255.f
			));
			return result;
		}

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& direction)
		{
			const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			return length > 0.001f
				? direction / length
				: sf::Vector2f{ 0.f, -1.f };
		}
	}

	RocketVisualActor::RocketVisualActor(
		World* world,
		const RocketPresentationProfile& profile
	)
		: Actor(world),
		mDefinition(profile.visual),
		mExplosionType(profile.explosionType),
		mOuterTrail(4),
		mCoreTrail(4),
		mFlightGlow(std::max(1.f, profile.visual.flightGlowRadius), 32),
		mImpactFlash(1.f, 48),
		mImpactRing(1.f, 64)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);

		const float glowRadius = std::max(1.f, mDefinition.flightGlowRadius);
		mFlightGlow.setOrigin({ glowRadius, glowRadius });
		mImpactFlash.setOrigin({ 1.f, 1.f });
		mImpactRing.setOrigin({ 1.f, 1.f });
		mImpactRing.setFillColor(sf::Color::Transparent);
		mImpactRing.setOutlineThickness(std::max(0.f, mDefinition.impactRingThickness));
	}

	void RocketVisualActor::Tick(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		mPhaseAge += safeDeltaTime;
		if (mPhase == Phase::Impact
			&& mPhaseAge >= std::max(0.f, mDefinition.impactVisualDuration))
		{
			Destroy();
			return;
		}

		Actor::Tick(deltaTime);
	}

	void RocketVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		Actor::Render(window);
		if (mPhase == Phase::Flight)
		{
			DrawFlight(window);
		}
		else
		{
			DrawImpact(window);
		}
	}

	void RocketVisualActor::SetFlightState(
		const sf::Vector2f& worldLocation,
		const sf::Vector2f& forwardDirection,
		float explosionRadius,
		float normalizedTravel
	)
	{
		if (mPhase != Phase::Flight)
		{
			return;
		}

		SetActorLocation(worldLocation);
		mForwardDirection = NormalizeOrDefault(forwardDirection);
		mExplosionRadius = std::max(1.f, explosionRadius);
		mNormalizedTravel = Saturate(normalizedTravel);
	}

	void RocketVisualActor::BeginImpact(
		const sf::Vector2f& worldLocation,
		float explosionRadius
	)
	{
		if (mPhase == Phase::Impact)
		{
			return;
		}

		mPhase = Phase::Impact;
		mPhaseAge = 0.f;
		mExplosionRadius = std::max(1.f, explosionRadius);
		SetActorLocation(worldLocation);

		ExplosionParams explosion = Explosion::GetPreset(mExplosionType);
		const float radiusScale = std::clamp(mExplosionRadius / 55.f, 0.75f, 2.25f);
		explosion.particleCount = std::clamp(
			static_cast<int>(static_cast<float>(explosion.particleCount) * radiusScale),
			12,
			48
		);
		explosion.sizeMin *= radiusScale;
		explosion.sizeMax *= radiusScale;
		explosion.speedMin *= 0.8f + 0.2f * radiusScale;
		explosion.speedMax *= 0.8f + 0.2f * radiusScale;
		Explosion::SpawnExplosion(GetWorld(), worldLocation, explosion);

		if (World* world = GetWorld())
		{
			world->PlayCameraShake(
				mDefinition.screenShakeAmplitude,
				mDefinition.screenShakeDuration,
				mDefinition.screenShakeFrequency
			);
		}
	}

	void RocketVisualActor::DrawFlight(sf::RenderWindow& window)
	{
		const float pulse = 0.78f
			+ 0.22f * std::sin(mPhaseAge * std::max(0.f, mDefinition.pulseSpeed));
		const float widthScale = 0.9f + 0.18f * pulse;
		ConfigureTrailGeometry(widthScale);

		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;
		mOuterTrail.setFillColor(WithIntensity(mDefinition.exhaustOuterColor, 0.75f + 0.25f * pulse));
		mCoreTrail.setFillColor(WithIntensity(mDefinition.exhaustCoreColor, pulse));
		window.draw(mOuterTrail, additiveStates);
		window.draw(mCoreTrail, additiveStates);

		const float glowScale = 0.85f + 0.18f * pulse + 0.08f * mNormalizedTravel;
		mFlightGlow.setPosition(GetActorLocation());
		mFlightGlow.setScale({ glowScale, glowScale });
		mFlightGlow.setFillColor(WithIntensity(mDefinition.flightGlowColor, pulse));
		window.draw(mFlightGlow, additiveStates);
	}

	void RocketVisualActor::DrawImpact(sf::RenderWindow& window)
	{
		const float duration = std::max(0.001f, mDefinition.impactVisualDuration);
		const float progress = Saturate(mPhaseAge / duration);
		const float easedProgress = progress * progress * (3.f - 2.f * progress);
		const float fade = 1.f - easedProgress;
		const sf::Vector2f location = GetActorLocation();

		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;

		const float flashRadius = std::max(1.f, mExplosionRadius);
		mImpactFlash.setRadius(flashRadius);
		mImpactFlash.setOrigin({ flashRadius, flashRadius });
		mImpactFlash.setPosition(location);
		const float flashScale = 0.18f
			+ (std::max(0.18f, mDefinition.impactFlashEndScale) - 0.18f) * easedProgress;
		mImpactFlash.setScale({ flashScale, flashScale });
		mImpactFlash.setFillColor(WithIntensity(mDefinition.impactFlashColor, fade));
		window.draw(mImpactFlash, additiveStates);

		mImpactRing.setRadius(flashRadius);
		mImpactRing.setOrigin({ flashRadius, flashRadius });
		mImpactRing.setPosition(location);
		const float ringScale = 0.55f
			+ (std::max(0.55f, mDefinition.impactRingEndScale) - 0.55f) * easedProgress;
		mImpactRing.setScale({ ringScale, ringScale });
		mImpactRing.setOutlineColor(WithIntensity(mDefinition.impactRingColor, fade));
		window.draw(mImpactRing, additiveStates);
	}

	void RocketVisualActor::ConfigureTrailGeometry(float widthScale)
	{
		const sf::Vector2f location = GetActorLocation();
		const sf::Vector2f forward = NormalizeOrDefault(mForwardDirection);
		const sf::Vector2f perpendicular{ -forward.y, forward.x };
		const float trailLength = std::max(1.f, mDefinition.trailLength);
		const float halfWidth = std::max(1.f, mDefinition.trailWidth * widthScale * 0.5f);
		const sf::Vector2f head = location - forward * std::max(2.f, halfWidth * 0.25f);
		const sf::Vector2f tail = location - forward * trailLength;

		mOuterTrail.setPoint(0, head - perpendicular * halfWidth);
		mOuterTrail.setPoint(1, head + perpendicular * halfWidth);
		mOuterTrail.setPoint(2, tail + perpendicular * (halfWidth * 0.08f));
		mOuterTrail.setPoint(3, tail - perpendicular * (halfWidth * 0.08f));

		const float coreHalfWidth = halfWidth
			* std::clamp(mDefinition.coreTrailWidthScale, 0.05f, 1.f);
		const sf::Vector2f coreTail = location - forward * (trailLength * 0.72f);
		mCoreTrail.setPoint(0, head - perpendicular * coreHalfWidth);
		mCoreTrail.setPoint(1, head + perpendicular * coreHalfWidth);
		mCoreTrail.setPoint(2, coreTail + perpendicular * (coreHalfWidth * 0.05f));
		mCoreTrail.setPoint(3, coreTail - perpendicular * (coreHalfWidth * 0.05f));
	}
}

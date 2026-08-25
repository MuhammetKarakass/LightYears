#include "gameplay/ability/overdriveCore/OverdriveCoreVisualActor.h"

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

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * Saturate(multiplier),
				0.f,
				255.f
			));
			return result;
		}
	}

	OverdriveCoreVisualActor::OverdriveCoreVisualActor(
		World* world,
		const OverdriveCorePresentationProfile& profile
	)
		: Actor(world),
		mDefinition(profile.visual),
		mExplosionType(profile.explosionType),
		mTrail(4),
		mBody(std::max(1.f, profile.visual.bodyRadius), 20),
		mGlow(std::max(1.f, profile.visual.glowRadius), 24),
		mImpactRing(1.f, 40)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
		mBody.setOrigin({ mDefinition.bodyRadius, mDefinition.bodyRadius });
		mGlow.setOrigin({ mDefinition.glowRadius, mDefinition.glowRadius });
		mImpactRing.setFillColor(sf::Color::Transparent);
		mImpactRing.setOutlineThickness(std::max(0.f, mDefinition.impactRingThickness));
	}

	void OverdriveCoreVisualActor::Tick(float deltaTime)
	{
		mPhaseAge += std::max(0.f, deltaTime);
		if (mPhase == Phase::Impact &&
			mPhaseAge >= std::max(0.f, mDefinition.impactVisualDuration))
		{
			Destroy();
			return;
		}
		Actor::Tick(deltaTime);
	}

	void OverdriveCoreVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() ||
			(mPhase == Phase::Flight && !mFlightVisible))
		{
			return;
		}
		if (mPhase == Phase::Flight)
		{
			DrawFlight(window);
		}
		else
		{
			DrawImpact(window);
		}
	}

	void OverdriveCoreVisualActor::SetFlightState(
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

	void OverdriveCoreVisualActor::BeginImpact(
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

	void OverdriveCoreVisualActor::DrawFlight(sf::RenderWindow& window)
	{
		const float pulse = 0.75f + 0.25f * std::sin(
			mPhaseAge * std::max(0.f, mDefinition.pulseSpeed)
		);
		const sf::Vector2f forward = NormalizeOrDefault(mForwardDirection);
		const sf::Vector2f perpendicular{ -forward.y, forward.x };
		const sf::Vector2f location = GetActorLocation();
		const float halfWidth = std::max(1.f, mDefinition.trailWidth * pulse * 0.5f);
		const sf::Vector2f head = location - forward * 2.f;
		const sf::Vector2f tail = location - forward * std::max(1.f, mDefinition.trailLength);
		mTrail.setPoint(0, head - perpendicular * halfWidth);
		mTrail.setPoint(1, head + perpendicular * halfWidth);
		mTrail.setPoint(2, tail + perpendicular * halfWidth * 0.08f);
		mTrail.setPoint(3, tail - perpendicular * halfWidth * 0.08f);
		mTrail.setFillColor(WithAlpha(mDefinition.trailColor, pulse));
		mBody.setPosition(location);
		mBody.setFillColor(WithAlpha(mDefinition.bodyColor, pulse));
		mGlow.setPosition(location);
		mGlow.setScale({ 0.85f + 0.15f * pulse + 0.08f * mNormalizedTravel,
			0.85f + 0.15f * pulse + 0.08f * mNormalizedTravel });
		mGlow.setFillColor(WithAlpha(mDefinition.coreColor, 0.28f * pulse));
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mTrail, additive);
		window.draw(mGlow, additive);
		window.draw(mBody, additive);
	}

	void OverdriveCoreVisualActor::DrawImpact(sf::RenderWindow& window)
	{
		const float duration = std::max(0.001f, mDefinition.impactVisualDuration);
		const float progress = Saturate(mPhaseAge / duration);
		const float fade = 1.f - progress * progress * (3.f - 2.f * progress);
		const float radius = std::max(1.f, mExplosionRadius);
		mImpactRing.setRadius(radius);
		mImpactRing.setOrigin({ radius, radius });
		mImpactRing.setPosition(GetActorLocation());
		const float scale = 0.4f +
			(std::max(0.4f, mDefinition.impactRingEndScale) - 0.4f) * progress;
		mImpactRing.setScale({ scale, scale });
		mImpactRing.setOutlineColor(WithAlpha(mDefinition.impactColor, fade));
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mImpactRing, additive);
	}

	sf::Vector2f OverdriveCoreVisualActor::NormalizeOrDefault(
		const sf::Vector2f& direction
	) const
	{
		const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
		return length > 0.001f
			? direction / length
			: sf::Vector2f{ 0.f, -1.f };
	}
}

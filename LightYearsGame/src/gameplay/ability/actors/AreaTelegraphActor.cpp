#include "gameplay/ability/actors/AreaTelegraphActor.h"

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

		sf::Color BlendTelegraphColor(const sf::Color& from, const sf::Color& to, float alpha)
		{
			const float t = Saturate(alpha);
			auto lerpChannel = [t](std::uint8_t a, std::uint8_t b)
			{
				return static_cast<std::uint8_t>(
					std::clamp(static_cast<float>(a) + (static_cast<float>(b) - a) * t, 0.f, 255.f)
				);
			};
			return {
				lerpChannel(from.r, to.r),
				lerpChannel(from.g, to.g),
				lerpChannel(from.b, to.b),
				lerpChannel(from.a, to.a)
			};
		}
	}

	AreaTelegraphActor::AreaTelegraphActor(
		World* world,
		const sf::Vector2f& worldLocation,
		float radius,
		float lifeTime,
		const AreaTelegraphVisualDefinition& definition
	)
		: Actor(world),
		mDefinition(definition),
		mFill(std::max(1.f, radius), 64),
		mCountdownRing(std::max(1.f, radius), 64),
		mOutline(std::max(1.f, radius), 64),
		mLifeTime(std::max(0.f, lifeTime))
	{
		SetActorLocation(worldLocation);
		SetRenderLayer(RenderLayer::GroundDecal);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);

		const float effectiveRadius = std::max(1.f, radius);
		mFill.setOrigin({ effectiveRadius, effectiveRadius });
		mCountdownRing.setOrigin({ effectiveRadius, effectiveRadius });
		mOutline.setOrigin({ effectiveRadius, effectiveRadius });
		UpdateVisuals();
	}

	void AreaTelegraphActor::Tick(float deltaTime)
	{
		mAge += deltaTime;
		if (mLifeTime > 0.f && mAge >= mLifeTime)
		{
			Destroy();
			return;
		}

		UpdateVisuals();
		Actor::Tick(deltaTime);
	}

	void AreaTelegraphActor::SetCountdownProgress(float normalizedProgress)
	{
		mHasExternalCountdown = true;
		mCountdownProgress = Saturate(normalizedProgress);
		UpdateVisuals();
	}

	void AreaTelegraphActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		Actor::Render(window);

		const sf::Vector2f location = GetActorLocation();
		mFill.setPosition(location);
		mCountdownRing.setPosition(location);
		mOutline.setPosition(location);
		window.draw(mFill);
		window.draw(mCountdownRing);
		window.draw(mOutline);
	}

	void AreaTelegraphActor::UpdateVisuals()
	{
		const float rawProgress = mHasExternalCountdown
			? mCountdownProgress
			: (mLifeTime > 0.f ? Saturate(mAge / mLifeTime) : 0.f);
		const float easedProgress = std::pow(
			rawProgress,
			std::max(1.f, mDefinition.countdownEaseExponent)
		);
		const float sinePulse = (std::sin(mAge * mDefinition.pulseSpeed) + 1.f) * 0.5f;
		const float intensity = mDefinition.minimumPulse
			+ (mDefinition.maximumPulse - mDefinition.minimumPulse) * sinePulse;
		const float baseScale = mDefinition.countdownStartScale
			+ (mDefinition.countdownEndScale - mDefinition.countdownStartScale) * easedProgress;
		const float pulseScale = 1.f + mDefinition.pulseScaleAmount * sinePulse;

		sf::Color fillColor = BlendTelegraphColor(mDefinition.fillColor, mDefinition.dangerFillColor, easedProgress);
		sf::Color outlineColor = BlendTelegraphColor(
			mDefinition.outlineColor,
			mDefinition.dangerOutlineColor,
			easedProgress
		);
		fillColor.a = static_cast<std::uint8_t>(std::clamp(fillColor.a * intensity, 0.f, 255.f));
		outlineColor.a = static_cast<std::uint8_t>(std::clamp(outlineColor.a * intensity, 0.f, 255.f));

		mFill.setFillColor(fillColor);
		mFill.setScale({ pulseScale, pulseScale });

		mCountdownRing.setFillColor(sf::Color::Transparent);
		mCountdownRing.setOutlineColor(outlineColor);
		mCountdownRing.setOutlineThickness(std::max(0.f, mDefinition.countdownRingThickness));
		mCountdownRing.setScale({ baseScale, baseScale });

		mOutline.setFillColor(sf::Color::Transparent);
		mOutline.setOutlineColor(outlineColor);
		mOutline.setOutlineThickness(mDefinition.outlineThickness);
		mOutline.setScale({ pulseScale, pulseScale });
	}
}

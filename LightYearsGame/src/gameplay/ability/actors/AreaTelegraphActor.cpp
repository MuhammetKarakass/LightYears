#include "gameplay/ability/actors/AreaTelegraphActor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
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
		mOutline(std::max(1.f, radius), 64),
		mLifeTime(std::max(0.f, lifeTime))
	{
		SetActorLocation(worldLocation);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);

		const float effectiveRadius = std::max(1.f, radius);
		mFill.setOrigin({ effectiveRadius, effectiveRadius });
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

	void AreaTelegraphActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		Actor::Render(window);

		const sf::Vector2f location = GetActorLocation();
		mFill.setPosition(location);
		mOutline.setPosition(location);
		window.draw(mFill);
		window.draw(mOutline);
	}

	void AreaTelegraphActor::UpdateVisuals()
	{
		const float sinePulse = (std::sin(mAge * mDefinition.pulseSpeed) + 1.f) * 0.5f;
		const float intensity = mDefinition.minimumPulse
			+ (mDefinition.maximumPulse - mDefinition.minimumPulse) * sinePulse;
		const float scale = 1.f + mDefinition.pulseScaleAmount * sinePulse;

		sf::Color fillColor = mDefinition.fillColor;
		sf::Color outlineColor = mDefinition.outlineColor;
		fillColor.a = static_cast<std::uint8_t>(std::clamp(fillColor.a * intensity, 0.f, 255.f));
		outlineColor.a = static_cast<std::uint8_t>(std::clamp(outlineColor.a * intensity, 0.f, 255.f));

		mFill.setFillColor(fillColor);
		mFill.setScale({ scale, scale });

		mOutline.setFillColor(sf::Color::Transparent);
		mOutline.setOutlineColor(outlineColor);
		mOutline.setOutlineThickness(mDefinition.outlineThickness);
		mOutline.setScale({ scale, scale });
	}
}

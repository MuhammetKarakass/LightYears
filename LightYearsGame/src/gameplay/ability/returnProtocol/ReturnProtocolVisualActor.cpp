#include "gameplay/ability/returnProtocol/ReturnProtocolVisualActor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * std::clamp(multiplier, 0.f, 1.f),
				0.f,
				255.f
			));
			return result;
		}
	}

	ReturnProtocolVisualActor::ReturnProtocolVisualActor(
		World* world,
		Actor* owner,
		const ReturnProtocolPresentationProfile& profile
	)
		: Actor(world),
		mOwner{ owner },
		mProfile{ profile },
		mOuterRing{ 1.f, 64 },
		mInnerRing{ 1.f, 64 }
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
	}

	void ReturnProtocolVisualActor::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		mAge += std::max(0.f, deltaTime);
		SetActorLocation(mOwner->GetActorLocation());
		Actor::Tick(deltaTime);
	}

	void ReturnProtocolVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const float pulse = 0.5f + 0.5f * std::sin(mAge * mProfile.pulseSpeed);
		const float outerRadius = std::max(1.f, mProfile.radius * (0.92f + pulse * 0.15f));
		const float innerRadius = outerRadius * 0.66f;
		const float thickness = std::max(1.f, mProfile.ringThickness);
		const sf::Vector2f location = GetActorLocation();

		mOuterRing.setRadius(outerRadius);
		mOuterRing.setOrigin({ outerRadius, outerRadius });
		mOuterRing.setPosition(location);
		mOuterRing.setFillColor(sf::Color::Transparent);
		mOuterRing.setOutlineThickness(thickness);
		mOuterRing.setOutlineColor(WithAlpha(mProfile.ringColor, 0.72f + pulse * 0.28f));

		mInnerRing.setRadius(innerRadius);
		mInnerRing.setOrigin({ innerRadius, innerRadius });
		mInnerRing.setPosition(location);
		mInnerRing.setFillColor(sf::Color::Transparent);
		mInnerRing.setOutlineThickness(std::max(1.f, thickness * 0.6f));
		mInnerRing.setOutlineColor(WithAlpha(mProfile.innerRingColor, 0.40f + pulse * 0.35f));

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mOuterRing, additive);
		window.draw(mInnerRing, additive);
	}
}

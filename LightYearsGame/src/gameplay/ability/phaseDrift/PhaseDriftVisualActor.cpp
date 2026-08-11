#include "gameplay/ability/phaseDrift/PhaseDriftVisualActor.h"

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

	PhaseDriftVisualActor::PhaseDriftVisualActor(
		World* world,
		Actor* owner,
		const PhaseDriftPresentationProfile& profile
	)
		: Actor(world),
		mOwner{ owner },
		mProfile{ profile },
		mAura{ 1.f, 64 },
		mOutline{ 1.f, 64 }
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
		mAura.setFillColor(sf::Color::Transparent);
		mOutline.setFillColor(sf::Color::Transparent);
		UpdateVisuals();
	}

	void PhaseDriftVisualActor::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		mAge += std::max(0.f, deltaTime);
		SetActorLocation(mOwner->GetActorLocation());
		UpdateVisuals();
		Actor::Tick(deltaTime);
	}

	void PhaseDriftVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const sf::Vector2f location = GetActorLocation();
		mAura.setPosition(location);
		mOutline.setPosition(location);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mAura, additive);
		window.draw(mOutline, additive);
	}

	void PhaseDriftVisualActor::UpdateVisuals()
	{
		const float pulse = 0.5f + 0.5f * std::sin(mAge * mProfile.pulseSpeed);
		const float auraRadius = std::max(1.f, mProfile.radius * (0.96f + 0.06f * pulse));
		const float outlineRadius = auraRadius + std::max(1.f, mProfile.outlineThickness);
		mAura.setRadius(auraRadius);
		mAura.setOrigin({ auraRadius, auraRadius });
		mAura.setFillColor(WithAlpha(mProfile.auraColor, 0.75f + 0.25f * pulse));
		mOutline.setRadius(outlineRadius);
		mOutline.setOrigin({ outlineRadius, outlineRadius });
		mOutline.setOutlineThickness(-std::max(1.f, mProfile.outlineThickness));
		mOutline.setOutlineColor(WithAlpha(mProfile.outlineColor, 0.75f + 0.25f * pulse));
	}
}

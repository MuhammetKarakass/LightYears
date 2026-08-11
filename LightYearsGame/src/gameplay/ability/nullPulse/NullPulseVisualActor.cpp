#include "gameplay/ability/nullPulse/NullPulseVisualActor.h"

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

	NullPulseVisualActor::NullPulseVisualActor(
		World* world,
		const sf::Vector2f& worldLocation,
		float radius,
		const NullPulsePresentationProfile& profile
	)
		: Actor(world),
		mProfile(profile),
		mPulseRing(1.f, 64),
		mCore(1.f, 48),
		mRadius(std::max(1.f, radius))
	{
		SetActorLocation(worldLocation);
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);

		mPulseRing.setOrigin({ 1.f, 1.f });
		mPulseRing.setFillColor(sf::Color::Transparent);
		mCore.setOrigin({ 1.f, 1.f });
		UpdateVisuals();
	}

	void NullPulseVisualActor::Tick(float deltaTime)
	{
		mAge += std::max(0.f, deltaTime);
		if (mAge >= std::max(0.f, mProfile.pulseDuration))
		{
			Destroy();
			return;
		}

		UpdateVisuals();
		Actor::Tick(deltaTime);
	}

	void NullPulseVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const sf::Vector2f location = GetActorLocation();
		mPulseRing.setPosition(location);
		mCore.setPosition(location);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mPulseRing, additive);
		window.draw(mCore, additive);
	}

	void NullPulseVisualActor::UpdateVisuals()
	{
		const float duration = std::max(0.001f, mProfile.pulseDuration);
		const float progress = Saturate(mAge / duration);
		const float fade = 1.f - progress;
		const float radius = std::max(1.f, mRadius);
		const float ringRadius = std::max(1.f, radius * (0.18f + 0.82f * progress));
		const float coreRadius = std::max(1.f, radius * 0.12f * (1.f + 0.35f * fade));

		mPulseRing.setRadius(ringRadius);
		mPulseRing.setOrigin({ ringRadius, ringRadius });
		mPulseRing.setOutlineThickness(std::max(2.f, radius * 0.018f));
		mPulseRing.setOutlineColor(WithAlpha(mProfile.pulseColor, fade));

		mCore.setRadius(coreRadius);
		mCore.setOrigin({ coreRadius, coreRadius });
		mCore.setFillColor(WithAlpha(mProfile.projectileBreakColor, fade * 0.75f));
	}
}

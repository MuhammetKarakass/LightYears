#include "presentation/ability/statusReaction/StatusReactionFlashActor.h"
#include "framework/World.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/common/PresentationColor.h"

#include <algorithm>

namespace vega
{
	StatusReactionFlashActor::StatusReactionFlashActor(World* world, const sf::Vector2f& worldLocation, float radius, const StatusReactionPresentationProfile& profile)
		: TimedPresentationActor(world, worldLocation, profile.duration, RenderLayer::WorldVfx), mProfile(profile), mRing(1.f, 64), mCore(1.f, 32), mRadius(std::max(1.f, radius))
	{
		mRing.setFillColor(sf::Color::Transparent);
		UpdateVisuals();
	}

	void StatusReactionFlashActor::Spawn(World& world, const sas::ContentId& profileId, const sf::Vector2f& location, float radius)
	{
		const auto* profile = PresentationProfileRegistry<StatusReactionPresentationProfile>::Find(profileId.ToString());
		if (!profile) return;
		world.SpawnActor<StatusReactionFlashActor>(location, radius > 0.f ? radius : profile->defaultRadius, *profile);
	}

	void StatusReactionFlashActor::Render(sf::RenderTarget& window)
	{
		if (GetIsPendingDestroy()) return;
		const sf::Vector2f location = GetActorLocation();
		mRing.setPosition(location);
		mCore.setPosition(location);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mRing, additive);
		window.draw(mCore, additive);
	}

	void StatusReactionFlashActor::UpdateVisuals()
	{
		const float progress = Saturate(GetAge() / std::max(0.001f, mProfile.duration));
		const float fade = 1.f - progress;
		const float scale = mProfile.contracting ? 1.f - 0.85f * progress : 0.2f + 0.8f * progress;
		const float ringRadius = std::max(1.f, mRadius * scale);
		mRing.setRadius(ringRadius);
		mRing.setOrigin({ ringRadius, ringRadius });
		mRing.setOutlineThickness(std::max(2.f, mRadius * 0.02f));
		mRing.setOutlineColor(WithAlpha(mProfile.ringColor, fade));
		const float coreRadius = std::max(4.f, 18.f * fade);
		mCore.setRadius(coreRadius);
		mCore.setOrigin({ coreRadius, coreRadius });
		mCore.setFillColor(WithAlpha(mProfile.coreColor, fade * 0.8f));
	}
}

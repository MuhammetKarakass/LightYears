#pragma once

#include "framework/Actor.h"
#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

namespace vega
{
	struct HullShockConePresentationProfile
	{
		sas::ContentId profileId;
		AreaTelegraphVisualDefinition style;
	};
	bool RegisterHullShockConePresentationProfiles();

	class HullShockConeVisualActor final : public Actor
	{
	public:
		HullShockConeVisualActor(World* world, Actor* owner, const sf::Vector2f& direction, float halfAngleRadians, float radius);
		void Tick(float deltaTime) override;
		void Render(sf::RenderTarget& target) override;
		void SetRadius(float radius) { mRadius = radius; }
		void Complete();
		bool IsComplete() const { return mComplete; }
	private:
		HullShockConePresentationProfile mProfile;
		weak_ptr<Actor> mOwner;
		sf::Vector2f mDirection;
		float mHalfAngle;
		float mRadius;
		float mAge = 0.f;
		bool mComplete = false;
	};
}

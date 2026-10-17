#pragma once
#include "framework/Actor.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"

namespace vega
{
	class ModuleAnnularBurstActor final : public Actor
	{
	public:
		ModuleAnnularBurstActor(World* world, const weak_ptr<Actor>& source, const ModuleAnnularBurstPresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, float innerRadius, float outerRadius);
		void Detonate(float damage);
		void Tick(float deltaTime) override;
		void Render(sf::RenderTarget& target) override;
	private:
		weak_ptr<Actor> mSource;
		ModuleAnnularBurstPresentationProfile mProfile;
		weak_ptr<ModuleGeneratedLease> mLease;
		float mInnerRadius = 0.f;
		float mOuterRadius = 0.f;
		float mRemaining = 0.f;
		bool mDetonated = false;
	};
}

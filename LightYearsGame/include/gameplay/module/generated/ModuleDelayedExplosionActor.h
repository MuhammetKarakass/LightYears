#pragma once
#include "framework/Actor.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"

namespace vega
{
	class ModuleDelayedExplosionActor final : public Actor
	{
	public:
		ModuleDelayedExplosionActor(World* world, const weak_ptr<Actor>& source, const ModuleExplosionPresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, float damage, float radius, float delay, const List<GameplayTag>& tags);
		void Tick(float deltaTime) override;
		void Render(sf::RenderTarget& window) override;
	private:
		weak_ptr<Actor> mSource;
		ModuleExplosionPresentationProfile mProfile;
		weak_ptr<ModuleGeneratedLease> mLease;
		float mDamage = 0.f;
		float mRadius = 0.f;
		float mRemaining = 0.f;
		List<GameplayTag> mTags;
		bool mExploded = false;
	};
}

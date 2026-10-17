#pragma once
#include "framework/Actor.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"

namespace vega
{
	class ModuleMineActor final : public Actor
	{
	public:
		ModuleMineActor(World* world, const weak_ptr<Actor>& source, const ModuleMinePresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, float damage, float radius, float fuse, int cryoStacks, const GameplayTag& damageType);
		void Tick(float deltaTime) override;
		void Render(sf::RenderTarget& window) override;
		bool HasExploded() const { return mExploded; }
	private:
		weak_ptr<Actor> mSource;
		ModuleMinePresentationProfile mProfile;
		weak_ptr<ModuleGeneratedLease> mLease;
		float mDamage = 0.f;
		float mRadius = 0.f;
		float mFuse = 0.f;
		float mRemaining = 0.f;
		int mCryoStacks = 0;
		GameplayTag mDamageType;
		bool mExploded = false;
	};
}

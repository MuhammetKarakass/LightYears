#pragma once
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "framework/GameplayTag.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"

namespace vega
{
	class ModuleGuidedProjectileActor final : public AbilityWorldActor
	{
	public:
		ModuleGuidedProjectileActor(World* world, Actor* source, const ModuleGuidedPresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, const weak_ptr<Actor>& target, float speed, float lifetime, float damage, const GameplayTag& damageType);
		void Tick(float deltaTime) override;
		void Render(sf::RenderTarget& window) override;
		void OnActorBeginOverlap(Actor* target) override;
		bool IsProjectileActor() const override { return true; }
		void SetUnguidedDirection(const sf::Vector2f& direction, const List<std::uint64_t>& excludedTargets) { mUnguidedDirection = direction; mExcludedTargets = excludedTargets; mUnguided = true; }
	private:
		ModuleGuidedPresentationProfile mProfile;
		weak_ptr<ModuleGeneratedLease> mLease;
		weak_ptr<Actor> mTarget;
		float mSpeed = 0.f;
		bool mUnguided = false;
		sf::Vector2f mUnguidedDirection{};
		List<std::uint64_t> mExcludedTargets;
	};
}

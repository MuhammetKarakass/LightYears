#pragma once
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"
#include <unordered_set>

namespace vega
{
	class ModuleSpeedLanceActor final : public AbilityWorldActor
	{
	public:
		ModuleSpeedLanceActor(World* world, Actor* source, const ModuleSpeedLancePresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& direction, float speed, float range, float damage, int firstTargetElectricStacks);
		void Tick(float deltaTime) override;
		void Render(sf::RenderTarget& target) override;
		void OnActorBeginOverlap(Actor* target) override;
		bool IsProjectileActor() const override { return true; }
	private:
		ModuleSpeedLancePresentationProfile mProfile;
		weak_ptr<ModuleGeneratedLease> mLease;
		sf::Vector2f mDirection{};
		float mSpeed = 0.f;
		int mFirstTargetElectricStacks = 0;
		std::unordered_set<std::uint64_t> mHitTargets;
	};
}

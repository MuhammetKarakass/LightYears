#pragma once
#include "framework/Actor.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "presentation/ability/moduleGenerated/ModuleGeneratedPresentationProfile.h"

#include <SFML/System/Vector2.hpp>

namespace vega
{
	// Smoldering Remains: the Thermal field left by an Ember burst. Once per second it adds stacksPerPulse Ignite stacks to the
	// combatants inside radius through status application only (no damage, no module triggers). It lasts seconds and ends early
	// when its owner or its lease is gone.
	class ModuleSmolderingFieldActor final : public Actor
	{
	public:
		ModuleSmolderingFieldActor(World* world, const weak_ptr<Actor>& source, const ModuleExplosionPresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, float seconds, float radius, int stacksPerPulse);
		void Tick(float deltaTime) override;
		void Render(sf::RenderTarget& window) override;
	private:
		void Pulse(Actor& source);
		weak_ptr<Actor> mSource;
		ModuleExplosionPresentationProfile mProfile;
		weak_ptr<ModuleGeneratedLease> mLease;
		float mRadius = 0.f;
		int mStacksPerPulse = 0;
		float mRemaining = 0.f;
		float mElapsed = 0.f;
		int mPulses = 0;
		int mMaxPulses = 0;
	};
}

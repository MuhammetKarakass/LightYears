#pragma once

#include "attributes/AttributeSystem.h"
#include "effects/AreaGameplayEffectApplicator.h"
#include "effects/GameplayEffectBindings.h"
#include "effects/GameplayEffectRuntimeEntry.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>

#include <memory>

namespace ly
{
	class GravityAnomalyRuntimeContext;

	class GravityAnomalyFieldActor final : public AbilityWorldActor
	{
	public:
		GravityAnomalyFieldActor(
			World* world,
			Actor* owner,
			const GravityAnomalyFieldPresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;

		float GetResolvedRadius() const { return mRadius; }
		float GetResolvedDuration() const { return mDuration; }
		float GetResolvedPullStrength() const { return mPullStrength; }
		float GetResolvedSlowMagnitude() const { return mSlowMagnitude; }
		size_t GetAffectedTargetCount() const
		{
			return mEffectApplicator.GetTrackedTargetCount();
		}

	private:
		bool IsEligibleTarget(const Actor& actor) const;
		void UpdateAffectedTargets();
		void ClearAppliedEffects();
		void ConfigureVisualGeometry();

		GravityAnomalyFieldPresentationProfile mPresentationProfile;
		std::shared_ptr<GravityAnomalyRuntimeContext> mRuntimeContext;
		sas::GameplayEffectSpec mInsideEffectSpec;
		sas::AreaGameplayEffectApplicator<
			Actor,
			sas::GameplayEffectRuntimeContext
		> mEffectApplicator;
		sf::CircleShape mCenter;
		sf::CircleShape mInnerRing;
		sf::CircleShape mOuterRing;
		sf::CircleShape mBoundary;
		List<sf::CircleShape> mInwardParticles;
		float mRadius = 0.f;
		float mDuration = 0.f;
		float mPullStrength = 0.f;
		float mSlowMagnitude = 0.f;
		float mVisualAge = 0.f;
	};

	bool RegisterGravityAnomalyFieldActorType();
}

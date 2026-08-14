#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>

#include <optional>

namespace ly
{
	class GravityAnomalyProjectileActor final : public AbilityWorldActor
	{
	public:
		GravityAnomalyProjectileActor(
			World* world,
			Actor* owner,
			const GravityAnomalyProjectilePresentationProfile& presentationProfile,
			std::optional<sf::Vector2f> requestedTargetLocation
		);

		void Tick(float deltaTime) override;
		bool IsProjectileActor() const override { return true; }
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;
		void SetRelayLaunchDirection(const sf::Vector2f& direction)
		{
			mRelayLaunchDirection = direction;
		}

		float GetProjectileSpeed() const { return mProjectileSpeed; }
		float GetCastRange() const { return mCastRange; }
		float GetTargetTravelDistance() const { return mTargetTravelDistance; }
		float GetTravelDistance() const { return mTravelDistance; }
		float GetResolvedFieldDuration() const { return mFieldDuration; }
		float GetResolvedFieldRadius() const { return mFieldRadius; }
		float GetResolvedPullStrength() const { return mPullStrength; }
		float GetResolvedSlowMagnitude() const { return mSlowMagnitude; }
		const sf::Vector2f& GetResolvedTargetLocation() const { return mResolvedTargetLocation; }
		bool HasSpawnedField() const { return mHasSpawnedField; }

	private:
		void ResolveTargetLocation();
		void MoveTowardTarget(float deltaTime);
		void SpawnField();
		void ConfigureVisualGeometry();

		GravityAnomalyProjectilePresentationProfile mPresentationProfile;
		std::optional<sf::Vector2f> mRequestedTargetLocation;
		sf::Vector2f mResolvedTargetLocation{ 0.f, 0.f };
		sf::Vector2f mFlightDirection{ 0.f, -1.f };
		sas::GameplayAttributeList mFieldAttributes;
		sf::CircleShape mGlow;
		sf::CircleShape mCore;
		sf::ConvexShape mTrail;
		float mProjectileSpeed = 0.f;
		float mCastRange = 0.f;
		float mTargetTravelDistance = 0.f;
		float mTravelDistance = 0.f;
		float mFieldDuration = 0.f;
		float mFieldRadius = 0.f;
		float mPullStrength = 0.f;
		float mSlowMagnitude = 0.f;
		float mInsideEffectDuration = 0.f;
		std::optional<sf::Vector2f> mRelayLaunchDirection;
		bool mHasSpawnedField = false;
	};

	bool RegisterGravityAnomalyProjectileActorType();
}

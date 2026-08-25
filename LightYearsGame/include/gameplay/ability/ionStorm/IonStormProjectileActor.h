#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/portal/PortalDestinationRebaser.h"
#include "presentation/ability/ionStorm/IonStormPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <optional>

namespace ly
{
	class IonStormProjectileActor final
		: public AbilityWorldActor,
		  public PortalDestinationRebaser
	{
	public:
		IonStormProjectileActor(
			World* world,
			Actor* owner,
			const IonStormProjectilePresentationProfile& presentationProfile,
			std::optional<sf::Vector2f> requestedTargetLocation
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void RebasePortalDestination(const sf::Vector2f& exitLocation) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;
		bool IsProjectileActor() const override { return true; }
		// Ion Storm is a delivery projectile, not a relay-clone projectile. It
		// must pass through Relay Prism so it can reach its destination and spawn
		// its field instead of being consumed without a replacement.
		bool CanBeCapturedByRelay() const override { return false; }

		float GetProjectileSpeed() const { return mProjectileSpeed; }
		float GetCastRange() const { return mCastRange; }
		float GetTargetTravelDistance() const { return mTargetTravelDistance; }
		float GetTravelDistance() const { return mTravelDistance; }
		const sf::Vector2f& GetResolvedTargetLocation() const
		{
			return mResolvedTargetLocation;
		}
		bool HasSpawnedField() const { return mHasSpawnedField; }

	private:
		void ResolveTargetLocation();
		void MoveTowardTarget(float deltaTime);
		void SpawnField();
		void ConfigureVisualGeometry();

		IonStormProjectilePresentationProfile mPresentationProfile;
		std::optional<sf::Vector2f> mRequestedTargetLocation;
		std::weak_ptr<Actor> mOwnerReference;
		sf::Vector2f mResolvedTargetLocation{ 0.f, 0.f };
		sf::Vector2f mFlightDirection{ 0.f, -1.f };
		sas::GameplayAttributeList mFieldAttributes;
		sf::CircleShape mGlow;
		sf::CircleShape mCore;
		sf::VertexArray mTrail;
		float mProjectileSpeed = 0.f;
		float mCastRange = 0.f;
		float mTargetTravelDistance = 0.f;
		float mTravelDistance = 0.f;
		float mFieldDuration = 0.f;
		float mFieldDamage = 0.f;
		float mVisualAge = 0.f;
		bool mHasSpawnedField = false;
	};

	bool RegisterIonStormProjectileActorType();
}

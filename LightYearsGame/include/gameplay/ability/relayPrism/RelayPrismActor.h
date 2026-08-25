#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/relayPrism/RelayPrismContracts.h"
#include "gameplay/portal/PortalDestinationRebaser.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "presentation/ability/relayPrism/RelayPrismPresentationProfile.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>

#include <cstdint>
#include <optional>

namespace ly
{
	class AreaTelegraphActor;

	class RelayPrismActor final
		: public AbilityWorldActor,
		  public ProjectileCaptureVolume,
		  public PortalDestinationRebaser
	{
	public:
		RelayPrismActor(
			World* world,
			Actor* owner,
			const RelayPrismPresentationProfile& presentationProfile,
			std::optional<sf::Vector2f> targetLocation = std::nullopt
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void BeginPortalTransit() override;
		void RebasePortalDestination(const sf::Vector2f& exitLocation) override;
		void Destroy() override;
		void Render(sf::RenderWindow& window) override;
		bool IsProjectileActor() const override { return true; }

		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		void ConfigureFromAbilityValues(
			const sas::GameplayAttributeList& values
		);

		bool TryCaptureProjectile(AbilityWorldActor& projectile) override;
		std::uint64_t GetCaptureVolumeId() const override
		{
			return mCaptureVolumeId;
		}
		float GetProjectileSpeed() const { return mProjectileSpeed; }
		float GetMaximumRange() const { return mMaximumRange; }
		float GetTravelDistance() const { return mTravelDistance; }
		bool HasReachedTarget() const { return mHasReachedTarget; }
		bool IsCaptureOpen() const { return mCaptureOpened; }

	private:
		void OpenCaptureVolume();
		void SpawnTelegraph();
		void CaptureNearbyProjectiles();
		void Move(float deltaTime);
		void ConfigureVisualGeometry();
		sf::Vector2f MakeScatterDirection(int projectileIndex);

		RelayPrismPresentationProfile mPresentationProfile;
		sf::CircleShape mGlow;
		sf::CircleShape mCore;
		sf::ConvexShape mTrail;
		weak_ptr<AreaTelegraphActor> mTelegraph;
		float mCaptureRadius = 100.f;
		float mDamageTransferRatio = 0.15f;
		float mAttackPowerCoefficient = 0.25f;
		float mMinimumScatterAngle = 30.f;
		float mMaximumScatterAngle = 90.f;
		float mMaximumBonusProjectileCount = 4.f;
		float mProjectileSpeed = 0.f;
		float mMaximumRange = 0.f;
		float mTravelDistance = 0.f;
		// Portal transit relocates the projectile. Cache the old remaining flight
		// distance before that relocation so the capture point keeps its intended
		// post-portal travel budget instead of measuring from the new exit.
		float mPortalRemainingTargetDistance = 0.f;
		std::optional<sf::Vector2f> mTargetLocation;
		sf::Vector2f mLaunchVelocity{};
		bool mHasReachedTarget = true;
		bool mCaptureOpened = false;
		int mBaseProjectileCount = 4;
		std::uint64_t mCaptureVolumeId = 0;
		float mLastScatterRotation = 0.f;
		bool mHasScatterRotation = false;
	};

	bool RegisterRelayPrismActorType();
}

#pragma once

#include "gameplay/ability/aegisReaver/AegisReaverFlightState.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/aegisReaver/AegisReaverPresentationProfile.h"

namespace ly
{
	class SpaceShip;

	class AegisReaverProjectileActor final : public AbilityWorldActor
	{
	private:
		enum class Phase
		{
			Outbound,
			HoveringAtMaximumRange,
			Returning
		};

	public:
		AegisReaverProjectileActor(
			World* world,
			Actor* owner,
			const AegisReaverPresentationProfile& presentationProfile
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Destroy() override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;
		bool IsProjectileActor() const override { return true; }
		bool CanBeReflected() const override { return mPhase == Phase::Outbound; }
		bool TryReflectProjectile(
			const ProjectileReflectionRequest& request
		) override;
		weak_ptr<AbilityWorldActor> SpawnRelayClone(
			const ProjectileRelayCloneRequest& request
		) const override;

	private:
		void BeginMaximumRangeHover();
		void TickMaximumRangeHover(float deltaTime);
		void BeginReturning();
		void MoveOutbound(float deltaTime);
		void MoveReturning(float deltaTime);
		bool ProcessOutboundContacts(
			const sf::Vector2f& start,
			const sf::Vector2f& end,
			float travelledDistance
		);
		void TryHitShip(SpaceShip& target);
		void UpdateVelocityAndRotation(const sf::Vector2f& direction, float speed);
		SpaceShip* GetReturnOwner() const;
		void ReportFlightTerminal(bool returned);

		AegisReaverPresentationProfile mPresentationProfile;
		shared_ptr<AegisReaverFlightState> mFlightState;
		float mProjectileSpeed = 0.f;
		float mReturnSpeed = 0.f;
		float mMaximumRange = 0.f;
		float mOutboundTravelDistance = 0.f;
		float mOutboundElapsed = 0.f;
		float mReturnElapsed = 0.f;
		float mMaximumRangeHoverElapsed = 0.f;
		sf::Vector2f mMaximumRangeHoverStartDirection{ 0.f, -1.f };
		float mShieldConversionRatio = 0.f;
		float mShieldStealRatio = 0.f;
		float mPostPrismShieldStealMultiplier = 1.f;
		float mReturnShieldPayload = 0.f;
		float mSpinDegrees = 0.f;
		Phase mPhase = Phase::Outbound;
		bool mFlightRegistered = false;
		bool mFlightTerminalReported = false;
	};

	bool RegisterAegisReaverProjectileActorType();
}

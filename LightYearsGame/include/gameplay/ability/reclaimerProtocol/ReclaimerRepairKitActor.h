#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/reclaimerProtocol/ReclaimerProtocolPresentationProfile.h"

namespace ly
{
	class ReclaimerRepairKitActor final : public AbilityWorldActor
	{
	public:
		ReclaimerRepairKitActor(
			World* world,
			Actor* owner,
			const ReclaimerProtocolPresentationProfile& presentationProfile
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void OnActorBeginOverlap(Actor* otherActor) override;
		bool IsProjectileActor() const override { return false; }

		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;

		void SetResolvedHealRatio(float healRatio) { mResolvedHealRatio = healRatio; }
		float GetResolvedHealRatio() const { return mResolvedHealRatio; }

	private:
		ReclaimerProtocolPresentationProfile mPresentationProfile;
		float mResolvedHealRatio = 0.04f;
		float mFlashTimeRemaining = 0.f;
		bool mIsCollected = false;
	};

	bool RegisterReclaimerRepairKitActorType();
}
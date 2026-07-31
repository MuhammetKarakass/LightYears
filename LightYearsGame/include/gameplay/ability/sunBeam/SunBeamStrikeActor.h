#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/ability/sunBeam/SunBeamActorBase.h"
#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"

namespace ly
{
	class AreaTelegraphActor;

	class SunBeamStrikeActor final : public SunBeamActorBase
	{
	public:
		SunBeamStrikeActor(
			World* world,
			Actor* owner,
			const SunBeamPresentationProfile& presentationProfile
		);

		void Destroy() override;

	protected:
		void OnSunBeamBeginPlay() override;
		void ConfigureSunBeam(const sas::GameplayAttributeList& attributes) override;
		void TickSunBeam(float deltaTime) override;
		SunBeamVisualFrame BuildSunBeamVisualFrame() const override;

	private:
		enum class StrikePhase
		{
			Telegraph,
			Arrival,
			Settle,
			Impact
		};

		void UpdatePreImpactTimeline();
		void BeginImpact(float impactElapsed = 0.f);
		void SynchronizeTelegraph();
		void SpawnTelegraph();
		void DestroyTelegraph();

		sf::Vector2f mImpactLocation{ 0.f, 0.f };
		AreaTelegraphVisualDefinition mTelegraphVisualDefinition;
		weak_ptr<AreaTelegraphActor> mTelegraph;
		float mImpactRadius = 0.f;
		float mTelegraphDuration = 0.f;
		float mArrivalDuration = 0.f;
		float mImpactDelay = 0.f;
		float mImpactVisualDuration = 0.f;
		float mTimelineElapsed = 0.f;
		float mPhaseElapsed = 0.f;
		StrikePhase mPhase = StrikePhase::Arrival;
		bool mHasImpacted = false;
	};

	bool RegisterSunBeamStrikeActorType();
}

#pragma once

#include "gameplay/ability/actors/sunBeam/SunBeamActorBase.h"
#include "gameConfigs/AbilityVisualStructs.h"

namespace ly
{
	class AreaTelegraphActor;

	class SunBeamStrikeActor final : public SunBeamActorBase
	{
	public:
		SunBeamStrikeActor(
			World* world,
			Actor* owner,
			const AreaTelegraphVisualDefinition& telegraphVisualDefinition,
			const SunBeamVisualDefinition& sunBeamVisualDefinition
		);

		void Destroy() override;

	protected:
		void OnSunBeamBeginPlay() override;
		void ConfigureSunBeam(const GameplayAttributeList& attributes) override;
		void TickSunBeam(float deltaTime) override;
		SunBeamVisualFrame BuildSunBeamVisualFrame() const override;

	private:
		enum class StrikePhase
		{
			Telegraph,
			Arrival,
			Impact
		};

		void BeginArrival();
		void BeginImpact();
		void SpawnTelegraph();
		void DestroyTelegraph();

		sf::Vector2f mImpactLocation{ 0.f, 0.f };
		AreaTelegraphVisualDefinition mTelegraphVisualDefinition;
		weak_ptr<AreaTelegraphActor> mTelegraph;
		float mImpactRadius = 0.f;
		float mTelegraphDuration = 0.f;
		float mArrivalDuration = 0.f;
		float mImpactVisualDuration = 0.f;
		float mPhaseElapsed = 0.f;
		StrikePhase mPhase = StrikePhase::Arrival;
		bool mHasImpacted = false;
	};

	bool RegisterSunBeamStrikeActorType();
}

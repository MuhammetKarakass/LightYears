#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/arcScythes/ArcScythesPresentationProfile.h"

namespace ly
{
	// One actor owns both opposing beams. This makes the 0.25-second combat tick
	// global to the ability and guarantees overlap cannot double-hit one target.
	class ArcScythesBeamActor final : public AbilityWorldActor
	{
	public:
		ArcScythesBeamActor(
			World* world,
			Actor* owner,
			const ArcScythesPresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(
			const sas::GameplayAttributeList& attributes
		) override;

	private:
		void PerformCombatTick();

		ArcScythesPresentationProfile mPresentationProfile;
		float mRange = 0.f;
		float mCombatTickInterval = 0.f;
		float mBeamHalfThickness = 0.f;
		float mCombatTickTimer = 0.f;
		float mVisualTime = 0.f;
	};

	bool RegisterArcScythesBeamActorType();
}

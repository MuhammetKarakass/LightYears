#pragma once

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"

namespace ly
{
	class InfernoSprayActor final : public AbilityWorldActor
	{
	public:
		InfernoSprayActor(
			World* world,
			Actor* owner,
			const InfernoSprayPresentationProfile& presentationProfile
		);

		void BeginPlay() override;
		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;
		void Destroy() override;

	private:
		void PerformCombatTick();
		sf::Vector2f GetMuzzleLocation() const;
		sf::Vector2f GetAimDirection() const;

		InfernoSprayPresentationProfile mPresentationProfile;
		float mRange = 0.f;
		float mConeAngleDegrees = 0.f;
		float mCombatTickInterval = 0.f;
		float mBaseDPS = 0.f;
		float mCombatTickTimer = 0.f;
		float mVisualTime = 0.f;
	};

	bool RegisterInfernoSprayActorType();
}

#pragma once

#include "gameplay/ability/GameAbility.h"

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class ScorchDriveTrailCoordinatorActor;

	class ScorchDriveAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

	private:
		void UpdateTrail(GameAbilityBehaviorContext& context);
		void SpawnSegment(
			GameAbilityBehaviorContext& context,
			const sf::Vector2f& location,
			const sf::Vector2f& direction
		);

		weak_ptr<ScorchDriveTrailCoordinatorActor> mCoordinator;
		sf::Vector2f mLastOwnerLocation{ 0.f, 0.f };
		float mDistanceSinceLastSegment = 0.f;
		float mSegmentSpawnDistance = 60.f;
		float mBaseSegmentLifetime = 5.f;
		float mFireDamage = 0.f;
		float mFireTickInterval = 0.25f;
		int mBurnThresholdTicks = 4;
		float mBurnDuration = 3.f;
		float mBurnTickInterval = 0.5f;
		float mBurnDamageRatio = 0.5f;
		float mReferenceMaxHealth = 100.f;
		float mMaxHealthLifetimeScale = 0.01f;
		bool mActive = false;
		bool mOwnerWasInPortalTransit = false;
	};
}

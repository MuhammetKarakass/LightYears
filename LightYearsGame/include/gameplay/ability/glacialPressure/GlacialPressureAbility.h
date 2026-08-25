#pragma once

#include "gameplay/ability/GameAbility.h"

#include <SFML/System/Vector2.hpp>

#include <set>
#include <utility>

namespace ly
{
	class GlacialPressureTelegraphActor;
	class SpaceShip;

	class GlacialPressureAbility final : public GameAbilityBehavior
	{
	public:
		bool Validate(
			const GameAbilityDefinition& definition,
			std::string* failureReason
		) const override;

		float ResolveActiveDuration(
			const GameAbilityBehaviorContext& context,
			float defaultDuration
		) const override;

		bool Activate(GameAbilityBehaviorContext& context) override;
		void Tick(GameAbilityBehaviorContext& context, float deltaTime) override;
		void End(
			GameAbilityBehaviorContext& context,
			sas::AbilityEndReason reason
		) override;

	private:
		struct PushState
		{
			weak_ptr<SpaceShip> target;
			sf::Vector2f previousLocation{ 0.f, 0.f };
			int cryoStacks = 0;
			float collisionDamage = 0.f;
			float collisionStunDuration = 0.f;
			float collisionTimeRemaining = 0.f;
			float initialImpulseSpeed = 0.f;
			float extraStunDuration = 0.f;
			bool extraStunPending = false;
			bool wasInPortalTransit = false;
		};

		using CollisionPair = std::pair<const Actor*, const Actor*>;

		void Discharge(GameAbilityBehaviorContext& context);
		void TickPushes(GameAbilityBehaviorContext& context, float deltaTime);
		void ResolveCollision(
			GameAbilityBehaviorContext& context,
			PushState& movingState,
			SpaceShip& movingTarget,
			SpaceShip& collidedTarget,
			float impactSpeed
		);
		void ApplyStun(
			GameAbilityBehaviorContext& context,
			SpaceShip& target,
			float duration
		) const;
		PushState* FindPushState(SpaceShip* target);
		void EmitEvent(
			GameAbilityBehaviorContext& context,
			const GameplayTag& eventTag
		) const;

		weak_ptr<GlacialPressureTelegraphActor> mTelegraph;
		List<PushState> mPushStates;
		std::set<CollisionPair> mResolvedCollisionPairs;
		float mFocusElapsed = 0.f;
		float mPushDuration = 0.f;
		bool mDischarged = false;
	};
}

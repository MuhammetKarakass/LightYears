#pragma once

#include "framework/Core.h"
#include "gameplay/enemy/EnemyBehaviorDecision.h"
#include "gameplay/enemy/EnemyBehaviorProfile.h"

#include <SFML/System/Vector2.hpp>
#include <memory>
#include <optional>
#include <string>

namespace ly
{
	class Actor;

	struct EnemySlotCommand
	{
		sas::AbilitySlot slot = sas::AbilitySlot::None;
		bool inputHeld = false;
	};

	struct EnemyBehaviorIntent
	{
		sf::Vector2f movementDirection{};
		sf::Vector2f aimTargetLocation{};
		weak_ptr<Actor> target;
		List<EnemySlotCommand> slotCommands;

		bool HasTarget() const noexcept { return !target.expired(); }
	};

	struct EnemySlotDecisionState
	{
		sas::AbilitySlot slot = sas::AbilitySlot::None;
		float retryRemaining = 0.f;
		bool releasePending = false;
		bool holdActive = false;
	};

	class EnemyBehaviorRuntime final
	{
	public:
		bool Initialize(const EnemyBehaviorProfile& profile, std::string* failureReason = nullptr);
		EnemyBehaviorIntent Tick(Actor& source, float deltaTime);
		void Clear();
		const EnemyBehaviorProfile* GetProfile() const noexcept
		{
			return mProfile ? &*mProfile : nullptr;
		}

	private:
		bool IsTargetValid(const Actor& source, const shared_ptr<Actor>& target) const;
		shared_ptr<Actor> FindTarget(const Actor& source) const;
		void RefreshTarget(const Actor& source);

		std::optional<EnemyBehaviorProfile> mProfile;
		weak_ptr<Actor> mTarget;
		bool mHadTarget = false;
		float mTargetRefreshRemaining = 0.f;
		float mStrafeDirection = 1.f;
		float mStrafeChangeRemaining = 0.f;
		List<EnemySlotDecisionState> mSlotStates;
	};
}

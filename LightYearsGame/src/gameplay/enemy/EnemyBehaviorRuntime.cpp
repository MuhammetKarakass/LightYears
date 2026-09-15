#include "gameplay/enemy/EnemyBehaviorRuntime.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/enemy/EnemyBehaviorProfileValidator.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/targeting/TargetRelation.h"
#include "spaceShip/SpaceShip.h"

#include <cmath>
#include <limits>
#include <vector>

namespace ly
{
	namespace
	{
		sf::Vector2f NormalizeOrZero(const sf::Vector2f& value)
		{
			const float length = GetVectorLength(value);
			return std::isfinite(length) && length > 0.001f ? value / length : sf::Vector2f{};
		}
	}

	bool EnemyBehaviorRuntime::Initialize(const EnemyBehaviorProfile& profile, std::string* failureReason)
	{
		if (!EnemyBehaviorProfileValidator::Validate(profile, failureReason)) return false;
		mProfile = profile;
		mTarget.reset();
		mHadTarget = false;
		mTargetRefreshRemaining = 0.f;
		mStrafeDirection = 1.f;
		mStrafeChangeRemaining = profile.strafeDirectionChangeInterval;
		mSlotStates.clear();
		mSlotStates.reserve(profile.slotRules.size());
		for (const EnemySlotDecisionRule& rule : profile.slotRules)
			mSlotStates.push_back({ rule.slot, 0.f, false, false });
		return true;
	}

	bool EnemyBehaviorRuntime::IsTargetValid(const Actor& source, const shared_ptr<Actor>& target) const
	{
		if (!mProfile || !target || target->GetIsPendingDestroy() || dynamic_cast<const Combatant*>(target.get()) == nullptr || !source.CanCollideWith(target.get())) return false;
		const CollisionLayer opposingLayer = targeting::ResolveOpposingLayer(source);
		if (opposingLayer == CollisionLayer::None || (target->GetCollisionLayer() & opposingLayer) == CollisionLayer::None) return false;
		if (const SpaceShip* ship = dynamic_cast<const SpaceShip*>(target.get()); ship && ship->GetHealthComponent().GetHealth() <= 0.f) return false;
		const sf::Vector2f delta = target->GetActorLocation() - source.GetActorLocation();
		return std::isfinite(delta.x) && std::isfinite(delta.y) && GetVectorLength(delta) <= mProfile->targetSearchRange;
	}

	shared_ptr<Actor> EnemyBehaviorRuntime::FindTarget(const Actor& source) const
	{
		if (!mProfile || !source.GetWorld()) return {};
		shared_ptr<Actor> nearest;
		float nearestDistanceSquared = std::numeric_limits<float>::max();
		for (const shared_ptr<Actor>& candidate : targeting::FindOpposingCombatants(*source.GetWorld(), source, mProfile->targetSearchRange))
		{
			if (!IsTargetValid(source, candidate)) continue;
			const sf::Vector2f delta = candidate->GetActorLocation() - source.GetActorLocation();
			const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
			if (!nearest || distanceSquared < nearestDistanceSquared)
			{
				nearest = candidate;
				nearestDistanceSquared = distanceSquared;
			}
		}
		return nearest;
	}

	void EnemyBehaviorRuntime::RefreshTarget(const Actor& source)
	{
		const shared_ptr<Actor> target = FindTarget(source);
		mTarget = target;
		mHadTarget = static_cast<bool>(target);
	}

	EnemyBehaviorIntent EnemyBehaviorRuntime::Tick(Actor& source, float deltaTime)
	{
		EnemyBehaviorIntent intent;
		if (!mProfile || source.GetIsPendingDestroy()) return intent;
		const float safeDeltaTime = std::isfinite(deltaTime) && deltaTime > 0.f ? deltaTime : 0.f;
		mTargetRefreshRemaining = std::max(0.f, mTargetRefreshRemaining - safeDeltaTime);
		shared_ptr<Actor> target = mTarget.lock();
		const bool targetInvalid = target ? !IsTargetValid(source, target) : mHadTarget;
		if (targetInvalid || mTargetRefreshRemaining <= 0.f)
		{
			RefreshTarget(source);
			target = mTarget.lock();
			mTargetRefreshRemaining = mProfile->targetRefreshInterval;
		}
		const bool hasTarget = IsTargetValid(source, target);
		float distance = 0.f;
		float alignment = 0.f;
		if (hasTarget)
		{
			intent.target = target;
			intent.aimTargetLocation = target->GetActorLocation();
			const sf::Vector2f delta = intent.aimTargetLocation - source.GetActorLocation();
			distance = GetVectorLength(delta);
			const sf::Vector2f radial = NormalizeOrZero(delta);
			intent.movementDirection = ResolveEnemyMovementDirection({ mProfile->movementMode, distance, mProfile->desiredDistance, mProfile->minimumDistance, mProfile->maximumDistance, mStrafeDirection, radial });
			const sf::Vector2f forward = NormalizeOrZero(source.GetActorForwardDirection());
			alignment = forward.x * radial.x + forward.y * radial.y;
		}

		std::vector<bool> eligible(mProfile->slotRules.size(), false);
		for (size_t i = 0; i < mProfile->slotRules.size(); ++i)
		{
			mSlotStates[i].retryRemaining = std::max(0.f, mSlotStates[i].retryRemaining - safeDeltaTime);
			eligible[i] = ShouldActivateEnemySlot(
				mProfile->slotRules[i],
				EnemySlotDecisionContext{ hasTarget, distance, alignment }
			);
		}

		const auto isAbilityRule = [](sas::AbilitySlot slot)
		{
			return sas::IsLoadoutAbilitySlot(slot);
		};
		const auto isHigherPriority = [&](size_t left, size_t right)
		{
			const EnemySlotDecisionRule& leftRule = mProfile->slotRules[left];
			const EnemySlotDecisionRule& rightRule = mProfile->slotRules[right];
			return leftRule.priority > rightRule.priority ||
				(leftRule.priority == rightRule.priority && static_cast<int>(leftRule.slot) < static_cast<int>(rightRule.slot));
		};
		int selectedAbilityRule = -1;
		for (size_t i = 0; i < mProfile->slotRules.size(); ++i)
		{
			const EnemySlotDecisionRule& rule = mProfile->slotRules[i];
			const EnemySlotDecisionState& state = mSlotStates[i];
			const bool canPulse = rule.inputMode != EnemySlotInputMode::Pulse ||
				(!state.releasePending && state.retryRemaining <= 0.f);
			if (isAbilityRule(rule.slot) && eligible[i] && canPulse &&
				(selectedAbilityRule < 0 || isHigherPriority(i, static_cast<size_t>(selectedAbilityRule))))
			{
				selectedAbilityRule = static_cast<int>(i);
			}
		}

		for (size_t i = 0; i < mProfile->slotRules.size(); ++i)
		{
			const EnemySlotDecisionRule& rule = mProfile->slotRules[i];
			EnemySlotDecisionState& state = mSlotStates[i];
			EnemySlotCommand command{ rule.slot, false };
			const bool selected = rule.slot == sas::AbilitySlot::PrimaryFire ||
				selectedAbilityRule == static_cast<int>(i);
			if (rule.inputMode == EnemySlotInputMode::Hold)
			{
				state.holdActive = selected && eligible[i];
				state.releasePending = false;
				command.inputHeld = state.holdActive;
			}
			else
			{
				state.holdActive = false;
				if (state.releasePending)
				{
					state.releasePending = false;
				}
				else if (selected && eligible[i] && state.retryRemaining <= 0.f)
				{
					command.inputHeld = true;
					state.releasePending = true;
					state.retryRemaining = rule.pulseRetryInterval;
				}
			}
			intent.slotCommands.push_back(command);
		}

		if (mProfile->movementMode == EnemyMovementMode::Strafe)
		{
			mStrafeChangeRemaining = std::max(0.f, mStrafeChangeRemaining - safeDeltaTime);
			if (mStrafeChangeRemaining <= 0.f)
			{
				mStrafeDirection = -mStrafeDirection;
				mStrafeChangeRemaining = mProfile->strafeDirectionChangeInterval;
			}
		}
		return intent;
	}

	void EnemyBehaviorRuntime::Clear()
	{
		mProfile.reset();
		mTarget.reset();
		mHadTarget = false;
		mTargetRefreshRemaining = 0.f;
		mStrafeDirection = 1.f;
		mStrafeChangeRemaining = 0.f;
		mSlotStates.clear();
	}
}

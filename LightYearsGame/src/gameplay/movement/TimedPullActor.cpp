#include "gameplay/movement/TimedPullActor.h"
#include "framework/World.h"
#include "gameplay/control/ControlEffects.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/targeting/CombatantTargetQuery.h"

#include <algorithm>
#include <cmath>

namespace vega
{
	TimedPullActor::TimedPullActor(World* world, const weak_ptr<Actor>& source, const weak_ptr<Actor>& anchor, float radius, float speed, float seconds)
		: Actor{ world }, mSource{ source }, mAnchor{ anchor }, mRadius{ std::max(0.f, radius) }, mSpeed{ std::max(0.f, speed) }, mSeconds{ std::max(0.f, seconds) }
	{
		SetEnablePhysics(false);
		if (const auto anchorActor = anchor.lock()) mCenter = anchorActor->GetActorLocation();
	}

	// Movement starts at spawn, before BeginPlay, so destruction without EndPlay must release it too.
	TimedPullActor::~TimedPullActor() { ReleaseAll(); }

	weak_ptr<TimedPullActor> TimedPullActor::Spawn(Actor& source, Actor& anchor, float radius, float speed, float seconds)
	{
		if (!source.GetWorld() || !(radius > 0.f) || !(speed > 0.f) || !(seconds > 0.f)) return {};
		auto actor = source.GetWorld()->SpawnActor<TimedPullActor>(source.GetWeakActor(), anchor.GetWeakActor(), radius, speed, seconds);
		if (const auto spawned = actor.lock())
		{
			spawned->SetActorLocation(anchor.GetActorLocation());
			// Targets are chosen and start moving at the moment of the pull, not when the pending actor first ticks.
			spawned->CollectTargets();
			spawned->Steer(0.f);
		}
		return actor;
	}

	void TimedPullActor::CollectTargets()
	{
		mCollected = true;
		const auto source = mSource.lock();
		if (!source || !source->GetWorld()) return;
		const auto anchor = mAnchor.lock();
		for (const auto& candidate : targeting::FindOpposingCombatants(*source->GetWorld(), *source, mCenter, mRadius, false))
		{
			if (!candidate || candidate->GetIsPendingDestroy() || candidate == anchor) continue;
			const float seconds = ResolveControlDuration(*candidate, GameplayTags::State::Effect::Control::Displaced, mSeconds);
			if (seconds > 0.f) mTargets.push_back({ candidate, seconds });
		}
	}

	void TimedPullActor::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);
		if (!mCollected) CollectTargets();
		const float step = std::max(0.f, deltaTime);
		for (Target& target : mTargets)
		{
			const auto actor = target.actor.lock();
			if (!actor || target.remaining <= 0.f) continue;
			target.remaining -= step;
			if (target.remaining <= 0.f || actor->GetIsPendingDestroy()) { target.remaining = 0.f; Release(*actor); }
		}
		if (!Steer(step)) Destroy();
	}

	bool TimedPullActor::Steer(float deltaTime)
	{
		if (const auto anchor = mAnchor.lock(); anchor && !anchor->GetIsPendingDestroy()) mCenter = anchor->GetActorLocation();
		SetActorLocation(mCenter);
		const movement::MovementInfluenceSourceId sourceId = GetUniqueID();
		bool anyActive = false;
		for (const Target& target : mTargets)
		{
			const auto actor = target.actor.lock();
			if (!actor || actor->GetIsPendingDestroy() || target.remaining <= 0.f) continue;
			anyActive = true;
			const sf::Vector2f offset = mCenter - actor->GetActorLocation();
			const float distance = std::sqrt(offset.x * offset.x + offset.y * offset.y);
			// Never overshoot the center within one step.
			const float speed = deltaTime > 0.f ? std::min(mSpeed, distance / deltaTime) : mSpeed;
			const sf::Vector2f velocity = distance > 0.001f ? offset / distance * speed : sf::Vector2f{};
			movement::MovementInfluenceService::SetForcedMovement(*actor, movement::ForcedMovementRequest{ sourceId, velocity, 0 });
		}
		return anyActive;
	}

	void TimedPullActor::EndPlay()
	{
		ReleaseAll();
		Actor::EndPlay();
	}

	void TimedPullActor::Release(Actor& target)
	{
		movement::MovementInfluenceService::RemoveSource(target, GetUniqueID());
	}

	void TimedPullActor::ReleaseAll()
	{
		for (Target& target : mTargets)
		{
			if (const auto actor = target.actor.lock()) Release(*actor);
			target.remaining = 0.f;
		}
		mTargets.clear();
	}
}

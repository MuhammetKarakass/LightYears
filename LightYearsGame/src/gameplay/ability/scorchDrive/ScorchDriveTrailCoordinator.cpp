#include "gameplay/ability/scorchDrive/ScorchDriveTrailCoordinator.h"

#include "framework/World.h"
#include "gameplay/ability/scorchDrive/ScorchDriveFireSegmentActor.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include "gameplay/targeting/TargetingTypes.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace ly
{
	namespace
	{
		weak_ptr<Actor> MakeWeakActor(Actor* actor)
		{
			if (!actor)
			{
				return {};
			}

			const shared_ptr<Object> object = actor->GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<Actor>(object)
				: weak_ptr<Actor>{};
		}
	}

	ScorchDriveTrailCoordinatorActor::ScorchDriveTrailCoordinatorActor(
		World* world,
		Actor* owner,
		float fireDamage,
		float fireTickInterval,
		int burnThresholdTicks,
		float burnDuration,
		float burnTickInterval,
		float burnDamageRatio,
		const sas::ContentId& sourceAbilityId,
		const List<GameplayTag>& sourceAbilityTags,
		const List<GameplayTag>& damageTags
	)
		: Actor(world)
		, mOwner(MakeWeakActor(owner))
		, mFireDamage(std::max(0.f, fireDamage))
		, mFireTickInterval(std::max(0.001f, fireTickInterval))
		, mBurnThresholdTicks(std::max(1, burnThresholdTicks))
		, mBurnDuration(std::max(0.f, burnDuration))
		, mBurnTickInterval(std::max(0.f, burnTickInterval))
		, mBurnDamageRatio(std::max(0.f, burnDamageRatio))
		, mSourceAbilityId(sourceAbilityId)
		, mSourceAbilityTags(sourceAbilityTags)
		, mDamageTags(damageTags)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void ScorchDriveTrailCoordinatorActor::AddSegment(
		const weak_ptr<ScorchDriveFireSegmentActor>& segment
	)
	{
		if (!segment.expired())
		{
			mSegments.push_back(segment);
		}
	}

	void ScorchDriveTrailCoordinatorActor::PruneSegments()
	{
		mSegments.erase(
			std::remove_if(
				mSegments.begin(),
				mSegments.end(),
				[](const weak_ptr<ScorchDriveFireSegmentActor>& segment)
				{
					const shared_ptr<ScorchDriveFireSegmentActor> locked = segment.lock();
					return !locked || locked->GetIsPendingDestroy();
				}
			),
			mSegments.end()
		);
	}

	void ScorchDriveTrailCoordinatorActor::DestroySegments()
	{
		for (const weak_ptr<ScorchDriveFireSegmentActor>& segmentWeak : mSegments)
		{
			if (const shared_ptr<ScorchDriveFireSegmentActor> segment = segmentWeak.lock())
			{
				if (!segment->GetIsPendingDestroy())
				{
					segment->Destroy();
				}
			}
		}
		mSegments.clear();
	}

	void ScorchDriveTrailCoordinatorActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		const shared_ptr<Actor> owner = mOwner.lock();
		if (!owner || owner->GetIsPendingDestroy())
		{
			DestroySegments();
			Destroy();
			return;
		}

		PruneSegments();
		if (!mCastActive && mSegments.empty())
		{
			Destroy();
			return;
		}

		const int tickCount = time::ConsumePeriodicTicks(
			mTickAccumulator,
			deltaTime,
			mFireTickInterval
		);
		for (int tickIndex = 0; tickIndex < tickCount; ++tickIndex)
		{
			PerformCombatTick();
		}
		Actor::Tick(deltaTime);
	}

	bool ScorchDriveTrailCoordinatorActor::IsOpposingCombatant(
		const Actor& source,
		const Actor& actor
	) const
	{
		return dynamic_cast<const Combatant*>(&actor) != nullptr &&
			HasCollisionLayer(
				actor.GetCollisionLayer(),
				targeting::ResolveOpposingLayer(source)
			);
	}

	void ScorchDriveTrailCoordinatorActor::PerformCombatTick()
	{
		World* world = GetWorld();
		const shared_ptr<Actor> owner = mOwner.lock();
		if (!world || !owner || owner->GetIsPendingDestroy() || mFireDamage <= 0.f)
		{
			return;
		}

		const CollisionLayer opposingLayer = targeting::ResolveOpposingLayer(*owner);
		if (opposingLayer == CollisionLayer::None)
		{
			return;
		}

		List<shared_ptr<Actor>> targets;
		std::unordered_set<Actor*> seenTargets;
		for (const weak_ptr<ScorchDriveFireSegmentActor>& segmentWeak : mSegments)
		{
			const shared_ptr<ScorchDriveFireSegmentActor> segment = segmentWeak.lock();
			if (!segment || segment->GetIsPendingDestroy())
			{
				continue;
			}

			targeting::TargetingQuery query;
			query.source = owner.get();
			query.origin = segment->GetActorLocation();
			query.shape = targeting::TargetingShape::Rectangle;
			query.direction = segment->GetSegmentDirection();
			query.rectangleHalfExtents = {
				segment->GetSegmentLength() * 0.5f,
				segment->GetSegmentWidth() * 0.5f
			};
			query.range = std::sqrt(
				query.rectangleHalfExtents.x * query.rectangleHalfExtents.x +
				query.rectangleHalfExtents.y * query.rectangleHalfExtents.y
			);
			query.requiredTargetLayers = opposingLayer;
			query.requireCollisionCompatibility = true;
			query.filter = [this, owner](
				const Actor*,
				const Actor& candidate,
				const targeting::TargetingCandidate&)
			{
				return IsOpposingCombatant(*owner, candidate);
			};

			for (const targeting::TargetingCandidate& candidate :
				targeting::AutoTargeting::FindTargets(*world, query))
			{
				if (candidate.actor && seenTargets.insert(candidate.actor.get()).second)
				{
					targets.push_back(candidate.actor);
				}
			}
		}

		DamagePayload payload = DamageTypeSystem::BuildPayload(mDamageTags);
		payload.igniteStacks = 1;
		payload.burnMaxStacks = mBurnThresholdTicks;
		payload.burnDuration = mBurnDuration;
		payload.burnTickInterval = mBurnTickInterval;
		payload.burnDamagePerTick = mFireDamage * mBurnDamageRatio;

		for (const shared_ptr<Actor>& target : targets)
		{
			if (!target || target->GetIsPendingDestroy())
			{
				continue;
			}
			ApplyCombatDamage(
				*target,
				mFireDamage,
				owner.get(),
				mDamageTags,
				payload,
				mSourceAbilityId,
				mSourceAbilityTags
			);
		}
	}
}

#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"

#include "framework/Actor.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
	class TestActor final : public ly::Actor
	{
	public:
		explicit TestActor(ly::World* world)
			: Actor{ world }
		{
		}
	};

	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}

	bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.0001f;
	}
}

int main()
{
	ly::World world{ nullptr };
	const ly::shared_ptr<TestActor> source = world.SpawnActor<TestActor>().lock();
	const ly::shared_ptr<TestActor> nearest = world.SpawnActor<TestActor>().lock();
	const ly::shared_ptr<TestActor> farthest = world.SpawnActor<TestActor>().lock();
	const ly::shared_ptr<TestActor> outside = world.SpawnActor<TestActor>().lock();
	const ly::shared_ptr<TestActor> rectangleSide = world.SpawnActor<TestActor>().lock();
	if (!source || !nearest || !farthest || !outside || !rectangleSide)
	{
		return Fail("AutoTargeting test actors could not be created");
	}

	source->SetActorLocation({ 0.f, 0.f });
	source->SetCollisionLayer(CollisionLayer::Player);
	nearest->SetActorLocation({ 100.f, 0.f });
	nearest->SetCollisionLayer(CollisionLayer::Enemy);
	nearest->SetCollisionMask(CollisionLayer::PlayerBullet);
	farthest->SetActorLocation({ 200.f, 0.f });
	farthest->SetCollisionLayer(CollisionLayer::Enemy);
	farthest->SetCollisionMask(CollisionLayer::PlayerBullet);
	outside->SetActorLocation({ 500.f, 0.f });
	outside->SetCollisionLayer(CollisionLayer::Enemy);
	outside->SetCollisionMask(CollisionLayer::PlayerBullet);
	rectangleSide->SetActorLocation({ 50.f, 60.f });
	// Keep the geometry-only fixture out of the earlier radius/cone assertions.
	rectangleSide->SetCollisionLayer(CollisionLayer::None);
	rectangleSide->SetCollisionMask(CollisionLayer::PlayerBullet);
	world.TickInternal(0.f);
	if (!ly::targeting::IsOpposingTarget(*source, *nearest) ||
		ly::targeting::IsOpposingTarget(*source, *source))
	{
		return Fail("Target relation policy did not resolve opposing collision layers");
	}

	ly::targeting::TargetingQuery nearestQuery;
	nearestQuery.source = source.get();
	nearestQuery.origin = source->GetActorLocation();
	nearestQuery.range = 250.f;
	nearestQuery.maxTargets = 1;
	nearestQuery.requiredTargetLayers = CollisionLayer::Enemy;
	nearestQuery.requiredCandidateCollisionMask = CollisionLayer::PlayerBullet;
	const ly::shared_ptr<ly::Actor> nearestTarget =
		ly::targeting::AutoTargeting::FindTarget(world, nearestQuery).lock();
	if (nearestTarget.get() != nearest.get())
	{
		return Fail("AutoTargeting did not select the nearest valid target");
	}

	ly::targeting::TargetingQuery allTargetsQuery = nearestQuery;
	allTargetsQuery.maxTargets = 0;
	const ly::List<ly::targeting::TargetingCandidate> allTargets =
		ly::targeting::AutoTargeting::FindTargets(world, allTargetsQuery);
	if (allTargets.size() != 2 ||
		allTargets[0].actor.get() != nearest.get() ||
		allTargets[1].actor.get() != farthest.get() ||
		!NearlyEqual(allTargets[0].distanceSquared, 10000.f))
	{
		return Fail("AutoTargeting did not apply range, filters, or nearest ordering");
	}

	ly::targeting::TargetingQuery coneQuery = nearestQuery;
	coneQuery.maxTargets = 0;
	coneQuery.shape = ly::targeting::TargetingShape::Cone;
	coneQuery.direction = { 1.f, 0.f };
	coneQuery.coneHalfAngleRadians = 0.2f;
	const ly::shared_ptr<TestActor> outsideCone = world.SpawnActor<TestActor>().lock();
	if (!outsideCone)
	{
		return Fail("AutoTargeting cone test actor could not be created");
	}
	outsideCone->SetActorLocation({ 100.f, 100.f });
	outsideCone->SetCollisionLayer(CollisionLayer::Enemy);
	outsideCone->SetCollisionMask(CollisionLayer::PlayerBullet);
	world.TickInternal(0.f);
	const ly::List<ly::targeting::TargetingCandidate> coneTargets =
		ly::targeting::AutoTargeting::FindTargets(world, coneQuery);
	if (coneTargets.size() != 2 || coneTargets[0].actor.get() != nearest.get())
	{
		return Fail("AutoTargeting cone shape accepted a target outside its angle");
	}

	ly::targeting::TargetingQuery rectangleQuery = nearestQuery;
	rectangleSide->SetCollisionLayer(CollisionLayer::Enemy);
	world.TickInternal(0.f);
	rectangleQuery.shape = ly::targeting::TargetingShape::Rectangle;
	rectangleQuery.direction = { 1.f, 0.f };
	rectangleQuery.range = 120.f;
	rectangleQuery.rectangleHalfExtents = { 110.f, 50.f };
	const ly::List<ly::targeting::TargetingCandidate> rectangleTargets =
		ly::targeting::AutoTargeting::FindTargets(world, rectangleQuery);
	if (rectangleTargets.size() != 1 ||
		rectangleTargets.front().actor.get() != nearest.get())
	{
		return Fail("AutoTargeting rectangle shape did not reject outside width or length");
	}
	// Restore the fixture so the remaining target-lock assertions keep their
	// original nearest-target setup.
	rectangleSide->SetCollisionLayer(CollisionLayer::None);
	world.TickInternal(0.f);
	nearestQuery.excludedTargets.push_back(outsideCone.get());

	ly::targeting::TargetingQuery customSelectionQuery = allTargetsQuery;
	customSelectionQuery.maxTargets = 1;
	customSelectionQuery.selector = [](
		ly::List<ly::targeting::TargetingCandidate>& candidates,
		std::size_t
	)
	{
		std::stable_sort(candidates.begin(), candidates.end(), [](
			const ly::targeting::TargetingCandidate& left,
			const ly::targeting::TargetingCandidate& right
		)
		{
			return left.distanceSquared > right.distanceSquared;
		});
	};
	const ly::shared_ptr<ly::Actor> customTarget =
		ly::targeting::AutoTargeting::FindTarget(world, customSelectionQuery).lock();
	if (customTarget.get() != farthest.get())
	{
		return Fail("AutoTargeting custom selector was not applied");
	}

	ly::targeting::TargetLock lock;
	if (lock.Acquire(world, nearestQuery).lock().get() != nearest.get())
	{
		return Fail("AutoTargeting target lock did not acquire its initial target");
	}
	nearest->SetActorLocation({ 400.f, 0.f });
	if (lock.Update(world, nearestQuery, ly::targeting::TargetLockMode::Sticky).lock())
	{
		return Fail("Sticky target lock did not clear an invalid target");
	}
	const ly::shared_ptr<ly::Actor> reacquired = lock.Update(
		world,
		nearestQuery,
		ly::targeting::TargetLockMode::ReacquireIfInvalid
	).lock();
	if (reacquired.get() != farthest.get())
	{
		return Fail("Reacquire target lock did not select a replacement target");
	}

	return 0;
}

#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"

#include "framework/Actor.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
	class TestActor final : public vega::Actor
	{
	public:
		explicit TestActor(vega::World* world)
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
	vega::InstallVegaEngineAssets();
	vega::World world{ nullptr };
	const vega::shared_ptr<TestActor> source = world.SpawnActor<TestActor>().lock();
	const vega::shared_ptr<TestActor> nearest = world.SpawnActor<TestActor>().lock();
	const vega::shared_ptr<TestActor> farthest = world.SpawnActor<TestActor>().lock();
	const vega::shared_ptr<TestActor> outside = world.SpawnActor<TestActor>().lock();
	const vega::shared_ptr<TestActor> rectangleSide = world.SpawnActor<TestActor>().lock();
	if (!source || !nearest || !farthest || !outside || !rectangleSide)
	{
		return Fail("AutoTargeting test actors could not be created");
	}

	source->SetActorLocation({ 0.f, 0.f });
	source->SetCollisionLayer(vega::GameCollision::Player);
	nearest->SetActorLocation({ 100.f, 0.f });
	nearest->SetCollisionLayer(vega::GameCollision::Enemy);
	nearest->SetCollisionMask(vega::GameCollision::PlayerBullet);
	farthest->SetActorLocation({ 200.f, 0.f });
	farthest->SetCollisionLayer(vega::GameCollision::Enemy);
	farthest->SetCollisionMask(vega::GameCollision::PlayerBullet);
	outside->SetActorLocation({ 500.f, 0.f });
	outside->SetCollisionLayer(vega::GameCollision::Enemy);
	outside->SetCollisionMask(vega::GameCollision::PlayerBullet);
	rectangleSide->SetActorLocation({ 50.f, 60.f });
	// Keep the geometry-only fixture out of the earlier radius/cone assertions.
	rectangleSide->SetCollisionLayer(vega::CollisionLayer::None);
	rectangleSide->SetCollisionMask(vega::GameCollision::PlayerBullet);
	world.TickInternal(0.f);
	if (!vega::targeting::IsOpposingTarget(*source, *nearest) ||
		vega::targeting::IsOpposingTarget(*source, *source))
	{
		return Fail("Target relation policy did not resolve opposing collision layers");
	}

	vega::targeting::TargetingQuery nearestQuery;
	nearestQuery.source = source.get();
	nearestQuery.origin = source->GetActorLocation();
	nearestQuery.range = 250.f;
	nearestQuery.maxTargets = 1;
	nearestQuery.requiredTargetLayers = vega::GameCollision::Enemy;
	nearestQuery.requiredCandidateCollisionMask = vega::GameCollision::PlayerBullet;
	const vega::shared_ptr<vega::Actor> nearestTarget =
		vega::targeting::AutoTargeting::FindTarget(world, nearestQuery).lock();
	if (nearestTarget.get() != nearest.get())
	{
		return Fail("AutoTargeting did not select the nearest valid target");
	}

	vega::targeting::TargetingQuery allTargetsQuery = nearestQuery;
	allTargetsQuery.maxTargets = 0;
	const vega::List<vega::targeting::TargetingCandidate> allTargets =
		vega::targeting::AutoTargeting::FindTargets(world, allTargetsQuery);
	if (allTargets.size() != 2 ||
		allTargets[0].actor.get() != nearest.get() ||
		allTargets[1].actor.get() != farthest.get() ||
		!NearlyEqual(allTargets[0].distanceSquared, 10000.f))
	{
		return Fail("AutoTargeting did not apply range, filters, or nearest ordering");
	}

	vega::targeting::TargetingQuery coneQuery = nearestQuery;
	coneQuery.maxTargets = 0;
	coneQuery.shape = vega::targeting::TargetingShape::Cone;
	coneQuery.direction = { 1.f, 0.f };
	coneQuery.coneHalfAngleRadians = 0.2f;
	const vega::shared_ptr<TestActor> outsideCone = world.SpawnActor<TestActor>().lock();
	if (!outsideCone)
	{
		return Fail("AutoTargeting cone test actor could not be created");
	}
	outsideCone->SetActorLocation({ 100.f, 100.f });
	outsideCone->SetCollisionLayer(vega::GameCollision::Enemy);
	outsideCone->SetCollisionMask(vega::GameCollision::PlayerBullet);
	world.TickInternal(0.f);
	const vega::List<vega::targeting::TargetingCandidate> coneTargets =
		vega::targeting::AutoTargeting::FindTargets(world, coneQuery);
	if (coneTargets.size() != 2 || coneTargets[0].actor.get() != nearest.get())
	{
		return Fail("AutoTargeting cone shape accepted a target outside its angle");
	}

	vega::targeting::TargetingQuery rectangleQuery = nearestQuery;
	rectangleSide->SetCollisionLayer(vega::GameCollision::Enemy);
	world.TickInternal(0.f);
	rectangleQuery.shape = vega::targeting::TargetingShape::Rectangle;
	rectangleQuery.direction = { 1.f, 0.f };
	rectangleQuery.range = 120.f;
	rectangleQuery.rectangleHalfExtents = { 110.f, 50.f };
	const vega::List<vega::targeting::TargetingCandidate> rectangleTargets =
		vega::targeting::AutoTargeting::FindTargets(world, rectangleQuery);
	if (rectangleTargets.size() != 1 ||
		rectangleTargets.front().actor.get() != nearest.get())
	{
		return Fail("AutoTargeting rectangle shape did not reject outside width or length");
	}
	// Restore the fixture so the remaining target-lock assertions keep their
	// original nearest-target setup.
	rectangleSide->SetCollisionLayer(vega::CollisionLayer::None);
	world.TickInternal(0.f);
	nearestQuery.excludedTargets.push_back(outsideCone.get());

	vega::targeting::TargetingQuery customSelectionQuery = allTargetsQuery;
	customSelectionQuery.maxTargets = 1;
	customSelectionQuery.selector = [](
		vega::List<vega::targeting::TargetingCandidate>& candidates,
		std::size_t
	)
	{
		std::stable_sort(candidates.begin(), candidates.end(), [](
			const vega::targeting::TargetingCandidate& left,
			const vega::targeting::TargetingCandidate& right
		)
		{
			return left.distanceSquared > right.distanceSquared;
		});
	};
	const vega::shared_ptr<vega::Actor> customTarget =
		vega::targeting::AutoTargeting::FindTarget(world, customSelectionQuery).lock();
	if (customTarget.get() != farthest.get())
	{
		return Fail("AutoTargeting custom selector was not applied");
	}

	vega::targeting::TargetLock lock;
	if (lock.Acquire(world, nearestQuery).lock().get() != nearest.get())
	{
		return Fail("AutoTargeting target lock did not acquire its initial target");
	}
	nearest->SetActorLocation({ 400.f, 0.f });
	if (lock.Update(world, nearestQuery, vega::targeting::TargetLockMode::Sticky).lock())
	{
		return Fail("Sticky target lock did not clear an invalid target");
	}
	const vega::shared_ptr<vega::Actor> reacquired = lock.Update(
		world,
		nearestQuery,
		vega::targeting::TargetLockMode::ReacquireIfInvalid
	).lock();
	if (reacquired.get() != farthest.get())
	{
		return Fail("Reacquire target lock did not select a replacement target");
	}

	return 0;
}

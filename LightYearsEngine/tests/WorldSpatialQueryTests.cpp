#include "framework/Actor.h"
#include "framework/PhysicsSystem.h"
#include "framework/World.h"
#include <iostream>
#include <unordered_set>

namespace
{
	class BoundsProbeActor final : public ly::Actor
	{
	public:
		BoundsProbeActor(
			ly::World* world,
			sf::Vector2f location,
			std::size_t boxCount,
			float collisionRadius = 0.f
		)
			: Actor(world), mBoxCount{ boxCount }, mCollisionRadius{ collisionRadius }
		{
			SetActorLocation(location);
			if (boxCount > 0)
			{
				SetEnablePhysics(true);
			}
		}

		std::size_t GetPhysicsCollisionBoxCount() const override
		{
			return mBoxCount;
		}

		ly::PhysicsCollisionBox GetPhysicsCollisionBox(std::size_t index) const override
		{
			return index < mBoxCount
				? ly::PhysicsCollisionBox{
					{ 8.f, 8.f },
					{ index == 0 ? -16.f : 16.f, 0.f },
					0.f
				}
				: ly::PhysicsCollisionBox{};
		}

		float GetPhysicsCollisionRadius() const override
		{
			return mCollisionRadius;
		}

	private:
		std::size_t mBoxCount = 0;
		float mCollisionRadius = 0.f;
	};

	bool Fail(const char* message)
	{
		std::cerr << message << '\n';
		return false;
	}

	bool CountStoppedQuery(void* context, ly::Actor*)
	{
		++*static_cast<int*>(context);
		return false;
	}
}

int main()
{
	ly::PhysicsSystem& physics = ly::PhysicsSystem::Get();
	physics.InitializeWorld();
	{
		ly::World world{ nullptr };
		const auto multiShape = world.SpawnActor<BoundsProbeActor>(
			sf::Vector2f{ 30.f, 30.f }, 2
		);
		const auto secondActor = world.SpawnActor<BoundsProbeActor>(
			sf::Vector2f{ 75.f, 75.f }, 1
		);
		const auto outsideActor = world.SpawnActor<BoundsProbeActor>(
			sf::Vector2f{ 500.f, 500.f }, 1
		);
		const auto bodylessActor = world.SpawnActor<BoundsProbeActor>(
			sf::Vector2f{ 45.f, 80.f }, 0, 10.f
		);
		world.TickInternal(0.f);

		const sf::FloatRect queryBounds{ { 0.f, 0.f }, { 120.f, 120.f } };
		const ly::List<ly::Actor*> physicsActors = physics.QueryActorsInBounds(queryBounds);
		std::unordered_set<ly::Actor*> physicsActorSet{
			physicsActors.begin(), physicsActors.end()
		};
		if (physicsActors.size() != 2 || physicsActorSet.size() != 2 ||
			physicsActorSet.count(multiShape.lock().get()) != 1 ||
			physicsActorSet.count(secondActor.lock().get()) != 1 ||
			physicsActorSet.count(outsideActor.lock().get()) != 0)
		{
			return Fail("Physics bounds query did not return each matching actor once");
		}

		const ly::List<ly::Actor*> outsideQuery = physics.QueryActorsInBounds(
			{ { 0.f, 0.f }, { 20.f, 20.f } }
		);
		if (!outsideQuery.empty())
		{
			return Fail("Physics bounds query returned actors outside its bounds");
		}

		int stoppedVisitCount = 0;
		physics.VisitActorsInBounds(
			queryBounds,
			&stoppedVisitCount,
			CountStoppedQuery
		);
		if (stoppedVisitCount != 1)
		{
			return Fail("Physics bounds query ignored visitor early exit");
		}

		const ly::List<ly::weak_ptr<ly::Actor>> worldActors =
			world.GetActorsInBounds(queryBounds);
		std::unordered_set<ly::Actor*> worldActorSet;
		for (const ly::weak_ptr<ly::Actor>& weakActor : worldActors)
		{
			if (const ly::shared_ptr<ly::Actor> actor = weakActor.lock())
			{
				worldActorSet.insert(actor.get());
			}
		}
		if (worldActors.size() != 3 || worldActorSet.size() != 3 ||
			worldActorSet.count(multiShape.lock().get()) != 1 ||
			worldActorSet.count(secondActor.lock().get()) != 1 ||
			worldActorSet.count(bodylessActor.lock().get()) != 1 ||
			worldActorSet.count(outsideActor.lock().get()) != 0)
		{
			return Fail("World bounds query did not combine physics and bodyless actors once");
		}

		// Body removal is deferred until the next physics step. During that window
		// the same actor is present in the physics query and the manual bodyless index.
		multiShape.lock()->SetEnablePhysics(false);
		const ly::List<ly::weak_ptr<ly::Actor>> overlappingSources =
			world.GetActorsInBounds(queryBounds);
		std::size_t multiShapeCount = 0;
		for (const ly::weak_ptr<ly::Actor>& weakActor : overlappingSources)
		{
			if (weakActor.lock().get() == multiShape.lock().get())
			{
				++multiShapeCount;
			}
		}
		if (multiShapeCount != 1)
		{
			return Fail("World query duplicated an actor shared by physics and manual indexes");
		}

		const ly::shared_ptr<BoundsProbeActor> destroyedActor = outsideActor.lock();
		destroyedActor->Destroy();
		const ly::List<ly::weak_ptr<ly::Actor>> afterDestroy =
			world.GetActorsInBounds({ { 450.f, 450.f }, { 100.f, 100.f } });
		if (!afterDestroy.empty())
		{
			return Fail("World bounds query returned a pending-destroy actor");
		}

		physics.Step(0.f);
	}
	physics.Cleanup();
	return 0;
}

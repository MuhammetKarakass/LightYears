#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/CollisionComponent.h"
#include "framework/PhysicsScene.h"
#include "framework/World.h"
#include <cmath>
#include <iostream>
#include <unordered_set>

namespace
{
	class BoundsProbeActor final : public vega::Actor
	{
	public:
		BoundsProbeActor(
			vega::World* world,
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

		vega::PhysicsCollisionBox GetPhysicsCollisionBox(std::size_t index) const override
		{
			return index < mBoxCount
				? vega::PhysicsCollisionBox{
					{ 8.f, 8.f },
					{ index == 0 ? -16.f : 16.f, 0.f },
					0.f
				}
				: vega::PhysicsCollisionBox{};
		}

		float GetPhysicsCollisionRadius() const override
		{
			return mCollisionRadius;
		}

	private:
		std::size_t mBoxCount = 0;
		float mCollisionRadius = 0.f;
	};

	// Overlapping physics actors must never be displaced by the Box2D solver and
	// must still produce exactly one begin-overlap callback.
	class OverlapProbeActor final : public vega::Actor
	{
	public:
		OverlapProbeActor(vega::World* world, sf::Vector2f location, CollisionLayer layer, CollisionLayer mask)
			: Actor(world)
		{
			SetActorLocation(location);
			SetCollisionLayer(layer);
			SetCollisionMask(mask);
			SetEnablePhysics(true);
		}

		float GetPhysicsCollisionRadius() const override
		{
			return 20.f;
		}

		void OnActorBeginOverlap(vega::Actor* otherActor) override
		{
			Actor::OnActorBeginOverlap(otherActor);
			++beginCount;
		}

		void OnActorEndOverlap(vega::Actor* otherActor) override
		{
			Actor::OnActorEndOverlap(otherActor);
			++endCount;
		}

		int beginCount = 0;
		int endCount = 0;
	};

	struct BodyPositionQuery
	{
		vega::Actor* actor = nullptr;
		b2Vec2 position{ 0.f, 0.f };
		bool found = false;
	};

	bool CollectBodyPosition(b2ShapeId shapeId, void* context)
	{
		auto* query = static_cast<BodyPositionQuery*>(context);
		const b2BodyId bodyId = b2Shape_GetBody(shapeId);
		if (static_cast<vega::Actor*>(b2Body_GetUserData(bodyId)) == query->actor)
		{
			query->position = b2Body_GetPosition(bodyId);
			query->found = true;
		}
		return true;
	}

	bool BodyMatchesActor(vega::PhysicsScene& physics, vega::Actor* actor)
	{
		BodyPositionQuery query{ actor };
		const b2AABB everything{ { -1000.f, -1000.f }, { 1000.f, 1000.f } };
		b2QueryFilter filter = b2DefaultQueryFilter();
		filter.categoryBits = ~uint64_t{ 0 };
		b2World_OverlapAABB(physics.GetWorldId(), everything, filter, CollectBodyPosition, &query);
		const float rate = physics.GetPhysicsRate();
		const sf::Vector2f location = actor->GetActorLocation();
		return query.found && std::abs(query.position.x - location.x * rate) < 1e-4f &&
			std::abs(query.position.y - location.y * rate) < 1e-4f;
	}

	struct BodyFilterQuery
	{
		vega::Actor* actor = nullptr;
		b2Filter filter = b2DefaultFilter();
		b2BodyType bodyType = b2_dynamicBody;
		bool found = false;
	};

	bool CollectBodyFilter(b2ShapeId shapeId, void* context)
	{
		auto* query = static_cast<BodyFilterQuery*>(context);
		const b2BodyId bodyId = b2Shape_GetBody(shapeId);
		if (static_cast<vega::Actor*>(b2Body_GetUserData(bodyId)) == query->actor)
		{
			query->filter = b2Shape_GetFilter(shapeId);
			query->bodyType = b2Body_GetType(bodyId);
			query->found = true;
		}
		return true;
	}

	BodyFilterQuery GetBodyFilter(vega::PhysicsScene& physics, vega::Actor* actor)
	{
		BodyFilterQuery query{ actor };
		const b2AABB everything{ { -1000.f, -1000.f }, { 1000.f, 1000.f } };
		b2QueryFilter filter = b2DefaultQueryFilter();
		filter.categoryBits = ~uint64_t{ 0 };
		b2World_OverlapAABB(physics.GetWorldId(), everything, filter, CollectBodyFilter, &query);
		return query;
	}

	bool Fail(const char* message)
	{
		std::cerr << message << '\n';
		return false;
	}

	// Unlike Fail, yields a non-zero process exit code from main.
	int FailExit(const char* message)
	{
		Fail(message);
		return 1;
	}

	bool CountStoppedQuery(void* context, vega::Actor*)
	{
		++*static_cast<int*>(context);
		return false;
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	{
		// CollisionComponent: created lazily by the first collision setter, defaults
		// without it, and layer/mask changes refresh the live Box2D filter while the
		// actor stays in the World spatial index exactly once.
		vega::World world{ nullptr };
		vega::PhysicsScene& physics = world.GetPhysicsScene();
		const auto untouched = world.SpawnActor<vega::Actor>().lock();
		if (untouched->FindComponent<vega::CollisionComponent>() != nullptr ||
			untouched->GetCollisionLayer() != CollisionLayer::None || untouched->GetCollisionMask() != CollisionLayer::None ||
			untouched->IsPhysicsEnabled() || untouched->HasPhysicsBody() ||
			untouched->GetPhysicsBodyType() != vega::PhysicsBodyType::Dynamic || untouched->CanCollideWith(untouched.get()))
		{
			return FailExit("An actor that never touched collision did not report the collision defaults");
		}
		untouched->SetCollisionLayer(CollisionLayer::Layer2);
		vega::CollisionComponent* untouchedCollision = untouched->FindComponent<vega::CollisionComponent>();
		if (!untouchedCollision || untouched->GetCollisionLayer() != CollisionLayer::Layer2 ||
			untouchedCollision->GetCollisionLayer() != CollisionLayer::Layer2)
		{
			return FailExit("SetCollisionLayer did not create the CollisionComponent");
		}

		const auto probe = world.SpawnActor<OverlapProbeActor>(
			sf::Vector2f{ 100.f, 100.f }, CollisionLayer::Layer0, CollisionLayer::Layer2
		).lock();
		world.TickInternal(0.f);
		vega::CollisionComponent* collision = probe->FindComponent<vega::CollisionComponent>();
		BodyFilterQuery filter = GetBodyFilter(physics, probe.get());
		if (!collision || !collision->HasPhysicsBody() || !probe->HasPhysicsBody() || !probe->IsPhysicsEnabled() ||
			!filter.found || filter.filter.categoryBits != static_cast<uint64_t>(CollisionLayer::Layer0) ||
			filter.filter.maskBits != static_cast<uint64_t>(CollisionLayer::Layer2))
		{
			return FailExit("Physics body did not start with the actor collision layer and mask");
		}

		probe->SetCollisionLayer(CollisionLayer::Layer1);
		probe->SetCollisionMask(CollisionLayer::Layer2 | CollisionLayer::Layer5);
		filter = GetBodyFilter(physics, probe.get());
		if (!filter.found || filter.filter.categoryBits != static_cast<uint64_t>(CollisionLayer::Layer1) ||
			filter.filter.maskBits != static_cast<uint64_t>(CollisionLayer::Layer2 | CollisionLayer::Layer5) ||
			probe->GetCollisionLayer() != CollisionLayer::Layer1 ||
			probe->GetCollisionMask() != (CollisionLayer::Layer2 | CollisionLayer::Layer5))
		{
			return FailExit("Changing collision layer/mask did not refresh the Box2D filter");
		}
		if (!probe->CanCollideWith(untouched.get()) || untouched->CanCollideWith(probe.get()))
		{
			return FailExit("CanCollideWith did not follow the component mask and layer");
		}

		int visits = 0;
		for (const vega::weak_ptr<vega::Actor>& weakActor : world.GetActorsInBounds({ { 60.f, 60.f }, { 80.f, 80.f } }))
		{
			if (weakActor.lock().get() == probe.get()) ++visits;
		}
		if (visits != 1)
		{
			return FailExit("Collision layer change duplicated or dropped the actor in the spatial query");
		}

		if (GetBodyFilter(physics, probe.get()).bodyType != b2_dynamicBody)
		{
			return FailExit("A default physics body was not dynamic");
		}
		probe->SetPhysicsBodyType(vega::PhysicsBodyType::Static);
		if (probe->GetPhysicsBodyType() != vega::PhysicsBodyType::Static || collision->GetPhysicsBodyType() != vega::PhysicsBodyType::Static ||
			GetBodyFilter(physics, probe.get()).bodyType != b2_staticBody)
		{
			return FailExit("SetPhysicsBodyType did not reach the component");
		}

		probe->SetEnablePhysics(false);
		if (probe->HasPhysicsBody() || probe->IsPhysicsEnabled() || collision->HasPhysicsBody())
		{
			return FailExit("SetEnablePhysics(false) left a physics body behind");
		}
		probe->SetEnablePhysics(true);
		if (!probe->HasPhysicsBody() || !probe->RemoveComponent(*collision) || probe->HasPhysicsBody() || GetBodyFilter(physics, probe.get()).found)
			return FailExit("Removing a collision component must release its live Box2D body");
		physics.Step(0.f);
	}

	{
		vega::World world{ nullptr };
		vega::PhysicsScene& physics = world.GetPhysicsScene();
		const auto first = world.SpawnActor<OverlapProbeActor>(
			sf::Vector2f{ 100.f, 100.f }, CollisionLayer::Layer0, CollisionLayer::Layer2
		).lock();
		const auto second = world.SpawnActor<OverlapProbeActor>(
			sf::Vector2f{ 110.f, 100.f }, CollisionLayer::Layer2, CollisionLayer::Layer0
		).lock();
		world.TickInternal(0.f);
		for (int frame = 0; frame < 10; ++frame)
		{
			physics.Step(1.f / 60.f);
		}

		if (!BodyMatchesActor(physics, first.get()) || !BodyMatchesActor(physics, second.get()))
		{
			return FailExit("Overlapping physics bodies were moved away from their actors by the solver");
		}
		if (first->beginCount != 1 || second->beginCount != 1 || first->endCount != 0 || second->endCount != 0)
		{
			return FailExit("Overlapping stationary actors did not produce exactly one begin overlap");
		}

		// Moving one actor away still ends the overlap exactly once.
		second->SetActorLocation({ 400.f, 100.f });
		for (int frame = 0; frame < 5; ++frame)
		{
			physics.Step(1.f / 60.f);
		}
		if (first->beginCount != 1 || first->endCount != 1 || second->endCount != 1 ||
			!BodyMatchesActor(physics, first.get()) || !BodyMatchesActor(physics, second.get()))
		{
			return FailExit("Separating overlapping actors did not produce exactly one end overlap");
		}
	}

	{
		vega::World world{ nullptr };
		vega::PhysicsScene& physics = world.GetPhysicsScene();
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
		const vega::List<vega::Actor*> physicsActors = physics.QueryActorsInBounds(queryBounds);
		std::unordered_set<vega::Actor*> physicsActorSet{
			physicsActors.begin(), physicsActors.end()
		};
		if (physicsActors.size() != 2 || physicsActorSet.size() != 2 ||
			physicsActorSet.count(multiShape.lock().get()) != 1 ||
			physicsActorSet.count(secondActor.lock().get()) != 1 ||
			physicsActorSet.count(outsideActor.lock().get()) != 0)
		{
			return Fail("Physics bounds query did not return each matching actor once");
		}

		const vega::List<vega::Actor*> outsideQuery = physics.QueryActorsInBounds(
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

		const vega::List<vega::weak_ptr<vega::Actor>> worldActors =
			world.GetActorsInBounds(queryBounds);
		std::unordered_set<vega::Actor*> worldActorSet;
		for (const vega::weak_ptr<vega::Actor>& weakActor : worldActors)
		{
			if (const vega::shared_ptr<vega::Actor> actor = weakActor.lock())
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
		const vega::List<vega::weak_ptr<vega::Actor>> overlappingSources =
			world.GetActorsInBounds(queryBounds);
		std::size_t multiShapeCount = 0;
		for (const vega::weak_ptr<vega::Actor>& weakActor : overlappingSources)
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

		const vega::shared_ptr<BoundsProbeActor> destroyedActor = outsideActor.lock();
		destroyedActor->Destroy();
		const vega::List<vega::weak_ptr<vega::Actor>> afterDestroy =
			world.GetActorsInBounds({ { 450.f, 450.f }, { 100.f, 100.f } });
		if (!afterDestroy.empty())
		{
			return Fail("World bounds query returned a pending-destroy actor");
		}

		physics.Step(0.f);
	}

	return 0;
}

#include "framework/World.h"
#include "gameplay/portal/PortalTransferParticipant.h"
#include "gameplay/portal/PortalTransferRuntimeActor.h"
#include "gameplay/portal/PortalTransferService.h"

#include <cmath>
#include <iostream>
#include <optional>

namespace
{
	using namespace ly;
	constexpr float Tolerance = 0.001f;

	bool Expect(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << "FAILED: " << message << '\n';
		}
		return condition;
	}

	bool NearlyEqual(float actual, float expected)
	{
		return std::abs(actual - expected) <= Tolerance;
	}

	class TestPortalAnchor final : public Actor
	{
	public:
		explicit TestPortalAnchor(World* world)
			: Actor(world)
		{
		}
	};

	class TestPortalParticipant final : public Actor, public PortalTransferParticipant
	{
	public:
		explicit TestPortalParticipant(World* world)
			: Actor(world)
		{
		}

		Actor& GetPortalTransferActor() override { return *this; }
		bool CanEnterPortalTransfer() const override { return true; }
		float GetPortalTransferRadius() const override { return 5.f; }
		bool IsInPortalTransit() const override { return mInTransit; }
		void BeginPortalTransit() override
		{
			mInTransit = true;
			++mBeginCount;
		}
		void CompletePortalTransit(const sf::Vector2f& exitLocation) override
		{
			SetActorLocation(exitLocation);
			mInTransit = false;
			++mCompleteCount;
		}
		float GetPhysicsCollisionRadius() const override { return 5.f; }

		bool mInTransit = false;
		int mBeginCount = 0;
		int mCompleteCount = 0;
	};

	template<typename ActorType>
	shared_ptr<ActorType> Spawn(World& world)
	{
		return world.SpawnActor<ActorType>().lock();
	}

	int CountActiveRuntimeActors(World& world)
	{
		int count = 0;
		for (const weak_ptr<PortalTransferRuntimeActor>& actorWeak :
			world.GetActorsByTypeIncludingPending<PortalTransferRuntimeActor>())
		{
			const shared_ptr<PortalTransferRuntimeActor> actor = actorWeak.lock();
			if (actor && !actor->GetIsPendingDestroy())
			{
				++count;
			}
		}
		return count;
	}

	bool TestRuntimeActorDeduplicationAndWorldIsolation()
	{
		World firstWorld{ nullptr };
		World secondWorld{ nullptr };
		const shared_ptr<TestPortalAnchor> firstA = Spawn<TestPortalAnchor>(firstWorld);
		const shared_ptr<TestPortalAnchor> firstB = Spawn<TestPortalAnchor>(firstWorld);
		const shared_ptr<TestPortalAnchor> secondA = Spawn<TestPortalAnchor>(secondWorld);
		const shared_ptr<TestPortalAnchor> secondB = Spawn<TestPortalAnchor>(secondWorld);
		if (!Expect(firstA && firstB && secondA && secondB, "Portal test anchors failed to spawn"))
		{
			return false;
		}

		const auto firstPair = PortalTransferService::CreatePair(
			firstWorld, *firstA, *firstB, 20.f, 0.5f, 0.75f
		);
		const auto secondPair = PortalTransferService::CreatePair(
			firstWorld, *firstA, *firstB, 20.f, 0.5f, 0.75f
		);
		const auto isolatedPair = PortalTransferService::CreatePair(
			secondWorld, *secondA, *secondB, 20.f, 0.5f, 0.75f
		);
		PortalTransferService::EnsureRuntimeActor(firstWorld);
		PortalTransferService::EnsureRuntimeActor(firstWorld);
		PortalTransferService::EnsureRuntimeActor(secondWorld);

		if (!Expect(firstPair == 1 && secondPair == 2 && isolatedPair == 1,
			"Portal pair IDs were not local to their owning World"))
		{
			return false;
		}
		if (!Expect(CountActiveRuntimeActors(firstWorld) == 1 &&
			CountActiveRuntimeActors(secondWorld) == 1,
			"Repeated same-frame requests created duplicate runtime actors"))
		{
			return false;
		}

		PortalTransferService::ResetWorld(firstWorld);
		const auto resetPair = PortalTransferService::CreatePair(
			firstWorld, *firstA, *firstB, 20.f, 0.5f, 0.75f
		);
		return Expect(resetPair == 1 && CountActiveRuntimeActors(firstWorld) == 1,
			"ResetWorld did not reset pair state while preserving its runtime actor");
	}

	bool TestTransferAndReentryTiming()
	{
		World world{ nullptr };
		const shared_ptr<TestPortalAnchor> firstPortal = Spawn<TestPortalAnchor>(world);
		const shared_ptr<TestPortalAnchor> secondPortal = Spawn<TestPortalAnchor>(world);
		const shared_ptr<TestPortalParticipant> participant = Spawn<TestPortalParticipant>(world);
		if (!Expect(firstPortal && secondPortal && participant, "Portal transfer actors failed to spawn"))
		{
			return false;
		}

		firstPortal->SetActorLocation({ 0.f, 0.f });
		secondPortal->SetActorLocation({ 500.f, 0.f });
		participant->SetActorLocation({ 0.f, 0.f });
		world.TickInternal(0.f);
		const auto pairId = PortalTransferService::CreatePair(
			world, *firstPortal, *secondPortal, 20.f, 0.2f, 0.5f
		);
		const auto tick = [&world](float deltaTime)
		{
			world.TickInternal(deltaTime);
		};
		if (!Expect(pairId != 0, "Portal pair creation failed"))
		{
			return false;
		}

		tick(0.f);
		if (!Expect(participant->mBeginCount == 1 && participant->mInTransit,
			"Participant did not begin transit on entering the first portal"))
		{
			return false;
		}
		tick(0.19f);
		if (!Expect(participant->mCompleteCount == 0 && participant->mInTransit,
			"Transit completed before its configured 0.2 second duration"))
		{
			return false;
		}
		tick(0.02f);
		if (!Expect(participant->mCompleteCount == 1 && !participant->mInTransit &&
			NearlyEqual(participant->GetActorLocation().x, 500.f),
			"Transit did not complete at the paired portal"))
		{
			return false;
		}

		tick(0.01f);
		if (!Expect(participant->mBeginCount == 1,
			"Participant re-entered immediately while still inside the exit portal"))
		{
			return false;
		}
		participant->SetActorLocation({ 700.f, 0.f });
		tick(0.1f);
		participant->SetActorLocation({ 500.f, 0.f });
		tick(0.49f);
		if (!Expect(participant->mBeginCount == 1,
			"Participant re-entered before the 0.5 second exit cooldown elapsed"))
		{
			return false;
		}
		tick(0.02f);
		return Expect(participant->mBeginCount == 2 && participant->mInTransit,
			"Participant did not re-enter after leaving and completing the cooldown");
	}

	bool TestWorldAddressReuseWithRetainedRuntimeActor()
	{
		std::optional<World> worldStorage;
		worldStorage.emplace(nullptr);
		World* const originalAddress = &*worldStorage;
		const shared_ptr<TestPortalAnchor> firstA = Spawn<TestPortalAnchor>(*originalAddress);
		const shared_ptr<TestPortalAnchor> firstB = Spawn<TestPortalAnchor>(*originalAddress);
		if (!Expect(firstA && firstB, "First World portal anchors failed to spawn"))
		{
			return false;
		}
		const auto originalPair = PortalTransferService::CreatePair(
			*originalAddress, *firstA, *firstB, 20.f, 0.5f, 0.75f
		);
		shared_ptr<PortalTransferRuntimeActor> retainedRuntimeActor;
		for (const weak_ptr<PortalTransferRuntimeActor>& actorWeak :
			originalAddress->GetActorsByTypeIncludingPending<PortalTransferRuntimeActor>())
		{
			retainedRuntimeActor = actorWeak.lock();
			if (retainedRuntimeActor)
			{
				break;
			}
		}
		if (!Expect(originalPair == 1 && retainedRuntimeActor,
			"The first World did not retain its own portal runtime actor"))
		{
			return false;
		}

		worldStorage.reset();
		worldStorage.emplace(nullptr);
		World& reusedWorld = *worldStorage;
		if (!Expect(&reusedWorld == originalAddress, "The test World address was not reused"))
		{
			return false;
		}
		const shared_ptr<TestPortalAnchor> secondA = Spawn<TestPortalAnchor>(reusedWorld);
		const shared_ptr<TestPortalAnchor> secondB = Spawn<TestPortalAnchor>(reusedWorld);
		if (!Expect(secondA && secondB, "Reused World portal anchors failed to spawn"))
		{
			return false;
		}
		const auto reusedPair = PortalTransferService::CreatePair(
			reusedWorld, *secondA, *secondB, 20.f, 0.5f, 0.75f
		);
		const int runtimeActorCount = CountActiveRuntimeActors(reusedWorld);
		worldStorage.reset();
		retainedRuntimeActor.reset();
		return Expect(reusedPair == 1 && runtimeActorCount == 1,
			"A retained runtime actor leaked old World state through address reuse");
	}
}

int main()
{
	const bool passed = TestRuntimeActorDeduplicationAndWorldIsolation() &&
		TestTransferAndReentryTiming() &&
		TestWorldAddressReuseWithRetainedRuntimeActor();
	if (passed)
	{
		std::cout << "All portal runtime tests passed.\n";
		return 0;
	}
	return 1;
}

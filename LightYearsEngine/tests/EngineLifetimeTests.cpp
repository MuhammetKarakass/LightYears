#include "framework/Delegate.h"
#include "framework/Actor.h"
#include "framework/Object.h"
#include "framework/PhysicsSystem.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "framework/debug/Assert.h"
#include "framework/debug/Log.h"
#include "framework/debug/Profiler.h"

#include <iostream>
#include <memory>
#include <vector>

namespace
{
	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}

	class WeakListener : public ly::Object
	{
	public:
		void OnValue(int value)
		{
			total += value;
		}

		int total{ 0 };
	};

	struct RawListener
	{
		void OnValue(int value)
		{
			total += value;
		}

		int total{ 0 };
	};

	struct ReentrantListener
	{
		void BindLateListener()
		{
			++calls;
			if (!boundLateListener)
			{
				boundLateListener = true;
				delegate->BindAction(lateListener, &ReentrantListener::OnLateBroadcast);
			}
		}

		void OnLateBroadcast()
		{
			++calls;
		}

		ly::Delegate<>* delegate{ nullptr };
		ReentrantListener* lateListener{ nullptr };
		bool boundLateListener{ false };
		int calls{ 0 };
	};

	struct UnbindingListener
	{
		void UnbindTarget()
		{
			++calls;
			delegate->UnbindAction(*targetHandle);
		}

		void OnTarget()
		{
			++calls;
		}

		ly::Delegate<>* delegate{ nullptr };
		ly::DelegateHandle* targetHandle{ nullptr };
		int calls{ 0 };
	};

	class TimerOwner : public ly::Object
	{
	public:
		void QueueManyTimers()
		{
			++firstCallbackCount;
			for (int index = 0; index < 128; ++index)
			{
				manager->SetTimer(GetWeakPtr(), &TimerOwner::OnNestedTimer, 0.f);
			}
		}

		void ClearAndReplaceTimers()
		{
			++clearCallbackCount;
			manager->ClearAllTimers();
			manager->SetTimer(GetWeakPtr(), &TimerOwner::OnReplacementTimer, 0.f);
		}

		void OnNestedTimer()
		{
			++nestedCallbackCount;
		}

		void OnReplacementTimer()
		{
			++replacementCallbackCount;
		}

		ly::TimerManager* manager{ nullptr };
		int firstCallbackCount{ 0 };
		int nestedCallbackCount{ 0 };
		int clearCallbackCount{ 0 };
		int replacementCallbackCount{ 0 };
	};

	class PhysicsProbeActor final : public ly::Actor
	{
	public:
		explicit PhysicsProbeActor(ly::World* world)
			: Actor(world)
		{
		}

		float GetPhysicsCollisionRadius() const override
		{
			return 1.f;
		}

		void OnActorBeginOverlap(ly::Actor* otherActor) override
		{
			Actor::OnActorBeginOverlap(otherActor);
			if (CanCollideWith(otherActor) && otherActor->CanCollideWith(this))
			{
				++overlapCount;
			}
		}

		int overlapCount{ 0 };
	};
}

int main()
{
	ly::debug::Log::Initialize();
	ly::debug::Log::SetMinimumLevel(ly::debug::LogLevel::Trace);
	std::vector<ly::debug::LogRecord> logRecords;
	ly::debug::Log::SetSink(
		[&logRecords](const ly::debug::LogRecord& record)
		{
			logRecords.push_back(record);
		}
	);
	LY_CORE_WARN("Diagnostics smoke test: %d", 7);
	if (logRecords.size() != 1 ||
		logRecords.front().level != ly::debug::LogLevel::Warning ||
		logRecords.front().channel != ly::debug::LogChannel::Core ||
		logRecords.front().message != "Diagnostics smoke test: 7" ||
		logRecords.front().location.line <= 0)
	{
		return Fail("Structured logging did not preserve level, channel, message, and source");
	}
	ly::debug::Log::SetMinimumLevel(ly::debug::LogLevel::Error);
	LY_CORE_WARN("This warning must be filtered");
	if (logRecords.size() != 1)
	{
		return Fail("Logging minimum-level filter did not suppress a warning");
	}
	ly::debug::Log::SetMinimumLevel(ly::debug::LogLevel::Trace);

	bool verificationEvaluated = false;
	LY_VERIFY(
		(verificationEvaluated = true),
		"LY_VERIFY must evaluate its expression in every build"
	);
	if (!verificationEvaluated)
	{
		return Fail("LY_VERIFY did not evaluate its expression");
	}

	ly::debug::Profiler::Initialize();
	{
		LY_PROFILE_SCOPE("Diagnostics.ProfileSmoke");
		volatile int profilerWork = 0;
		for (int index = 0; index < 64; ++index)
		{
			profilerWork += index;
		}
		(void)profilerWork;
	}
	LY_PROFILE_COUNTER("Diagnostics.Counter", 3);
	const std::vector<ly::debug::ProfileAggregate> profileSnapshot =
		ly::debug::Profiler::BuildSnapshot();
	const std::unordered_map<std::string, double> counterSnapshot =
		ly::debug::Profiler::BuildCounterSnapshot();
	if (ly::debug::Profiler::IsCompiledIn())
	{
		if (profileSnapshot.size() != 1 ||
			profileSnapshot.front().name != "Diagnostics.ProfileSmoke" ||
			profileSnapshot.front().callCount != 1 ||
			profileSnapshot.front().maximumMicroseconds < 0.0 ||
			counterSnapshot.find("Diagnostics.Counter") == counterSnapshot.end() ||
			counterSnapshot.at("Diagnostics.Counter") != 3.0)
		{
			return Fail("Debug profiler did not record its scope aggregate and counter");
		}
	}
	else if (!profileSnapshot.empty() || !counterSnapshot.empty())
	{
		return Fail("Release build recorded a compile-disabled profile scope");
	}

	ly::PhysicsSystem& physicsSystem = ly::PhysicsSystem::Get();
	physicsSystem.InitializeWorld();
	{
		ly::World physicsWorld{ nullptr };
		PhysicsProbeActor probe{ &physicsWorld };
		PhysicsProbeActor otherProbe{ &physicsWorld };
		probe.SetCollisionLayer(CollisionLayer::Player);
		probe.SetCollisionMask(CollisionLayer::Enemy);
		otherProbe.SetCollisionLayer(CollisionLayer::PlayerBullet);
		otherProbe.SetCollisionMask(CollisionLayer::Enemy);
		probe.SetEnablePhysics(true);
		otherProbe.SetEnablePhysics(true);
		if (!probe.HasPhysicsBody())
		{
			return Fail("Physics probe did not create its explicit-radius body");
		}
		bool probeFoundBySpatialQuery = false;
		for (ly::Actor* actor : physicsSystem.QueryActorsInBounds(
			{ { -2.f, -2.f }, { 4.f, 4.f } }
		))
		{
			probeFoundBySpatialQuery = probeFoundBySpatialQuery || actor == &probe;
		}
		if (!probeFoundBySpatialQuery)
		{
			return Fail("Spatial query incorrectly applied an actor collision mask");
		}
		physicsSystem.Step(1.f / 60.f);
		if (probe.overlapCount != 0 || otherProbe.overlapCount != 0)
		{
			return Fail("Physics emitted an overlap outside the gameplay collision masks");
		}

		otherProbe.SetCollisionLayer(CollisionLayer::Enemy);
		otherProbe.SetCollisionMask(CollisionLayer::Player);
		physicsSystem.Step(1.f / 60.f);
		if (probe.overlapCount != 1 || otherProbe.overlapCount != 1)
		{
			return Fail("Runtime collision filter update did not create the expected overlap");
		}
	}
	physicsSystem.Cleanup();

	ly::Delegate<int> weakDelegate;
	std::shared_ptr<WeakListener> weakListener = std::make_shared<WeakListener>();
	weakDelegate.BindAction(weakListener->GetWeakPtr(), &WeakListener::OnValue);
	weakDelegate.Broadcast(3);
	if (weakListener->total != 3)
	{
		return Fail("Weak delegate did not invoke its live listener");
	}
	weakListener.reset();
	weakDelegate.Broadcast(3);

	ly::Delegate<int> rawDelegate;
	std::unique_ptr<RawListener> rawListener = std::make_unique<RawListener>();
	const ly::DelegateHandle rawHandle =
		rawDelegate.BindAction(rawListener.get(), &RawListener::OnValue);
	rawDelegate.Broadcast(4);
	if (rawListener->total != 4 || !rawDelegate.UnbindAction(rawHandle))
	{
		return Fail("Raw delegate connection could not be removed");
	}
	rawListener.reset();
	rawDelegate.Broadcast(4);

	ly::Delegate<> reentrantDelegate;
	ReentrantListener firstListener;
	ReentrantListener lateListener;
	firstListener.delegate = &reentrantDelegate;
	firstListener.lateListener = &lateListener;
	reentrantDelegate.BindAction(&firstListener, &ReentrantListener::BindLateListener);
	reentrantDelegate.Broadcast();
	if (firstListener.calls != 1 || lateListener.calls != 0)
	{
		return Fail("Delegate invoked a listener added during the same broadcast");
	}
	reentrantDelegate.Broadcast();
	if (firstListener.calls != 2 || lateListener.calls != 1)
	{
		return Fail("Delegate did not publish its deferred listener");
	}

	ly::Delegate<> unbindingDelegate;
	UnbindingListener unbinder;
	UnbindingListener target;
	unbinder.delegate = &unbindingDelegate;
	ly::DelegateHandle targetHandle;
	unbinder.targetHandle = &targetHandle;
	unbindingDelegate.BindAction(&unbinder, &UnbindingListener::UnbindTarget);
	targetHandle = unbindingDelegate.BindAction(&target, &UnbindingListener::OnTarget);
	unbindingDelegate.Broadcast();
	if (unbinder.calls != 1 || target.calls != 0)
	{
		return Fail("Delegate invoked a listener removed during broadcast");
	}

	ly::TimerManager& timerManager = ly::TimerManager::GetTimerManager();
	timerManager.ClearAllTimers();
	std::shared_ptr<TimerOwner> timerOwner = std::make_shared<TimerOwner>();
	timerOwner->manager = &timerManager;

	timerManager.SetTimer(
		timerOwner->GetWeakPtr(),
		&TimerOwner::QueueManyTimers,
		0.f
	);
	timerManager.UpdateTimer(0.f);
	if (timerOwner->firstCallbackCount != 1 || timerOwner->nestedCallbackCount != 0)
	{
		return Fail("TimerManager executed timers queued during the same update");
	}
	timerManager.UpdateTimer(0.f);
	if (timerOwner->nestedCallbackCount != 128)
	{
		return Fail("TimerManager lost timers queued from a callback");
	}

	timerManager.SetTimer(
		timerOwner->GetWeakPtr(),
		&TimerOwner::ClearAndReplaceTimers,
		0.f
	);
	timerManager.SetTimer(
		timerOwner->GetWeakPtr(),
		&TimerOwner::OnNestedTimer,
		10.f
	);
	timerManager.UpdateTimer(0.f);
	if (timerOwner->clearCallbackCount != 1 || timerOwner->replacementCallbackCount != 0)
	{
		return Fail("TimerManager mishandled ClearAllTimers during an update");
	}
	timerManager.UpdateTimer(0.f);
	if (timerOwner->replacementCallbackCount != 1)
	{
		return Fail("TimerManager lost a replacement timer queued after ClearAllTimers");
	}

	timerManager.ClearAllTimers();
	timerOwner.reset();
	ly::TimerManager::ShutdownTimerManagers();
	ly::debug::Profiler::Shutdown();
	ly::debug::Log::SetSink({});
	ly::debug::Log::Shutdown();
	return 0;
}

#include "framework/Delegate.h"
#include "framework/Actor.h"
#include "framework/ActorComponent.h"
#include "framework/AssetManager.h"
#include "framework/LightComponent.h"
#include "framework/Object.h"
#include "framework/PhysicsScene.h"
#include "framework/SpriteComponent.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "framework/debug/Assert.h"
#include "framework/debug/Log.h"
#include "framework/debug/Profiler.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
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

		void OnRepeatingTimer()
		{
			++repeatingCallbackCount;
		}

		void CancelRepeatingTimer()
		{
			++repeatingCallbackCount;
			manager->ClearTimer(repeatingTimerHandle);
		}

		void ReleaseOwnerDuringRepeatingTimer()
		{
			++*externalCallbackCount;
			ownerToRelease->reset();
		}

		ly::TimerManager* manager{ nullptr };
		ly::TimerHandle repeatingTimerHandle{};
		std::shared_ptr<TimerOwner>* ownerToRelease{ nullptr };
		int* externalCallbackCount{ nullptr };
		int firstCallbackCount{ 0 };
		int nestedCallbackCount{ 0 };
		int clearCallbackCount{ 0 };
		int replacementCallbackCount{ 0 };
		int repeatingCallbackCount{ 0 };
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

	using EventLog = std::vector<std::string>;

	class RecordingComponent : public ly::ActorComponent
	{
	public:
		RecordingComponent(ly::Actor& owner, EventLog* eventLog, std::string componentName, bool ticks)
			: ActorComponent(owner), log{ eventLog }, name{ std::move(componentName) }
		{
			mCanEverTick = ticks;
		}

		void BeginPlay() override { log->push_back(name + ".BeginPlay"); }
		void TickComponent(float) override
		{
			log->push_back(name + ".Tick");
			if (spawnOnTick)
			{
				ly::Actor& owner = GetOwner();
				spawnOnTick = false;
				owner.AddComponent<RecordingComponent>(log, "Spawned", false);
			}
		}
		void EndPlay() override { log->push_back(name + ".EndPlay"); }

		EventLog* log;
		std::string name;
		bool spawnOnTick{ false };
	};

	class ComponentProbeActor final : public ly::Actor
	{
	public:
		ComponentProbeActor(ly::World* world, EventLog* eventLog)
			: Actor(world), log{ eventLog }
		{
		}

		void BeginPlay() override { log->push_back("Actor.BeginPlay"); }
		void Tick(float deltaTime) override
		{
			log->push_back("Actor.Tick");
			Actor::Tick(deltaTime);
		}

		EventLog* log;
	};

	bool Near(float a, float b)
	{
		return std::fabs(a - b) < 0.01f;
	}
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

	{
		ly::World physicsWorld{ nullptr };
		ly::PhysicsScene& physicsScene = physicsWorld.GetPhysicsScene();
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
		for (ly::Actor* actor : physicsScene.QueryActorsInBounds(
			{ { -2.f, -2.f }, { 4.f, 4.f } }
		))
		{
			probeFoundBySpatialQuery = probeFoundBySpatialQuery || actor == &probe;
		}
		if (!probeFoundBySpatialQuery)
		{
			return Fail("Spatial query incorrectly applied an actor collision mask");
		}
		physicsScene.Step(1.f / 60.f);
		if (probe.overlapCount != 0 || otherProbe.overlapCount != 0)
		{
			return Fail("Physics emitted an overlap outside the gameplay collision masks");
		}

		otherProbe.SetCollisionLayer(CollisionLayer::Enemy);
		otherProbe.SetCollisionMask(CollisionLayer::Player);
		physicsScene.Step(1.f / 60.f);
		if (probe.overlapCount != 1 || otherProbe.overlapCount != 1)
		{
			return Fail("Runtime collision filter update did not create the expected overlap");
		}
	}

	{
		ly::World componentWorld{ nullptr };
		EventLog log;
		ComponentProbeActor actor{ &componentWorld, &log };
		RecordingComponent& first = actor.AddComponent<RecordingComponent>(&log, "A", true);
		actor.AddComponent<RecordingComponent>(&log, "B", false);
		if (actor.FindComponent<RecordingComponent>() != &first || &first.GetOwner() != &actor ||
			first.GetWorld() != &componentWorld || actor.FindComponent<ly::LightComponent>() != nullptr)
		{
			return Fail("Actor component add/find did not return the expected component");
		}
		if (&actor.GetOrAddComponent<RecordingComponent>(&log, "X", false) != &first)
		{
			return Fail("GetOrAddComponent created a duplicate component");
		}
		actor.TickInternal(0.1f);
		if (!log.empty())
		{
			return Fail("Actor ticked before BeginPlay");
		}
		actor.BeginPlayInternal();
		actor.BeginPlayInternal();
		if (log != EventLog{ "A.BeginPlay", "B.BeginPlay", "Actor.BeginPlay" })
		{
			return Fail("Component BeginPlay did not run once, before Actor::BeginPlay");
		}
		log.clear();
		first.spawnOnTick = true;
		actor.TickInternal(0.1f);
		if (log != EventLog{ "Actor.Tick", "A.Tick", "Spawned.BeginPlay" })
		{
			return Fail("Component tick/add-during-tick order was wrong");
		}
		log.clear();
		first.SetActive(false);
		actor.TickInternal(0.1f);
		if (log != EventLog{ "Actor.Tick" })
		{
			return Fail("Inactive or non-ticking component ticked");
		}
		log.clear();
		actor.Destroy();
		actor.Destroy();
		if (log != EventLog{ "A.EndPlay", "B.EndPlay", "Spawned.EndPlay" })
		{
			return Fail("Component EndPlay did not run once, in add order, on Destroy");
		}

		EventLog neverBegunLog;
		ComponentProbeActor neverBegun{ &componentWorld, &neverBegunLog };
		neverBegun.AddComponent<RecordingComponent>(&neverBegunLog, "N", true);
		neverBegun.Destroy();
		if (!neverBegunLog.empty())
		{
			return Fail("EndPlay ran for a component that never began play");
		}
	}

	{
		ly::World lightWorld{ nullptr };
		ly::Actor actor{ &lightWorld };
		actor.SetActorLocation({ 100.f, 50.f });
		if (actor.GetRenderBounds())
		{
			return Fail("Actor without sprite or lights reported render bounds");
		}
		ly::LightComponent& lights = actor.GetOrAddComponent<ly::LightComponent>();
		if (&actor.GetOrAddComponent<ly::LightComponent>() != &lights || actor.FindComponent<ly::LightComponent>() != &lights)
		{
			return Fail("LightComponent was not unique per actor");
		}
		const ly::GameplayTag base{ "Test.Light" };
		const ly::GameplayTag light0 = lights.AddLight(base, "", sf::Color::White, 1.f, { 20.f, 20.f }, { 0.f, 0.f }, false, false, 0.f, 1.f);
		const ly::GameplayTag light1 = lights.AddLight(base, "", sf::Color::White, 1.f, { 20.f, 20.f }, { 200.f, 0.f }, false, false, 0.f, 1.f, 1.f, ly::LightSpace::World);
		if (light0.ToString() != "Test.Light_0" || light1.ToString() != "Test.Light_1" ||
			!lights.HasLight(light0) || lights.GetLightsByBaseTag(base).size() != 2u)
		{
			return Fail("LightComponent did not auto-index lights as Tag_0, Tag_1");
		}
		lights.RemoveLight(light0);
		if (lights.HasLight(light0) || lights.GetLightsByBaseTag(base).size() != 1u ||
			lights.AddLight(base, "", sf::Color::White, 1.f, { 20.f, 20.f }, { 0.f, 0.f }, false, false, 0.f, 1.f).ToString() != "Test.Light_2")
		{
			return Fail("LightComponent removal or next-index selection was wrong");
		}
		lights.SetAllLightsIntensity(base, 2.5f);
		if (lights.GetLightData(light1)->intensity != 2.5f)
		{
			return Fail("SetAllLightsIntensity did not update matching lights");
		}

		// Bounds only count enabled lights that have a shader; give them a
		// shader object without needing a GL context or asset files.
		for (const ly::GameplayTag& tag : lights.GetLightsByBaseTag(base))
		{
			lights.GetLightData(tag)->shader = std::make_shared<sf::Shader>();
		}
		lights.SetLightEnabled(ly::GameplayTag{ "Test.Light_2" }, false);
		const std::optional<sf::FloatRect> bounds = actor.GetRenderBounds();
		const float extent = 0.5f * std::sqrt(20.f * 20.f + 20.f * 20.f);
		// Remaining light: Test.Light_1, world space at (200, 0).
		if (!bounds || !Near(bounds->position.x, 200.f - extent) || !Near(bounds->position.y, -extent) ||
			!Near(bounds->size.x, extent * 2.f))
		{
			return Fail("Render bounds did not cover the single enabled light");
		}
		lights.SetLightEnabled(ly::GameplayTag{ "Test.Light_2" }, true);
		const std::optional<sf::FloatRect> unionBounds = actor.GetRenderBounds();
		// Test.Light_2 is local at the actor (100, 50): union spans x 100-extent .. 200+extent.
		if (!unionBounds || !Near(unionBounds->position.x, 100.f - extent) || !Near(unionBounds->position.y, -extent) ||
			!Near(unionBounds->size.x, 100.f + extent * 2.f) || !Near(unionBounds->size.y, 50.f + extent * 2.f))
		{
			return Fail("Render bounds did not union every enabled light");
		}
		lights.SetActive(false);
		if (actor.GetRenderBounds())
		{
			return Fail("Inactive LightComponent still contributed render bounds");
		}
	}

	{
		// SpriteComponent: the sprite follows the actor transform and the actor
		// bounds come from it. The texture is a PNG written to a temp directory so
		// no asset files are needed; if the host cannot create a GL texture the
		// sprite-dependent checks are skipped (the component then has no sprite).
		const std::filesystem::path textureDirectory = std::filesystem::temp_directory_path() / "ly_sprite_component_test";
		std::filesystem::create_directories(textureDirectory);
		const bool pngWritten = sf::Image{ sf::Vector2u{ 40u, 20u }, sf::Color::White }.saveToFile(textureDirectory / "sprite40x20.png");
		ly::AssetManager& assets = ly::AssetManager::GetAssetManager();
		const std::string previousRoot = assets.GetAssetRootDirectory();
		assets.SetAssetRootDirectory(textureDirectory.generic_string() + "/");

		ly::World spriteWorld{ nullptr };
		ly::Actor plain{ &spriteWorld };
		plain.SetActorLocation({ 12.f, 34.f });
		const sf::FloatRect plainBounds = plain.GetActorGlobalBounds();
		if (plain.FindComponent<ly::SpriteComponent>() != nullptr || !Near(plainBounds.position.x, 12.f) ||
			!Near(plainBounds.position.y, 34.f) || plainBounds.size.x != 0.f || plainBounds.size.y != 0.f)
		{
			return Fail("Actor without a texture path got a SpriteComponent or non-point bounds");
		}

		ly::Actor textured{ &spriteWorld, "sprite40x20.png" };
		ly::SpriteComponent* spriteComponent = textured.FindComponent<ly::SpriteComponent>();
		if (!spriteComponent)
		{
			return Fail("Actor(World*, texturePath) did not create a SpriteComponent");
		}
		if (pngWritten && spriteComponent->HasSprite())
		{
			sf::Sprite& sprite = *spriteComponent->GetSprite();
			sf::FloatRect bounds = textured.GetActorGlobalBounds();
			// Centered pivot at the actor origin: the 40x20 texture spans (-20,-10)..(20,10).
			if (!Near(bounds.position.x, -20.f) || !Near(bounds.position.y, -10.f) ||
				!Near(bounds.size.x, 40.f) || !Near(bounds.size.y, 20.f))
			{
				return Fail("Textured actor bounds were not the centered texture rect");
			}
			textured.SetActorLocation({ 100.f, 50.f });
			if (!Near(sprite.getPosition().x, 100.f) || !Near(sprite.getPosition().y, 50.f))
			{
				return Fail("Sprite did not follow SetActorLocation");
			}
			bounds = textured.GetActorGlobalBounds();
			if (!Near(bounds.position.x, 80.f) || !Near(bounds.position.y, 40.f))
			{
				return Fail("Actor bounds did not follow SetActorLocation");
			}
			textured.SetActorRotation(90.f);
			if (!Near(sprite.getRotation().asDegrees(), 90.f))
			{
				return Fail("Sprite did not follow SetActorRotation");
			}
			bounds = textured.GetActorGlobalBounds();
			// Rotated a quarter turn the footprint is 20 wide and 40 tall around (100, 50).
			if (!Near(bounds.size.x, 20.f) || !Near(bounds.size.y, 40.f) ||
				!Near(bounds.position.x, 90.f) || !Near(bounds.position.y, 30.f))
			{
				return Fail("Actor bounds did not follow SetActorRotation");
			}
			textured.AddActorLocationOffset({ 10.f, 0.f });
			if (!Near(sprite.getPosition().x, 110.f))
			{
				return Fail("Sprite did not follow AddActorLocationOffset");
			}
			const std::optional<sf::FloatRect> renderBounds = textured.GetRenderBounds();
			if (!renderBounds || !Near(renderBounds->size.x, 20.f) || !Near(renderBounds->size.y, 40.f))
			{
				return Fail("Render bounds did not use the sprite bounds");
			}
			spriteComponent->SetVisibility(false);
			if (sprite.getColor() != sf::Color::Transparent)
			{
				return Fail("SpriteComponent::SetVisibility(false) did not clear the tint");
			}
			spriteComponent->SetVisibility(true);
			if (sprite.getColor() != sf::Color::White)
			{
				return Fail("SpriteComponent::SetVisibility(true) did not restore the tint");
			}
			// A late-added component picks up the actor's current transform.
			ly::Actor late{ &spriteWorld };
			late.SetActorLocation({ 300.f, 200.f });
			late.SetActorRotation(45.f);
			ly::SpriteComponent& lateSprite = late.GetOrAddComponent<ly::SpriteComponent>();
			if (!lateSprite.SetTexture("sprite40x20.png") || !Near(lateSprite.GetSprite()->getPosition().x, 300.f) ||
				!Near(lateSprite.GetSprite()->getRotation().asDegrees(), 45.f) || &late.GetOrAddComponent<ly::SpriteComponent>() != &lateSprite)
			{
				return Fail("A late-added SpriteComponent did not adopt the actor transform");
			}
		}
		else
		{
			std::cout << "SKIP: no GL texture available, sprite transform checks skipped" << std::endl;
		}

		assets.SetAssetRootDirectory(previousRoot);
		std::error_code removeError;
		std::filesystem::remove_all(textureDirectory, removeError);
	}

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
	timerOwner->repeatingCallbackCount = 0;
	timerOwner->repeatingTimerHandle = timerManager.SetTimer(timerOwner->GetWeakPtr(), &TimerOwner::OnRepeatingTimer, 0.1f, true);
	timerManager.UpdateTimer(0.35f);
	if (timerOwner->repeatingCallbackCount != 3)
	{
		return Fail("TimerManager did not catch up repeating timer intervals");
	}
	timerManager.UpdateTimer(0.049f);
	if (timerOwner->repeatingCallbackCount != 3)
	{
		return Fail("TimerManager discarded the repeating timer remainder");
	}
	timerManager.UpdateTimer(0.002f);
	if (timerOwner->repeatingCallbackCount != 4)
	{
		return Fail("TimerManager did not preserve repeating timer cadence");
	}

	timerManager.ClearAllTimers();
	timerOwner->repeatingCallbackCount = 0;
	timerOwner->repeatingTimerHandle = timerManager.SetTimer(timerOwner->GetWeakPtr(), &TimerOwner::OnRepeatingTimer, 0.1f, true);
	timerManager.UpdateTimer(1.25f);
	if (timerOwner->repeatingCallbackCount != 8)
	{
		return Fail("TimerManager exceeded the repeating timer catch-up cap");
	}
	timerManager.UpdateTimer(0.f);
	if (timerOwner->repeatingCallbackCount != 12)
	{
		return Fail("TimerManager discarded repeating timer debt after the catch-up cap");
	}

	timerManager.ClearAllTimers();
	timerOwner->repeatingCallbackCount = 0;
	timerOwner->repeatingTimerHandle = timerManager.SetTimer(timerOwner->GetWeakPtr(), &TimerOwner::OnRepeatingTimer, 0.f, true);
	timerManager.UpdateTimer(1.f);
	timerManager.UpdateTimer(1.f);
	if (timerOwner->repeatingCallbackCount != 0)
	{
		return Fail("TimerManager invoked a zero-duration repeating timer");
	}

	timerManager.ClearAllTimers();
	timerOwner->repeatingCallbackCount = 0;
	timerOwner->repeatingTimerHandle = timerManager.SetTimer(timerOwner->GetWeakPtr(), &TimerOwner::CancelRepeatingTimer, 0.1f, true);
	timerManager.UpdateTimer(0.35f);
	if (timerOwner->repeatingCallbackCount != 1)
	{
		return Fail("TimerManager invoked a repeating timer after its callback canceled it");
	}

	timerManager.ClearAllTimers();
	int lifetimeCallbackCount = 0;
	std::shared_ptr<TimerOwner> lifetimeOwner = std::make_shared<TimerOwner>();
	lifetimeOwner->manager = &timerManager;
	lifetimeOwner->ownerToRelease = &lifetimeOwner;
	lifetimeOwner->externalCallbackCount = &lifetimeCallbackCount;
	timerManager.SetTimer(lifetimeOwner->GetWeakPtr(), &TimerOwner::ReleaseOwnerDuringRepeatingTimer, 0.1f, true);
	timerManager.UpdateTimer(0.35f);
	if (lifetimeOwner || lifetimeCallbackCount != 1)
	{
		return Fail("TimerManager invoked a repeating timer after its owner expired");
	}

	timerManager.ClearAllTimers();
	timerOwner.reset();
	ly::TimerManager::ShutdownTimerManagers();
	ly::debug::Profiler::Shutdown();
	ly::debug::Log::SetSink({});
	ly::debug::Log::Shutdown();
	return 0;
}

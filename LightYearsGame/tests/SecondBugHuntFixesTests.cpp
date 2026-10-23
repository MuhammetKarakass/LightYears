#include "gameFramework/LightYearsAssets.h"
#include "VFX/ExplosionLights.h"
#include "framework/EngineContext.h"
#include "framework/World.h"
#include "gameplay/ability/frozenThrong/FrozenThrongHuskActor.h"
#include "player/Reward.h"

#include <cmath>
#include <iostream>
#include <optional>
#include <vector>

namespace
{
	using namespace vega;
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

	bool TestRewardSpeedPassThrough()
	{
		World world{ EngineContext::Current() };
		const shared_ptr<Reward> defaultReward = CreateReward(&world, "", RewardHealth).lock();
		const shared_ptr<Reward> fastReward = CreateReward(&world, "", RewardHealth, 75.f).lock();
		if (!Expect(defaultReward && fastReward, "Reward factories failed to create actors"))
		{
			return false;
		}

		defaultReward->SetActorLocation({ 0.f, 0.f });
		fastReward->SetActorLocation({ 500.f, 0.f });
		// The reward offset is local: rotate both actors so +y is the local
		// forward axis and the asserted world displacement is the speed itself.
		defaultReward->SetActorRotation(180.f);
		fastReward->SetActorRotation(180.f);
		defaultReward->Tick(1.f);
		fastReward->Tick(1.f);
		return Expect(NearlyEqual(defaultReward->GetActorLocation().y, 200.f),
				"The default reward must drift down at +200 world units per second") &&
			Expect(NearlyEqual(fastReward->GetActorLocation().y, 75.f),
				"An explicitly requested reward speed must reach the actor");
	}

	bool TestHuskPortalTransitSkipsTimeline()
	{
		World world{ EngineContext::Current() };
		const shared_ptr<Actor> owner = world.SpawnActor<Actor>().lock();
		const FrozenThrongPresentationProfile profile;
		const shared_ptr<FrozenThrongHuskActor> husk =
			world.SpawnActor<FrozenThrongHuskActor>(
				owner.get(),
				profile,
				std::optional<sf::Vector2f>{ sf::Vector2f{ 0.f, 40.f } },
				weak_ptr<Actor>{}
			).lock();
		if (!Expect(owner && husk, "Frozen Throng husk failed to spawn"))
		{
			return false;
		}

		husk->SetActorLocation({ 0.f, 0.f });
		// BeginPlay resolves the launch direction toward the recorded target.
		world.TickInternal(0.f);
		husk->BeginPortalTransit();
		world.TickInternal(0.2f);
		const sf::Vector2f location = husk->GetActorLocation();
		return Expect(!husk->GetIsPendingDestroy(),
				"A husk in portal transit must not explode") &&
			Expect(NearlyEqual(location.x, 0.f) && NearlyEqual(location.y, 0.f),
				"A husk in portal transit must not move toward its target");
	}

	bool TestExplosionLightsAreWorldScoped()
	{
		World worldA{ EngineContext::Current() };
		World worldB{ EngineContext::Current() };
		ExplosionLights::Clear();
		ExplosionFlash flash;
		flash.world = &worldA;
		flash.lifetime = 1.f;
		ExplosionLights::Add(flash);
		if (!Expect(ExplosionLights::Active(&worldB).empty(),
			"A flash spawned in one world must not be submitted for another"))
		{
			return false;
		}
		if (!Expect(ExplosionLights::Active(&worldA).size() == 1,
			"The spawning world must still own its flash"))
		{
			return false;
		}

		ExplosionLights::Update(0.5f, &worldB);
		const std::vector<ExplosionFlash> afterForeignUpdate = ExplosionLights::Active(&worldA);
		if (!Expect(afterForeignUpdate.size() == 1 &&
			NearlyEqual(afterForeignUpdate.front().age, 0.f),
			"Rendering another world must not age this world's flash"))
		{
			return false;
		}

		ExplosionLights::Update(1.f, &worldA);
		if (!Expect(ExplosionLights::Active(&worldA).empty(),
			"A flash must disappear after its own world ages it past its lifetime"))
		{
			return false;
		}

		ExplosionLights::Add(flash);
		ExplosionLights::Clear();
		return Expect(ExplosionLights::Active(&worldA).empty(),
			"Clear must drop every world's flashes");
	}
}

int main()
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	const bool passed = TestRewardSpeedPassThrough() &&
		TestHuskPortalTransitSkipsTimeline() &&
		TestExplosionLightsAreWorldScoped();
	if (passed)
	{
		std::cout << "All second bug hunt fix tests passed.\n";
		return 0;
	}
	return 1;
}

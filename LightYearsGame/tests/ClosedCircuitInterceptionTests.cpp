#include "framework/World.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitFieldActor.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitFieldRegistryActor.h"
#include "presentation/ability/closedCircuit/ClosedCircuitPresentationProfile.h"

#include <cmath>
#include <iostream>

namespace
{
	using namespace ly;
	constexpr float Tolerance = 0.001f;

	bool Expect(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << "FAILED: " << message << '\n';
			return false;
		}
		return true;
	}

	bool NearlyEqual(float actual, float expected)
	{
		return std::abs(actual - expected) <= Tolerance;
	}

	void ConfigureField(
		ClosedCircuitFieldActor& field,
		float barrierRadius,
		float barrierHealth,
		bool activate = true
	)
	{
		field.SetActorLocation({ 0.f, 0.f });
		field.ConfigureDelivery({ 0.f, 0.f }, 280.f, 0.5f, barrierRadius, barrierHealth);
		field.Tick(0.f);
		if (activate)
		{
			field.Tick(0.5f);
		}
	}

	bool CheckSegmentCase(
		const char* name,
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		float barrierRadius,
		float projectileRadius,
		bool expectedHit,
		const sf::Vector2f& expectedImpact = {}
	)
	{
		World world{ nullptr };
		ClosedCircuitPresentationProfile profile{};
		ClosedCircuitFieldActor field{ &world, nullptr, profile };
		ConfigureField(field, barrierRadius, 1000.f);
		AbilityWorldActor projectile{ &world, nullptr };
		projectile.SetCollisionLayer(CollisionLayer::EnemyBullet);
		projectile.SetAbilityCollisionRadius(projectileRadius);
		projectile.SetDamage(10.f);
		projectile.SetActorLocation(end);

		const bool intercepted = field.TryInterceptProjectile(projectile, start);
		if (!Expect(intercepted == expectedHit, name))
		{
			return false;
		}
		if (expectedHit)
		{
			return Expect(NearlyEqual(projectile.GetActorLocation().x, expectedImpact.x) &&
				NearlyEqual(projectile.GetActorLocation().y, expectedImpact.y),
				"Projectile was not moved to the first segment-circle entry point");
		}
		return Expect(NearlyEqual(projectile.GetActorLocation().x, end.x) &&
			NearlyEqual(projectile.GetActorLocation().y, end.y),
			"A rejected segment unexpectedly moved the projectile");
	}

	bool TestSweptCircleGeometry()
	{
		return CheckSegmentCase(
			"Outside-to-inside projectile was not intercepted",
			{ -150.f, 0.f }, { 0.f, 0.f }, 100.f, 5.f, true, { -105.f, 0.f }
		) &&
		CheckSegmentCase(
			"Fast outside-to-outside transit was not intercepted at first entry",
			{ -150.f, 0.f }, { 150.f, 0.f }, 100.f, 5.f, true, { -105.f, 0.f }
		) &&
		CheckSegmentCase(
			"Outside-to-outside miss was incorrectly intercepted",
			{ -150.f, 106.f }, { 150.f, 106.f }, 100.f, 5.f, false
		) &&
		CheckSegmentCase(
			"Tangent segment was not intercepted",
			{ -150.f, 105.f }, { 150.f, 105.f }, 100.f, 5.f, true, { 0.f, 105.f }
		) &&
		CheckSegmentCase(
			"Projectile starting inside the field was intercepted",
			{ -50.f, 0.f }, { 150.f, 0.f }, 100.f, 5.f, false
		) &&
		CheckSegmentCase(
			"Zero-length projectile segment was intercepted",
			{ -150.f, 0.f }, { -150.f, 0.f }, 100.f, 5.f, false
		) &&
		CheckSegmentCase(
			"Projectile collision radius was not added to the barrier radius",
			{ -150.f, 104.f }, { 150.f, 104.f }, 100.f, 5.f, true,
			{ -std::sqrt(209.f), 104.f }
		) &&
		CheckSegmentCase(
			"Zero configured radius did not normalize to the minimum one-unit field",
			{ -5.f, 0.f }, { 5.f, 0.f }, 0.f, 0.f, true, { -1.f, 0.f }
		);
	}

	bool TestInterceptionGuardsAndDurability()
	{
		World world{ nullptr };
		World otherWorld{ nullptr };
		ClosedCircuitPresentationProfile profile{};
		AbilityWorldActor projectile{ &world, nullptr };
		projectile.SetCollisionLayer(CollisionLayer::EnemyBullet);
		projectile.SetAbilityCollisionRadius(5.f);
		projectile.SetDamage(10.f);
		projectile.SetActorLocation({ 0.f, 0.f });

		ClosedCircuitFieldActor formingField{ &world, nullptr, profile };
		ConfigureField(formingField, 100.f, 100.f, false);
		if (!Expect(!formingField.TryInterceptProjectile(projectile, { -150.f, 0.f }),
			"Forming field intercepted a projectile before activation"))
		{
			return false;
		}

		ClosedCircuitFieldActor emptyField{ &world, nullptr, profile };
		ConfigureField(emptyField, 100.f, 0.f);
		if (!Expect(!emptyField.TryInterceptProjectile(projectile, { -150.f, 0.f }),
			"Zero-health field intercepted a projectile"))
		{
			return false;
		}

		ClosedCircuitFieldActor destroyedField{ &world, nullptr, profile };
		ConfigureField(destroyedField, 100.f, 100.f);
		destroyedField.Destroy();
		if (!Expect(!destroyedField.TryInterceptProjectile(projectile, { -150.f, 0.f }),
			"Destroyed field intercepted a projectile"))
		{
			return false;
		}

		ClosedCircuitFieldActor wrongWorldField{ &world, nullptr, profile };
		ConfigureField(wrongWorldField, 100.f, 100.f);
		AbilityWorldActor foreignProjectile{ &otherWorld, nullptr };
		foreignProjectile.SetCollisionLayer(CollisionLayer::EnemyBullet);
		foreignProjectile.SetAbilityCollisionRadius(5.f);
		foreignProjectile.SetActorLocation({ 0.f, 0.f });
		if (!Expect(!wrongWorldField.TryInterceptProjectile(foreignProjectile, { -150.f, 0.f }),
			"Field intercepted a projectile from a different world"))
		{
			return false;
		}

		ClosedCircuitFieldActor wrongLayerField{ &world, nullptr, profile };
		ConfigureField(wrongLayerField, 100.f, 100.f);
		projectile.SetCollisionLayer(CollisionLayer::PlayerBullet);
		if (!Expect(!wrongLayerField.TryInterceptProjectile(projectile, { -150.f, 0.f }),
			"Field intercepted a non-enemy projectile"))
		{
			return false;
		}

		ClosedCircuitFieldActor durableField{ &world, nullptr, profile };
		ConfigureField(durableField, 100.f, 5.f);
		projectile.SetCollisionLayer(CollisionLayer::EnemyBullet);
		if (!Expect(durableField.TryInterceptProjectile(projectile, { -150.f, 0.f }) &&
				durableField.GetIsPendingDestroy(),
			"A lethal intercepted projectile did not break the barrier"))
		{
			return false;
		}
		return Expect(!durableField.TryInterceptProjectile(projectile, { -150.f, 0.f }),
			"Broken barrier intercepted another projectile");
	}

	class TestEnemyProjectile final : public AbilityWorldActor
	{
	public:
		TestEnemyProjectile(World* world, Actor* owner = nullptr)
			: AbilityWorldActor(world, owner)
		{
			SetCollisionLayer(CollisionLayer::EnemyBullet);
			SetAbilityCollisionRadius(5.f);
			SetDamage(10.f);
		}

		bool IsProjectileActor() const override { return true; }
	};

	class TestOwner final : public Actor
	{
	public:
		explicit TestOwner(World* world)
			: Actor(world)
		{
		}
	};

	bool TestWorldOwnedRegistryAndReplacement()
	{
		World world{ nullptr };
		World otherWorld{ nullptr };
		ClosedCircuitPresentationProfile profile{};
		const shared_ptr<ClosedCircuitFieldRegistryActor> registry =
			ClosedCircuitFieldRegistryActor::GetOrCreate(world);
		if (!Expect(static_cast<bool>(registry), "Could not create the world-owned Closed Circuit registry"))
		{
			return false;
		}
		const shared_ptr<ClosedCircuitFieldRegistryActor> repeatedRegistry =
			ClosedCircuitFieldRegistryActor::GetOrCreate(world);
		if (!Expect(repeatedRegistry == registry,
			"A pending world registry was not reused by a second same-frame lookup") ||
			!Expect(world.GetActorsByTypeIncludingPending<ClosedCircuitFieldRegistryActor>().size() == 1u,
				"More than one Closed Circuit registry was created in one world"))
		{
			return false;
		}

		const shared_ptr<TestOwner> owner = world.SpawnActor<TestOwner>().lock();
		const shared_ptr<ClosedCircuitFieldActor> firstField =
			world.SpawnActor<ClosedCircuitFieldActor>(owner.get(), profile).lock();
		if (!Expect(owner && firstField, "Could not create the owner and first field"))
		{
			return false;
		}
		ConfigureField(*firstField, 100.f, 100.f);
		if (!Expect(registry->FindActiveField(*owner) == firstField,
			"The first field was not registered for its owner"))
		{
			return false;
		}

		const shared_ptr<ClosedCircuitFieldActor> replacementField =
			world.SpawnActor<ClosedCircuitFieldActor>(owner.get(), profile).lock();
		if (!Expect(static_cast<bool>(replacementField), "Could not create a replacement field"))
		{
			return false;
		}
		ConfigureField(*replacementField, 120.f, 150.f);
		if (!Expect(firstField->GetIsPendingDestroy(),
			"Re-registering an owner did not destroy its previous field") ||
			!Expect(registry->FindActiveField(*owner) == replacementField,
				"The previous field's RAII cleanup erased the replacement generation"))
		{
			return false;
		}

		const shared_ptr<TestOwner> otherOwner = otherWorld.SpawnActor<TestOwner>().lock();
		const shared_ptr<ClosedCircuitFieldActor> otherField =
			otherWorld.SpawnActor<ClosedCircuitFieldActor>(otherOwner.get(), profile).lock();
		const shared_ptr<ClosedCircuitFieldRegistryActor> otherRegistry =
			ClosedCircuitFieldRegistryActor::GetOrCreate(otherWorld);
		if (!Expect(otherOwner && otherField && otherRegistry,
			"Could not create the isolated second-world registry and actors"))
		{
			return false;
		}
		ConfigureField(*otherField, 100.f, 100.f);
		if (!Expect(otherRegistry != registry,
				"Two worlds shared the same Closed Circuit registry actor") ||
			!Expect(otherRegistry->FindActiveField(*otherOwner) == otherField,
				"The second world could not find its own active field") ||
			!Expect(!registry->FindActiveField(*otherOwner),
				"The first world returned a field for a foreign owner"))
		{
			return false;
		}

		replacementField->Destroy();
		return Expect(!registry->FindActiveField(*owner),
			"Destroy() left a stale owner-to-field registry entry");
	}

	bool TestRegistryWorldTeardownWithRetainedField()
	{
		weak_ptr<ClosedCircuitFieldRegistryActor> registryWeak;
		shared_ptr<ClosedCircuitFieldActor> retainedField;
		{
			World world{ nullptr };
			ClosedCircuitPresentationProfile profile{};
			const shared_ptr<TestOwner> owner = world.SpawnActor<TestOwner>().lock();
			retainedField = world.SpawnActor<ClosedCircuitFieldActor>(owner.get(), profile).lock();
			const shared_ptr<ClosedCircuitFieldRegistryActor> registry =
				ClosedCircuitFieldRegistryActor::GetOrCreate(world);
			if (!Expect(owner && retainedField && registry,
				"Could not create teardown test actors"))
			{
				return false;
			}
			ConfigureField(*retainedField, 100.f, 100.f);
			registryWeak = registry;
		}

		if (!Expect(registryWeak.expired(),
			"The registry outlived its World after the World released its actors"))
		{
			return false;
		}
		retainedField.reset();
		return true;
	}

	bool TestWorldProjectileTickUsesInterceptionService()
	{
		World world{ nullptr };
		ClosedCircuitPresentationProfile profile{};
		const shared_ptr<ClosedCircuitFieldActor> field =
			world.SpawnActor<ClosedCircuitFieldActor>(nullptr, profile).lock();
		if (!Expect(static_cast<bool>(field), "Could not spawn a Closed Circuit field for integration test"))
		{
			return false;
		}
		field->ConfigureDelivery({ 0.f, 0.f }, 280.f, 0.5f, 100.f, 15.f);
		world.TickInternal(0.f);
		world.TickInternal(0.5f);

		const auto interceptThroughTick = [&world](float expectedImpactX)
		{
			const shared_ptr<TestEnemyProjectile> projectile =
				world.SpawnActor<TestEnemyProjectile>(nullptr).lock();
			if (!projectile)
			{
				return false;
			}
			projectile->SetActorLocation({ -150.f, 0.f });
			world.TickInternal(0.f);
			projectile->SetActorLocation({ 150.f, 0.f });
			world.TickInternal(0.016f);
			return projectile->GetIsPendingDestroy() &&
				NearlyEqual(projectile->GetActorLocation().x, expectedImpactX);
		};

		if (!Expect(interceptThroughTick(-105.f),
			"Normal projectile tick did not route a fast transit through the interception service") ||
			!Expect(!field->GetIsPendingDestroy(),
			"One projectile consumed barrier durability more than once") ||
			!Expect(interceptThroughTick(-105.f),
			"Second projectile was not intercepted by the still-active barrier"))
		{
			return false;
		}
		return Expect(field->GetIsPendingDestroy(),
			"Two 10-damage projectiles did not consume the barrier's 15 durability exactly once each");
	}
}

int main()
{
	if (!TestSweptCircleGeometry() ||
		!TestInterceptionGuardsAndDurability() ||
		!TestWorldOwnedRegistryAndReplacement() ||
		!TestRegistryWorldTeardownWithRetainedField() ||
		!TestWorldProjectileTickUsesInterceptionService())
	{
		return 1;
	}

	std::cout << "Closed Circuit interception tests passed.\n";
	return 0;
}

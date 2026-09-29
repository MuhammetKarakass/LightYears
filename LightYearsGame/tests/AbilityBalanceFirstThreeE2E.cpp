#include "attributes/AttributeMath.h"
#include "framework/World.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/glacialPressure/GlacialPressureContracts.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/ability/orbitalDrones/OrbitalDronesContracts.h"
#include "gameplay/ability/orbitalDrones/OrbitingDroneActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "spaceShip/SpaceShip.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace ly
{
	namespace
	{
		constexpr float HealthTolerance = 0.05f;
		constexpr float ProgressionTolerance = 0.001f;
		constexpr float TestShipHealth = 1000.f;

		ShipDefinition MakeShipDefinition(float maximumHealth = TestShipHealth)
		{
			return ShipDefinition{
				"SpaceShooterRedux/PNG/Enemies/enemyRed5.png",
				maximumHealth,
				{},
				0.f,
				0,
				0,
				{},
				{}
			};
		}

		class BalanceE2EShip final : public SpaceShip
		{
		public:
			explicit BalanceE2EShip(World* world, float maximumHealth = TestShipHealth)
				: SpaceShip{ world, MakeShipDefinition(maximumHealth) }
			{
				CenterPivot();
			}

			float GetPhysicsCollisionRadius() const override { return 14.f; }
			float GetHealth() const { return GetHealthComponent().GetHealth(); }
		};

		bool NearlyEqual(float lhs, float rhs, float tolerance = ProgressionTolerance)
		{
			return std::isfinite(lhs) && std::isfinite(rhs) && std::abs(lhs - rhs) <= tolerance;
		}

		const sas::AttributeModifier* FindModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId
		)
		{
			const auto found = std::find_if(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[&attributeId](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == attributeId;
				}
			);
			return found == step.attributeModifiers.end() ? nullptr : &*found;
		}

		const sas::AttributeModifier* FindStep0Modifier(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			const AbilityLevelStep* step = definition.ResolveLevelStep(0);
			return step ? FindModifier(*step, attributeId) : nullptr;
		}

		bool HasUnboundedSteps(const GameAbilityDefinition& definition)
		{
			return definition.ResolveLevelStep(0) && definition.ResolveLevelStep(30);
		}

		// Cooldown reduction is no longer authored in steps; it is appended globally.
		bool CooldownProgressionMatchesFormula(const GameAbilityDefinition& definition)
		{
			const float base = definition.cooldown;
			const float r0 = 0.175f + 0.025f * base;
			const float expectedFirst = std::min(std::max(r0, 0.02f), std::max(0.f, base - 1.f));
			return base > 1.f &&
				NearlyEqual(GetGlobalAbilityCooldownTotalReduction(base, 1), expectedFirst) &&
				GetGlobalAbilityCooldownTotalReduction(base, 24) >=
					GetGlobalAbilityCooldownTotalReduction(base, 23) &&
				base - GetGlobalAbilityCooldownTotalReduction(base, 24) >= 1.f - ProgressionTolerance;
		}

		float ResolveAbilityValue(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			const sas::GameplayAttribute* authored = sas::FindAttribute(
				definition.attributes,
				attributeId
			);
			const sas::GameplayAttribute attribute = authored
				? *authored
				: sas::GameplayAttribute{ attributeId, fallback, 0.f };
			return sas::CalculateModifiedAttributeValue(attribute, definition.attributeModifiers);
		}

		int ActiveCryoStacks(const BalanceE2EShip& ship)
		{
			const sas::ActiveGameplayEffect* effect =
				ship.GetAbilitySystemComponent().FindGameplayEffectById(
					DamageStatusEffectIds::CryoSlowedEffectId
				);
			return effect ? effect->stackCount : 0;
		}

		void SetOwnerAttribute(BalanceE2EShip& ship, const sas::AttributeId& id, float value)
		{
			ship.GetAbilitySystemComponent().GetAttributes().SetBaseValue(id, value);
		}

		sas::AbilityHandle GrantAbility(
			BalanceE2EShip& owner,
			const GameAbilityDefinition& definition,
			std::string& failure
		)
		{
			return owner.GetAbilitySystemComponent().GrantAbility(
				definition,
				definition.slot,
				&failure
			);
		}

		bool WriteArtifact(const std::filesystem::path& path, const nlohmann::json& artifact)
		{
			std::error_code error;
			if (!path.parent_path().empty())
			{
				std::filesystem::create_directories(path.parent_path(), error);
				if (error) return false;
			}

			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) return false;
			output << artifact.dump(2) << '\n';
			return output.good();
		}
	}

	int RunAbilityBalanceFirstThreeE2E(const char* artifactPath, const std::string& setupError)
	{
		nlohmann::json assertions = nlohmann::json::object();
		nlohmann::json outcomes = nlohmann::json::object();
		std::string failure;
		const auto check = [&assertions, &failure](const char* name, bool passed)
		{
			assertions[name] = passed;
			if (!passed && failure.empty()) failure = name;
		};

		check("contentBootstrapSucceeded", setupError.empty());
		const GameAbilityDefinition* hullDefinition = content::AbilityContentCatalog::FindById(
			AbilityData::HullShock::AbilityId::Basic
		);
		const GameAbilityDefinition* glacialDefinition = content::AbilityContentCatalog::FindById(
			AbilityData::GlacialPressure::AbilityId::Basic
		);
		const GameAbilityDefinition* orbitalDefinition = content::AbilityContentCatalog::FindById(
			AbilityData::OrbitalDrones::AbilityId::Basic
		);
		check("allThreeShippedDefinitionsLoaded", hullDefinition && glacialDefinition && orbitalDefinition);

		if (setupError.empty() && hullDefinition && glacialDefinition && orbitalDefinition)
		{
			try
			{
				const sas::AttributeModifier* hullDamageStep = FindStep0Modifier(*hullDefinition, CommonAttributeIds::Damage);
				const sas::AttributeModifier* hullCooldownStep = FindStep0Modifier(*hullDefinition, CommonAttributeIds::Cooldown);
				check("hullShockCatalogProgression", HasUnboundedSteps(*hullDefinition) &&
					!hullDefinition->levelUpgradeScrapCosts.empty() && hullDamageStep &&
					NearlyEqual(hullDamageStep->magnitude, 4.f) && !hullCooldownStep &&
					CooldownProgressionMatchesFormula(*hullDefinition));

				const sas::AttributeModifier* glacialPushDamageStep = FindStep0Modifier(*glacialDefinition,
						AbilityData::GlacialPressure::Attribute::InitialDamage);
				const sas::AttributeModifier* glacialPushEnergyStep = FindStep0Modifier(*glacialDefinition,
						AbilityData::GlacialPressure::Attribute::EnergyPowerInitialScale);
				const sas::AttributeModifier* glacialCollisionDamageStep = FindStep0Modifier(*glacialDefinition,
						AbilityData::GlacialPressure::Attribute::CollisionDamage);
				const sas::AttributeModifier* glacialCollisionEnergyStep = FindStep0Modifier(*glacialDefinition,
						AbilityData::GlacialPressure::Attribute::EnergyPowerCollisionScale);
				const sas::AttributeModifier* glacialCooldownStep = FindStep0Modifier(*glacialDefinition, CommonAttributeIds::Cooldown);
				check("glacialPressureCatalogProgression", HasUnboundedSteps(*glacialDefinition) &&
					!glacialDefinition->levelUpgradeScrapCosts.empty() && glacialPushDamageStep &&
					glacialPushEnergyStep && glacialCollisionDamageStep && glacialCollisionEnergyStep &&
					NearlyEqual(glacialPushDamageStep->magnitude, 5.f) &&
					NearlyEqual(glacialPushEnergyStep->magnitude, 0.02f) &&
					NearlyEqual(glacialCollisionDamageStep->magnitude, 10.f) &&
					NearlyEqual(glacialCollisionEnergyStep->magnitude, 0.02f) &&
					!glacialCooldownStep && CooldownProgressionMatchesFormula(*glacialDefinition));

				const sas::AttributeModifier* orbitalDamageStep = FindStep0Modifier(*orbitalDefinition, CommonAttributeIds::Damage);
				const sas::AttributeModifier* orbitalCooldownStep = FindStep0Modifier(*orbitalDefinition, CommonAttributeIds::Cooldown);
				const float orbitalBaseDamage = sas::FindAttributeValue(
					orbitalDefinition->attributes,
					CommonAttributeIds::Damage,
					0.f
				);
				const float orbitalBaseRadius = sas::FindAttributeValue(
					orbitalDefinition->attributes,
					CommonAttributeIds::Radius,
					0.f
				);
				const float orbitalAngularSpeed = sas::FindAttributeValue(
					orbitalDefinition->attributes,
					AbilityData::OrbitalDrones::Attribute::BaseAngularSpeedRadiansPerSecond,
					0.f
				);
				check("orbitalDronesCatalogProgression", HasUnboundedSteps(*orbitalDefinition) &&
					!orbitalDefinition->levelUpgradeScrapCosts.empty() && orbitalDamageStep &&
					NearlyEqual(orbitalBaseDamage, 25.f) &&
					NearlyEqual(orbitalBaseRadius, 500.f) && NearlyEqual(orbitalAngularSpeed, 2.5f) &&
					NearlyEqual(orbitalDamageStep->magnitude, 4.f) &&
					!orbitalCooldownStep && CooldownProgressionMatchesFormula(*orbitalDefinition));

				// Hull Shock exercises the shipped focus/charge path and its real
				// shared damage resolver against a live opposing SpaceShip.
				World hullWorld{ nullptr };
				const shared_ptr<BalanceE2EShip> hullOwner = hullWorld.SpawnActor<BalanceE2EShip>(100.f).lock();
				const shared_ptr<BalanceE2EShip> hullTarget = hullWorld.SpawnActor<BalanceE2EShip>().lock();
				if (!hullOwner || !hullTarget) throw std::runtime_error("Hull Shock E2E actor spawn failed");
			hullOwner->SetCollisionLayer(CollisionLayer::Player);
			hullOwner->SetCollisionMask(CollisionLayer::Enemy);
			hullTarget->SetCollisionLayer(CollisionLayer::Enemy);
			hullTarget->SetCollisionMask(CollisionLayer::Player);
			hullTarget->SetActorLocation({ 0.f, -100.f });
			hullWorld.TickInternal(0.f);
			SetOwnerAttribute(*hullOwner, OwnerAttributeIds::MaxHealth, 100.f);
			std::string hullGrantError;
			const sas::AbilityHandle hullHandle = GrantAbility(*hullOwner, *hullDefinition, hullGrantError);
			if (!hullHandle.IsValid()) throw std::runtime_error("Hull Shock ability grant failed: " + hullGrantError);
			auto& hullSystem = hullOwner->GetAbilitySystemComponent();
			check("hullShockLevelTwoAppliedAtRuntime", hullSystem.SetAbilityLevel(hullHandle, 2) &&
				NearlyEqual(ResolveAbilityValue(hullSystem.GetAbility(hullHandle)->GetDefinition(),
					CommonAttributeIds::Damage, 0.f), 34.f) &&
				NearlyEqual(ResolveAbilityValue(hullSystem.GetAbility(hullHandle)->GetDefinition(),
					CommonAttributeIds::Cooldown, hullSystem.GetAbility(hullHandle)->GetDefinition().cooldown),
						hullDefinition->cooldown - GetGlobalAbilityCooldownTotalReduction(hullDefinition->cooldown, 1)));
			hullSystem.SetAbilityLevel(hullHandle, 1);
			const float hullHealthBefore = hullTarget->GetHealth();
			hullSystem.SetAbilitySlotInput(hullDefinition->slot, true);
			hullWorld.TickInternal(0.f);
			hullWorld.TickInternal(hullDefinition->duration);
			hullSystem.SetAbilitySlotInput(hullDefinition->slot, false);
			hullWorld.TickInternal(0.f);
			const float hullDamage = hullHealthBefore - hullTarget->GetHealth();
			outcomes["hullShock"] = {
				{ "baseDamage", 30.f }, { "maxHealthScalingCoefficientPreserved", "0.30 (existing content contract)" },
				{ "ownerMaxHealth", 100.f }, { "targetDamage", hullDamage },
				{ "targetHealthBefore", hullHealthBefore }, { "targetHealthAfter", hullTarget->GetHealth() }
			};
			check("hullShockRealRuntimeDamage", NearlyEqual(hullDamage, 60.f, HealthTolerance));

			// Glacial Pressure tests the real cone blast, push window, collision
			// damage, and transfer of the moving ship's CURRENT Cryo stack count.
			World glacialWorld{ nullptr };
			const shared_ptr<BalanceE2EShip> glacialOwner = glacialWorld.SpawnActor<BalanceE2EShip>(100.f).lock();
			const shared_ptr<BalanceE2EShip> movingTarget = glacialWorld.SpawnActor<BalanceE2EShip>().lock();
			const shared_ptr<BalanceE2EShip> collisionTarget = glacialWorld.SpawnActor<BalanceE2EShip>().lock();
			if (!glacialOwner || !movingTarget || !collisionTarget)
				throw std::runtime_error("Glacial Pressure E2E actor spawn failed");
			glacialOwner->SetCollisionLayer(CollisionLayer::Player);
			glacialOwner->SetCollisionMask(CollisionLayer::Enemy);
			movingTarget->SetCollisionLayer(CollisionLayer::Enemy);
			movingTarget->SetCollisionMask(CollisionLayer::Player);
			movingTarget->SetActorLocation({ 0.f, -100.f });
			collisionTarget->SetCollisionLayer(CollisionLayer::Enemy);
			collisionTarget->SetCollisionMask(CollisionLayer::Player);
			collisionTarget->SetActorLocation({ 0.f, -2000.f });
			glacialWorld.TickInternal(0.f);
			SetOwnerAttribute(*glacialOwner, OwnerAttributeIds::MaxHealth, 100.f);
			SetOwnerAttribute(*glacialOwner, OwnerAttributeIds::EnergyPower, 100.f);
			SetOwnerAttribute(*movingTarget, OwnerAttributeIds::MaxHealth, 1000.f);
			SetOwnerAttribute(*collisionTarget, OwnerAttributeIds::MaxHealth, 1000.f);
			std::string glacialGrantError;
			const sas::AbilityHandle glacialHandle = GrantAbility(
				*glacialOwner,
				*glacialDefinition,
				glacialGrantError
			);
			if (!glacialHandle.IsValid())
				throw std::runtime_error("Glacial Pressure ability grant failed: " + glacialGrantError);
			auto& glacialSystem = glacialOwner->GetAbilitySystemComponent();
			check("glacialPressureLevelTwoAppliedAtRuntime", glacialSystem.SetAbilityLevel(glacialHandle, 2) &&
				NearlyEqual(ResolveAbilityValue(glacialSystem.GetAbility(glacialHandle)->GetDefinition(),
					AbilityData::GlacialPressure::Attribute::InitialDamage, 0.f), 35.f) &&
				NearlyEqual(ResolveAbilityValue(glacialSystem.GetAbility(glacialHandle)->GetDefinition(),
					CommonAttributeIds::Cooldown, glacialSystem.GetAbility(glacialHandle)->GetDefinition().cooldown),
					glacialDefinition->cooldown - GetGlobalAbilityCooldownTotalReduction(glacialDefinition->cooldown, 1)));
			glacialSystem.SetAbilityLevel(glacialHandle, 1);
			const float glacialInitialHealth = movingTarget->GetHealth();
			glacialSystem.SetAbilitySlotInput(glacialDefinition->slot, true);
			glacialWorld.TickInternal(0.f);
			glacialWorld.TickInternal(glacialDefinition->duration);
			glacialSystem.SetAbilitySlotInput(glacialDefinition->slot, false);
			const float glacialBlastDamage = glacialInitialHealth - movingTarget->GetHealth();
			const int cryoStacksBeforeCollision = ActiveCryoStacks(*movingTarget);
			const float movingHealthBeforeCollision = movingTarget->GetHealth();
			const float collisionHealthBefore = collisionTarget->GetHealth();
			collisionTarget->SetActorLocation(movingTarget->GetActorLocation());
			glacialWorld.TickInternal(0.f);
			const float movingCollisionDamage = movingHealthBeforeCollision - movingTarget->GetHealth();
			const float targetCollisionDamage = collisionHealthBefore - collisionTarget->GetHealth();
			const int cryoStacksAfterCollision = ActiveCryoStacks(*movingTarget);
			const int transferredCryoStacks = ActiveCryoStacks(*collisionTarget);
			const float expectedCollisionDamage = (60.f + std::sqrt(1000.f * 1000.f + 1000.f * 1000.f) * 0.40f +
				100.f * 0.20f) * 1.40f;
			outcomes["glacialPressure"] = {
				{ "initialDamage", glacialBlastDamage }, { "expectedInitialDamage", 70.f },
				{ "collisionDamageMovingShip", movingCollisionDamage },
				{ "collisionDamageOtherShip", targetCollisionDamage },
				{ "expectedCollisionDamage", expectedCollisionDamage },
				{ "cryoStacksBeforeCollision", cryoStacksBeforeCollision },
				{ "cryoStacksAfterCollision", cryoStacksAfterCollision },
				{ "transferredCryoStacks", transferredCryoStacks }
			};
			check("glacialPressureRealRuntimeBlast", NearlyEqual(glacialBlastDamage, 70.f, HealthTolerance));
			check("glacialPressureCollisionUsesBothMaxHealthAndEnergyPower",
				NearlyEqual(movingCollisionDamage, expectedCollisionDamage, HealthTolerance) &&
				NearlyEqual(targetCollisionDamage, expectedCollisionDamage, HealthTolerance));
			check("glacialPressureTransfersCurrentCryoWithoutRebuildingSource",
				cryoStacksBeforeCollision == 4 && cryoStacksAfterCollision == cryoStacksBeforeCollision &&
				transferredCryoStacks == cryoStacksBeforeCollision);

			// The shipped Orbital Drones behavior spawns live actors. Contact calls
			// then exercise the actor's production hit gate and shared damage path.
			World orbitalWorld{ nullptr };
			const shared_ptr<BalanceE2EShip> orbitalOwner = orbitalWorld.SpawnActor<BalanceE2EShip>(100.f).lock();
			const shared_ptr<BalanceE2EShip> continuousTarget = orbitalWorld.SpawnActor<BalanceE2EShip>().lock();
			const shared_ptr<BalanceE2EShip> reentryTarget = orbitalWorld.SpawnActor<BalanceE2EShip>().lock();
			if (!orbitalOwner || !continuousTarget || !reentryTarget)
				throw std::runtime_error("Orbital Drones E2E actor spawn failed");
			orbitalOwner->SetCollisionLayer(CollisionLayer::Player);
			orbitalOwner->SetCollisionMask(CollisionLayer::Enemy);
			continuousTarget->SetCollisionLayer(CollisionLayer::Enemy);
			continuousTarget->SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
			reentryTarget->SetCollisionLayer(CollisionLayer::Enemy);
			reentryTarget->SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
			orbitalWorld.TickInternal(0.f);
			SetOwnerAttribute(*orbitalOwner, OwnerAttributeIds::AttackPower, 0.f);
			SetOwnerAttribute(*orbitalOwner, OwnerAttributeIds::AttackSpeed, 100.f);
			std::string orbitalGrantError;
			const sas::AbilityHandle orbitalHandle = GrantAbility(*orbitalOwner, *orbitalDefinition, orbitalGrantError);
			if (!orbitalHandle.IsValid())
				throw std::runtime_error("Orbital Drones ability grant failed: " + orbitalGrantError);
			auto& orbitalSystem = orbitalOwner->GetAbilitySystemComponent();
			check("orbitalDronesLevelTwoAppliedAtRuntime", orbitalSystem.SetAbilityLevel(orbitalHandle, 2) &&
				NearlyEqual(ResolveAbilityValue(orbitalSystem.GetAbility(orbitalHandle)->GetDefinition(),
					CommonAttributeIds::Damage, 0.f), 29.f) &&
				NearlyEqual(ResolveAbilityValue(orbitalSystem.GetAbility(orbitalHandle)->GetDefinition(),
					CommonAttributeIds::Cooldown, orbitalSystem.GetAbility(orbitalHandle)->GetDefinition().cooldown),
					orbitalDefinition->cooldown - GetGlobalAbilityCooldownTotalReduction(orbitalDefinition->cooldown, 1)) &&
				NearlyEqual(ResolveAbilityValue(orbitalSystem.GetAbility(orbitalHandle)->GetDefinition(),
					CommonAttributeIds::Radius, 0.f), 500.f));
			orbitalSystem.SetAbilityLevel(orbitalHandle, 1);
			orbitalSystem.SetAbilitySlotInput(orbitalDefinition->slot, true);
			orbitalWorld.TickInternal(0.f);
			orbitalSystem.SetAbilitySlotInput(orbitalDefinition->slot, false);
			orbitalWorld.TickInternal(0.f);
			const List<weak_ptr<OrbitingDroneActor>> drones = orbitalWorld.GetActorsByType<OrbitingDroneActor>();
			if (drones.size() != 4) throw std::runtime_error("Orbital Drones did not spawn four actors");
			const shared_ptr<OrbitingDroneActor> drone = drones.front().lock();
			if (!drone) throw std::runtime_error("Orbital Drones first actor expired unexpectedly");
			const OrbitingDroneActor::OrbitConfiguration shippedOrbit = drone->GetOrbitConfiguration();
			const float droneLifetime = drone->GetLifeTime();
			const float droneHitCooldown = drone->GetSameTargetHitCooldown();
			drone->SetOrbitConfiguration(0.f, 0.f, 0.f);
			drone->SetContactRadius(30.f);
			drone->SetActorLocation(orbitalOwner->GetActorLocation());
			continuousTarget->SetActorLocation(drone->GetActorLocation());
			reentryTarget->SetActorLocation({ 1000.f, 1000.f });
			const float continuousHealthBefore = continuousTarget->GetHealth();
			drone->OnActorBeginOverlap(continuousTarget.get());
			const float continuousHealthAfterHit = continuousTarget->GetHealth();
			drone->Tick(0.60f);
			const float continuousHealthAfterStay = continuousTarget->GetHealth();
			reentryTarget->SetActorLocation(drone->GetActorLocation());
			const float reentryHealthBefore = reentryTarget->GetHealth();
			drone->OnActorBeginOverlap(reentryTarget.get());
			const float reentryHealthAfterFirstHit = reentryTarget->GetHealth();
			reentryTarget->SetActorLocation({ 1000.f, 1000.f });
			drone->Tick(0.20f);
			reentryTarget->SetActorLocation(drone->GetActorLocation());
			drone->OnActorBeginOverlap(reentryTarget.get());
			const float reentryHealthAfterEarlyReentry = reentryTarget->GetHealth();
			drone->Tick(0.40f);
			const float reentryHealthAfterCooldownWhileTouching = reentryTarget->GetHealth();
			reentryTarget->SetActorLocation({ 1000.f, 1000.f });
			drone->Tick(0.f);
			reentryTarget->SetActorLocation(drone->GetActorLocation());
			drone->OnActorBeginOverlap(reentryTarget.get());
			const float reentryHealthAfterAllowedReentry = reentryTarget->GetHealth();
			outcomes["orbitalDrones"] = {
				{ "spawnedDroneCount", drones.size() },
				{ "shippedOrbitRadius", shippedOrbit.radius },
				{ "shippedOrbitAngularSpeedAt100PercentAttackSpeed", shippedOrbit.angularSpeedRadiansPerSecond },
				{ "shippedDroneLifetime", droneLifetime }, { "shippedSameTargetCooldown", droneHitCooldown },
				{ "continuousTargetHealthBefore", continuousHealthBefore },
				{ "continuousTargetHealthAfterFirstHit", continuousHealthAfterHit },
				{ "continuousTargetHealthAfterStayingOverCooldown", continuousHealthAfterStay },
				{ "reentryTargetHealthBefore", reentryHealthBefore },
				{ "reentryTargetHealthAfterFirstHit", reentryHealthAfterFirstHit },
				{ "reentryTargetHealthAfterEarlyReentry", reentryHealthAfterEarlyReentry },
				{ "reentryTargetHealthAfterCooldownWhileTouching", reentryHealthAfterCooldownWhileTouching },
				{ "reentryTargetHealthAfterExitAndAllowedReentry", reentryHealthAfterAllowedReentry }
			};
			check("orbitalDronesRuntimeFormationAndDuration", drones.size() == 4 &&
				NearlyEqual(shippedOrbit.radius, 500.f) &&
				NearlyEqual(shippedOrbit.angularSpeedRadiansPerSecond, 5.f) &&
				NearlyEqual(droneLifetime, 6.f) && NearlyEqual(droneHitCooldown, 0.5f));
			check("orbitalDronesContinuousContactDoesNotRehitAfterCooldown",
				NearlyEqual(continuousHealthBefore - continuousHealthAfterHit, 25.f, HealthTolerance) &&
				NearlyEqual(continuousHealthAfterStay, continuousHealthAfterHit, HealthTolerance));
			check("orbitalDronesEarlyReentryWaitsForExitAfterGate",
				NearlyEqual(reentryHealthBefore - reentryHealthAfterFirstHit, 25.f, HealthTolerance) &&
				NearlyEqual(reentryHealthAfterEarlyReentry, reentryHealthAfterFirstHit, HealthTolerance) &&
				NearlyEqual(reentryHealthAfterCooldownWhileTouching, reentryHealthAfterFirstHit, HealthTolerance) &&
				NearlyEqual(reentryHealthAfterAllowedReentry, reentryHealthAfterFirstHit - 25.f, HealthTolerance));
			outcomes["orbitalAbilityRemainsActiveDuringContactTest"] = orbitalSystem.GetAbility(orbitalHandle) &&
				orbitalSystem.GetAbility(orbitalHandle)->IsActive();
			check("orbitalAbilityRemainsActiveDuringContactTest",
				outcomes["orbitalAbilityRemainsActiveDuringContactTest"].get<bool>());
			outcomes["abilityCatalogLoaded"] = content::AbilityContentCatalog::IsLoaded();
			outcomes["failure"] = failure;
			}
			catch (const std::exception& error)
			{
				outcomes["exception"] = error.what();
				if (failure.empty()) failure = error.what();
			}
		}

		const bool passed = setupError.empty() && assertions.size() >= 10 && failure.empty();
		const nlohmann::json artifact{
			{ "assertions", assertions },
			{ "outcome", outcomes },
			{ "passed", passed },
			{ "scenario", "ability_balance_batch_one.first_three_runtime" },
			{ "schemaVersion", 1 },
			{ "setupError", setupError }
		};
		if (!WriteArtifact(artifactPath, artifact))
		{
			std::cerr << "Could not write first-three ability balance E2E artifact.\n";
			return 1;
		}
		std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
		if (!passed)
		{
			std::cerr << "First-three ability balance E2E failed; see artifact outcomes.\n";
			return 1;
		}
		std::cout << "First-three ability balance E2E passed.\n";
		return 0;
	}
}

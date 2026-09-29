#include "framework/World.h"
#include "framework/Core.h"
#include "framework/Application.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/infernoSpray/InfernoSprayActor.h"
#include "gameplay/ability/infernoSpray/InfernoSprayContracts.h"
#include "gameplay/ability/rocket/RocketContracts.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/sunBeam/SunBeamContracts.h"
#include "gameplay/ability/sunBeam/SunBeamStrikeActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/damage/DamageContext.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationIds.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"
#include "presentation/ability/rocket/RocketPresentationIds.h"
#include "presentation/ability/rocket/RocketPresentationProfile.h"
#include "presentation/ability/sunBeam/SunBeamPresentationIds.h"
#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"
#include "spaceShip/SpaceShip.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace ly
{
	namespace
	{
		using Json = nlohmann::json;
		constexpr float FrameDeltaSeconds = 1.f / 60.f;
		constexpr float TestPower = 100.f;
		constexpr float HealthCapacity = 10000.f;
		constexpr float ValueTolerance = 0.001f;

		std::string& GetWindowTitle()
		{
			static std::string title{ "LightYears Ability Balance Second-Three E2E" };
			return title;
		}

		class AbilityBalanceE2EApplication final : public Application
		{
		public:
			AbilityBalanceE2EApplication()
				: Application({ 320, 240 }, 32, GetWindowTitle(), sf::Style::None)
			{
				GetRenderWindow().setVisible(false);
			}
		};

		bool NearlyEqual(float left, float right)
		{
			return std::abs(left - right) <= ValueTolerance;
		}

		ShipDefinition MakeTestShipDefinition()
		{
			return ShipDefinition{
				"SpaceShooterRedux/PNG/Enemies/enemyRed5.png",
				HealthCapacity,
				{},
				0.f,
				0,
				0,
				{},
				{}
			};
		}

		class AbilityBalanceE2EShip final : public SpaceShip
		{
		public:
			explicit AbilityBalanceE2EShip(World* world)
				: SpaceShip{ world, MakeTestShipDefinition() }
			{
				CenterPivot();
			}

			float GetPhysicsCollisionRadius() const override { return 14.f; }
		};

		struct DamageProbe
		{
			Actor* expectedTarget = nullptr;
			GameplayTag expectedDamageType;
			int resolvedCount = 0;
			float originalDamage = 0.f;
			float appliedDamage = 0.f;
			bool damageTypeMatched = false;

			void OnDamageResolved(const DamageContext& context)
			{
				if (context.target != expectedTarget)
				{
					return;
				}
				if (resolvedCount == 0)
				{
					originalDamage = context.originalDamage;
					appliedDamage = context.appliedDamage;
					damageTypeMatched = std::find(
						context.damageTags.begin(),
						context.damageTags.end(),
						expectedDamageType
					) != context.damageTags.end();
				}
				++resolvedCount;
			}
		};

		struct ScenarioResult
		{
			bool activated = false;
			bool leveled = false;
			bool actorSpawned = false;
			bool damageTypeMatched = false;
			bool abilityEnded = false;
			bool actorCleanedUp = false;
			int damageEvents = 0;
			int firstHitStatusStacks = 0;
			int statusStacks = 0;
			float expectedDamage = 0.f;
			float expectedAppliedDamage = -1.f;
			float originalDamage = 0.f;
			float appliedDamage = 0.f;
			std::string error;
		};

		float ReadActorAttribute(const AbilityActorDefinition* actor, const sas::AttributeId& id)
		{
			if (!actor)
			{
				return -1.f;
			}
			const sas::GameplayAttribute* attribute = sas::FindAttribute(actor->attributes, id);
			return attribute ? attribute->baseValue : -1.f;
		}

		const sas::AttributeModifier* FindAdditiveModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& id
		)
		{
			const auto found = std::find_if(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[&](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == id &&
						modifier.operation == sas::AttributeModifierOperation::Add;
				}
			);
			return found == step.attributeModifiers.end() ? nullptr : &*found;
		}

		bool HasUnboundedSteps(const GameAbilityDefinition& definition)
		{
			return definition.ResolveLevelStep(0) && definition.ResolveLevelStep(30);
		}

		// Steps carry only damage (+ scaling); cooldown reduction is appended globally at
		// level resolution, so verify the shared formula instead of per-step modifiers.
		bool MatchesCooldownProgression(
			const GameAbilityDefinition& definition,
			float damagePerLevel
		)
		{
			if (definition.cooldown <= 1.f)
			{
				return false;
			}

			for (std::size_t index : { std::size_t{ 0 }, std::size_t{ 30 } })
			{
				const AbilityLevelStep* step = definition.ResolveLevelStep(index);
				const sas::AttributeModifier* damage = step
					? FindAdditiveModifier(*step, CommonAttributeIds::Damage)
					: nullptr;
				if (!step || step->attributeModifiers.size() != 1 || !damage ||
					FindAdditiveModifier(*step, CommonAttributeIds::Cooldown) ||
					!NearlyEqual(damage->magnitude, damagePerLevel))
				{
					return false;
				}
			}

			const float firstReduction = 0.175f + 0.025f * definition.cooldown;
			const float maxReduction = definition.cooldown - 1.f;
			return NearlyEqual(
					GetGlobalAbilityCooldownTotalReduction(definition.cooldown, 1),
					std::min(firstReduction, maxReduction)) &&
				GetGlobalAbilityCooldownTotalReduction(definition.cooldown, 24) >=
					GetGlobalAbilityCooldownTotalReduction(definition.cooldown, 23) &&
				definition.cooldown - GetGlobalAbilityCooldownTotalReduction(definition.cooldown, 24) >=
					1.f - ValueTolerance;
		}

		void TickWorld(World& world, int frameCount)
		{
			for (int frame = 0; frame < frameCount; ++frame)
			{
				world.TickInternal(FrameDeltaSeconds);
			}
		}

		template <typename ActorType>
		std::size_t CountActors(const World& world)
		{
			return world.GetActorsByTypeIncludingPending<ActorType>().size();
		}

		template <typename ActorType>
		ScenarioResult RunRuntimeScenario(
			const GameAbilityDefinition& definition,
			int level,
			float targetDistance,
			float expectedDamage,
			const GameplayTag& expectedDamageType,
			const char* expectedStatusId,
			int expectedStatusStacks,
			int cleanupFrames,
			bool holdInputForDuration = false,
			float targetArmor = 0.f,
			float expectedAppliedDamage = -1.f,
			bool targetAtMouse = false
		)
		{
			ScenarioResult result;
			result.expectedDamage = expectedDamage;
			result.expectedAppliedDamage = expectedAppliedDamage;
			AbilityBalanceE2EApplication application;
			World world{ targetAtMouse ? &application : nullptr };
			const shared_ptr<AbilityBalanceE2EShip> owner = world.SpawnActor<AbilityBalanceE2EShip>().lock();
			const shared_ptr<AbilityBalanceE2EShip> target = world.SpawnActor<AbilityBalanceE2EShip>().lock();
			if (!owner || !target)
			{
				result.error = "Could not spawn the real owner and damage target";
				return result;
			}

			owner->SetActorLocation({ 0.f, 0.f });
			const sf::Vector2f targetLocation = targetAtMouse
				? sf::Vector2f{ 100.f, 0.f }
				: targetDistance > 0.f
					? owner->GetActorLocation() + owner->GetActorForwardDirection() * targetDistance
					: sf::Vector2f{ 0.f, 0.f };
			target->SetActorLocation(targetLocation);
			owner->SetCollisionLayer(CollisionLayer::Player);
			owner->SetCollisionMask(CollisionLayer::Enemy);
			target->SetCollisionLayer(CollisionLayer::Enemy);
			target->SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
			sas::AttributeSystem& ownerAttributes =
				owner->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			ownerAttributes.SetBaseValue(OwnerAttributeIds::AttackPower, TestPower);
			ownerAttributes.SetBaseValue(OwnerAttributeIds::EnergyPower, TestPower);
			ownerAttributes.SetBaseValue(OwnerAttributeIds::CriticalChance, 0.f);
			target->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes().SetBaseValue(
				OwnerAttributeIds::Armor,
				targetArmor
			);
			world.TickInternal(0.f);
			if (targetAtMouse)
			{
				target->SetActorLocation(world.GetMouseWorldPosition());
			}

			LightYearsAbilitySystemComponent& abilitySystem =
				owner->GetCombatRuntime().GetAbilitySystemComponent();
			std::string grantFailure;
			(void)abilitySystem.GrantAbility(definition, sas::AbilitySlot::Ability1, &grantFailure);
			GameAbility* ability = abilitySystem.GetAbilityById(definition.abilityId);
			if (!ability)
			{
				result.error = grantFailure.empty() ? "Could not grant the shipped ability" : grantFailure;
				return result;
			}
			result.leveled = ability->GetLevel() == level || ability->SetLevel(level);
			if (!result.leveled)
			{
				result.error = "Could not set the shipped ability to its catalog max level";
				return result;
			}

			DamageProbe probe;
			probe.expectedTarget = target.get();
			probe.expectedDamageType = expectedDamageType;
			const DelegateHandle damageHandle = target->GetCombatRuntime().onDamageResolved.BindAction(
				&probe,
				&DamageProbe::OnDamageResolved
			);
			if (holdInputForDuration)
			{
				abilitySystem.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			}
			result.activated = ability->TryActivate();
			if (!result.activated)
			{
				result.error = "The shipped ability instance rejected activation";
			}
			else
			{
				result.actorSpawned = CountActors<ActorType>(world) > 0;
				for (int frame = 0; frame < 240; ++frame)
				{
					TickWorld(world, 1);
					if (probe.resolvedCount > 0 && result.firstHitStatusStacks == 0 &&
						expectedStatusId && *expectedStatusId != '\0')
					{
						const sas::ActiveGameplayEffect* firstHitStatus =
							target->GetCombatRuntime().GetAbilitySystemComponent().FindGameplayEffectById(expectedStatusId);
						result.firstHitStatusStacks = firstHitStatus ? firstHitStatus->stackCount : 0;
					}
					if ((!holdInputForDuration && probe.resolvedCount > 0) ||
						(holdInputForDuration && !ability->IsActive()))
					{
						break;
					}
				}
				if (holdInputForDuration)
				{
					abilitySystem.SetAbilitySlotInput(sas::AbilitySlot::Ability1, false);
				}
				result.damageEvents = probe.resolvedCount;
				result.originalDamage = probe.originalDamage;
				result.appliedDamage = probe.appliedDamage;
				result.damageTypeMatched = probe.damageTypeMatched;
				if (expectedStatusId && *expectedStatusId != '\0')
				{
					const sas::ActiveGameplayEffect* status =
						target->GetCombatRuntime().GetAbilitySystemComponent().FindGameplayEffectById(expectedStatusId);
					result.statusStacks = status ? status->stackCount : 0;
				}
				TickWorld(world, cleanupFrames);
				result.abilityEnded = !ability->IsActive();
				result.actorCleanedUp = CountActors<ActorType>(world) == 0;
			}
			target->GetCombatRuntime().onDamageResolved.UnbindAction(damageHandle);
			return result;
		}

		bool WriteArtifact(const std::filesystem::path& path, const Json& artifact)
		{
			std::error_code error;
			if (!path.parent_path().empty())
			{
				std::filesystem::create_directories(path.parent_path(), error);
				if (error)
				{
					return false;
				}
			}
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output)
			{
				return false;
			}
			output << artifact.dump(2) << '\n';
			return output.good();
		}

		Json SerializeScenario(const ScenarioResult& result)
		{
			return {
				{ "abilityEnded", result.abilityEnded },
				{ "activated", result.activated },
				{ "actorSpawned", result.actorSpawned },
				{ "actorCleanedUp", result.actorCleanedUp },
				{ "appliedDamage", result.appliedDamage },
				{ "damageEvents", result.damageEvents },
				{ "damageTypeMatched", result.damageTypeMatched },
				{ "error", result.error },
				{ "expectedDamage", result.expectedDamage },
				{ "expectedAppliedDamage", result.expectedAppliedDamage },
				{ "firstHitStatusStacks", result.firstHitStatusStacks },
				{ "leveled", result.leveled },
				{ "originalDamage", result.originalDamage },
				{ "statusStacks", result.statusStacks }
			};
		}
	}

	int RunAbilityBalanceSecondThreeE2E(const char* artifactPath, const std::string& setupError)
	{
		const GameAbilityDefinition* sunBeam = content::AbilityContentCatalog::FindById(
			AbilityData::SunBeam::AbilityId::Strike::Basic
		);
		const GameAbilityDefinition* rocket = content::AbilityContentCatalog::FindById(
			AbilityData::Rocket::AbilityId::Basic
		);
		const GameAbilityDefinition* inferno = content::AbilityContentCatalog::FindById(
			AbilityData::InfernoSpray::AbilityId::Basic
		);
		const AbilityActorDefinition* sunBeamActor = content::AbilityContentCatalog::FindActorById(
			AbilityData::SunBeam::Actor::Strike::BasicDefinitionId
		);
		const AbilityActorDefinition* rocketActor = content::AbilityContentCatalog::FindActorById(
			AbilityData::Rocket::Actor::Projectile::BasicDefinitionId
		);
		const AbilityActorDefinition* infernoActor = content::AbilityContentCatalog::FindActorById(
			AbilityData::InfernoSpray::Actor::FlameCone::BasicDefinitionId
		);

		const bool sunBeamContentMatches = sunBeam && sunBeamActor &&
			NearlyEqual(sunBeam->cooldown, 9.f) && HasUnboundedSteps(*sunBeam) &&
			NearlyEqual(ReadActorAttribute(sunBeamActor, CommonAttributeIds::Damage), 40.f) &&
			NearlyEqual(ReadActorAttribute(sunBeamActor, CommonAttributeIds::Radius), 110.f) &&
			NearlyEqual(ReadActorAttribute(sunBeamActor, AbilityData::SunBeam::Actor::Strike::TelegraphDuration), 0.5f) &&
			NearlyEqual(ReadActorAttribute(sunBeamActor, AbilityData::SunBeam::Actor::Strike::ArrivalDuration), 0.2f) &&
			NearlyEqual(ReadActorAttribute(sunBeamActor, AbilityData::SunBeam::Actor::Strike::ImpactDelay), 0.05f) &&
			MatchesCooldownProgression(*sunBeam, 10.f) &&
			sunBeam->scalingRules.size() == 1 &&
			sunBeam->scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
			sunBeam->scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyPower &&
			sunBeam->scalingRules.front().operation == sas::AttributeModifierOperation::Add &&
			NearlyEqual(sunBeam->scalingRules.front().coefficient, 0.7f) &&
			[&]()
			{
				const AbilityLevelStep* step = sunBeam->ResolveLevelStep(0);
				return step && step->scalingRules.size() == 1 &&
					step->scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
					step->scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyPower &&
					step->scalingRules.front().operation == sas::AttributeModifierOperation::Add &&
					NearlyEqual(step->scalingRules.front().coefficient, 0.1f);
			}();

		const bool rocketContentMatches = rocket && rocketActor &&
			NearlyEqual(rocket->cooldown, 6.f) && HasUnboundedSteps(*rocket) &&
			NearlyEqual(ReadActorAttribute(rocketActor, CommonAttributeIds::Damage), 55.f) &&
			NearlyEqual(ReadActorAttribute(rocketActor, CommonAttributeIds::Radius), 55.f) &&
			NearlyEqual(ReadActorAttribute(rocketActor, CommonAttributeIds::Range), 1100.f) &&
			NearlyEqual(ReadActorAttribute(rocketActor, CollisionAttributeIds::Radius), 8.f) &&
			NearlyEqual(ReadActorAttribute(rocketActor, DamageAttributeIds::KineticStacks), 2.f) &&
			MatchesCooldownProgression(*rocket, 5.f) && rocket->scalingRules.size() == 1 &&
			rocket->scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
			rocket->scalingRules.front().sourceAttributeId == OwnerAttributeIds::AttackPower &&
			rocket->scalingRules.front().operation == sas::AttributeModifierOperation::Add &&
			NearlyEqual(rocket->scalingRules.front().coefficient, 1.25f);

		const bool infernoContentMatches = inferno && infernoActor &&
			NearlyEqual(inferno->cooldown, 3.f) && NearlyEqual(inferno->duration, 3.f) &&
			(!inferno->ResolveLevelStep(0) || inferno->ResolveLevelStep(0)->attributeModifiers.empty()) &&
			GetGlobalAbilityCooldownTotalReduction(inferno->cooldown, 1) > 0.f &&
			inferno->scalingRules.empty() &&
			NearlyEqual(ReadActorAttribute(infernoActor, AbilityData::InfernoSpray::Actor::FlameCone::BaseDPS), 24.f) &&
			NearlyEqual(ReadActorAttribute(infernoActor, AbilityData::InfernoSpray::Actor::FlameCone::CombatTickInterval), 0.25f) &&
			NearlyEqual(ReadActorAttribute(infernoActor, DamageAttributeIds::IgniteStacks), 1.f);

		const bool presentationProfilesRegistered =
			PresentationProfileRegistry<SunBeamPresentationProfile>::Find(SunBeamPresentationIds::StrikeBasic) &&
			PresentationProfileRegistry<RocketPresentationProfile>::Find(RocketPresentationIds::ProjectileBasic) &&
			PresentationProfileRegistry<InfernoSprayPresentationProfile>::Find(InfernoSprayPresentationIds::FlameConeBasic);

		ScenarioResult sunBeamRuntime;
		if (sunBeam)
		{
			sunBeamRuntime = RunRuntimeScenario<SunBeamStrikeActor>(
				*sunBeam, 5, 50.f, 190.f, DamageTypeSchema::Photonic, "", 0, 120, false, 0.f, -1.f, true
			);
		}
		else
		{
			sunBeamRuntime.error = "SunBeam definition is missing";
		}
		ScenarioResult rocketRuntime;
		if (rocket)
		{
			rocketRuntime = RunRuntimeScenario<RocketProjectileActor>(
				*rocket, 15, 150.f, 250.f, DamageTypeSchema::Kinetic,
				DamageStatusEffectIds::KineticEffectId, 2, 120, false, 100.f, 125.f
			);
		}
		else
		{
			rocketRuntime.error = "Rocket definition is missing";
		}
		ScenarioResult infernoRuntime;
		if (inferno)
		{
			infernoRuntime = RunRuntimeScenario<InfernoSprayActor>(
				*inferno, 1, 150.f, 24.75f, DamageTypeSchema::Thermal,
				DamageStatusEffectIds::IgniteEffectId, 4, 240, true
			);
		}
		else
		{
			infernoRuntime.error = "InfernoSpray definition is missing";
		}

		const bool bootstrapSucceeded = setupError.empty();
		const bool sunBeamDamageMatched = sunBeamRuntime.activated && sunBeamRuntime.damageEvents > 0 &&
			sunBeamRuntime.actorSpawned &&
			NearlyEqual(sunBeamRuntime.originalDamage, sunBeamRuntime.expectedDamage) &&
			sunBeamRuntime.damageTypeMatched && sunBeamRuntime.abilityEnded && sunBeamRuntime.actorCleanedUp;
		const bool rocketDamageAndStatusMatched = rocketRuntime.activated && rocketRuntime.damageEvents > 0 &&
			rocketRuntime.actorSpawned && rocketRuntime.firstHitStatusStacks == 2 &&
			NearlyEqual(rocketRuntime.originalDamage, rocketRuntime.expectedDamage) &&
			NearlyEqual(rocketRuntime.appliedDamage, rocketRuntime.expectedAppliedDamage) &&
			rocketRuntime.damageTypeMatched && rocketRuntime.statusStacks == 2 &&
			rocketRuntime.abilityEnded && rocketRuntime.actorCleanedUp;
		const bool infernoDamageAndStatusMatched = infernoRuntime.activated && infernoRuntime.damageEvents > 0 &&
			infernoRuntime.actorSpawned && infernoRuntime.firstHitStatusStacks == 1 &&
			infernoRuntime.damageEvents >= 4 &&
			NearlyEqual(infernoRuntime.originalDamage, infernoRuntime.expectedDamage) &&
			infernoRuntime.damageTypeMatched && infernoRuntime.statusStacks == 4 &&
			infernoRuntime.abilityEnded && infernoRuntime.actorCleanedUp;
		const bool passed = bootstrapSucceeded && sunBeamContentMatches && rocketContentMatches &&
			infernoContentMatches && presentationProfilesRegistered && sunBeamDamageMatched &&
			rocketDamageAndStatusMatched && infernoDamageAndStatusMatched;

		const Json artifact{
			{ "assertions", {
				{ "bootstrapSucceeded", bootstrapSucceeded },
				{ "infernoCatalogFormulaAndSingleIgniteStackMatch", infernoContentMatches },
				{ "infernoRuntimeHeldDurationDamageIgniteAndCleanupMatch", infernoDamageAndStatusMatched },
				{ "presentationProfilesRegisteredInTypedRegistries", presentationProfilesRegistered },
				{ "rocketCatalogFormulaAndCooldownProgressionMatch", rocketContentMatches },
				{ "rocketRuntimeDamageSameHitPenetrationStacksAndCleanupMatch", rocketDamageAndStatusMatched },
				{ "sunBeamCatalogFormulaAndCooldownProgressionMatch", sunBeamContentMatches },
				{ "sunBeamRuntimeDamageAndCleanupMatch", sunBeamDamageMatched }
			} },
			{ "input", {
				{ "attackPower", TestPower },
				{ "energyPower", TestPower },
				{ "frameDeltaSeconds", FrameDeltaSeconds },
				{ "scenario", "loaded ability definitions granted to live SpaceShip combat runtimes" }
			} },
			{ "outcome", {
				{ "infernoSpray", SerializeScenario(infernoRuntime) },
				{ "rocket", SerializeScenario(rocketRuntime) },
				{ "sunBeamStrike", SerializeScenario(sunBeamRuntime) },
				{ "setupError", setupError }
			} },
			{ "passed", passed },
			{ "scenario", "d2.ability_balance_second_three_live_runtime" },
			{ "schemaVersion", 1 }
		};

		if (!WriteArtifact(std::filesystem::absolute(artifactPath), artifact))
		{
			std::cerr << "Could not write ability balance second-three E2E artifact.\n";
			return 1;
		}
		std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
		if (!passed)
		{
			std::cerr << "Ability balance second-three E2E failed; see the artifact for scenario outcomes.\n";
			return 1;
		}
		std::cout << "Ability balance second-three live-runtime E2E passed.\n";
		return 0;
	}
}

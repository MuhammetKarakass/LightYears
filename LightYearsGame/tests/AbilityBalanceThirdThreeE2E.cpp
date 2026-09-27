#include "attributes/AttributeSystem.h"
#include "effects/GameplayEffectSpec.h"
#include "framework/World.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/ability/mineLayer/MineLayerContracts.h"
#include "gameplay/ability/mineLayer/MineLayerMineActor.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "spaceShip/SpaceShip.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ly
{
	namespace
	{
		constexpr float E2ETolerance = 0.05f;
		constexpr float OwnerHealth = 10000.f;
		constexpr float TargetHealth = 10000.f;

		ShipDefinition MakeBalanceShipDefinition(float health)
		{
			return ShipDefinition{
				"SpaceShooterRedux/PNG/Enemies/enemyRed5.png",
				health,
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
			explicit BalanceE2EShip(World* world)
				: SpaceShip{ world, MakeBalanceShipDefinition(OwnerHealth) }
			{
				CenterPivot();
			}
		};

		class BalanceE2ETarget final : public SpaceShip
		{
		public:
			explicit BalanceE2ETarget(World* world)
				: SpaceShip{ world, MakeBalanceShipDefinition(TargetHealth) }
			{
				CenterPivot();
			}
		};

		bool Near(float left, float right, float tolerance = E2ETolerance)
		{
			return std::isfinite(left) && std::abs(left - right) <= tolerance;
		}

		GameAbility* GrantAbility(
			SpaceShip& owner,
			const std::string& abilityId,
			std::string& failureReason
		)
		{
			const GameAbilityDefinition* definition =
				content::AbilityContentCatalog::FindById(abilityId);
			if (!definition)
			{
				failureReason = "Shipped ability was not loaded: " + abilityId;
				return nullptr;
			}
			LightYearsAbilitySystemComponent& abilitySystem =
				owner.GetAbilitySystemComponent();
			const sas::AbilityHandle handle = abilitySystem.GrantAbility(
				*definition,
				&failureReason
			);
			if (!handle.IsValid())
			{
				return nullptr;
			}
			GameAbility* ability = abilitySystem.GetAbilityById(abilityId);
			if (!ability)
			{
				failureReason = "Granted ability could not be found by its shipped ID: " + abilityId;
			}
			return ability;
		}

		void ConfigureCombatLayers(SpaceShip& owner, SpaceShip& target)
		{
			owner.SetCollisionLayer(CollisionLayer::Player);
			owner.SetCollisionMask(CollisionLayer::Enemy);
			target.SetCollisionLayer(CollisionLayer::Enemy);
			target.SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
		}

		std::vector<shared_ptr<AbilityWorldActor>> CollectAbilityActors(World& world)
		{
			std::vector<shared_ptr<AbilityWorldActor>> result;
			for (const weak_ptr<AbilityWorldActor>& weakActor :
				world.GetActorsByTypeIncludingPending<AbilityWorldActor>())
			{
				if (const shared_ptr<AbilityWorldActor> actor = weakActor.lock())
				{
					result.push_back(actor);
				}
			}
			return result;
		}

		void CollectLaunchedRockets(
			World& world,
			std::unordered_map<std::size_t, float>& launchedRockets
		)
		{
			for (const shared_ptr<AbilityWorldActor>& rocket : CollectAbilityActors(world))
			{
				launchedRockets.emplace(
					static_cast<std::size_t>(rocket->GetUniqueID()),
					rocket->GetDamage()
				);
			}
		}

		std::vector<shared_ptr<MineLayerMineActor>> CollectMines(World& world)
		{
			std::vector<shared_ptr<MineLayerMineActor>> result;
			for (const weak_ptr<MineLayerMineActor>& weakMine :
				world.GetActorsByTypeIncludingPending<MineLayerMineActor>())
			{
				if (const shared_ptr<MineLayerMineActor> mine = weakMine.lock())
				{
					result.push_back(mine);
				}
			}
			return result;
		}

		struct OverdriveResult
		{
			bool activated = false;
			bool rocketCountMatchedExternalAttackSpeed = false;
			bool rocketDamageMatchedLevelScaling = false;
			bool cooldownStartedAfterSixSecondActiveWindow = false;
			bool attackSpeedAfterSalvoMatched = false;
			bool attackSpeedAfterEndMatched = false;
			std::size_t rocketCount = 0;
			float rocketDamage = 0.f;
			float cooldownAfterEnd = 0.f;
			float attackSpeedBeforeBoost = 0.f;
			float attackSpeedWithBoost = 0.f;
			float attackSpeedAfterEnd = 0.f;
			std::string error;
		};

		OverdriveResult RunOverdriveScenario(int level, bool addOverlappingBoost)
		{
			OverdriveResult result;
			World world{ nullptr };
			const shared_ptr<BalanceE2EShip> owner =
				world.SpawnActor<BalanceE2EShip>().lock();
			const shared_ptr<BalanceE2ETarget> target =
				world.SpawnActor<BalanceE2ETarget>().lock();
			if (!owner || !target)
			{
				result.error = "Could not spawn Overdrive owner and target.";
				return result;
			}
			ConfigureCombatLayers(*owner, *target);
			owner->SetActorLocation({ 0.f, 0.f });
			target->SetActorLocation({ 0.f, -1050.f });
			world.TickInternal(0.f);

			LightYearsAbilitySystemComponent& abilitySystem =
				owner->GetAbilitySystemComponent();
			sas::AttributeSystem& attributes = abilitySystem.GetAttributes();
			attributes.SetBaseValue(OwnerAttributeIds::AttackPower, 100.f);
			attributes.SetBaseValue(OwnerAttributeIds::AttackSpeed, addOverlappingBoost ? 15.f : 0.f);
			if (addOverlappingBoost)
			{
				attributes.AddModifier(sas::AttributeModifier{
					OwnerAttributeIds::AttackSpeed,
					sas::AttributeModifierOperation::Multiply,
					2.f
				});
				const sas::GameplayEffectDefinition* boostDefinition =
					EffectData::FindGameplayEffectDefinition(
						AbilityData::OverdriveCore::Effect::AttackSpeedBoostId
					);
				if (!boostDefinition)
				{
					result.error = "Overdrive attack-speed effect definition was not loaded.";
					return result;
				}
				sas::GameplayEffectSpec overlappingBoost =
					sas::MakeGameplayEffectSpec(*boostDefinition);
				overlappingBoost.duration = 30.f;
				overlappingBoost.maxStacks = 1;
				overlappingBoost.modifiers = {
					sas::AttributeModifier{
						OwnerAttributeIds::AttackSpeed,
						sas::AttributeModifierOperation::Add,
						40.f
					}
				};
				if (!abilitySystem.ApplyGameplayEffect(overlappingBoost).IsValid())
				{
					result.error = "Could not apply the overlapping Overdrive speed effect.";
					return result;
				}
			}
			result.attackSpeedBeforeBoost = attributes.GetCurrentValue(OwnerAttributeIds::AttackSpeed);

			GameAbility* ability = GrantAbility(
				*owner,
				AbilityData::OverdriveCore::AbilityId::Basic,
				result.error
			);
			if (!ability || (ability->GetLevel() != level && !ability->SetLevel(level)))
			{
				if (result.error.empty()) result.error = "Could not set Overdrive level.";
				return result;
			}
			result.activated = ability->TryActivate();
			if (!result.activated)
			{
				result.error = "Overdrive failed to activate against its in-range target.";
				return result;
			}

			std::unordered_map<std::size_t, float> launchedRockets;
			CollectLaunchedRockets(world, launchedRockets);
			for (int tick = 0; tick < 101; ++tick)
			{
				world.TickInternal(0.01f);
				CollectLaunchedRockets(world, launchedRockets);
			}
			result.rocketCount = launchedRockets.size();
			if (!launchedRockets.empty()) result.rocketDamage = launchedRockets.begin()->second;
			result.attackSpeedWithBoost = attributes.GetCurrentValue(OwnerAttributeIds::AttackSpeed);
			const std::size_t expectedRocketCount = addOverlappingBoost ? 11u : 8u;
			const float expectedDamage = level == 5 ? 52.f : 32.f;
			const float expectedAttackSpeedWithBoost = level == 5 ? 86.f : 20.f;
			result.rocketCountMatchedExternalAttackSpeed =
				result.rocketCount == expectedRocketCount;
			result.rocketDamageMatchedLevelScaling = launchedRockets.size() == expectedRocketCount &&
				std::all_of(launchedRockets.begin(), launchedRockets.end(), [&](const auto& rocket)
				{
					return Near(rocket.second, expectedDamage);
				});
			result.attackSpeedAfterSalvoMatched = Near(
				result.attackSpeedWithBoost,
				expectedAttackSpeedWithBoost
			);
			const bool stillActiveBeforeSixSeconds = ability->IsActive() &&
				Near(ability->GetCooldownRemaining(), 0.f);
			world.TickInternal(4.98f);
			const bool activeAtFivePointNineNine = ability->IsActive() &&
				Near(ability->GetCooldownRemaining(), 0.f);
			world.TickInternal(0.02f);
			result.cooldownAfterEnd = ability->GetCooldownRemaining();
			result.attackSpeedAfterEnd = attributes.GetCurrentValue(OwnerAttributeIds::AttackSpeed);
			const float expectedCooldown = level == 5 ? 8.3f : 10.f;
			result.cooldownStartedAfterSixSecondActiveWindow =
				stillActiveBeforeSixSeconds && activeAtFivePointNineNine &&
				!ability->IsActive() && Near(result.cooldownAfterEnd, expectedCooldown);
			result.attackSpeedAfterEndMatched = Near(
				result.attackSpeedAfterEnd,
				level == 5 ? 30.f : 0.f
			);
			if (!result.rocketCountMatchedExternalAttackSpeed ||
				!result.rocketDamageMatchedLevelScaling ||
				!result.cooldownStartedAfterSixSecondActiveWindow ||
				!result.attackSpeedAfterSalvoMatched || !result.attackSpeedAfterEndMatched)
			{
				result.error = "Overdrive count, damage, attack speed, or post-active cooldown did not match shipped progression.";
			}
			return result;
		}

		struct ExecutionDriveResult
		{
			bool activated = false;
			bool snapshottedAttackPower = false;
			bool appliedMoveSpeedBonus = false;
			bool endedCleanly = false;
			float attackPowerWhileActive = 0.f;
			float movementMultiplierWhileActive = 0.f;
			float attackPowerAfterEnd = 0.f;
			float movementMultiplierAfterEnd = 0.f;
			std::string error;
		};

		ExecutionDriveResult RunExecutionDriveScenario()
		{
			ExecutionDriveResult result;
			World world{ nullptr };
			const shared_ptr<BalanceE2EShip> owner =
				world.SpawnActor<BalanceE2EShip>().lock();
			if (!owner)
			{
				result.error = "Could not spawn Execution Drive owner.";
				return result;
			}
			LightYearsAbilitySystemComponent& abilitySystem =
				owner->GetAbilitySystemComponent();
			abilitySystem.GetAttributes().SetBaseValue(OwnerAttributeIds::AttackPower, 100.f);
			GameAbility* ability = GrantAbility(
				*owner,
				AbilityData::ExecutionDrive::AbilityId::Basic,
				result.error
			);
			if (!ability || !ability->SetLevel(5))
			{
				if (result.error.empty()) result.error = "Could not set Execution Drive level.";
				return result;
			}
			result.activated = ability->TryActivate();
			if (!result.activated)
			{
				result.error = "Execution Drive failed to activate.";
				return result;
			}
			result.attackPowerWhileActive = abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::AttackPower
			);
			result.movementMultiplierWhileActive = owner->GetMovementSpeedMultiplier();
			result.snapshottedAttackPower = Near(result.attackPowerWhileActive, 142.f);
			result.appliedMoveSpeedBonus = Near(result.movementMultiplierWhileActive, 1.19f);
			world.TickInternal(5.1f);
			result.attackPowerAfterEnd = abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::AttackPower
			);
			result.movementMultiplierAfterEnd = owner->GetMovementSpeedMultiplier();
			result.endedCleanly = !ability->IsActive() &&
				Near(result.attackPowerAfterEnd, 100.f) &&
				Near(result.movementMultiplierAfterEnd, 1.f);
			if (!result.snapshottedAttackPower || !result.appliedMoveSpeedBonus ||
				!result.endedCleanly)
			{
				result.error = "Execution Drive snapshot, movement bonus, or timed cleanup did not match level-five runtime values.";
			}
			return result;
		}

		struct MineResult
		{
			bool activated = false;
			bool spawnedExactlyThree = false;
			bool levelFifteenDamageMatched = false;
			bool stunApplied = false;
			bool stunDurationRefreshed = false;
			bool stunExpiredAfterRefresh = false;
			std::size_t mineCount = 0;
			float mineDamage = 0.f;
			std::vector<float> cooldowns;
			std::string error;
		};

		bool RunMineCooldownScenario(int level, float expectedCooldown, float* outDamage, std::string& error)
		{
			World world{ nullptr };
			const shared_ptr<BalanceE2EShip> owner =
				world.SpawnActor<BalanceE2EShip>().lock();
			if (!owner)
			{
				error = "Could not spawn Mine Layer owner.";
				return false;
			}
			owner->GetAbilitySystemComponent().GetAttributes().SetBaseValue(
				OwnerAttributeIds::AttackPower,
				100.f
			);
			GameAbility* ability = GrantAbility(
				*owner,
				AbilityData::MineLayer::AbilityId::Basic,
				error
			);
			if (!ability || (ability->GetLevel() != level && !ability->SetLevel(level)) ||
				!ability->TryActivate())
			{
				if (error.empty()) error = "Mine Layer failed to activate at level " + std::to_string(level) + ".";
				return false;
			}
			const std::vector<shared_ptr<MineLayerMineActor>> mines = CollectMines(world);
			if (outDamage && !mines.empty()) *outDamage = mines.front()->GetDamage();
			return mines.size() == 3 && Near(ability->GetCooldownRemaining(), expectedCooldown);
		}

		MineResult RunMineLayerScenario()
		{
			MineResult result;
			const std::vector<std::pair<int, float>> expectedCooldowns{
				{ 1, 9.f }, { 5, 7.4f }, { 9, 6.2f }, { 13, 5.4f }, { 15, 5.2f }
			};
			bool cooldownsMatched = true;
			for (const auto& expected : expectedCooldowns)
			{
				float damage = 0.f;
				std::string error;
				const bool passed = RunMineCooldownScenario(
					expected.first,
					expected.second,
					expected.first == 15 ? &damage : nullptr,
					error
				);
				cooldownsMatched = cooldownsMatched && passed;
				result.cooldowns.push_back(expected.second);
				if (!passed && result.error.empty()) result.error = error.empty()
					? "Mine Layer mine count or cooldown did not match the expected level."
					: error;
				if (expected.first == 15) result.mineDamage = damage;
			}
			result.spawnedExactlyThree = cooldownsMatched;
			result.levelFifteenDamageMatched = Near(result.mineDamage, 448.f);

			World world{ nullptr };
			const shared_ptr<BalanceE2EShip> owner =
				world.SpawnActor<BalanceE2EShip>().lock();
			const shared_ptr<BalanceE2ETarget> target =
				world.SpawnActor<BalanceE2ETarget>().lock();
			if (!owner || !target)
			{
				result.error = "Could not spawn Mine Layer stun-refresh actors.";
				return result;
			}
			ConfigureCombatLayers(*owner, *target);
			owner->SetActorLocation({ 0.f, 0.f });
			target->SetActorLocation({ 3000.f, 3000.f });
			owner->GetAbilitySystemComponent().GetAttributes().SetBaseValue(
				OwnerAttributeIds::AttackPower,
				100.f
			);
			GameAbility* ability = GrantAbility(
				*owner,
				AbilityData::MineLayer::AbilityId::Basic,
				result.error
			);
			if (!ability || !ability->TryActivate())
			{
				if (result.error.empty()) result.error = "Mine Layer failed to activate for stun-refresh scenario.";
				return result;
			}
			std::vector<shared_ptr<MineLayerMineActor>> mines = CollectMines(world);
			result.mineCount = mines.size();
			result.spawnedExactlyThree = result.spawnedExactlyThree && mines.size() == 3;
			if (mines.size() != 3)
			{
				result.error = "Mine Layer did not spawn exactly three runtime mines.";
				return result;
			}

			world.TickInternal(0.f);
			for (int tick = 0; tick < 100; ++tick)
			{
				bool allDeployed = true;
				for (const shared_ptr<MineLayerMineActor>& mine : mines)
				{
					allDeployed = allDeployed && !mine->IsDeploying();
				}
				if (allDeployed) break;
				world.TickInternal(0.05f);
			}
			if (mines[0]->IsDeploying() || mines[1]->IsDeploying())
			{
				result.error = "Mine Layer mines did not finish their deployment flight.";
				return result;
			}
			const sf::Vector2f refreshTestOrigin = mines[0]->GetActorLocation();
			mines[1]->SetActorLocation(refreshTestOrigin + sf::Vector2f{ 400.f, 0.f });
			mines[1]->SetVelocity({});
			mines[2]->SetActorLocation(refreshTestOrigin + sf::Vector2f{ -400.f, 0.f });
			mines[2]->SetVelocity({});

			target->SetActorLocation(mines[0]->GetActorLocation());
			world.TickInternal(0.f);
			result.stunApplied = target->GetAbilitySystemComponent().HasOwnedTag(
				GameplayTags::State::Effect::Control::Stunned
			);
			if (!result.stunApplied)
			{
				result.error = "First mine did not apply the shared Stun effect.";
				return result;
			}

			world.TickInternal(0.6f);
			target->SetVelocity({});
			target->SetActorLocation(mines[1]->GetActorLocation());
			world.TickInternal(0.f);
			world.TickInternal(0.5f);
			result.stunDurationRefreshed = target->GetAbilitySystemComponent().HasOwnedTag(
				GameplayTags::State::Effect::Control::Stunned
			);
			world.TickInternal(0.6f);
			result.stunExpiredAfterRefresh = !target->GetAbilitySystemComponent().HasOwnedTag(
				GameplayTags::State::Effect::Control::Stunned
			);
			if (!result.levelFifteenDamageMatched || !result.spawnedExactlyThree ||
				!result.stunDurationRefreshed || !result.stunExpiredAfterRefresh)
			{
				result.error = "Mine Layer damage, three-mine count, or shared stun refresh did not match runtime expectations.";
			}
			result.activated = cooldownsMatched && result.stunApplied;
			return result;
		}

		bool WriteArtifact(const char* artifactPath, const nlohmann::json& artifact)
		{
			if (!artifactPath || !*artifactPath) return false;
			const std::filesystem::path path{ artifactPath };
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

	int RunAbilityBalanceThirdThreeE2E(
		const char* artifactPath,
		const std::string& setupError
	)
	{
		OverdriveResult overdriveLevelOne;
		OverdriveResult overdriveLevelFive;
		ExecutionDriveResult executionDrive;
		MineResult mineLayer;
		if (setupError.empty())
		{
			overdriveLevelOne = RunOverdriveScenario(1, false);
			overdriveLevelFive = RunOverdriveScenario(5, true);
			executionDrive = RunExecutionDriveScenario();
			mineLayer = RunMineLayerScenario();
		}
		else
		{
			overdriveLevelOne.error = setupError;
			overdriveLevelFive.error = setupError;
			executionDrive.error = setupError;
			mineLayer.error = setupError;
		}

		const bool passed = overdriveLevelOne.rocketCountMatchedExternalAttackSpeed &&
			overdriveLevelOne.rocketDamageMatchedLevelScaling &&
			overdriveLevelOne.cooldownStartedAfterSixSecondActiveWindow &&
			overdriveLevelOne.attackSpeedAfterSalvoMatched &&
			overdriveLevelOne.attackSpeedAfterEndMatched &&
			overdriveLevelFive.rocketCountMatchedExternalAttackSpeed &&
			overdriveLevelFive.rocketDamageMatchedLevelScaling &&
			overdriveLevelFive.cooldownStartedAfterSixSecondActiveWindow &&
			overdriveLevelFive.attackSpeedAfterSalvoMatched &&
			overdriveLevelFive.attackSpeedAfterEndMatched &&
			executionDrive.snapshottedAttackPower && executionDrive.appliedMoveSpeedBonus &&
			executionDrive.endedCleanly && mineLayer.activated &&
			mineLayer.spawnedExactlyThree && mineLayer.levelFifteenDamageMatched &&
			mineLayer.stunDurationRefreshed && mineLayer.stunExpiredAfterRefresh;
		const nlohmann::json artifact{
			{ "assertions", {
				{ "executionDriveAppliedLevelFiveMovementBonus", executionDrive.appliedMoveSpeedBonus },
				{ "executionDriveCleanedUpAtFiveSeconds", executionDrive.endedCleanly },
				{ "executionDriveUsedPreBuffAttackPowerSnapshot", executionDrive.snapshottedAttackPower },
				{ "mineLayerAppliedCanonicalStun", mineLayer.stunApplied },
				{ "mineLayerLevelFifteenDamage", mineLayer.levelFifteenDamageMatched },
				{ "mineLayerSpawnedExactlyThree", mineLayer.spawnedExactlyThree },
				{ "mineLayerStunExpiredAfterRefreshedWindow", mineLayer.stunExpiredAfterRefresh },
				{ "mineLayerStunRefreshExtendedDuration", mineLayer.stunDurationRefreshed },
				{ "overdriveLevelFiveCooldownStartsAfterSixSeconds", overdriveLevelFive.cooldownStartedAfterSixSecondActiveWindow },
				{ "overdriveLevelFiveDamageAndExternalSpeedCount", overdriveLevelFive.rocketCountMatchedExternalAttackSpeed && overdriveLevelFive.rocketDamageMatchedLevelScaling },
				{ "overdriveLevelFiveAttackSpeedDuringBoost", overdriveLevelFive.attackSpeedAfterSalvoMatched },
				{ "overdriveLevelFiveAttackSpeedRestoredAfterEnd", overdriveLevelFive.attackSpeedAfterEndMatched },
				{ "overdriveLevelOneCooldownStartsAfterSixSeconds", overdriveLevelOne.cooldownStartedAfterSixSecondActiveWindow },
				{ "overdriveLevelOneDamageAndRocketCount", overdriveLevelOne.rocketCountMatchedExternalAttackSpeed && overdriveLevelOne.rocketDamageMatchedLevelScaling },
				{ "overdriveLevelOneAttackSpeedDuringBoost", overdriveLevelOne.attackSpeedAfterSalvoMatched },
				{ "overdriveLevelOneAttackSpeedRestoredAfterEnd", overdriveLevelOne.attackSpeedAfterEndMatched }
			} },
			{ "input", {
				{ "executionDriveBaseAttackPower", 100.f },
				{ "executionDriveLevel", 5 },
				{ "mineLayerLevels", { 1, 5, 9, 13, 15 } },
				{ "mineLayerExpectedCooldowns", { 9.f, 7.4f, 6.2f, 5.4f, 5.2f } },
				{ "mineLayerExpectedLevelFifteenDamage", 448.f },
				{ "overdriveAttackPower", 100.f },
				{ "overdriveExpectedCooldowns", { 10.f, 8.3f } },
				{ "overdriveExpectedRocketCounts", { 8, 11 } },
				{ "overdriveExpectedRocketDamage", { 32.f, 52.f } },
				{ "overdriveExpectedAttackSpeedBoostModifier", { 20.f, 28.f } },
				{ "overdriveExpectedAttackSpeedAfterSalvo", { 20.f, 86.f } },
				{ "overdriveExpectedAttackSpeedAfterEnd", { 0.f, 30.f } },
				{ "overdriveLevelFiveAttackSpeedBase", 15.f },
				{ "overdriveLevelFiveExternalAttackSpeedMultiplier", 2.f },
				{ "overdriveLevelFivePreexistingAttackSpeedBoost", 40.f },
				{ "overdriveLevels", { 1, 5 } }
			} },
			{ "outcome", {
				{ "executionDrive", {
					{ "attackPowerAfterEnd", executionDrive.attackPowerAfterEnd },
					{ "attackPowerWhileActive", executionDrive.attackPowerWhileActive },
					{ "movementMultiplierAfterEnd", executionDrive.movementMultiplierAfterEnd },
					{ "movementMultiplierWhileActive", executionDrive.movementMultiplierWhileActive }
				} },
				{ "mineLayer", {
					{ "cooldowns", mineLayer.cooldowns },
					{ "levelFifteenDamage", mineLayer.mineDamage },
					{ "mineCountInStunScenario", mineLayer.mineCount },
					{ "stunApplied", mineLayer.stunApplied },
					{ "stunDurationRefreshed", mineLayer.stunDurationRefreshed },
					{ "stunExpiredAfterRefresh", mineLayer.stunExpiredAfterRefresh }
				} },
				{ "overdriveLevelFive", {
					{ "attackSpeedAfterEnd", overdriveLevelFive.attackSpeedAfterEnd },
					{ "attackSpeedBeforeOverdriveBoost", overdriveLevelFive.attackSpeedBeforeBoost },
					{ "attackSpeedWithOverdriveBoost", overdriveLevelFive.attackSpeedWithBoost },
					{ "cooldownAfterEnd", overdriveLevelFive.cooldownAfterEnd },
					{ "rocketCount", overdriveLevelFive.rocketCount },
					{ "rocketDamage", overdriveLevelFive.rocketDamage }
				} },
				{ "overdriveLevelOne", {
					{ "attackSpeedAfterEnd", overdriveLevelOne.attackSpeedAfterEnd },
					{ "attackSpeedBeforeOverdriveBoost", overdriveLevelOne.attackSpeedBeforeBoost },
					{ "attackSpeedWithOverdriveBoost", overdriveLevelOne.attackSpeedWithBoost },
					{ "cooldownAfterEnd", overdriveLevelOne.cooldownAfterEnd },
					{ "rocketCount", overdriveLevelOne.rocketCount },
					{ "rocketDamage", overdriveLevelOne.rocketDamage }
				} }
			} },
			{ "errors", {
				{ "executionDrive", executionDrive.error },
				{ "mineLayer", mineLayer.error },
				{ "overdriveLevelFive", overdriveLevelFive.error },
				{ "overdriveLevelOne", overdriveLevelOne.error },
				{ "setup", setupError }
			} },
			{ "passed", passed }
		};
		if (!WriteArtifact(artifactPath, artifact))
		{
			std::cerr << "Could not write third ability balance E2E artifact.\n";
			return 1;
		}
		if (!passed)
		{
			std::cerr << artifact.dump(2) << '\n';
			return 1;
		}
		return 0;
	}
}

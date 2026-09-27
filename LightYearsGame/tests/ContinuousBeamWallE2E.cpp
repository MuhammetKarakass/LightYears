#include "attributes/AttributeMath.h"
#include "framework/AssetManager.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/weapon/visuals/ContinuousBeamVisualActor.h"
#include "framework/MathUtility.h"
#include "spaceShip/SpaceShip.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace ly
{
	int RunAbilityContentRegistrationE2E(const char* artifactPath, const std::string& setupError);
	int RunAbilityLoaderPublicLoadE2E(const char* artifactPath, const std::string& setupError);
	int RunTimerManagerSceneE2E(const char* artifactPath, const std::string& setupError);
	int RunGameHUDDamageEventE2E(const char* artifactPath, const std::string& setupError);
	int RunAuditFixesE2E(const char* artifactPath, const std::string& setupError);

	namespace
	{
		constexpr const char* ContinuousHeatLaserId = "Weapon.Beam.ContinuousHeatLaser.Basic";
		constexpr float ShipMaximumHealth = 250.f;
		constexpr float FireTickSeconds = 0.25f;
		constexpr int FireTickCount = 2;
		constexpr float HealthTolerance = 0.001f;
		const sf::Vector2f OwnerLocation{ 0.f, 0.f };
		const sf::Vector2f FrontTargetLocation{ 0.f, -200.f };
		const sf::Vector2f FirstWallLocation{ 0.f, -350.f };
		const sf::Vector2f BetweenWallsTargetLocation{ 0.f, -500.f };
		const sf::Vector2f SecondWallLocation{ 0.f, -650.f };
		const sf::Vector2f RearTargetLocation{ 0.f, -800.f };
		const sf::Vector2f WallHalfExtents{ 100.f, 10.f };

		ShipDefinition MakeE2EShipDefinition()
		{
			return ShipDefinition{ "", ShipMaximumHealth, {}, 0.f, 0, 0, {}, {} };
		}

		ShipDefinition MakeE2ETargetDefinition()
		{
			return ShipDefinition{
				"SpaceShooterRedux/PNG/Enemies/enemyRed5.png",
				ShipMaximumHealth,
				{},
				0.f,
				0,
				0,
				{},
				{}
			};
		}

		class BeamE2EShip final : public SpaceShip
		{
		public:
			explicit BeamE2EShip(World* world)
				: SpaceShip{ world, MakeE2EShipDefinition() }
			{
			}
		};

		class BeamE2ETarget final : public SpaceShip
		{
		public:
			explicit BeamE2ETarget(World* world)
				: SpaceShip{ world, MakeE2ETargetDefinition() }
			{
			}

			float GetPhysicsCollisionRadius() const override { return 14.f; }
			float GetHealth() const { return GetHealthComponent().GetHealth(); }
		};

		class StaticBeamE2EWall final : public Actor
		{
		public:
			explicit StaticBeamE2EWall(World* world)
				: Actor{ world }
			{
				SetPhysicsBodyType(PhysicsBodyType::Static);
				SetCollisionLayer(CollisionLayer::Environment);
				SetCollisionMask(CollisionLayer::None);
			}

			sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override
			{
				return WallHalfExtents;
			}
		};

		struct EndpointRuntimeSample
		{
			int tickIndex = -1;
			float deltaTime = 0.f;
			float elapsedSeconds = 0.f;
			sf::Vector2f ownerLocation{};
			float ownerRotationDegrees = 0.f;
			sf::Vector2f targetLocation{};
			sf::FloatRect targetBounds{};
			float targetHealth = 0.f;
			float targetHealthDamageSincePreviousSample = 0.f;
			std::size_t beamActorCount = 0;
			bool beamActorFound = false;
			sf::Vector2f beamActorLocation{};
			float beamActorRotationDegrees = 0.f;
			sf::Vector2f observedBeamDirection{};
			sf::Vector2f observedBeamEnd{};
		};

		struct ScenarioResult
		{
			bool completed = false;
			bool primaryFireActive = false;
			std::size_t beamActorCount = 0;
			float frontHealthBefore = 0.f;
			float frontHealthAfter = 0.f;
			float frontHealthAfterFirstTick = 0.f;
			float betweenWallsHealthBefore = 0.f;
			float betweenWallsHealthAfter = 0.f;
			float betweenWallsHealthAfterFirstTick = 0.f;
			float rearHealthBefore = 0.f;
			float rearHealthAfter = 0.f;
			float frontTargetNearBoundsY = 0.f;
			float firstWallNearSurfaceY = 0.f;
			bool tieFirstWallToTarget = false;
			bool movedBetweenWallsTargetAfterFirstTick = false;
			bool equalContactGeometryMatches = false;
			bool endpointGeometryMeasured = false;
			sf::Vector2f endpointOwnerLocationBeforeZeroTick{};
			float endpointOwnerRotationBeforeZeroTick = 0.f;
			sf::Vector2f endpointMuzzleOffset{};
			float endpointMuzzleRotationOffsetDegrees = 0.f;
			sf::Vector2f endpointMuzzleStart{};
			sf::Vector2f endpointDirection{};
			sf::Vector2f endpointBeamEnd{};
			sf::Vector2f endpointTargetLocationBefore{};
			sf::Vector2f endpointTargetLocationAfter{};
			sf::FloatRect endpointTargetBoundsBefore{};
			sf::FloatRect endpointTargetBounds{};
			float endpointDirectionRotationDegrees = 0.f;
			float endpointRange = 0.f;
			float endpointHalfWidth = 0.f;
			float endpointPlacementDeltaY = 0.f;
			float endpointTargetNearEdgeY = 0.f;
			float endpointExpandedNearEdgeGap = 0.f;
			std::vector<EndpointRuntimeSample> endpointRuntimeSamples;
			std::string error;
		};

		ScenarioResult RunScenario(
			const PrimaryWeaponDefinition& weapon,
			bool spawnWall,
			bool tieFirstWallToTarget = false,
			bool moveTargetToFrontAfterFirstTick = false,
			bool endpointContact = false
		)
		{
			ScenarioResult result;
			result.tieFirstWallToTarget = tieFirstWallToTarget;
			result.movedBetweenWallsTargetAfterFirstTick = moveTargetToFrontAfterFirstTick;
			World world{ nullptr };
			const shared_ptr<BeamE2EShip> owner = world.SpawnActor<BeamE2EShip>().lock();
			const shared_ptr<BeamE2ETarget> frontTarget = world.SpawnActor<BeamE2ETarget>().lock();
			const shared_ptr<BeamE2ETarget> betweenWallsTarget = world.SpawnActor<BeamE2ETarget>().lock();
			const shared_ptr<BeamE2ETarget> rearTarget = world.SpawnActor<BeamE2ETarget>().lock();
			shared_ptr<StaticBeamE2EWall> wall;
			shared_ptr<StaticBeamE2EWall> secondWall;
			if (spawnWall)
			{
				wall = world.SpawnActor<StaticBeamE2EWall>().lock();
				secondWall = world.SpawnActor<StaticBeamE2EWall>().lock();
			}
			if (!owner || !frontTarget || !betweenWallsTarget || !rearTarget ||
				(spawnWall && (!wall || !secondWall)))
			{
				result.error = "Failed to spawn one or more E2E actors";
				return result;
			}

			owner->SetActorLocation(OwnerLocation);
			owner->SetActorRotation(0.f);
			owner->SetCollisionLayer(CollisionLayer::Player);
			frontTarget->CenterPivot();
			frontTarget->SetActorLocation(FrontTargetLocation);
			frontTarget->SetCollisionLayer(CollisionLayer::Enemy);
			frontTarget->SetCollisionMask(CollisionLayer::PlayerBullet);
			betweenWallsTarget->CenterPivot();
			betweenWallsTarget->SetActorLocation(BetweenWallsTargetLocation);
			betweenWallsTarget->SetCollisionLayer(CollisionLayer::Enemy);
			betweenWallsTarget->SetCollisionMask(CollisionLayer::PlayerBullet);
			rearTarget->CenterPivot();
			rearTarget->SetActorLocation(RearTargetLocation);
			rearTarget->SetCollisionLayer(CollisionLayer::Enemy);
			rearTarget->SetCollisionMask(CollisionLayer::PlayerBullet);
			if (endpointContact)
			{
				const float range = sas::FindAttributeValue(weapon.attributes, PrimaryWeaponSchema::Beam::Delivery::Range, 0.f);
				const float halfWidth = sas::FindAttributeValue(weapon.attributes, PrimaryWeaponSchema::Beam::Delivery::Width, 0.f) * 0.5f;
				const auto muzzle = weapon.muzzleDefinitions.empty() ? WeaponMuzzleDefinition{} : weapon.muzzleDefinitions.front();
				const sf::Vector2f ownerLocationBeforeZeroTick = owner->GetActorLocation();
				const float ownerRotationBeforeZeroTick = owner->GetActorRotation();
				const sf::Vector2f muzzleStart = ownerLocationBeforeZeroTick + owner->TransformLocalToWorld(muzzle.offset);
				const float directionRotationDegrees = owner->GetActorRotation() + muzzle.rotationOffset - 90.f;
				const sf::Vector2f direction = RotationToVector(directionRotationDegrees);
				const sf::Vector2f beamEnd = muzzleStart + direction * range;
				const float endpointY = muzzleStart.y - range;
				const auto bounds = frontTarget->GetActorGlobalBounds();
				const sf::Vector2f targetLocationBefore = frontTarget->GetActorLocation();
				const float targetDeltaY = endpointY - halfWidth - (bounds.position.y + bounds.size.y);
				frontTarget->SetActorLocation(targetLocationBefore + sf::Vector2f{ 0.f, targetDeltaY });
				const sf::FloatRect endpointTargetBounds = frontTarget->GetActorGlobalBounds();
				const float targetNearEdgeY = endpointTargetBounds.position.y + endpointTargetBounds.size.y;
				result.endpointGeometryMeasured = true;
				result.endpointOwnerLocationBeforeZeroTick = ownerLocationBeforeZeroTick;
				result.endpointOwnerRotationBeforeZeroTick = ownerRotationBeforeZeroTick;
				result.endpointMuzzleOffset = muzzle.offset;
				result.endpointMuzzleRotationOffsetDegrees = muzzle.rotationOffset;
				result.endpointMuzzleStart = muzzleStart;
				result.endpointDirection = direction;
				result.endpointBeamEnd = beamEnd;
				result.endpointTargetLocationBefore = targetLocationBefore;
				result.endpointTargetLocationAfter = frontTarget->GetActorLocation();
				result.endpointTargetBoundsBefore = bounds;
				result.endpointTargetBounds = endpointTargetBounds;
				result.endpointDirectionRotationDegrees = directionRotationDegrees;
				result.endpointRange = range;
				result.endpointHalfWidth = halfWidth;
				result.endpointPlacementDeltaY = targetDeltaY;
				result.endpointTargetNearEdgeY = targetNearEdgeY;
				result.endpointExpandedNearEdgeGap = targetNearEdgeY + halfWidth - beamEnd.y;
			}
			if (wall)
			{
				const sf::FloatRect frontTargetBounds = frontTarget->GetActorGlobalBounds();
				result.frontTargetNearBoundsY = frontTargetBounds.position.y + frontTargetBounds.size.y;
				const sf::Vector2f firstWallLocation = tieFirstWallToTarget
					? sf::Vector2f{ FirstWallLocation.x, result.frontTargetNearBoundsY - WallHalfExtents.y }
					: FirstWallLocation;
				wall->SetActorLocation(firstWallLocation);
				result.firstWallNearSurfaceY = firstWallLocation.y + WallHalfExtents.y;
				result.equalContactGeometryMatches = tieFirstWallToTarget &&
					std::abs(result.firstWallNearSurfaceY - result.frontTargetNearBoundsY) <= HealthTolerance;
				secondWall->SetActorLocation(endpointContact ? sf::Vector2f{ 2000.f, 2000.f } : SecondWallLocation);
			}

			world.TickInternal(0.f);
			result.frontHealthBefore = frontTarget->GetHealth();
			result.betweenWallsHealthBefore = betweenWallsTarget->GetHealth();
			result.rearHealthBefore = rearTarget->GetHealth();
			const auto captureEndpointRuntimeSample = [&](int tickIndex, float deltaTime, float elapsedSeconds, float previousTargetHealth)
			{
				if (!endpointContact) return;
				EndpointRuntimeSample sample;
				sample.tickIndex = tickIndex;
				sample.deltaTime = deltaTime;
				sample.elapsedSeconds = elapsedSeconds;
				sample.ownerLocation = owner->GetActorLocation();
				sample.ownerRotationDegrees = owner->GetActorRotation();
				sample.targetLocation = frontTarget->GetActorLocation();
				sample.targetBounds = frontTarget->GetActorGlobalBounds();
				sample.targetHealth = frontTarget->GetHealth();
				sample.targetHealthDamageSincePreviousSample = previousTargetHealth - sample.targetHealth;
				const auto beamActors = world.GetActorsByType<ContinuousBeamVisualActor>();
				sample.beamActorCount = beamActors.size();
				for (const auto& weakBeam : beamActors)
				{
					const auto beam = weakBeam.lock();
					if (!beam) continue;
					sample.beamActorFound = true;
					sample.beamActorLocation = beam->GetActorLocation();
					sample.beamActorRotationDegrees = beam->GetActorRotation();
					sample.observedBeamDirection = RotationToVector(sample.beamActorRotationDegrees - 90.f);
					sample.observedBeamEnd = sample.beamActorLocation + sample.observedBeamDirection * result.endpointRange;
					break;
				}
				result.endpointRuntimeSamples.push_back(sample);
			};
			captureEndpointRuntimeSample(-1, 0.f, 0.f, result.frontHealthBefore);

			auto& abilitySystem = owner->GetCombatRuntime().GetAbilitySystemComponent();
			std::string grantFailure;
			const sas::AbilityHandle abilityHandle = abilitySystem.GrantAbility(
				AbilityData::MakePrimaryFireAbilityDefinition(weapon),
				sas::AbilitySlot::PrimaryFire,
				&grantFailure
			);
			if (!abilityHandle.IsValid())
			{
				result.error = "Failed to grant shipped primary fire ability: " + grantFailure;
				return result;
			}

			abilitySystem.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
			float previousFrontHealth = result.frontHealthBefore;
			for (int tick = 0; tick < FireTickCount; ++tick)
			{
				world.TickInternal(FireTickSeconds);
				captureEndpointRuntimeSample(tick, FireTickSeconds, (tick + 1) * FireTickSeconds, previousFrontHealth);
				previousFrontHealth = frontTarget->GetHealth();
				if (tick == 0)
				{
					result.frontHealthAfterFirstTick = frontTarget->GetHealth();
					result.betweenWallsHealthAfterFirstTick = betweenWallsTarget->GetHealth();
					if (moveTargetToFrontAfterFirstTick)
					{
						betweenWallsTarget->SetActorLocation(FrontTargetLocation);
					}
				}
			}

			GameAbility* ability = abilitySystem.GetAbility(abilityHandle);
			result.primaryFireActive = ability && ability->GetPrimaryWeaponRuntime().isFiring;
			result.beamActorCount = world.GetActorsByType<ContinuousBeamVisualActor>().size();
			result.frontHealthAfter = frontTarget->GetHealth();
			result.betweenWallsHealthAfter = betweenWallsTarget->GetHealth();
			result.rearHealthAfter = rearTarget->GetHealth();
			result.completed = true;
			return result;
		}

		nlohmann::json SerializeScenario(const ScenarioResult& result)
		{
			return {
				{ "beamActorCount", result.beamActorCount },
				{ "completed", result.completed },
				{ "betweenWallsTargetHealthAfter", result.betweenWallsHealthAfter },
				{ "betweenWallsTargetHealthBefore", result.betweenWallsHealthBefore },
				{ "betweenWallsTargetHealthAfterFirstTick", result.betweenWallsHealthAfterFirstTick },
				{ "betweenWallsTargetMovedToFrontAfterFirstTick", result.movedBetweenWallsTargetAfterFirstTick },
				{ "equalContactGeometryMatches", result.equalContactGeometryMatches },
				{ "error", result.error },
				{ "firstWallNearSurfaceY", result.firstWallNearSurfaceY },
				{ "frontTargetNearBoundsY", result.frontTargetNearBoundsY },
				{ "frontTargetHealthAfter", result.frontHealthAfter },
				{ "frontTargetHealthBefore", result.frontHealthBefore },
				{ "frontTargetHealthAfterFirstTick", result.frontHealthAfterFirstTick },
				{ "frontTargetDamageAfterFirstTick", result.frontHealthBefore - result.frontHealthAfterFirstTick },
				{ "frontTargetTotalDamage", result.frontHealthBefore - result.frontHealthAfter },
				{ "endpointGeometryMeasured", result.endpointGeometryMeasured },
				{ "endpointGeometry", result.endpointGeometryMeasured ? nlohmann::json{
					{ "ownerLocationBeforeZeroTick", { result.endpointOwnerLocationBeforeZeroTick.x, result.endpointOwnerLocationBeforeZeroTick.y } },
					{ "ownerRotationBeforeZeroTickDegrees", result.endpointOwnerRotationBeforeZeroTick },
					{ "muzzleOffset", { result.endpointMuzzleOffset.x, result.endpointMuzzleOffset.y } },
					{ "muzzleRotationOffsetDegrees", result.endpointMuzzleRotationOffsetDegrees },
					{ "muzzleStart", { result.endpointMuzzleStart.x, result.endpointMuzzleStart.y } },
					{ "directionRotationDegrees", result.endpointDirectionRotationDegrees },
					{ "direction", { result.endpointDirection.x, result.endpointDirection.y } },
					{ "range", result.endpointRange }, { "halfWidth", result.endpointHalfWidth },
					{ "beamEnd", { result.endpointBeamEnd.x, result.endpointBeamEnd.y } },
					{ "targetLocationBefore", { result.endpointTargetLocationBefore.x, result.endpointTargetLocationBefore.y } },
					{ "targetLocationAfter", { result.endpointTargetLocationAfter.x, result.endpointTargetLocationAfter.y } },
					{ "targetBoundsBefore", { { "position", { result.endpointTargetBoundsBefore.position.x, result.endpointTargetBoundsBefore.position.y } },
						{ "size", { result.endpointTargetBoundsBefore.size.x, result.endpointTargetBoundsBefore.size.y } } } },
					{ "targetBounds", { { "position", { result.endpointTargetBounds.position.x, result.endpointTargetBounds.position.y } },
						{ "size", { result.endpointTargetBounds.size.x, result.endpointTargetBounds.size.y } } } },
					{ "targetPlacementDeltaY", result.endpointPlacementDeltaY },
					{ "targetNearEdgeY", result.endpointTargetNearEdgeY },
					{ "expandedTargetNearEdgeMinusBeamEndY", result.endpointExpandedNearEdgeGap }
				} : nlohmann::json(nullptr) },
				{ "endpointRuntimeSamples", [&result]()
					{
						nlohmann::json samples = nlohmann::json::array();
						for (const EndpointRuntimeSample& sample : result.endpointRuntimeSamples)
						{
							samples.push_back({
								{ "tickIndex", sample.tickIndex }, { "deltaTime", sample.deltaTime }, { "elapsedSeconds", sample.elapsedSeconds },
								{ "ownerLocation", { sample.ownerLocation.x, sample.ownerLocation.y } },
								{ "ownerRotationDegrees", sample.ownerRotationDegrees },
								{ "targetLocation", { sample.targetLocation.x, sample.targetLocation.y } },
								{ "targetBounds", { { "position", { sample.targetBounds.position.x, sample.targetBounds.position.y } },
									{ "size", { sample.targetBounds.size.x, sample.targetBounds.size.y } } } },
								{ "targetHealth", sample.targetHealth },
								{ "targetHealthDamageSincePreviousSample", sample.targetHealthDamageSincePreviousSample },
								{ "beamActorCount", sample.beamActorCount }, { "beamActorFound", sample.beamActorFound },
								{ "beamActorLocation", { sample.beamActorLocation.x, sample.beamActorLocation.y } },
								{ "beamActorRotationDegrees", sample.beamActorRotationDegrees },
								{ "observedBeamDirection", { sample.observedBeamDirection.x, sample.observedBeamDirection.y } },
								{ "observedBeamEnd", { sample.observedBeamEnd.x, sample.observedBeamEnd.y } }
							});
						}
						return samples;
					}() },
				{ "primaryFireActive", result.primaryFireActive },
				{ "rearTargetHealthAfter", result.rearHealthAfter },
				{ "rearTargetHealthBefore", result.rearHealthBefore },
				{ "tieFirstWallToTarget", result.tieFirstWallToTarget }
			};
		}

		bool WriteArtifact(const std::filesystem::path& path, const nlohmann::json& artifact)
		{
			std::error_code error;
			if (!path.parent_path().empty())
			{
				std::filesystem::create_directories(path.parent_path(), error);
				if (error)
				{
					std::cerr << "Could not create E2E artifact directory: " << error.message() << '\n';
					return false;
				}
			}

			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output)
			{
				std::cerr << "Could not open E2E artifact path: " << path.string() << '\n';
				return false;
			}
			output << artifact.dump(2) << '\n';
			return output.good();
		}
	}

	int RunContinuousBeamWallE2E(const char* artifactPath, const std::string& setupError)
	{
		const PrimaryWeaponDefinition* weapon =
			content::WeaponContentCatalog::FindById(ContinuousHeatLaserId);
		ScenarioResult wallScenario;
		ScenarioResult openLaneScenario;
		ScenarioResult equalContactScenario;
		ScenarioResult movedTargetScenario;
		ScenarioResult endpointScenario;
		ScenarioResult endpointOpenScenario;
		if (!setupError.empty())
		{
			wallScenario.error = setupError;
			openLaneScenario.error = setupError;
			equalContactScenario.error = setupError;
			movedTargetScenario.error = setupError;
		}
		else if (weapon)
		{
			wallScenario = RunScenario(*weapon, true);
			openLaneScenario = RunScenario(*weapon, false);
			equalContactScenario = RunScenario(*weapon, true, true);
			movedTargetScenario = RunScenario(*weapon, true, false, true);
			endpointScenario = RunScenario(*weapon, true, true, false, true);
			endpointOpenScenario = RunScenario(*weapon, false, false, false, true);
		}
		else
		{
			wallScenario.error = "Shipped continuous heat laser was not loaded";
			openLaneScenario.error = wallScenario.error;
			equalContactScenario.error = wallScenario.error;
			movedTargetScenario.error = wallScenario.error;
		}

		const bool endpointBlocked = endpointScenario.completed && endpointScenario.primaryFireActive &&
			endpointScenario.equalContactGeometryMatches &&
			std::abs(endpointScenario.frontHealthAfter - endpointScenario.frontHealthBefore) <= HealthTolerance;
		const bool endpointOpenDamaged = endpointOpenScenario.completed && endpointOpenScenario.primaryFireActive &&
			endpointOpenScenario.frontHealthAfter < endpointOpenScenario.frontHealthBefore - HealthTolerance;
		const bool wallScenarioReady = wallScenario.completed && wallScenario.primaryFireActive &&
			wallScenario.beamActorCount == 1;
		const bool openLaneScenarioReady = openLaneScenario.completed && openLaneScenario.primaryFireActive &&
			openLaneScenario.beamActorCount == 1;
		const bool equalContactScenarioReady = equalContactScenario.completed && equalContactScenario.primaryFireActive &&
			equalContactScenario.beamActorCount == 1;
		const bool movedTargetScenarioReady = movedTargetScenario.completed && movedTargetScenario.primaryFireActive &&
			movedTargetScenario.beamActorCount == 1;
		const bool frontTargetDamaged = wallScenarioReady &&
			wallScenario.frontHealthAfter < wallScenario.frontHealthBefore - HealthTolerance;
		const bool betweenWallsTargetBlocked = wallScenarioReady &&
			std::abs(wallScenario.betweenWallsHealthAfter - wallScenario.betweenWallsHealthBefore) <= HealthTolerance;
		const bool rearTargetBlocked = wallScenarioReady &&
			std::abs(wallScenario.rearHealthAfter - wallScenario.rearHealthBefore) <= HealthTolerance;
		const bool betweenWallsTargetDamagedWithoutWall = openLaneScenarioReady &&
			openLaneScenario.betweenWallsHealthAfter < openLaneScenario.betweenWallsHealthBefore - HealthTolerance;
		const bool rearTargetDamagedWithoutWall = openLaneScenarioReady &&
			openLaneScenario.rearHealthAfter < openLaneScenario.rearHealthBefore - HealthTolerance;
		const bool frontTargetDamagedWithoutWall = openLaneScenarioReady &&
			openLaneScenario.frontHealthAfter < openLaneScenario.frontHealthBefore - HealthTolerance;
		const bool frontTargetBlockedAtEqualContact = equalContactScenarioReady &&
			equalContactScenario.equalContactGeometryMatches &&
			std::abs(equalContactScenario.frontHealthAfter - equalContactScenario.frontHealthBefore) <= HealthTolerance;
		const bool movedTargetBlockedBeforeMove = movedTargetScenarioReady &&
			std::abs(movedTargetScenario.betweenWallsHealthAfterFirstTick -
				movedTargetScenario.betweenWallsHealthBefore) <= HealthTolerance;
		const bool movedTargetDamagedAfterMove = movedTargetScenarioReady &&
			movedTargetScenario.betweenWallsHealthAfter < movedTargetScenario.betweenWallsHealthBefore - HealthTolerance;
		const bool passed = endpointBlocked && endpointOpenDamaged && frontTargetDamaged && betweenWallsTargetBlocked && rearTargetBlocked &&
			betweenWallsTargetDamagedWithoutWall && rearTargetDamagedWithoutWall && frontTargetDamagedWithoutWall &&
			frontTargetBlockedAtEqualContact && movedTargetBlockedBeforeMove && movedTargetDamagedAfterMove;

		const float beamRange = sas::FindAttributeValue(
			weapon ? weapon->attributes : List<sas::GameplayAttribute>{},
			PrimaryWeaponSchema::Beam::Delivery::Range,
			0.f
		);
		const float beamWidth = sas::FindAttributeValue(
			weapon ? weapon->attributes : List<sas::GameplayAttribute>{},
			PrimaryWeaponSchema::Beam::Delivery::Width,
			0.f
		);
		const float beamDamagePerSecond = sas::FindAttributeValue(
			weapon ? weapon->attributes : List<sas::GameplayAttribute>{},
			CommonAttributeIds::Damage,
			0.f
		);

		nlohmann::json artifact{
			{ "assertions", {
				{ "endpointWallWinsTie", endpointBlocked },
				{ "endpointTargetDamagedWithoutWall", endpointOpenDamaged },
				{ "frontTargetDamagedWithWall", frontTargetDamaged },
				{ "frontTargetDamagedWithoutWall", frontTargetDamagedWithoutWall },
				{ "frontTargetBlockedAtEqualFirstContact", frontTargetBlockedAtEqualContact },
				{ "movingTargetDamagedAfterCrossingInFrontOfWall", movedTargetDamagedAfterMove },
				{ "movingTargetBlockedBehindWallBeforeMove", movedTargetBlockedBeforeMove },
				{ "targetBetweenWallsBlockedByFirstWall", betweenWallsTargetBlocked },
				{ "rearTargetBlockedByFirstStaticWall", rearTargetBlocked },
				{ "targetBetweenWallsDamagedWithoutWall", betweenWallsTargetDamagedWithoutWall },
				{ "rearTargetDamagedWithoutWall", rearTargetDamagedWithoutWall }
			} },
			{ "input", {
				{ "beamDamagePerSecond", beamDamagePerSecond },
				{ "beamRange", beamRange },
				{ "beamWidth", beamWidth },
				{ "fireTickCount", FireTickCount },
				{ "fireTickSeconds", FireTickSeconds },
				{ "firstWallCenter", { FirstWallLocation.x, FirstWallLocation.y } },
				{ "frontTarget", { FrontTargetLocation.x, FrontTargetLocation.y } },
				{ "movingTargetFirstTick", { BetweenWallsTargetLocation.x, BetweenWallsTargetLocation.y } },
				{ "movingTargetSecondTick", { FrontTargetLocation.x, FrontTargetLocation.y } },
				{ "owner", { OwnerLocation.x, OwnerLocation.y } },
				{ "ownerRotationDegrees", 0.f },
				{ "primaryFireHeld", true },
				{ "rearTarget", { RearTargetLocation.x, RearTargetLocation.y } },
				{ "secondWallCenter", { SecondWallLocation.x, SecondWallLocation.y } },
				{ "targetBetweenWalls", { BetweenWallsTargetLocation.x, BetweenWallsTargetLocation.y } },
				{ "wallCollisionLayer", "Environment" },
				{ "wallCollisionMask", "None" },
				{ "wallHalfExtents", { WallHalfExtents.x, WallHalfExtents.y } }
			} },
			{ "equalContactScenario", SerializeScenario(equalContactScenario) },
			{ "endpointScenario", SerializeScenario(endpointScenario) },
			{ "endpointOpenScenario", SerializeScenario(endpointOpenScenario) },
			{ "movingTargetScenario", SerializeScenario(movedTargetScenario) },
			{ "openLaneControl", SerializeScenario(openLaneScenario) },
			{ "passed", passed },
			{ "schemaVersion", 1 },
			{ "scenario", "d1.continuous_beam_first_static_wall" },
			{ "wallScenario", SerializeScenario(wallScenario) },
			{ "weaponId", ContinuousHeatLaserId }
		};

		if (!WriteArtifact(artifactPath, artifact))
		{
			return 1;
		}
		std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
		if (!passed)
		{
			std::cerr << "Continuous beam wall E2E failed; see the artifact for scenario outcomes.\n";
			return 1;
		}
		std::cout << "Continuous beam wall E2E passed.\n";
		return 0;
	}
}

int main(int argc, char** argv)
{
	using namespace ly;
	const bool abilityContentRegistrationMode = argc == 3 &&
		std::string{ argv[1] } == "--ability-content-registration";
	const bool abilityLoaderPublicLoadMode = argc == 3 &&
		std::string{ argv[1] } == "--ability-loader-public-load";
	const bool timerManagerSceneMode = argc == 3 &&
		std::string{ argv[1] } == "--timer-manager-scene";
	const bool gameHUDDamageEventMode = argc == 3 &&
		std::string{ argv[1] } == "--game-hud-damage-event";
	const bool auditFixesMode = argc == 3 && std::string{ argv[1] } == "--audit-fixes";
	if (argc != 2 && !abilityContentRegistrationMode && !abilityLoaderPublicLoadMode &&
		!timerManagerSceneMode && !gameHUDDamageEventMode && !auditFixesMode)
	{
		std::cerr << "Usage: LightYearsContinuousBeamWallE2ETests <artifact-path> | "
			"--ability-content-registration <artifact-path> | "
			"--ability-loader-public-load <artifact-path> | "
			"--timer-manager-scene <artifact-path> | "
			"--game-hud-damage-event <artifact-path>\n";
		return 2;
	}

	AssetManager::GetAssetManager().SetAssetRootDirectory(
		(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
	);
	std::string setupError;
	if (!GameContentBootstrap::Register())
	{
		setupError = "GameContentBootstrap::Register failed";
	}
	if (abilityContentRegistrationMode)
	{
		return RunAbilityContentRegistrationE2E(argv[2], setupError);
	}
	if (auditFixesMode) return RunAuditFixesE2E(argv[2], setupError);
	if (abilityLoaderPublicLoadMode)
	{
		return RunAbilityLoaderPublicLoadE2E(argv[2], setupError);
	}
	if (timerManagerSceneMode)
	{
		return RunTimerManagerSceneE2E(argv[2], setupError);
	}
	if (gameHUDDamageEventMode)
	{
		return RunGameHUDDamageEventE2E(argv[2], setupError);
	}

	if (setupError.empty() && !content::WeaponContentCatalog::IsLoaded())
	{
		content::WeaponContentCatalog::LoadFromFile("LightYearsGame/assets/content/data/weapons.json");
		if (!content::WeaponContentCatalog::IsLoaded())
		{
			content::WeaponContentCatalog::LoadFromFile("assets/content/data/weapons.json");
		}
		if (!content::WeaponContentCatalog::IsLoaded())
		{
			setupError = "Shipped weapon content failed to load";
		}
	}

	return RunContinuousBeamWallE2E(argv[1], setupError);
}

#include "framework/Application.h"
#include "framework/AudioManager.h"
#include "framework/PhysicsSystem.h"
#include "framework/TimerManager.h"
#include "enemy/EnemyActor.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/module/ModuleDefinition.h"
#include "gameplay/enemy/EnemyBehaviorProfile.h"
#include "gameplay/enemy/EnemyCombatProfile.h"
#include "gameplay/enemy/EnemySpawnContext.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/damage/DamageContext.h"
#include "level/ArenaLevel.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "widget/GameHUD.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace ly
{
	struct GameHUDDamageE2ETestAccess
	{
		static int GetDamageNumberCountForShip(const GameHUD& hud, unsigned int shipId)
		{
			int count = 0;
			for (const auto& entry : hud.mDamageNumbers)
			{
				if (entry.shipId == shipId) ++count;
			}
			return count;
		}

		static std::string GetDamageTextForShip(const GameHUD& hud, unsigned int shipId)
		{
			for (const auto& entry : hud.mDamageNumbers)
			{
				if (entry.shipId != shipId) continue;
				const shared_ptr<TextWidget> widget = entry.widget.lock();
				if (widget) return widget->mText.getString().toAnsiString();
			}
			return {};
		}

		static bool IsShipObserved(const GameHUD& hud, unsigned int shipId)
		{
			return hud.mObservedDamageShips.find(shipId) != hud.mObservedDamageShips.end();
		}

		static bool HasWindowReference(const GameHUD& hud)
		{
			return hud.mWindowRef != nullptr;
		}
	};

	namespace
	{
		using Json = nlohmann::json;
		constexpr float SimulationDeltaSeconds = 1.f / 60.f;
		constexpr float NonlethalDamage = 25.f;
		constexpr float LethalDamage = 1000000.f;
		constexpr int MaximumRespawnFrames = 120;

		std::string& GetWindowTitle()
		{
			static std::string title{ "LightYears Game HUD Damage Event E2E" };
			return title;
		}

		class DamageEventApplication final : public Application
		{
		public:
			DamageEventApplication()
				: Application({ 320, 240 }, 32, GetWindowTitle(), sf::Style::None)
			{
				GetRenderWindow().setVisible(false);
			}
		};

		struct DamageProbe
		{
			Actor* expectedTarget{ nullptr };
			int resolvedCount{ 0 };
			float appliedDamage{ 0.f };
			bool targetMatched{ false };

			void OnDamageResolved(const DamageContext& context)
			{
				++resolvedCount;
				appliedDamage = context.appliedDamage;
				targetMatched = context.target == expectedTarget;
			}
		};

		struct DamageEventResult
		{
			int resolvedCount{ 0 };
			float appliedDamage{ 0.f };
			bool targetMatched{ false };
			std::string displayedText;
			int displayedNumberCount{ 0 };
		};

		DamageEventResult ApplyAndObserveDamage(
			const shared_ptr<SpaceShip>& ship,
			const shared_ptr<GameHUD>& hud,
			float damage
		)
		{
			DamageEventResult result;
			if (!ship || !hud) return result;

			DamageProbe probe{ ship.get() };
			const DelegateHandle handle = ship->GetCombatRuntime().onDamageResolved.BindAction(
				&probe,
				&DamageProbe::OnDamageResolved
			);
			ship->ApplyDamage(damage);
			ship->GetCombatRuntime().onDamageResolved.UnbindAction(handle);

			result.resolvedCount = probe.resolvedCount;
			result.appliedDamage = probe.appliedDamage;
			result.targetMatched = probe.targetMatched;
			result.displayedText = GameHUDDamageE2ETestAccess::GetDamageTextForShip(*hud, ship->GetUniqueID());
			result.displayedNumberCount = GameHUDDamageE2ETestAccess::GetDamageNumberCountForShip(
				*hud,
				ship->GetUniqueID()
			);
			return result;
		}

		void TickScene(ArenaLevel& level, float deltaTime)
		{
			level.TickInternal(deltaTime);
			TimerManager::GetGlobalTimerManager().UpdateTimer(deltaTime);
			if (!level.IsPaused()) TimerManager::GetGameTimerManager().UpdateTimer(deltaTime);
			PhysicsSystem::Get().Step(deltaTime);
			AudioManager::GetAudioManager().Update(deltaTime);
		}

		bool WriteArtifact(const std::filesystem::path& path, const Json& artifact)
		{
			std::error_code error;
			if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), error);
			if (error) return false;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) return false;
			output << artifact.dump(2) << '\n';
			return output.good();
		}

		Json SerializeDamage(const DamageEventResult& damage)
		{
			const std::string expectedText = std::to_string(
				static_cast<int>(std::round(damage.appliedDamage))
			);
			return {
				{ "appliedDamage", damage.appliedDamage },
				{ "displayedNumberCount", damage.displayedNumberCount },
				{ "displayedText", damage.displayedText },
				{ "expectedRoundedText", expectedText },
				{ "resolvedCount", damage.resolvedCount },
				{ "targetMatched", damage.targetMatched }
			};
		}
	}

	int RunGameHUDDamageEventE2E(const char* artifactPath, const std::string& setupError)
	{
		TimerManager::GetGlobalTimerManager().ClearAllTimers();
		TimerManager::GetGameTimerManager().ClearAllTimers();
		PlayerManager& playerManager = PlayerManager::GetPlayerManager();

		std::unique_ptr<DamageEventApplication> application;
		std::shared_ptr<ArenaLevel> level;
		std::shared_ptr<GameHUD> hud;
		std::shared_ptr<PlayerSpaceShip> initialShip;
		std::shared_ptr<PlayerSpaceShip> respawnedShip;
		DamageEventResult initialDamage;
		DamageEventResult respawnDamage;
		std::string runtimeError = setupError;
		bool levelStarted = false;
		bool hudInitialized = false;
		bool hudNativeInitialized = false;
		bool hudHasWindowReference = false;
		bool initialShipSpawned = false;
		bool initialShipObservedByHud = false;
		bool initialShipDestroyed = false;
		bool respawned = false;
		bool newShipObserved = false;
		int respawnFrames = 0;
		bool enemyReadoutOnShown = false;
		bool enemyReadoutOffHidden = false;
		bool enemyReadoutBackOnShown = false;
		bool playerDamageStillShownWhenOff = false;
		bool readoutsSurviveRespawn = false;

		if (runtimeError.empty())
		{
			try
			{
				application = std::make_unique<DamageEventApplication>();
				level = std::make_shared<ArenaLevel>(application.get());
				PhysicsSystem::Get().InitializeWorld({ 0.f, 0.f });
				level->BeginPlayInternal();
				levelStarted = true;

				Player* player = playerManager.GetPlayer();
				initialShip = player ? player->GetCurrentSpaceShip().lock() : nullptr;
				hud = level->GetGameHUD().lock();
				initialShipSpawned = static_cast<bool>(initialShip);
				if (!initialShip || !hud)
				{
					throw std::runtime_error("ArenaLevel did not create the player ship and GameHUD");
				}

				hud->NativeInit(application->GetRenderWindow());
				initialShip->BeginPortalTransit();
				level->TickInternal(SimulationDeltaSeconds);
				hudNativeInitialized = hud->HasInit();
				hudHasWindowReference = GameHUDDamageE2ETestAccess::HasWindowReference(*hud);
				initialShipObservedByHud = GameHUDDamageE2ETestAccess::IsShipObserved(
					*hud,
					initialShip->GetUniqueID()
				);
				hudInitialized = hudNativeInitialized && hudHasWindowReference && initialShipObservedByHud;
				if (!hudInitialized)
				{
					throw std::runtime_error(
						"GameHUD initialization or first ship observation failed after one scene tick"
					);
				}

				initialShip->SetInvulnerability(false);
				initialDamage = ApplyAndObserveDamage(initialShip, hud, NonlethalDamage);

				initialShip->SetInvulnerability(false);
				initialShip->ApplyDamage(LethalDamage);
				initialShipDestroyed = initialShip->GetIsPendingDestroy();

				for (int frame = 1; frame <= MaximumRespawnFrames; ++frame)
				{
					TickScene(*level, SimulationDeltaSeconds);
					Player* currentPlayer = playerManager.GetPlayer();
					respawnedShip = currentPlayer ? currentPlayer->GetCurrentSpaceShip().lock() : nullptr;
					if (respawnedShip && respawnedShip != initialShip && !respawnedShip->GetIsPendingDestroy())
					{
						respawned = true;
						respawnFrames = frame;
						break;
					}
				}

				if (!respawned)
				{
					throw std::runtime_error("PlayerRespawnSystem did not spawn a new ship within the frame limit");
				}

				respawnedShip->BeginPortalTransit();
				TickScene(*level, SimulationDeltaSeconds);
				newShipObserved = GameHUDDamageE2ETestAccess::IsShipObserved(
					*hud,
					respawnedShip->GetUniqueID()
				);
				respawnedShip->SetInvulnerability(false);
				respawnDamage = ApplyAndObserveDamage(respawnedShip, hud, NonlethalDamage);

				// DamageReadout module: damage dealt to non-player ships is gated by the player's module.
				const ShipDefinition* enemyDefinition = content::ShipContentCatalog::FindById("Ship.Enemy.ApproachGunner.Basic");
				if (!enemyDefinition) throw std::runtime_error("enemy ship definition missing");
				EnemyCombatProfile enemyProfile;
				enemyProfile.profileId = "EnemyCombat.HudTest.DamageReadout";
				enemyProfile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
				EnemyBehaviorProfile enemyBehavior{ "EnemyBehavior.PlayerShipSelection.ShieldGrowth", 1000.f, 0.2f, 300.f, 0.f, 0.f,
					EnemyMovementMode::Approach, 1.f, 1.f };
				const shared_ptr<EnemyActor> enemy = level->SpawnActor<EnemyActor>(
					*enemyDefinition, "Enemy.HudDamageReadout", enemyProfile, enemyBehavior, 1.5f, EnemySpawnContext{ 1, 0 }
				).lock();
				if (!enemy) throw std::runtime_error("enemy did not spawn");
				enemy->BeginPlay();
				TickScene(*level, SimulationDeltaSeconds);
				const unsigned int enemyId = enemy->GetUniqueID();
				Player* readoutPlayer = playerManager.GetPlayer();
				const DamageEventResult enemyOn = ApplyAndObserveDamage(enemy, hud, 1.f);
				enemyReadoutOnShown = enemyOn.displayedNumberCount == 1;

				const ModulePlacement* readoutModule = nullptr;
				for (const ModulePlacement& placement : readoutPlayer->GetModuleLoadout().GetPlacements())
				{
					const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(placement.moduleId);
					if (definition && definition->systemCategory == GameplayTags::Module::System::DamageReadout) readoutModule = &placement;
				}
				if (!readoutModule) throw std::runtime_error("DamageReadout module not installed by default");
				// Let the combat window lapse so the reinstall below is not held out by a reboot.
				for (int frame = 0; frame < 240; ++frame) TickScene(*level, SimulationDeltaSeconds);
				if (respawnedShip->GetIsPendingDestroy()) throw std::runtime_error("player ship died while waiting for combat to lapse");
				const ModulePlacement readoutPlacement = *readoutModule;
				if (!readoutPlayer->TryUninstallModule(readoutPlacement.instanceId)) throw std::runtime_error("DamageReadout uninstall failed");
				const int beforeOff = GameHUDDamageE2ETestAccess::GetDamageNumberCountForShip(*hud, enemyId);
				const DamageEventResult enemyOff = ApplyAndObserveDamage(enemy, hud, 1.f);
				enemyReadoutOffHidden = enemyOff.resolvedCount == 1 && enemyOff.displayedNumberCount == beforeOff;

				playerDamageStillShownWhenOff = ApplyAndObserveDamage(respawnedShip, hud, NonlethalDamage).displayedNumberCount >= 1;
				for (int frame = 0; frame < 240; ++frame) TickScene(*level, SimulationDeltaSeconds);
				if (respawnedShip->GetIsPendingDestroy()) throw std::runtime_error("player ship died while waiting for combat to lapse");
				if (readoutPlayer->TryInstallModule(readoutPlacement.instanceId, readoutPlacement.origin,
					readoutPlacement.rotationQuarterTurns) != ModulePlacementFailure::None)
					throw std::runtime_error("DamageReadout reinstall failed");
				const int beforeBack = GameHUDDamageE2ETestAccess::GetDamageNumberCountForShip(*hud, enemyId);
				const DamageEventResult enemyBack = ApplyAndObserveDamage(enemy, hud, 1.f);
				enemyReadoutBackOnShown = enemyBack.displayedNumberCount == beforeBack + 1;
				readoutsSurviveRespawn = respawnedShip->GetModuleApplier().IsReadoutEnabled(GameplayTags::Module::System::DamageReadout);
			}
			catch (const std::exception& exception)
			{
				runtimeError = exception.what();
			}
			catch (...)
			{
				runtimeError = "Unknown exception while running the real ArenaLevel HUD lifecycle";
			}
		}

		const std::string initialExpectedText = std::to_string(
			static_cast<int>(std::round(initialDamage.appliedDamage))
		);
		const std::string respawnExpectedText = std::to_string(
			static_cast<int>(std::round(respawnDamage.appliedDamage))
		);
		const bool initialDamageNumberCreated = initialDamage.resolvedCount == 1 &&
			initialDamage.targetMatched && initialDamage.displayedNumberCount == 1 &&
			initialDamage.displayedText == initialExpectedText;
		const bool respawnDamageNumberCreated = respawnDamage.resolvedCount == 1 &&
			respawnDamage.targetMatched && respawnDamage.displayedNumberCount == 1 &&
			respawnDamage.displayedText == respawnExpectedText;
		const bool passed = runtimeError.empty() && levelStarted && hudInitialized && initialShipSpawned &&
			initialDamageNumberCreated && initialShipDestroyed && respawned && newShipObserved &&
			respawnDamageNumberCreated && enemyReadoutOnShown && enemyReadoutOffHidden && enemyReadoutBackOnShown &&
			playerDamageStillShownWhenOff && readoutsSurviveRespawn;

		const Json artifact{
			{ "assertions", {
				{ "damageNumberMatchesResolvedDamage", initialDamageNumberCreated && respawnDamageNumberCreated },
				{ "initialPlayerShipObservedAndDamaged", initialDamageNumberCreated },
				{ "playerShipDeathTriggeredRespawn", initialShipDestroyed && respawned },
				{ "respawnedShipWasObservedAndDamaged", newShipObserved && respawnDamageNumberCreated },
				{ "damageReadoutGatesPlayerDealtNumbers", enemyReadoutOnShown && enemyReadoutOffHidden && enemyReadoutBackOnShown },
				{ "damageReadoutKeepsDamageTakenNumbers", playerDamageStillShownWhenOff },
				{ "damageReadoutReinstallReachesRespawnedShip", readoutsSurviveRespawn },
				{ "runtimeEventTargetsDamagedShip", initialDamage.targetMatched && respawnDamage.targetMatched },
				{ "runtimeDamageEventResolvedPositiveHits", initialDamage.resolvedCount == 1 && respawnDamage.resolvedCount == 1 }
			} },
			{ "input", {
				{ "frameDeltaSeconds", SimulationDeltaSeconds },
				{ "initialAndRespawnDamage", NonlethalDamage },
				{ "lethalDamage", LethalDamage },
				{ "maximumRespawnFrames", MaximumRespawnFrames },
				{ "playerMovementState", "portal transit during headless simulation" },
				{ "sceneClass", "ArenaLevel" },
				{ "syntheticActorsAdded", 0 },
				{ "syntheticTimersAdded", 0 }
			} },
			{ "initialShipDamage", SerializeDamage(initialDamage) },
			{ "outcome", {
				{ "gameHudInitialized", hudInitialized },
				{ "gameHudNativeInitialized", hudNativeInitialized },
				{ "gameHudHasWindowReference", hudHasWindowReference },
				{ "initialShipDestroyed", initialShipDestroyed },
				{ "initialShipSpawned", initialShipSpawned },
				{ "initialShipObservedByHud", initialShipObservedByHud },
				{ "newShipObservedByHud", newShipObserved },
				{ "passed", passed },
				{ "respawnFrames", respawnFrames },
				{ "respawned", respawned },
				{ "runtimeError", runtimeError }
			} },
			{ "respawnedShipDamage", SerializeDamage(respawnDamage) },
			{ "scenario", "d2.6.ship_damage_runtime_event_to_hud_lifecycle" },
			{ "schemaVersion", 1 }
		};

		hud.reset();
		level.reset();
		TimerManager::GetGlobalTimerManager().ClearAllTimers();
		TimerManager::GetGameTimerManager().ClearAllTimers();
		playerManager.Reset();
		application.reset();

		if (!WriteArtifact(artifactPath, artifact)) return 1;
		std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
		if (!passed)
		{
			std::cerr << "GameHUD damage event E2E failed; see the artifact for lifecycle observations.\n";
			return 1;
		}
		std::cout << "GameHUD damage event lifecycle E2E passed.\n";
		return 0;
	}
}

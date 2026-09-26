#include "framework/Application.h"
#include "framework/AudioManager.h"
#include "framework/PhysicsSystem.h"
#include "framework/TimerManager.h"
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
			const shared_ptr<PlayerSpaceShip>& ship,
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
			respawnDamageNumberCreated;

		const Json artifact{
			{ "assertions", {
				{ "damageNumberMatchesResolvedDamage", initialDamageNumberCreated && respawnDamageNumberCreated },
				{ "initialPlayerShipObservedAndDamaged", initialDamageNumberCreated },
				{ "playerShipDeathTriggeredRespawn", initialShipDestroyed && respawned },
				{ "respawnedShipWasObservedAndDamaged", newShipObserved && respawnDamageNumberCreated },
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

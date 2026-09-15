#include "level/ArenaTestLevel.h"
#include "enemy/EnemyActor.h"
#include "framework/AudioManager.h"
#include "framework/Actor.h"
#include "enemy/DummyEnemy.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/encounter/EncounterHUDController.h"
#include "widget/GameHUD.h"

#include <array>

namespace ly
{
	ArenaTestLevel::ArenaTestLevel(Application* owningApp)
		:ArenaLevel(owningApp)
	{
	}

	ArenaTestLevel::~ArenaTestLevel()
	{
		ResetEncounter();
	}

	ArenaDefinition ArenaTestLevel::CreateArenaDefinition() const
	{
		ArenaDefinition arenaDefinition;
		arenaDefinition.size = sf::Vector2f{ 6000.f, 3000.f };
		arenaDefinition.legalBounds = sf::FloatRect{ sf::Vector2f{ 0.f, 0.f }, arenaDefinition.size };
		arenaDefinition.outOfBoundsMargin = 20.f;
		arenaDefinition.outOfBoundsTime = 5.f;

		arenaDefinition.boundaryVisual.enabled = true;
		arenaDefinition.boundaryVisual.visualType = ArenaBoundaryVisualType::DebugRectangle;
		arenaDefinition.boundaryVisual.outlineColor = sf::Color{ 90, 230, 255, 180 };
		arenaDefinition.boundaryVisual.warningColor = sf::Color{ 255, 80, 80, 220 };
		arenaDefinition.boundaryVisual.outlineThickness = 4.f;

		return arenaDefinition;
	}

	PlayerRespawnDefinition ArenaTestLevel::CreatePlayerRespawnDefinition() const
	{
		PlayerRespawnDefinition respawnDefinition = ArenaLevel::CreatePlayerRespawnDefinition();
		respawnDefinition.respawnDelay = 1.f;
		respawnDefinition.playerIndex = 0;
		respawnDefinition.createPlayerIfMissing = true;
		respawnDefinition.useScreenClamp = false;
		respawnDefinition.respawnWhenDestroyed = true;
		respawnDefinition.gameOverWhenRespawnFails = true;
		respawnDefinition.playerShipId = "Default";

		return respawnDefinition;
	}

	void ArenaTestLevel::CreateHUDControllers()
	{
		ArenaLevel::CreateHUDControllers();

		// The controller outlives the level, so it observes the level through a weak handle instead of raw 'this'.
		const weak_ptr<ArenaTestLevel> weakLevel = std::dynamic_pointer_cast<ArenaTestLevel>(GetWeakPtr().lock());
		AddHUDController(std::make_shared<EncounterHUDController>(GetGameHUD(),
			[weakLevel]()
			{
				if (const shared_ptr<ArenaTestLevel> level = weakLevel.lock()) return level->GetEncounterWaveSnapshot();
				return EncounterWaveSnapshot{};
			}));
	}

	void ArenaTestLevel::OnGameStart()
	{
		ArenaLevel::OnGameStart();
		SpawnDummyTargets();

		AudioManager::GetAudioManager().FadeToMusic("SpaceShooterRedux/Musics/cosmic_reverie.ogg",
			AudioType::Music,
			5.0f,
			1.0f,
			true,
			1.0f);

		StartEncounter();
	}

	void ArenaTestLevel::SpawnDummyTargets()
	{
		const ShipDefinition* dummySource = content::ShipContentCatalog::FindById(
			"Ship.Enemy.RangeKeeper.Basic"
		);
		if (!dummySource)
		{
			LY_GAME_ERROR("Arena test dummy ship definition is unavailable.");
			return;
		}
		ShipDefinition oldDummyDefinition = *dummySource;
		oldDummyDefinition.health = 99999.f;
		oldDummyDefinition.speed = { 0.f, 0.f };
		oldDummyDefinition.primaryWeaponId = "Weapon.Projectile.FighterRapidLaser.Basic";

		ShipDefinition newDummyDefinition = oldDummyDefinition;
		newDummyDefinition.health = 50.f;

		const sf::FloatRect& arenaBounds = GetArenaDefinition().legalBounds;
		const sf::Vector2f arenaCenter{
			arenaBounds.position.x + arenaBounds.size.x * 0.5f,
			arenaBounds.position.y + arenaBounds.size.y * 0.5f
		};

		const float squareHalfExtent = 250.f;
		const auto spawnDummy = [&](const ShipDefinition& definition, float horizontalOffset, float verticalOffset)
		{
			const sf::Vector2f dummyLocation{
				arenaCenter.x + horizontalOffset,
				arenaCenter.y + verticalOffset
			};
			if (auto dummy = SpawnActor<DummyEnemy>(definition).lock())
			{
				dummy->SetActorLocation(dummyLocation);
				dummy->SetVelocity({ 0.f, 0.f });
				dummy->SetActorRotation(0.f);
			}
		};

		// Existing stationary targets remain in their original horizontal layout.
		spawnDummy(oldDummyDefinition, -900.f, 450.f);
		spawnDummy(oldDummyDefinition, -300.f, 450.f);
		spawnDummy(oldDummyDefinition, 300.f, 450.f);
		spawnDummy(oldDummyDefinition, 900.f, 450.f);

		// New 50-health stationary targets at the corners of a square around arena center.
		spawnDummy(newDummyDefinition, -squareHalfExtent, -squareHalfExtent);
		spawnDummy(newDummyDefinition, squareHalfExtent, -squareHalfExtent);
		spawnDummy(newDummyDefinition, -squareHalfExtent, squareHalfExtent);
		spawnDummy(newDummyDefinition, squareHalfExtent, squareHalfExtent);
	}

	void ArenaTestLevel::Tick(float deltaTime)
	{
		ArenaLevel::Tick(deltaTime);
		if (mEncounterState != ArenaEncounterState::Running)
		{
			return;
		}

		if (!BindEncounterPlayerDeath())
		{
			FailEncounter("The encounter no longer has an active player ship.");
			return;
		}
		mEncounterWaveRuntime.Tick(deltaTime);
		if (mEncounterWaveRuntime.HasFailed())
		{
			FailEncounter(mEncounterWaveRuntime.GetFailureReason());
			return;
		}

		if (mEncounterWaveRuntime.IsCompleted())
		{
			mEncounterState = ArenaEncounterState::Completed;
			UnbindEncounterPlayerDeath();
		}
	}

	void ArenaTestLevel::OnRestartLevel()
	{
		ResetEncounter();
		ArenaLevel::OnRestartLevel();
		RestartEncounter();
	}

	bool ArenaTestLevel::StartEncounter()
	{
		ResetEncounter();

		const List<EnemyWaveDefinition> definitions{
			{
				{ { sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 2, 0.5f } },
				1.5f
			},
			{
				{
					{ sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.4f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.f }
				},
				1.5f
			},
			{
				{
					{ sas::ContentId{ EnemyIds::ApproachGunnerBasic }, 1, 0.3f },
					{ sas::ContentId{ EnemyIds::StrafeSkirmisherBasic }, 1, 0.3f },
					{ sas::ContentId{ EnemyIds::RangeKeeperBasic }, 1, 0.3f }
				},
				1.5f
			}
		};

		mEncounterFailureReported = false;
		const EncounterProgression progression{ 2, 2, 6, 15, 1337u };
		std::string failureReason;
		if (!mEncounterWaveRuntime.Start(definitions,
			[this](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
			{
				return SpawnEncounterEnemy(enemyId, spawnIndex, context);
			},
			progression,
			&failureReason))
		{
			LY_GAME_ERROR("Arena encounter could not start: %s", failureReason.c_str());
			mEncounterFailureReported = true;
			mEncounterState = ArenaEncounterState::Failed;
			return false;
		}

		mEncounterState = ArenaEncounterState::Running;
		if (!BindEncounterPlayerDeath())
		{
			FailEncounter("No active player ship is available for the encounter.");
			return false;
		}
		return true;
	}

	bool ArenaTestLevel::RestartEncounter()
	{
		return StartEncounter();
	}

	weak_ptr<PlayerSpaceShip> ArenaTestLevel::ResolveEncounterPlayerShip() const
	{
		const Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		return player ? player->GetCurrentSpaceShip() : weak_ptr<PlayerSpaceShip>{};
	}

	bool ArenaTestLevel::BindEncounterPlayerDeath()
	{
		if (mEncounterPlayerDestroyedDelegateHandle.IsValid())
		{
			return true;
		}

		const shared_ptr<PlayerSpaceShip> playerShip = ResolveEncounterPlayerShip().lock();
		if (!playerShip || playerShip->GetIsPendingDestroy())
		{
			return false;
		}

		mEncounterPlayer = playerShip;
		mEncounterPlayerDestroyedDelegateHandle = playerShip->onActorDestroyed.BindAction(
			GetWeakPtr(), &ArenaTestLevel::HandleEncounterPlayerDestroyed
		);
		return mEncounterPlayerDestroyedDelegateHandle.IsValid();
	}

	void ArenaTestLevel::UnbindEncounterPlayerDeath()
	{
		if (const shared_ptr<Actor> player = mEncounterPlayer.lock(); player && mEncounterPlayerDestroyedDelegateHandle.IsValid())
		{
			player->onActorDestroyed.UnbindAction(mEncounterPlayerDestroyedDelegateHandle);
		}
		mEncounterPlayerDestroyedDelegateHandle.Reset();
		mEncounterPlayer.reset();
	}

	void ArenaTestLevel::HandleEncounterPlayerDestroyed(Actor* destroyedActor)
	{
		const shared_ptr<Actor> player = mEncounterPlayer.lock();
		if (mEncounterState != ArenaEncounterState::Running || !player || player.get() != destroyedActor)
		{
			return;
		}

		FailEncounter("The player was destroyed.");
	}

	void ArenaTestLevel::ResetEncounter()
	{
		UnbindEncounterPlayerDeath();
		DestroyEncounterEnemies();
		mEncounterWaveRuntime.Reset();
		mEncounterState = ArenaEncounterState::Idle;
		mEncounterFailureReported = false;
	}

	void ArenaTestLevel::FailEncounter(const std::string& failureReason)
	{
		if (mEncounterState != ArenaEncounterState::Running)
		{
			return;
		}

		UnbindEncounterPlayerDeath();
		DestroyEncounterEnemies();
		mEncounterWaveRuntime.Reset();
		mEncounterState = ArenaEncounterState::Failed;
		if (!mEncounterFailureReported)
		{
			LY_GAME_ERROR("Arena encounter failed: %s", failureReason.c_str());
			mEncounterFailureReported = true;
		}
	}

	void ArenaTestLevel::DestroyEncounterEnemies()
	{
		for (const weak_ptr<EnemyActor>& enemy : mEncounterWaveRuntime.GetOwnedEnemies())
		{
			if (const shared_ptr<EnemyActor> actor = enemy.lock()) actor->Destroy();
		}
	}

	weak_ptr<EnemyActor> ArenaTestLevel::SpawnEncounterEnemy(
		const sas::ContentId& enemyId,
		size_t spawnIndex,
		const EnemySpawnContext& context)
	{
		const sf::FloatRect& bounds = GetArenaDefinition().legalBounds;
		const std::array<sf::Vector2f, 4> spawnPoints{
			sf::Vector2f{ bounds.position.x + 900.f, bounds.position.y + 600.f },
			sf::Vector2f{ bounds.position.x + bounds.size.x - 900.f, bounds.position.y + 600.f },
			sf::Vector2f{ bounds.position.x + 900.f, bounds.position.y + bounds.size.y - 600.f },
			sf::Vector2f{ bounds.position.x + bounds.size.x - 900.f, bounds.position.y + bounds.size.y - 600.f }
		};
		return content::SpawnEnemy(*this, enemyId.ToString(), spawnPoints[spawnIndex % spawnPoints.size()], 1.f, context);
	}
}



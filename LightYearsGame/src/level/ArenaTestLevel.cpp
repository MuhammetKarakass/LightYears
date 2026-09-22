#include "level/ArenaTestLevel.h"
#include "enemy/EnemyActor.h"
#include "framework/AudioManager.h"
#include "framework/Actor.h"
#include "enemy/DummyEnemy.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/enemy/EnemyIds.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/encounter/EncounterHUDController.h"
#include "widget/GameHUD.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace ly
{
	namespace
	{
		constexpr int ArenaMaximumConcurrentEnemies = 24;
		constexpr float EncounterSpawnOffsetSpacing = 50.f;

		sf::Vector2f ResolveEncounterSpawnOffset(size_t localSpawnIndex)
		{
			if (localSpawnIndex == 0) return {};
			const int ring = static_cast<int>(std::ceil((std::sqrt(static_cast<double>(localSpawnIndex) + 1.0) - 1.0) * 0.5));
			const int legLength = ring * 2;
			const int maximumValue = (2 * ring + 1) * (2 * ring + 1) - 1;
			const int distance = maximumValue - static_cast<int>(localSpawnIndex);
			const int side = distance / legLength;
			const int offset = distance % legLength;
			int x = 0;
			int y = 0;
			switch (side)
			{
			case 0: x = ring - offset; y = ring; break;
			case 1: x = -ring; y = ring - offset; break;
			case 2: x = -ring + offset; y = -ring; break;
			default: x = ring; y = -ring + offset; break;
			}
			return sf::Vector2f{ static_cast<float>(x) * EncounterSpawnOffsetSpacing, static_cast<float>(y) * EncounterSpawnOffsetSpacing };
		}

		// Presentation/debug wording only; the resolved DamageContext is never
		// reinterpreted by telemetry.
		std::string PlaytestDeliveryTypeName(DamageDeliveryType deliveryType)
		{
			switch (deliveryType)
			{
			case DamageDeliveryType::Direct: return "Direct";
			case DamageDeliveryType::Projectile: return "Projectile";
			case DamageDeliveryType::Beam: return "Beam";
			case DamageDeliveryType::Area: return "Area";
			case DamageDeliveryType::Contact: return "Contact";
			}
			return "Unknown";
		}

		std::string PlaytestDamageTagList(const List<GameplayTag>& damageTags)
		{
			std::string joined;
			for (const GameplayTag& tag : damageTags)
			{
				if (!joined.empty())
				{
					joined += "+";
				}
				joined += tag.name;
			}
			return joined;
		}
	}
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
		// Arena dummy targets are retained for future use but are not spawned.
		// Flip this flag to re-enable the stationary target layout below.
		constexpr bool SpawnDummyTargetsEnabled = false;
		if (!SpawnDummyTargetsEnabled)
		{
			return;
		}

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
			FinalizePlaytestTerminalState();
			return;
		}

		if (!BindEncounterPlayerDeath())
		{
			FailEncounter("The encounter no longer has an active player ship.");
			EmitPlaytestSummaryIfPending();
			return;
		}
		EnsurePlaytestPlayerDamageObservation();
		mEncounterWaveRuntime.Tick(deltaTime);
		SamplePlaytestMeasurements(deltaTime);
		if (mEncounterWaveRuntime.HasFailed())
		{
			FailEncounter(mEncounterWaveRuntime.GetFailureReason());
			EmitPlaytestSummaryIfPending();
			return;
		}

		if (mEncounterWaveRuntime.IsCompleted())
		{
			// The arena runs an endless sequence, so exhausting the wave templates is never a
			// legitimate outcome here. Treat it as a runtime contract violation instead of
			// reporting a successful arena completion.
			FailEncounter("The endless encounter unexpectedly exhausted its wave templates.");
			EmitPlaytestSummaryIfPending();
			return;
		}
		EmitPlaytestSummaryIfPending();
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
		// The arena is an endless run: the three definitions are cyclic composition
		// templates, the level rises every three waves, and the planned total follows the
		// absolute wave number. The first two fields are finite-mode legacy and unused here.
		const EncounterProgression progression{ 1, 3, 0, 15, 1337u, ArenaMaximumConcurrentEnemies };
		std::string failureReason;
		if (!mEncounterWaveRuntime.Start(definitions,
			[this](const sas::ContentId& enemyId, size_t spawnIndex, const EnemySpawnContext& context)
			{
				return SpawnEncounterEnemy(enemyId, spawnIndex, context);
			},
			progression,
			EncounterSequenceMode::EndlessCycle,
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
		mPlaytestMetrics.active = true;
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

		if (mPlaytestMetrics.active)
		{
			++mPlaytestMetrics.playerDeathCount;
			mPlaytestMetrics.playerDeathWave = mEncounterWaveRuntime.BuildSnapshot().currentWaveNumber;
			mPlaytestMetrics.playerDeathTime = mPlaytestMetrics.duration;
		}
		FailEncounter("The player was destroyed.");
	}

	void ArenaTestLevel::ResetEncounter()
	{
		EmitPlaytestSummaryIfPending();
		UnbindEncounterPlayerDeath();
		DestroyEncounterEnemies();
		mEncounterWaveRuntime.Reset();
		mEncounterState = ArenaEncounterState::Idle;
		mEncounterFailureReported = false;
		ResetPlaytestMetrics();
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
		if (mPlaytestMetrics.active)
		{
			mPlaytestMetrics.failed = true;
			mPlaytestSummaryPending = true;
		}
		if (!mEncounterFailureReported)
		{
			LY_GAME_ERROR("Arena encounter failed: %s", failureReason.c_str());
			mEncounterFailureReported = true;
		}
	}

	void ArenaTestLevel::DestroyEncounterEnemies()
	{
		// Enemies removed by encounter teardown are not kills; drop their live
		// telemetry records before they are destroyed.
		ClearPlaytestLiveEnemies();
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
		// Encounter-owned enemies are owned by the encounter lifecycle, so camera
		// visibility must not remove them.
		EnemySpawnContext encounterContext = context;
		encounterContext.windowCullEnabled = false;
		const size_t spawnPointIndex = spawnIndex % spawnPoints.size();
		const size_t localSpawnIndex = spawnIndex / spawnPoints.size();
		const sf::Vector2f spawnLocation = spawnPoints[spawnPointIndex] + ResolveEncounterSpawnOffset(localSpawnIndex);
		const weak_ptr<EnemyActor> spawned = content::SpawnEnemy(
			*this,
			enemyId.ToString(),
			spawnLocation,
			1.f,
			encounterContext
		);
		if (!spawned.expired() && mPlaytestMetrics.active)
		{
			++mPlaytestMetrics.totalSpawned;
			// The wave record is opened here when a wave spawns before its first
			// measurement sample, so spawns always attribute to their own wave.
			ArenaPlaytestWaveMetrics& wave = EnsurePlaytestWave(
				mEncounterWaveRuntime.BuildSnapshot().currentWaveNumber,
				mPlaytestMetrics.duration
			);
			++wave.spawned;
			ArenaPlaytestLiveEnemy record;
			record.enemy = spawned;
			record.enemyId = enemyId.ToString();
			record.spawnTime = mPlaytestMetrics.duration;
			record.waveNumber = mEncounterWaveRuntime.BuildSnapshot().currentWaveNumber;
			mPlaytestLiveEnemies.push_back(std::move(record));
		}
		return spawned;
	}

	void ArenaTestLevel::OnArenaBoundaryPenaltyTriggered(weak_ptr<Actor> trackedActor)
	{
		if (mPlaytestMetrics.active)
		{
			++mPlaytestMetrics.boundaryPenaltyCount;
		}
		ArenaLevel::OnArenaBoundaryPenaltyTriggered(trackedActor);
	}

	void ArenaTestLevel::ResetPlaytestMetrics()
	{
		UnbindPlaytestPlayerDamageObservation();
		ClearPlaytestLiveEnemies();
		mPlaytestMetrics = ArenaPlaytestMetrics{};
		mPlaytestWasReloading = false;
		mPlaytestSummaryPending = false;
		mPlaytestTerminalFinalizeFrames = 0;
	}

	void ArenaTestLevel::ClearPlaytestLiveEnemies()
	{
		mPlaytestLiveEnemies.clear();
	}

	ArenaPlaytestWaveMetrics& ArenaTestLevel::EnsurePlaytestWave(size_t waveNumber, double startTime)
	{
		if (ArenaPlaytestWaveMetrics* existing = FindPlaytestWave(waveNumber))
		{
			return *existing;
		}
		ArenaPlaytestWaveMetrics opened;
		opened.waveNumber = waveNumber;
		opened.startTime = startTime;
		opened.started = true;
		mPlaytestMetrics.waves.push_back(std::move(opened));
		return mPlaytestMetrics.waves.back();
	}

	ArenaPlaytestWaveMetrics* ArenaTestLevel::FindPlaytestWave(size_t waveNumber)
	{
		for (ArenaPlaytestWaveMetrics& wave : mPlaytestMetrics.waves)
		{
			if (wave.waveNumber == waveNumber)
			{
				return &wave;
			}
		}
		return nullptr;
	}

	ArenaPlaytestEnemyTypeMetrics& ArenaTestLevel::GetOrCreatePlaytestEnemyType(const std::string& enemyId)
	{
		for (ArenaPlaytestEnemyTypeMetrics& type : mPlaytestMetrics.enemyTypes)
		{
			if (type.enemyId == enemyId)
			{
				return type;
			}
		}
		ArenaPlaytestEnemyTypeMetrics type;
		type.enemyId = enemyId;
		mPlaytestMetrics.enemyTypes.push_back(std::move(type));
		return mPlaytestMetrics.enemyTypes.back();
	}

	void ArenaTestLevel::SamplePlaytestMeasurements(double deltaTime)
	{
		if (!mPlaytestMetrics.active)
		{
			return;
		}

		const double frameStartTime = mPlaytestMetrics.duration;
		mPlaytestMetrics.duration += static_cast<double>(deltaTime);
		const EncounterWaveSnapshot snapshot = mEncounterWaveRuntime.BuildSnapshot();
		SamplePlaytestWave(snapshot, deltaTime, frameStartTime);
		mPlaytestMetrics.aliveIntegral +=
			static_cast<double>(snapshot.aliveEnemyCount) * static_cast<double>(deltaTime);
		mPlaytestMetrics.aliveSampledDuration += static_cast<double>(deltaTime);
		mPlaytestMetrics.peakAlive = std::max(mPlaytestMetrics.peakAlive, snapshot.aliveEnemyCount);
		SamplePlaytestReload(deltaTime);
		UpdatePlaytestEnemyRecords();
	}

	void ArenaTestLevel::SamplePlaytestWave(
		const EncounterWaveSnapshot& snapshot,
		double deltaTime,
		double frameStartTime)
	{
		// A wave is opened exactly once, keyed on the canonical wave number. The
		// runtime may enter and leave Spawning inside a single Tick, so a wave is
		// never opened by requiring a transient state to be observed; it is opened
		// when a wave number first becomes observable during the active run.
		ArenaPlaytestWaveMetrics* wave = nullptr;
		if (snapshot.currentWaveNumber > 0)
		{
			wave = &EnsurePlaytestWave(snapshot.currentWaveNumber, frameStartTime);
		}
		if (!wave)
		{
			return;
		}

		// A wave ends exactly once: when it leaves the clear gate into the
		// inter-wave delay, or when the encounter reaches a terminal state.
		if (!wave->ended &&
			(snapshot.state == EncounterWaveState::InterWaveDelay ||
				snapshot.state == EncounterWaveState::Completed ||
				snapshot.state == EncounterWaveState::Failed))
		{
			wave->ended = true;
			wave->endTime = mPlaytestMetrics.duration;
			wave->duration = std::max(0.0, wave->endTime - wave->startTime);
			return;
		}
		if (wave->ended)
		{
			return;
		}

		wave->aliveIntegral +=
			static_cast<double>(snapshot.aliveEnemyCount) * static_cast<double>(deltaTime);
		wave->aliveSampledDuration += static_cast<double>(deltaTime);
		wave->peakAlive = std::max(wave->peakAlive, snapshot.aliveEnemyCount);
	}

	void ArenaTestLevel::SamplePlaytestReload(double deltaTime)
	{
		const shared_ptr<PlayerSpaceShip> player =
			std::dynamic_pointer_cast<PlayerSpaceShip>(mEncounterPlayer.lock());
		GameAbility* primary = player
			? player->GetCombatRuntime().GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire)
			: nullptr;
		const bool reloading = primary &&
			primary->GetPrimaryWeaponRuntime().magazineState.reloadRemaining > 0.0;
		if (reloading && !mPlaytestWasReloading)
		{
			++mPlaytestMetrics.reloadCount;
		}
		if (reloading)
		{
			mPlaytestMetrics.totalReloadTime += static_cast<double>(deltaTime);
		}
		mPlaytestWasReloading = reloading;
	}

	void ArenaTestLevel::UpdatePlaytestEnemyRecords()
	{
		for (size_t index = 0; index < mPlaytestLiveEnemies.size();)
		{
			const ArenaPlaytestLiveEnemy& record = mPlaytestLiveEnemies[index];
			if (!record.enemy.expired())
			{
				++index;
				continue;
			}

			const double ttk = std::max(0.0, mPlaytestMetrics.duration - record.spawnTime);
			++mPlaytestMetrics.totalKilled;
			if (ArenaPlaytestWaveMetrics* wave = FindPlaytestWave(record.waveNumber))
			{
				++wave->killed;
			}
			ArenaPlaytestEnemyTypeMetrics& type = GetOrCreatePlaytestEnemyType(record.enemyId);
			++type.kills;
			++type.ttkSamples;
			type.totalTtk += ttk;
			mPlaytestLiveEnemies.erase(mPlaytestLiveEnemies.begin() + static_cast<std::ptrdiff_t>(index));
		}
	}

	void ArenaTestLevel::EnsurePlaytestPlayerDamageObservation()
	{
		const shared_ptr<PlayerSpaceShip> player =
			std::dynamic_pointer_cast<PlayerSpaceShip>(mEncounterPlayer.lock());
		if (!player)
		{
			UnbindPlaytestPlayerDamageObservation();
			return;
		}
		if (mPlaytestDamageShip.lock() == player)
		{
			return;
		}
		UnbindPlaytestPlayerDamageObservation();
		mPlaytestDamageShip = player;
		mPlaytestDamageDelegateHandle =
			player->GetCombatRuntime().onDamageResolved.BindAction(
				GetWeakPtr(),
				&ArenaTestLevel::HandlePlaytestPlayerDamage
			);
	}

	void ArenaTestLevel::UnbindPlaytestPlayerDamageObservation()
	{
		if (const shared_ptr<PlayerSpaceShip> player = mPlaytestDamageShip.lock();
			player && mPlaytestDamageDelegateHandle.IsValid())
		{
			player->GetCombatRuntime().onDamageResolved.UnbindAction(mPlaytestDamageDelegateHandle);
		}
		mPlaytestDamageDelegateHandle.Reset();
		mPlaytestDamageShip.reset();
	}

	void ArenaTestLevel::HandlePlaytestPlayerDamage(const DamageContext& context)
	{
		if (!mPlaytestMetrics.active)
		{
			return;
		}

		// Values are read from the already-resolved context; mitigation is never
		// recomputed and the hit is never counted twice.
		const double applied = static_cast<double>(context.appliedDamage);
		const double absorbed = static_cast<double>(context.absorbedDamage);
		mPlaytestMetrics.totalDamageTaken += applied;
		mPlaytestMetrics.shieldDamageTaken += absorbed;
		mPlaytestMetrics.hullDamageTaken += std::max(0.0, applied - absorbed);
		if (mPlaytestWasReloading)
		{
			mPlaytestMetrics.damageWhileReloading += applied;
		}

		if (context.targetWasKilled && !mPlaytestMetrics.hasDeathCause)
		{
			mPlaytestMetrics.hasDeathCause = true;
			mPlaytestMetrics.deathCauseDeliveryType = PlaytestDeliveryTypeName(context.deliveryType);
			mPlaytestMetrics.deathCauseAbilityId = context.sourceAbilityId.ToString();
			mPlaytestMetrics.deathCauseDamageTags = PlaytestDamageTagList(context.damageTags);
		}
	}

	void ArenaTestLevel::FinalizePlaytestTerminalState()
	{
		if (!mPlaytestSummaryPending)
		{
			return;
		}

		// A completed encounter may still hold a pending-destroy enemy whose
		// telemetry record only finalizes when the weak_ptr actually expires, which
		// happens after the world releases the actor. Wait a bounded number of
		// frames for that last record so the summary reports the real outcome.
		// Failed encounters keep their existing behaviour: forced cleanup already
		// dropped the live records, so their summary must not wait or count them.
		if (mPlaytestMetrics.completed && !mPlaytestLiveEnemies.empty())
		{
			UpdatePlaytestEnemyRecords();
			if (!mPlaytestLiveEnemies.empty() &&
				mPlaytestTerminalFinalizeFrames < MaxPlaytestTerminalFinalizeFrames)
			{
				++mPlaytestTerminalFinalizeFrames;
				return;
			}
		}

		mPlaytestTerminalFinalizeFrames = 0;
		EmitPlaytestSummaryIfPending();
	}

	void ArenaTestLevel::EmitPlaytestSummaryIfPending()
	{
		if (!mPlaytestSummaryPending)
		{
			return;
		}
		mPlaytestSummaryPending = false;
		++mPlaytestMetrics.summaryEmitCount;

		const char* const result = mPlaytestMetrics.completed
			? "Completed"
			: (mPlaytestMetrics.failed ? "Failed" : "Partial");
		const double averageAlive = mPlaytestMetrics.aliveSampledDuration > 0.0
			? mPlaytestMetrics.aliveIntegral / mPlaytestMetrics.aliveSampledDuration
			: 0.0;

		std::string summary = "[ARENA PLAYTEST]\n";
		summary += "RESULT\n";
		summary += "  result=" + std::string{ result } +
			" duration=" + std::to_string(mPlaytestMetrics.duration) + "\n";
		summary += "WAVES\n";
		for (const ArenaPlaytestWaveMetrics& wave : mPlaytestMetrics.waves)
		{
			const double waveAverageAlive = wave.aliveSampledDuration > 0.0
				? wave.aliveIntegral / wave.aliveSampledDuration
				: 0.0;
			summary += "  wave=" + std::to_string(wave.waveNumber) +
				" start=" + std::to_string(wave.startTime) +
				" end=" + std::to_string(wave.endTime) +
				" duration=" + std::to_string(wave.duration) +
				" spawned=" + std::to_string(wave.spawned) +
				" killed=" + std::to_string(wave.killed) +
				" peakAlive=" + std::to_string(wave.peakAlive) +
				" avgAlive=" + std::to_string(waveAverageAlive) + "\n";
		}
		summary += "ENEMIES\n";
		for (const ArenaPlaytestEnemyTypeMetrics& type : mPlaytestMetrics.enemyTypes)
		{
			const double averageTtk = type.ttkSamples > 0
				? type.totalTtk / static_cast<double>(type.ttkSamples)
				: 0.0;
			summary += "  " + type.enemyId +
				" kills=" + std::to_string(type.kills) +
				" ttkSamples=" + std::to_string(type.ttkSamples) +
				" avgTTK=" + std::to_string(averageTtk) + "\n";
		}
		summary += "TOTALS\n";
		summary += "  spawned=" + std::to_string(mPlaytestMetrics.totalSpawned) +
			" killed=" + std::to_string(mPlaytestMetrics.totalKilled) +
			" peakAlive=" + std::to_string(mPlaytestMetrics.peakAlive) +
			" avgAlive=" + std::to_string(averageAlive) + "\n";
		summary += "PLAYER DAMAGE\n";
		summary += "  total=" + std::to_string(mPlaytestMetrics.totalDamageTaken) +
			" shield=" + std::to_string(mPlaytestMetrics.shieldDamageTaken) +
			" hull=" + std::to_string(mPlaytestMetrics.hullDamageTaken) + "\n";
		summary += "RELOAD\n";
		summary += "  count=" + std::to_string(mPlaytestMetrics.reloadCount) +
			" totalReloadTime=" + std::to_string(mPlaytestMetrics.totalReloadTime) +
			" damageWhileReloading=" + std::to_string(mPlaytestMetrics.damageWhileReloading) + "\n";
		summary += "PLAYER DEATH\n";
		summary += "  count=" + std::to_string(mPlaytestMetrics.playerDeathCount) +
			" wave=" + std::to_string(mPlaytestMetrics.playerDeathWave) +
			" time=" + std::to_string(mPlaytestMetrics.playerDeathTime) + "\n";
		if (mPlaytestMetrics.hasDeathCause)
		{
			summary += "  cause delivery=" + mPlaytestMetrics.deathCauseDeliveryType +
				" ability=" + mPlaytestMetrics.deathCauseAbilityId +
				" damageTags=" + mPlaytestMetrics.deathCauseDamageTags + "\n";
		}
		summary += "BOUNDARY\n";
		summary += "  penalties=" + std::to_string(mPlaytestMetrics.boundaryPenaltyCount) + "\n";

		LY_GAME_INFO("%s", summary.c_str());
	}
}



#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/enemy/EnemyIds.h"
#include "enemy/EnemyActor.h"
#include "level/GameLevel.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	using Json = nlohmann::json;
	using AbilityLevels = std::map<std::string, int>;
	using AbilityMaxLevels = std::map<std::string, int>;
	struct ProgressionMilestone
	{
		int totalKills;
		int level;
		float xp;
		float nextLevelXP;
		int abilityLevelsEarned;
		const char* name;
	};

	class HeadlessProgressionLevel final : public ly::GameLevel
	{
	public:
		explicit HeadlessProgressionLevel(ly::Application* application)
			: GameLevel{ application }
		{
		}

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};

	struct PlayerManagerResetGuard
	{
		ly::PlayerManager& manager;
		~PlayerManagerResetGuard() { manager.Reset(); }
	};

	struct Report
	{
		Json artifact{
			{ "test", "PlayerLevelTestProgressionE2E" },
			{ "passed", false },
			{ "enemyXpPerKill", 1.0f },
			{ "cumulativeKillMilestones", Json::array({ 6, 13, 21 }) },
			{ "testedNextLevelXPThresholds", Json::array({ 6, 7, 8, 9, 11 }) },
			{ "checks", Json::array() }
		};

		void Check(bool passed, const std::string& name, const Json& actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = actual;
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}
	};

	AbilityLevels ReadAbilityLevels(ly::PlayerSpaceShip& ship, AbilityMaxLevels* maximumLevels = nullptr)
	{
		auto& abilities = ship.GetAbilitySystemComponent();
		AbilityLevels levels;
		for (const sas::AbilityRuntimeSnapshot& snapshot : abilities.BuildAbilitySnapshots())
		{
			const ly::GameAbility* ability = abilities.FindAbility<ly::GameAbility>(snapshot.handle);
			if (!ability || snapshot.abilityId.empty()) continue;
			levels[snapshot.abilityId] = ability->GetLevel();
			if (maximumLevels) (*maximumLevels)[snapshot.abilityId] = ability->GetMaxLevel();
		}
		return levels;
	}

	AbilityLevels ExpectedAbilityLevels(
		const AbilityLevels& initial,
		const AbilityMaxLevels& maximumLevels,
		int levelUps
	)
	{
		AbilityLevels expected;
		for (const auto& [abilityId, level] : initial)
		{
			const auto maximum = maximumLevels.find(abilityId);
			const int maxLevel = maximum == maximumLevels.end() ? level : maximum->second;
			expected[abilityId] = std::min(maxLevel, level + levelUps);
		}
		return expected;
	}

	bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.0001f;
	}

	void WriteArtifact(const std::filesystem::path& path, const Json& artifact)
	{
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		if (!output) throw std::runtime_error{ "Could not open E2E artifact path" };
		output << artifact.dump(2) << '\n';
	}

	void RunProgressionE2E(Report& report)
	{
		using namespace ly;
		std::string windowTitle = "Player progression E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, windowTitle, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		PlayerManager& playerManager = PlayerManager::GetPlayerManager();
		playerManager.Reset();
		PlayerManagerResetGuard resetGuard{ playerManager };
		auto level = std::make_shared<HeadlessProgressionLevel>(&application);
		Player& player = playerManager.CreateNewPlayer();
		const shared_ptr<PlayerSpaceShip> initialShip = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(initialShip), "initial player ship spawned");
		level->BeginPlayInternal();
		level->TickInternal(0.f);

		AbilityMaxLevels maximumLevels;
		const AbilityLevels initialAbilityLevels = ReadAbilityLevels(*initialShip, &maximumLevels);
		report.Check(!initialAbilityLevels.empty(), "default loadout contains installed abilities");
		player.AwardScrap(17);
		report.artifact["initialAbilityLevels"] = initialAbilityLevels;

		const std::array<const char*, 3> enemyIds{
			EnemyIds::ApproachGunnerBasic,
			EnemyIds::StrafeSkirmisherBasic,
			EnemyIds::RangeKeeperBasic
		};
		const std::array<ProgressionMilestone, 6> milestones{{
			{ 5, 1, 5.f, 6.f, 0, "five kills leave one XP below level two" },
			{ 6, 2, 0.f, 7.f, 1, "six kills reach level two and carry the next threshold" },
			{ 12, 2, 6.f, 7.f, 1, "six more XP are carried toward level three" },
			{ 13, 3, 0.f, 8.f, 2, "thirteen kills reach level three" },
			{ 20, 3, 7.f, 8.f, 2, "seven XP are carried toward level four" },
			{ 21, 4, 0.f, 9.f, 3, "twenty-one kills reach level four" }
		}};
		constexpr int totalKills = 21;
		int expectedAbilityLevelsEarned = 0;
		for (int kill = 1; kill <= totalKills; ++kill)
		{
			const char* enemyId = enemyIds[static_cast<std::size_t>(kill - 1) % enemyIds.size()];
			const shared_ptr<EnemyActor> enemy = content::SpawnEnemy(
				*level,
				enemyId,
				sf::Vector2f{ 24.f, 24.f }
			).lock();
			report.Check(static_cast<bool>(enemy), "shipped enemy spawned", enemyId);
			level->TickInternal(0.f);
			report.Check(NearlyEqual(enemy->GetShipXPReward(), 1.f), "shipped enemy grants exactly one XP", {
				{ "enemyId", enemyId }, { "reward", enemy->GetShipXPReward() }
			});
			enemy->ApplyDamage(100000.f);

			const auto milestone = std::find_if(milestones.begin(), milestones.end(), [kill](const ProgressionMilestone& value) {
				return value.totalKills == kill;
			});
			if (milestone != milestones.end())
			{
				expectedAbilityLevelsEarned = milestone->abilityLevelsEarned;
				report.Check(
					player.GetShipProgression().GetLevel() == milestone->level &&
					NearlyEqual(player.GetShipProgression().GetXP(), milestone->xp) &&
					NearlyEqual(player.GetShipProgression().GetXPRequiredForNextLevel(), milestone->nextLevelXP),
					milestone->name,
					{
						{ "kills", kill },
						{ "level", player.GetShipProgression().GetLevel() },
						{ "xp", player.GetShipProgression().GetXP() },
						{ "nextLevelXP", player.GetShipProgression().GetXPRequiredForNextLevel() }
					}
				);
			}
			level->TickInternal(0.f);
			const AbilityLevels expectedAbilities = ExpectedAbilityLevels(
				initialAbilityLevels,
				maximumLevels,
				expectedAbilityLevelsEarned
			);
			report.Check(
				ReadAbilityLevels(*initialShip) == expectedAbilities,
				"all installed abilities gain one level per player level",
				ReadAbilityLevels(*initialShip)
			);
			report.Check(player.GetScrap() == 17, "enemy XP progression does not spend scrap", player.GetScrap());
		}

		report.artifact["levelAfterTwentyOneEnemyKills"] = player.GetShipProgression().GetLevel();
		report.artifact["xpAfterTwentyOneEnemyKills"] = player.GetShipProgression().GetXP();
		player.AwardShipXP(19.f);
		report.Check(
			player.GetShipProgression().GetLevel() == 6 &&
			NearlyEqual(player.GetShipProgression().GetXP(), 0.f) &&
			NearlyEqual(player.GetShipProgression().GetXPRequiredForNextLevel(), 11.f),
			"a 19 XP award crosses the level four thresholds of nine and ten XP",
			{
				{ "level", player.GetShipProgression().GetLevel() },
				{ "xp", player.GetShipProgression().GetXP() },
				{ "nextLevelXP", player.GetShipProgression().GetXPRequiredForNextLevel() }
			}
		);
		level->TickInternal(0.f);
		const AbilityLevels expectedAfterBatch = ExpectedAbilityLevels(initialAbilityLevels, maximumLevels, 5);
		report.Check(
			ReadAbilityLevels(*initialShip) == expectedAfterBatch,
			"the multi-level XP award advances all installed abilities through level six",
			ReadAbilityLevels(*initialShip)
		);
		report.Check(player.GetScrap() == 17, "automatic ability levels do not spend scrap", player.GetScrap());

		initialShip->Destroy();
		level->TickInternal(0.f);
		const shared_ptr<PlayerSpaceShip> respawnedShip = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(respawnedShip), "player respawned with remaining lives");
		level->TickInternal(0.f);
		report.Check(
			player.GetShipProgression().GetLevel() == 6 &&
			NearlyEqual(player.GetShipProgression().GetXP(), 0.f),
			"respawn preserves ship level and XP",
			{
				{ "level", player.GetShipProgression().GetLevel() },
				{ "xp", player.GetShipProgression().GetXP() }
			}
		);
		report.Check(
			ReadAbilityLevels(*respawnedShip) == expectedAfterBatch,
			"respawn restores all automatically earned ability levels",
			ReadAbilityLevels(*respawnedShip)
		);
		report.Check(player.GetScrap() == 17, "respawn and automatic levels preserve scrap", player.GetScrap());
		report.artifact["respawnAbilityLevels"] = ReadAbilityLevels(*respawnedShip);
		report.artifact["passed"] = true;
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: LightYearsPlayerLevelTestProgressionE2ETests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!ly::GameContentBootstrap::Register())
		{
			throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		}
		RunProgressionE2E(report);
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << "Player level test progression E2E failed: " << exception.what() << '\n';
	}

	try
	{
		WriteArtifact(artifactPath, report.artifact);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write E2E artifact: " << exception.what() << '\n';
		return 1;
	}

	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}

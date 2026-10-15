#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace vega
{
	enum class GameExportConfiguration { Automatic, Release, Debug };

	// Where the export reads from, and the trees it must never write into.
	struct GameExportLayout
	{
		// LightYearsGame build output: LightYearsGame.exe, the DLLs next to it and the built assets folder.
		std::filesystem::path gameBuildDir;
		// LightYearsVisual/assets in the source tree, exported as <output>/visual.
		std::filesystem::path visualAssetsDir;
		// The repository. Exports may not touch it, except inside generatedRoot (the build tree).
		std::filesystem::path sourceTreeRoot;
		std::filesystem::path generatedRoot;
		// A path only: the editor does not require a Release build to exist.
#ifdef VEGA_GAME_RELEASE_BUILD_DIR
		std::filesystem::path releaseGameBuildDir = VEGA_GAME_RELEASE_BUILD_DIR;
#else
		std::filesystem::path releaseGameBuildDir;
#endif
	};

	struct ExportOptions
	{
		std::filesystem::path outputDir;
		// Allows a non-empty folder. Only files listed in the previous export's manifest are removed; any other
		// file the folder already holds is kept, and an export that would replace one fails instead.
		bool overwrite = false;
		bool runSmokeTest = true;
		// Resolve at export time so a newly built Release executable is picked up.
		GameExportConfiguration configuration = GameExportConfiguration::Automatic;
	};
	using GameExportRequest = ExportOptions;

	struct GameExportResult
	{
		// The copy finished and the manifest was written. The smoke test is reported separately.
		bool success = false;
		GameExportConfiguration configuration = GameExportConfiguration::Debug;
		std::string warning;
		std::size_t filesCopied = 0;
		std::uintmax_t bytes = 0;
		bool smokeTestRan = false;
		bool smokeTestPassed = false;
		std::string smokeTestDetail;
		std::string error;
	};

	struct GameSmokeTestResult
	{
		bool passed = false;
		std::string detail;
	};

	// Names written inside every export folder.
	inline constexpr const char* kExportManifestName = "vega_export_manifest.txt";
	inline constexpr const char* kExportSmokeCaptureName = "export_smoke.png";
	// The game writes this log into its working directory, which is the export folder during the smoke test.
	inline constexpr const char* kExportGameLogName = "LightYears.log";

	GameExportResult ExportGame(const GameExportLayout& layout, const GameExportRequest& request);
	// Runs <gameDir>/LightYearsGame.exe with --visual-capture from that folder. Passes on exit code 0 with a
	// non-empty PNG; a game that does not finish within the timeout is terminated.
	GameSmokeTestResult RunGameSmokeTest(const std::filesystem::path& gameDir, unsigned int timeoutMs = 60000);
	// Missing logs fail closed; Windows separators and letter case are normalized.
	GameSmokeTestResult CheckGameLogPortability(const std::filesystem::path& log, const std::filesystem::path& sourceTreeRoot);
	// One line for the editor dialog and the script report.
	std::string DescribeGameExport(const GameExportResult& result);
}

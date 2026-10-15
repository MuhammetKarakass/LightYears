#include "editor/GameExporter.h"
#include <framework/ProjectSettings.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	int failures = 0;
	void Check(bool condition, const std::string& message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
			++failures;
		}
	}

	void WriteFile(const fs::path& path, const std::string& content)
	{
		fs::create_directories(path.parent_path());
		std::ofstream out{ path, std::ios::binary };
		out << content;
	}

	std::string ReadFile(const fs::path& path)
	{
		std::ifstream in{ path, std::ios::binary };
		return std::string{ std::istreambuf_iterator<char>{ in }, std::istreambuf_iterator<char>{} };
	}

	bool Exists(const fs::path& path)
	{
		std::error_code error;
		return fs::exists(path, error);
	}

	std::vector<std::string> ManifestLines(const fs::path& outputDir)
	{
		std::vector<std::string> lines;
		std::istringstream text{ ReadFile(outputDir / vega::kExportManifestName) };
		for (std::string line; std::getline(text, line);)
		{
			if (!line.empty()) lines.push_back(line);
		}
		return lines;
	}

	// Contents are kept in constants so the expected byte totals come from the same data that was written.
	const std::string kExe = "exe-bytes";
	const std::string kDll = "dll-bytes-1234";
	const std::string kContentJson = "{}";
	const std::string kBee = "bee";
	const std::string kPipeline = "{ \"v\": 1 }";
	const std::string kShader = "void main() {}";
	const std::string kNotes = "not exported";

	// game/ stands in for the LightYearsGame build output, visual_src/ for LightYearsVisual/assets, tree/ for the
	// repository and build/ for the generated tree. Exports are expected under build/.
	struct Fixture
	{
		fs::path root;
		vega::GameExportLayout layout;
		std::uintmax_t expectedBytes = 0;
	};

	Fixture MakeFixture(const fs::path& root)
	{
		fs::remove_all(root);
		Fixture fixture;
		fixture.root = root;
		WriteFile(root / "game" / "LightYearsGame.exe", kExe);
		WriteFile(root / "game" / "sfml-graphics-d-3.dll", kDll);
		WriteFile(root / "game" / "notes.txt", kNotes);
		WriteFile(root / "game" / "assets" / "content.json", kContentJson);
		WriteFile(root / "game" / "assets" / "sub" / "b.txt", kBee);
		WriteFile(root / "visual_src" / "palette" / "pipeline.json", kPipeline);
		WriteFile(root / "visual_src" / "shader.glsl", kShader);
		fs::create_directories(root / "tree");
		fixture.layout = vega::GameExportLayout{ root / "game", root / "visual_src", root / "tree", root / "build" };
		// Exported: exe, dll, two asset files, two visual files. notes.txt is not part of the game.
		fixture.expectedBytes = kExe.size() + kDll.size() + kContentJson.size() + kBee.size() + kPipeline.size() + kShader.size();
		return fixture;
	}

	vega::GameExportRequest Request(const fs::path& outputDir, bool overwrite = false)
	{
		vega::GameExportRequest request;
		request.outputDir = outputDir;
		request.overwrite = overwrite;
		request.runSmokeTest = false;
		return request;
	}
}

int main()
{
	const fs::path temp{ VEGA_EXPORT_TEST_TEMP_DIR };
	fs::remove_all(temp);
	{
		const auto f = MakeFixture(temp / "project");
		auto layout = f.layout;
		layout.assetRoot = f.root / "authoring-assets";
		ly::ProjectSettings project;
		project.name = "My/Game";
		project.startupScene = "scenes/custom.scene.json";
		WriteFile(layout.assetRoot / "vega.project.json", project.ToJson().dump());
		WriteFile(layout.assetRoot / "scenes/custom.scene.json", "{}");
		const auto out = f.root / "build/export";
		Check(vega::ExportGame(layout, Request(out)).success, "Project override export failed");
		Check(ReadFile(out / "My_Game.exe") == kExe && !Exists(out / "LightYearsGame.exe"), "Project executable was not sanitized and renamed");
		Check(ReadFile(out / "assets/vega.project.json") == project.ToJson().dump() && Exists(out / "assets/scenes/custom.scene.json"),
			"Export did not copy the authoring project's assets");
		Check(!Exists(f.layout.gameBuildDir / "assets/vega.project.json"), "Override export changed the built project");
		project.name = "Other";
		WriteFile(layout.assetRoot / "vega.project.json", project.ToJson().dump());
		Check(vega::ExportGame(layout, Request(out, true)).success && !Exists(out / "My_Game.exe") && Exists(out / "Other.exe"),
			"Overwrite after project rename must remove the old executable");
		WriteFile(layout.assetRoot / "vega.project.json", "{}");
		Check(!vega::ExportGame(layout, Request(out, true)).success && Exists(out / "Other.exe"),
			"Invalid project must fail before modifying the previous export");
	}

	{
		// Targets in the repository are refused. Nothing may be created there.
		const Fixture f = MakeFixture(temp / "refuse");
		Check(!vega::ExportGame(f.layout, Request(f.root / "tree" / "export")).success, "Export into the source tree was accepted");
		Check(!Exists(f.root / "tree" / "export"), "Refused export into the source tree still created a folder");
		Check(!vega::ExportGame(f.layout, Request(f.root / "tree")).success, "Export onto the source tree root was accepted");
		Check(!vega::ExportGame(f.layout, Request(f.root / "TREE" / "export")).success, "Case variant of the source tree was accepted");
		// Copying a folder into itself, or over the inputs, must be refused too.
		Check(!vega::ExportGame(f.layout, Request(f.root / "game" / "out")).success, "Export inside the game build folder was accepted");
		Check(!vega::ExportGame(f.layout, Request(f.root / "visual_src" / "out")).success, "Export inside the visual assets was accepted");
		Check(!vega::ExportGame(f.layout, Request(f.root)).success, "Export onto a parent of the inputs was accepted");
		Check(!Exists(f.root / "game" / "out") && !Exists(f.root / "visual_src" / "out"), "Refused export wrote into an input folder");
		// A sibling whose name starts like the repository is a different folder, not a target inside it.
		const auto sibling = vega::ExportGame(f.layout, Request(f.root / "tree2" / "export"));
		Check(sibling.success, "Sibling folder named like the source tree was refused: " + sibling.error);
		// A missing game build cannot be exported at all.
		vega::GameExportLayout missing = f.layout;
		missing.gameBuildDir = f.root / "no-such-build";
		Check(!vega::ExportGame(missing, Request(f.root / "build" / "export")).success, "Missing game build was accepted");
	}

	{
		// Copy: layout, counts, bytes and manifest all match the fixture.
		const Fixture f = MakeFixture(temp / "copy");
		const fs::path out = f.root / "build" / "export";
		const auto result = vega::ExportGame(f.layout, Request(out));
		Check(result.success, "Copy failed: " + result.error);
		Check(result.configuration == vega::GameExportConfiguration::Debug, "Missing Release should select Debug");
		Check(result.warning == "not portable: reads assets from the source tree", "Debug portability warning missing");
		Check(vega::DescribeGameExport(result).find("Debug") != std::string::npos &&
			vega::DescribeGameExport(result).find(result.warning) != std::string::npos, "Debug description omits configuration or warning");
		Check(result.filesCopied == 6, "Expected 6 files, got " + std::to_string(result.filesCopied));
		Check(result.bytes == f.expectedBytes, "Byte count mismatch");
		Check(!result.smokeTestRan, "Smoke test ran although it was disabled");
		Check(ReadFile(out / "LightYears.exe") == kExe, "Executable not copied");
		Check(ReadFile(out / "sfml-graphics-d-3.dll") == kDll, "DLL not copied");
		Check(ReadFile(out / "assets" / "sub" / "b.txt") == kBee, "Nested asset not copied");
		Check(ReadFile(out / "visual" / "palette" / "pipeline.json") == kPipeline, "Visual palette not copied");
		Check(!Exists(out / "notes.txt"), "Non-runtime file was exported");
		const auto manifest = ManifestLines(out);
		Check(manifest.size() == 6, "Manifest should list the 6 exported files");
		Check(std::find(manifest.begin(), manifest.end(), std::string{ vega::kExportManifestName }) == manifest.end(),
			"Manifest lists itself");
		Check(std::find(manifest.begin(), manifest.end(), std::string{ "visual/shader.glsl" }) != manifest.end(),
			"Manifest misses a visual file");
		// Existing export without overwrite is refused, and the copy is left as it was.
		const auto again = vega::ExportGame(f.layout, Request(out));
		Check(!again.success && !again.error.empty(), "Non-empty folder accepted without overwrite");
		Check(ReadFile(out / "LightYears.exe") == kExe, "Refused export changed the folder");
	}

	{
		// Overwrite removes only what the manifest lists. Stray files and the removed source asset follow the rules.
		Fixture f = MakeFixture(temp / "overwrite");
		const fs::path out = f.root / "build" / "export";
		Check(vega::ExportGame(f.layout, Request(out)).success, "Initial export failed");
		WriteFile(out / "stray.txt", "mine");
		WriteFile(out / "assets" / "mine.txt", "mine too");
		fs::remove(f.root / "game" / "assets" / "sub" / "b.txt");
		const auto result = vega::ExportGame(f.layout, Request(out, true));
		Check(result.success, "Overwrite failed: " + result.error);
		Check(result.filesCopied == 5, "Overwrite should copy 5 files, got " + std::to_string(result.filesCopied));
		Check(ReadFile(out / "stray.txt") == "mine", "Stray file was removed by overwrite");
		Check(ReadFile(out / "assets" / "mine.txt") == "mine too", "Stray asset was removed by overwrite");
		Check(!Exists(out / "assets" / "sub" / "b.txt"), "File removed from the source survived overwrite");
		Check(!Exists(out / "assets" / "sub"), "Empty folder left behind by overwrite");
		Check(ManifestLines(out).size() == 5, "Manifest should list the new 5 files");
	}

	{
		// A file the export did not create is never replaced, even with overwrite on.
		const Fixture f = MakeFixture(temp / "collide");
		const fs::path out = f.root / "build" / "export";
		WriteFile(out / "LightYears.exe", "mine");
		const auto result = vega::ExportGame(f.layout, Request(out, true));
		Check(!result.success && !result.error.empty(), "Export replaced a file it did not create");
		Check(ReadFile(out / "LightYears.exe") == "mine", "Stray executable was overwritten");
	}

	{
		// A manifest that points outside the export folder is refused and nothing outside is deleted.
		const Fixture f = MakeFixture(temp / "escape");
		const fs::path out = f.root / "build" / "export";
		WriteFile(f.root / "build" / "victim.txt", "keep me");
		WriteFile(out / vega::kExportManifestName, "../victim.txt\n");
		const auto result = vega::ExportGame(f.layout, Request(out, true));
		Check(!result.success, "Tampered manifest was accepted");
		Check(ReadFile(f.root / "build" / "victim.txt") == "keep me", "File outside the export folder was deleted");
	}

	{
		// Selection uses existence, with explicit choices taking precedence over the default.
		Fixture f = MakeFixture(temp / "configuration");
		f.layout.releaseGameBuildDir = f.root / "release";
		auto request = Request(f.root / "build" / "missing-release");
		request.configuration = vega::GameExportConfiguration::Release;
		Check(!vega::ExportGame(f.layout, request).success, "Explicit missing Release silently fell back to Debug");
		WriteFile(f.layout.releaseGameBuildDir / "LightYearsGame.exe", "release-exe");
		WriteFile(f.layout.releaseGameBuildDir / "sfml-system-3.dll", "release-dll");
		WriteFile(f.layout.releaseGameBuildDir / "assets" / "scripts" / "test.lua", "return {}");
		WriteFile(f.layout.releaseGameBuildDir / "visual" / "pipeline.json", "release-visual");
		const fs::path out = f.root / "build" / "automatic";
		const auto release = vega::ExportGame(f.layout, Request(out));
		Check(release.success && release.configuration == vega::GameExportConfiguration::Release, "Default did not choose existing Release");
		Check(release.warning.empty(), "Release has a Debug warning");
		Check(vega::DescribeGameExport(release).find("Release") != std::string::npos, "Release description omits configuration");
		Check(ReadFile(out / "LightYears.exe") == "release-exe", "Release executable not selected");
		Check(ReadFile(out / "visual" / "pipeline.json") == "release-visual", "Release visual copy came from source tree");
		Check(ReadFile(out / "assets" / "scripts" / "test.lua") == "return {}", "Release scripts not exported");
		Check(Exists(out / "sfml-system-3.dll") && !Exists(out / "sfml-graphics-d-3.dll"), "Release DLL selection incorrect");
		request = Request(f.root / "build" / "explicit-debug");
		request.configuration = vega::GameExportConfiguration::Debug;
		const auto debug = vega::ExportGame(f.layout, request);
		Check(debug.success && debug.configuration == vega::GameExportConfiguration::Debug && !debug.warning.empty(), "Explicit Debug ignored");
		Check(ReadFile(request.outputDir / "LightYears.exe") == kExe, "Explicit Debug copied Release executable");
		request = Request(f.root / "build" / "explicit-release");
		request.configuration = vega::GameExportConfiguration::Release;
		Check(vega::ExportGame(f.layout, request).success, "Explicit Release failed");
	}

	{
		// Fake logs cover both Windows separators, case, other worktrees and unreadable logs.
		const fs::path log = temp / "logs" / "LightYears.log";
		const fs::path source = temp / "source-tree";
		Check(!vega::CheckGameLogPortability(log, source).passed, "Missing portability log accepted");
		WriteFile(log, "");
		Check(!vega::CheckGameLogPortability(log, source).passed, "Empty portability log accepted");
		WriteFile(log, "Loaded assets/scripts/test.lua\nVisual pipeline assets: visual\n");
		Check(vega::CheckGameLogPortability(log, source).passed, "Portable relative paths refused");
		WriteFile(log, "Loaded C:/portable/assets/test.png\n");
		Check(vega::CheckGameLogPortability(log, source).passed, "Portable absolute path refused");
		WriteFile(log, "Loaded " + source.generic_u8string() + "/assets/test.png\n");
		Check(!vega::CheckGameLogPortability(log, source).passed, "Configured source tree not detected");
		WriteFile(log, "Loaded D:/Vega/LightYearsGame/assets/test.png\n");
		Check(vega::CheckGameLogPortability(log, source).passed, "Unrelated D:/Vega path must be portable");
		WriteFile(log, "Loaded d:/vega-ui/assets/test.png\n");
		Check(vega::CheckGameLogPortability(log, "D:/Vega").passed, "Vega-ui is a sibling of a configured Vega root");
		WriteFile(log, "Loaded " + source.generic_u8string() + "-sibling/assets/test.png\n");
		Check(vega::CheckGameLogPortability(log, source).passed, "Root prefix must not match a sibling directory");
		WriteFile(log, "Loaded d:\\VEGA-ui\\LightYearsGame\\assets\\test.png\n");
		Check(vega::CheckGameLogPortability(log, source).passed, "Unrelated Vega-like path must be portable");
		auto mixedCase = source.generic_u8string();
		std::transform(mixedCase.begin(), mixedCase.end(), mixedCase.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		std::replace(mixedCase.begin(), mixedCase.end(), '/', '\\');
		WriteFile(log, "Loaded " + mixedCase + "\\assets\\test.png\n");
		const auto leaked = vega::CheckGameLogPortability(log, source);
		Check(!leaked.passed && leaked.detail.find("line 1") != std::string::npos, "Backslash or mixed-case source path not detected");
		const fs::path nestedLog = source / "build" / "export" / "LightYears.log";
		WriteFile(nestedLog, "Loaded " + nestedLog.parent_path().generic_u8string() + "/assets/test.png\n");
		Check(vega::CheckGameLogPortability(nestedLog, source).passed, "Export's copied assets under the build tree were refused");
		WriteFile(nestedLog, "Loaded " + nestedLog.parent_path().generic_u8string() + "/assets/test.png\nLoaded " +
			source.generic_u8string() + "/LightYearsGame/assets/test.png\n");
		Check(!vega::CheckGameLogPortability(nestedLog, source).passed, "Valid export path hid a source-tree leak");
	}

	{
		// Without a game executable the smoke test fails quickly and says why.
		const auto smoke = vega::RunGameSmokeTest(temp / "no-game");
		Check(!smoke.passed, "Smoke test passed without a game");
		Check(smoke.detail.find("Cannot start") != std::string::npos, "Smoke test detail does not name the start failure: " + smoke.detail);
		vega::GameExportResult failed;
		failed.error = "No output folder";
		Check(vega::DescribeGameExport(failed) == "Export failed: No output folder", "Failure description changed");
	}

	if (failures)
	{
		std::cerr << failures << " GameExporter check(s) failed\n";
		return 1;
	}
	std::cout << "Game exporter tests passed\n";
	return 0;
}

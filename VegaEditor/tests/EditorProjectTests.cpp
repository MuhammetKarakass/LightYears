#include "editor/EditorProject.h"
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
	const fs::path root{ VEGA_PROJECT_TEST_ROOT };
	const bool selectedProject = argc > 1 && std::string{ argv[1] } == "--project";
	const auto selected = root / "selected/project/vega.project.json";
	fs::create_directories(selected.parent_path());
	vega::ProjectSettings project;
	project.name = "Other";
	project.editor = { { "assetRoot", "../content" }, { "font", "../ui/font.ttf" }, { "visualAssets", "../art" },
		{ "gameBuildDir", "../runtime" }, { "releaseGameBuildDir", "../release" }, { "exportDir", "../export" } };
	project.runtimeExecutable = "OtherRuntime.exe";
	project.runtimeAssetDirectory = "data";
	project.runtimeVisualDirectory = "art";
	project.runtimeLog = "Other.log";
	if (selectedProject) std::ofstream{ selected } << project.ToJson();
	const auto resolved = vega::EditorProject::FromCommandLine();
	const auto absolute = [](const fs::path& path) { return fs::weakly_canonical(path); };
	if (selectedProject)
	{
		if (resolved.settings.name != "Other" || resolved.file != absolute(selected)) return 1;
		if (resolved.sourceAssetRoot != absolute(root / "selected/content") || resolved.assetRoot != absolute(root / "authoring")) return 1;
		if (resolved.font != absolute(root / "selected/ui/font.ttf") || resolved.exportDir != absolute(root / "selected/export")) return 1;
		const auto& layout = resolved.exportLayout;
		if (layout.gameBuildDir != absolute(root / "selected/runtime") || layout.releaseGameBuildDir != absolute(root / "selected/release")) return 1;
		if (layout.visualAssetsDir != absolute(root / "selected/art") || layout.runtimeExecutable != "OtherRuntime.exe") return 1;
		if (layout.assetDirectory != "data" || layout.visualDirectory != "art" || layout.gameLogName != "Other.log") return 1;
	}
	else
	{
		if (resolved.settings.name != "Game" || !resolved.font.empty() || !resolved.exportLayout.visualAssetsDir.empty()) return 1;
		if (resolved.assetRoot != absolute(root / "default") || resolved.exportLayout.gameBuildDir != absolute(root / "build/Game")) return 1;
		if (resolved.exportDir != absolute(root / "build/export/Game")) return 1;
	}
	std::cout << "Project paths and overrides passed\n";
	return 0;
}

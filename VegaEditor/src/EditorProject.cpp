#include "editor/EditorProject.h"
#include <cstdlib>

namespace vega
{
	EditorProject EditorProject::FromCommandLine()
	{
		EditorProject project;
		project.file = VEGA_DEFAULT_PROJECT_FILE;
		std::filesystem::path assetOverride;
#ifdef _MSC_VER
		for (int i = 1; i + 1 < __argc; ++i)
		{
			const std::string argument{ __argv[i] };
			if (argument == "--project") project.file = std::filesystem::u8path(__argv[++i]);
			else if (argument == "--asset-root") assetOverride = std::filesystem::u8path(__argv[++i]);
		}
#endif
		project.file = std::filesystem::absolute(project.file).lexically_normal();
		if (std::filesystem::exists(project.file)) project.settings = ly::ProjectSettings::Load(project.file);
		const auto resolve = [&](const char* key, const std::string& fallback = std::string{}) -> std::filesystem::path
		{
			const auto value = project.settings.editor.value(key, fallback);
			if (value.empty()) return {};
			auto path = std::filesystem::weakly_canonical(project.file.parent_path() / std::filesystem::u8path(value));
			// A missing root resolved from "." retains a trailing separator. Keep
			// root comparisons stable before the first document creates that folder.
			if (path.has_relative_path() && !path.has_filename()) path = path.parent_path();
			return path;
		};
		project.sourceAssetRoot = resolve("assetRoot", ".");
		project.assetRoot = assetOverride.empty() ? project.sourceAssetRoot : std::filesystem::weakly_canonical(assetOverride);
		project.font = resolve("font");
		project.exportLayout.gameBuildDir = resolve("gameBuildDir", (std::filesystem::path{ VEGA_BUILD_ROOT } / "Game").generic_string());
		project.exportLayout.releaseGameBuildDir = resolve("releaseGameBuildDir", (std::filesystem::path{ VEGA_SOURCE_ROOT } / "build-release/Game").generic_string());
		project.exportLayout.visualAssetsDir = resolve("visualAssets");
		project.exportLayout.sourceTreeRoot = VEGA_SOURCE_ROOT;
		project.exportLayout.generatedRoot = VEGA_BUILD_ROOT;
		project.exportLayout.assetRoot = project.assetRoot;
		project.exportLayout.sourceAssetRoot = project.sourceAssetRoot;
		project.exportLayout.runtimeExecutable = project.settings.runtimeExecutable;
		project.exportLayout.assetDirectory = project.settings.runtimeAssetDirectory;
		project.exportLayout.visualDirectory = project.settings.runtimeVisualDirectory;
		project.exportLayout.gameLogName = project.settings.runtimeLog;
		project.exportDir = resolve("exportDir");
		if (project.exportDir.empty()) project.exportDir = std::filesystem::path{ VEGA_BUILD_ROOT } / "export" / project.settings.ExecutableName().substr(0, project.settings.ExecutableName().size() - 4);
		return project;
	}
}

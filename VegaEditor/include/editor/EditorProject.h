#pragma once

#include <framework/ProjectSettings.h>
#include "editor/GameExporter.h"

namespace vega
{
	struct EditorProject
	{
		ly::ProjectSettings settings;
		std::filesystem::path file, assetRoot, sourceAssetRoot, font, exportDir;
		GameExportLayout exportLayout;
		static EditorProject FromCommandLine();
	};
}

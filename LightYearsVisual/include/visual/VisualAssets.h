#pragma once

#include <filesystem>
#include <string>

#include "visual/ColorScript.h"
#include "visual/PipelineSettings.h"
#include "visual/ReloadReport.h"
#include "visual/ShaderLibrary.h"
#include "visual/ShipVisual.h"

namespace ly::visual
{
	// The hot-reloadable data of the library: shaders/, palettes/ and pipeline.json under one root.
	class VisualAssets
	{
	public:
		bool Load(const std::filesystem::path& root, std::string& error);

		// Reloads files whose timestamp changed (everything when force). Failed files keep their old version.
		ReloadReport Reload(bool force);
		// Frame hook: runs a non-forced Reload every intervalSeconds.
		ReloadReport Poll(float deltaSeconds, float intervalSeconds = 0.5f);

		PipelineSettings& Settings() { return mSettings; }
		ShaderLibrary& Shaders() { return mShaders; }
		ColorScriptLibrary& Scripts() { return mScripts; }
		const ColorScript& Script() const { return mScripts.Active(); }
		ShipVisualLibrary& Ships() { return mShips; }
		const ShipVisualLibrary& Ships() const { return mShips; }
		// Files that failed to load during Load (a bad ship definition does not stop the rest from loading).
		const ReloadReport& LoadReport() const { return mLoadReport; }

	private:
		std::filesystem::path mRoot;
		PipelineSettings mSettings;
		std::filesystem::file_time_type mSettingsStamp;
		ShaderLibrary mShaders;
		ColorScriptLibrary mScripts;
		ShipVisualLibrary mShips;
		ReloadReport mLoadReport;
		float mPollTimer = 0.f;
	};
}

#pragma once

#include <filesystem>
#include <string>

#include "visual/ColorScript.h"
#include "visual/PipelineSettings.h"
#include "visual/ReloadReport.h"
#include "visual/ShaderLibrary.h"

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

	private:
		std::filesystem::path mRoot;
		PipelineSettings mSettings;
		std::filesystem::file_time_type mSettingsStamp;
		ShaderLibrary mShaders;
		ColorScriptLibrary mScripts;
		float mPollTimer = 0.f;
	};
}

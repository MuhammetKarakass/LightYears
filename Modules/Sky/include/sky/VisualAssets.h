#pragma once

#include <filesystem>
#include <string>

#include "sky/ColorScript.h"
#include "sky/DeepSky.h"
#include "sky/PipelineSettings.h"
#include "sky/ReloadReport.h"
#include "sky/ShaderLibrary.h"


namespace vega::sky
{
	// The hot-reloadable data of the library: shaders/, palettes/ and pipeline.json under one root.
	class VisualAssets
	{
	public:
		bool Load(const std::filesystem::path& root, std::string& error, std::filesystem::path skyFile = "deepsky.json");

		// Reloads files whose timestamp changed (everything when force). Failed files keep their old version.
		ReloadReport Reload(bool force);
		// Frame hook: runs a non-forced Reload every intervalSeconds.
		ReloadReport Poll(float deltaSeconds, float intervalSeconds = 0.5f);

		PipelineSettings& Settings() { return mSettings; }
		const PipelineSettings& Settings() const { return mSettings; }
		ShaderLibrary& Shaders() { return mShaders; }
		ColorScriptLibrary& Scripts() { return mScripts; }
		const ColorScript& Script() const { return mScripts.Active(); }
		// Named sky assets from the selected document (optional: empty when the file is missing).
		const DeepSkyCatalog& DeepSky() const { return mDeepSky; }
		const std::filesystem::path& Root() const { return mRoot; }
		// Editor previews share the reload parser/version path and suspend disk polling for this file only.
		bool PreviewDeepSky(const nlohmann::json& data, std::string& error);
		void PreviewDeepSky(const DeepSkyCatalog& catalog) { mDeepSky.Replace(catalog); mDeepSkyPreview = true; }
		void EndDeepSkyPreview();
		// Optional assets that failed to load; required shader/palette failures fail Load.
		const ReloadReport& LoadReport() const { return mLoadReport; }

	private:
		std::filesystem::path mRoot;
		std::filesystem::path mSkyFile;
		PipelineSettings mSettings;
		std::filesystem::file_time_type mSettingsStamp;
		ShaderLibrary mShaders;
		ColorScriptLibrary mScripts;
		DeepSkyCatalog mDeepSky;
		std::filesystem::file_time_type mDeepSkyStamp;
		ReloadReport mLoadReport;
		float mPollTimer = 0.f;
		bool mDeepSkyPreview = false;
	};
}

#include "visual/VisualAssets.h"

#include "FileUtil.h"

namespace ly::visual
{
	bool VisualAssets::Load(const std::filesystem::path& root, std::string& error)
	{
		mRoot = root;
		if (!mShaders.LoadDirectory(root / "shaders", error))
		{
			return false;
		}
		if (!mScripts.LoadDirectory(root / "palettes", error))
		{
			return false;
		}
		std::filesystem::path settingsFile = root / "pipeline.json";
		mSettingsStamp = FileStamp(settingsFile);
		if (std::filesystem::exists(settingsFile) && !LoadPipelineSettings(settingsFile, mSettings, error))
		{
			return false;
		}
		// Ship definitions are optional and never fatal: a bad file lands in the load report.
		mLoadReport = {};
		mShips.LoadDirectory(root / "ships", mLoadReport);
		// The deep-sky catalog is optional too: without it every stop keeps its plain sky.
		const std::filesystem::path deepSkyFile = root / "deepsky.json";
		mDeepSkyStamp = FileStamp(deepSkyFile);
		std::string deepSkyError;
		if (std::filesystem::exists(deepSkyFile) && !mDeepSky.Load(deepSkyFile, deepSkyError))
		{
			mLoadReport.errors.push_back("deepsky.json: " + deepSkyError);
		}
		return true;
	}

	ReloadReport VisualAssets::Reload(bool force)
	{
		ReloadReport report;
		mShaders.Reload(force, report);
		mScripts.Reload(force, report);
		mShips.Reload(force, report);

		std::filesystem::path settingsFile = mRoot / "pipeline.json";
		auto stamp = FileStamp(settingsFile);
		if (force || stamp != mSettingsStamp)
		{
			mSettingsStamp = stamp;
			std::string error;
			if (!std::filesystem::exists(settingsFile))
			{
				if (force)
				{
					report.errors.push_back("pipeline.json is missing; keeping current settings");
				}
			}
			else if (LoadPipelineSettings(settingsFile, mSettings, error))
			{
				report.reloaded.push_back("pipeline.json");
			}
			else
			{
				report.errors.push_back("pipeline.json: " + error);
			}
		}

		const std::filesystem::path deepSkyFile = mRoot / "deepsky.json";
		const auto deepSkyStamp = FileStamp(deepSkyFile);
		if (!mDeepSkyPreview && (force || deepSkyStamp != mDeepSkyStamp) && std::filesystem::exists(deepSkyFile))
		{
			mDeepSkyStamp = deepSkyStamp;
			std::string error;
			if (mDeepSky.Load(deepSkyFile, error))
			{
				report.reloaded.push_back("deepsky.json");
			}
			else
			{
				report.errors.push_back("deepsky.json: " + error);
			}
		}
		return report;
	}

	ReloadReport VisualAssets::Poll(float deltaSeconds, float intervalSeconds)
	{
		mPollTimer += deltaSeconds;
		if (mPollTimer < intervalSeconds)
		{
			return {};
		}
		mPollTimer = 0.f;
		return Reload(false);
	}

	bool VisualAssets::PreviewDeepSky(const nlohmann::json& data, std::string& error)
	{
		if (!mDeepSky.Apply(data, error)) return false;
		mDeepSkyPreview = true;
		return true;
	}

	void VisualAssets::EndDeepSkyPreview()
	{
		if (!mDeepSkyPreview) return;
		mDeepSkyPreview = false;
		std::string error;
		mDeepSky.Load(mRoot / "deepsky.json", error);
		mDeepSkyStamp = FileStamp(mRoot / "deepsky.json");
	}
}

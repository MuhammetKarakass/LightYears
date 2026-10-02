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
		return true;
	}

	ReloadReport VisualAssets::Reload(bool force)
	{
		ReloadReport report;
		mShaders.Reload(force, report);
		mScripts.Reload(force, report);

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
}

#include "BlankGameModule.h"
#include <framework/ProjectSettings.h>
#include <framework/debug/Log.h>
#include <fstream>
#include <cstdlib>
#include <algorithm>

namespace
{
	std::string Option(const std::string& name, const std::string& fallback = {})
	{
#ifdef _MSC_VER
		for (int i = 1; i + 1 < __argc; ++i)
			if (__argv[i] == name) return __argv[i + 1];
#endif
		return fallback;
	}
	class BlankApplication final : public ly::Application
	{
	public:
		BlankApplication(const std::filesystem::path& root, const ly::ProjectSettings& project)
			: Application{ { project.windowWidth, project.windowHeight }, 32, project.name, sf::Style::Default }
		{
			ly::debug::Log::SetMinimumLevel(ly::debug::LogLevel::Info);
			project.ApplyCollisionLayers(GetEngineContext());
			mModule.ConfigureProject(root / "vega.project.json");
			RegisterGameModule(mModule, false);
			const auto scene = Option("--scene", project.startupScene);
			std::ifstream input{ root / scene };
			if (!input) throw std::runtime_error("Cannot open startup scene: " + scene);
			nlohmann::json document;
			input >> document;
			LoadWorld(mModule.CreateLevel(*this, document, false));
			ly::debug::Log::Write(ly::debug::LogLevel::Info, ly::debug::LogChannel::Scene, {},
				"Blank startup scene: " + scene);
			mCapture = Option("--visual-capture");
			mFrames = std::max(1, std::stoi(Option("--capture-frames", "120")));
		}
		~BlankApplication() override { UnregisterGameModule(); }
		int GetExitCode() const override { return mFailed ? 1 : 0; }
	private:
		void AfterRender() override
		{
			if (mCapture.empty() || ++mFrame < mFrames) return;
			// Use the same capture CLI as other engine hosts, including exported smoke tests.
			sf::Texture texture;
			mFailed = !texture.resize(GetWindowSize());
			if (!mFailed)
			{
				texture.update(GetRenderWindow());
				mFailed = !texture.copyToImage().saveToFile(mCapture);
			}
			QuitApplication();
		}
		blank::BlankGameModule mModule;
		std::string mCapture;
		int mFrames = 120, mFrame = 0;
		bool mFailed = false;
	};
}

ly::Application* GetApplication()
{
	// Packaged assets resolve beside the executable, independent of the launch directory.
	std::filesystem::path executable;
#ifdef _MSC_VER
	executable = std::filesystem::absolute(__argv[0]).parent_path();
#endif
	const std::filesystem::path root = Option("--asset-root", (executable / "assets").string());
	const auto project = ly::ProjectSettings::Load(root / "vega.project.json");
	return new BlankApplication(root, project);
}

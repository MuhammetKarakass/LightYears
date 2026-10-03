#include "gameFramework/GameApplication.h"

#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "level/ArenaTestLevel.h"
#include "player/PlayerManager.h"
#include "presentation/visual/GameVisualPipeline.h"

#include <framework/AssetManager.h>
#include <Config.h>
#include "framework/PerfMonitor.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>

ly::Application* GetApplication()
{
	return new ly::GameApplication();
}

namespace ly
{
	GameApplication::GameApplication()
		: Application({ 1920, 1080}, 64, std::string("LightYears"), sf::Style::Close | sf::Style::Titlebar)
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory(getResourceDir());
		if (!GameContentBootstrap::Register())
		{
			LY_GAME_ERROR("Failed to register shipped game content; application will not start the game world");
			QuitApplication();
			return;
		}

		ly::perf::g_disableLights.store(false);

		mDevOptions = ParseDevOptions();
		InitializeVisualPipeline();
		if (mDevOptions.shipIndex >= 0)
		{
			const List<std::string> shipIds = content::ShipContentCatalog::GetPlayerShipIds();
			if (static_cast<std::size_t>(mDevOptions.shipIndex) < shipIds.size())
			{
				PlayerManager::GetPlayerManager().SetSelectedShipId(shipIds[static_cast<std::size_t>(mDevOptions.shipIndex)]);
			}
		}

		LoadWorld<ArenaTestLevel>();
	}

	GameApplication::~GameApplication()
	{
		SetWorldRenderPipeline(nullptr);
	}

	GameApplication::DevOptions GameApplication::ParseDevOptions()
	{
		DevOptions options;
#ifdef _MSC_VER
		for (int i = 1; i < __argc; ++i)
		{
			const std::string arg = __argv[i];
			const bool hasValue = i + 1 < __argc;
			if (arg == "--visual-off") options.visualOff = true;
			else if (arg == "--visual-script" && hasValue) options.colorScript = __argv[++i];
			else if (arg == "--visual-capture" && hasValue) options.capturePath = __argv[++i];
			else if (arg == "--capture-frames" && hasValue) options.captureFrames = std::max(1, std::atoi(__argv[++i]));
			else if (arg == "--visual-player-hull" && hasValue) options.playerHull = std::clamp(static_cast<float>(std::atof(__argv[++i])), 0.f, 1.f);
			else if (arg == "--visual-ship" && hasValue) options.shipIndex = std::atoi(__argv[++i]);
			else if (arg == "--visual-assets" && hasValue) options.visualAssets = __argv[++i];
		}
#endif
		return options;
	}

	void GameApplication::InitializeVisualPipeline()
	{
		// Order: --visual-assets, the packaged <exe dir>/visual copy (non-Debug builds), then the source tree so debug
		// builds hot-reload shader, palette and pipeline.json edits without a rebuild.
		std::filesystem::path assetsDir;
		const char* origin = "--visual-assets";
		if (!mDevOptions.visualAssets.empty()) assetsDir = mDevOptions.visualAssets;
		else
		{
			std::error_code error;
			assetsDir = (std::filesystem::path(getResourceDir()) / ".." / "visual").lexically_normal();
			origin = "packaged copy";
			if (!std::filesystem::is_directory(assetsDir, error))
			{
				assetsDir = LY_VISUAL_ASSETS_DIR;
				origin = "source tree";
			}
		}
		LY_GAME_INFO("Visual pipeline assets (%s): %s", origin, assetsDir.string().c_str());
		mVisualPipeline = std::make_unique<GameVisualPipeline>();
		mVisualPipeline->Initialize(GetRenderWindow(), assetsDir, mDevOptions.colorScript);
		mVisualPipeline->SetEnabled(!mDevOptions.visualOff);
		if (mDevOptions.playerHull >= 0.f)
		{
			mVisualPipeline->SetPlayerHullOverride(mDevOptions.playerHull);
		}
		SetWorldRenderPipeline(mVisualPipeline.get());
	}

	void GameApplication::Tick(float deltaTime)
	{
		if (mVisualPipeline)
		{
			mVisualPipeline->Update(deltaTime);
		}
	}

	bool GameApplication::HandleApplicationEvent(const sf::Event& event)
	{
		if (!mVisualPipeline)
		{
			return false;
		}
		// F7 cycles the color script, F8 toggles the visual pipeline, F9 forces an asset reload. The keys are consumed so levels never see them;
		// key repeat is ignored so holding a key does not flicker the pipeline.
		static bool f7Down = false;
		static bool f8Down = false;
		static bool f9Down = false;
		if (const auto* pressed = event.getIf<sf::Event::KeyPressed>())
		{
			if (pressed->code == sf::Keyboard::Key::F7)
			{
				if (!f7Down) mVisualPipeline->CycleColorScript();
				f7Down = true;
				return true;
			}
			if (pressed->code == sf::Keyboard::Key::F8)
			{
				if (!f8Down) mVisualPipeline->Toggle();
				f8Down = true;
				return true;
			}
			if (pressed->code == sf::Keyboard::Key::F9)
			{
				if (!f9Down) mVisualPipeline->ForceReload();
				f9Down = true;
				return true;
			}
		}
		else if (const auto* released = event.getIf<sf::Event::KeyReleased>())
		{
			if (released->code == sf::Keyboard::Key::F7)
			{
				f7Down = false;
				return true;
			}
			if (released->code == sf::Keyboard::Key::F8)
			{
				f8Down = false;
				return true;
			}
			if (released->code == sf::Keyboard::Key::F9)
			{
				f9Down = false;
				return true;
			}
		}
		return false;
	}

	void GameApplication::AfterRender()
	{
		SaveCaptureIfDue();
	}

	void GameApplication::SaveCaptureIfDue()
	{
		if (mDevOptions.capturePath.empty() || ++mRenderedFrames < mDevOptions.captureFrames)
		{
			return;
		}
		// Runs before the window is presented, so the back buffer still holds the finished frame including the HUD.
		sf::RenderWindow& window = GetRenderWindow();
		sf::Texture texture(window.getSize());
		texture.update(window);
		const std::filesystem::path out(mDevOptions.capturePath);
		if (out.has_parent_path())
		{
			std::filesystem::create_directories(out.parent_path());
		}
		if (texture.copyToImage().saveToFile(out))
		{
			LY_GAME_INFO("Captured %s after %d frames (visual pipeline %s)", out.string().c_str(), mRenderedFrames, mVisualPipeline && mVisualPipeline->IsActive() ? "ON" : "OFF");
		}
		else
		{
			LY_GAME_ERROR("Cannot write capture %s", out.string().c_str());
		}
		mDevOptions.capturePath.clear();
		QuitApplication();
	}
}

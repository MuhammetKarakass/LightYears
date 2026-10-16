#include "gameFramework/LightYearsGameModule.h"
#include "gameFramework/LightYearsAssets.h"
#include "level/PlayerStart.h"
#include <AbilitySystemDebugView.h>
#include <AbilitySystemInterface.h>
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameConfigs/combat/CombatTick.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "level/GameSceneSettings.h"
#include <framework/scene/SceneSettings.h>
#include "reflection/GameReflection.h"
#include <script/ScriptRuntime.h>

#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "level/ArenaTestLevel.h"
#include "level/SceneLevel.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "player/PlayerMovementComponent.h"
#include "presentation/visual/GameVisualPipeline.h"

#include <framework/AssetManager.h>
#include <framework/ProjectSettings.h>
#include <Config.h>
#include "framework/PerfMonitor.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>

namespace ly
{
	LightYearsGameModule::LightYearsGameModule() = default;
	namespace
	{
		std::filesystem::path ResourceRoot(bool allowOverride = true)
		{
#ifdef _MSC_VER
			for (int index = 1; index + 1 < __argc; ++index)
				if (allowOverride && std::string{ __argv[index] } == "--asset-root") return std::filesystem::u8path(__argv[index + 1]);
#endif
			return getResourceDir();
		}
		sf::Vector2u ProjectWindowSize(bool editor)
		{
			const auto path = ResourceRoot(!editor) / "vega.project.json";
			if (!std::filesystem::exists(path)) return { 1920, 1080 };
			try
			{
				const auto project = ProjectSettings::Load(path);
				return { project.windowWidth, project.windowHeight };
			}
			catch (const std::exception&) { return { 1920, 1080 }; }
		}
	}

	void LightYearsGameModule::Initialize(Application& application, bool editor)
	{
		InstallLightYearsEngineAssets();
		mApplication = &application;
		if (mProjectFile.empty()) mProjectFile = ResourceRoot(!editor) / "vega.project.json";
		// In the editor --asset-root selects authoring documents; content tests use a partial copy.
		auto runtimeRoot = ResourceRoot(!editor);
		if (editor && std::filesystem::exists(mProjectFile))
		{
			const auto project = ProjectSettings::Load(mProjectFile);
			runtimeRoot = std::filesystem::weakly_canonical(mProjectFile.parent_path() / project.editor.value("assetRoot", std::string{ "." }));
		}
		AssetManager::GetAssetManager().SetAssetRootDirectory(runtimeRoot.generic_string() + "/");
		sas::AbilitySystemDebugView::effectPeriod = [](const sas::GameplayEffectRuntimeSnapshot& effect)
		{
			if (effect.effectId == DamageStatusEffectIds::IgniteEffectId) return CombatTick::Interval;
			return sas::FindAttributeValue(effect.runtimeAttributes, CommonAttributeIds::Interval, 0.f);
		};
		sas::AbilitySystemDebugView::resolve = [](Actor& actor) -> sas::AbilitySystemComponent*
		{
			auto* provider = dynamic_cast<sas::AbilitySystemInterface*>(&actor);
			return provider ? &provider->GetAbilitySystemComponent() : nullptr;
		};
		sas::AbilitySystemDebugView::player = [](World& world) -> std::shared_ptr<Actor>
		{
			const auto* player = PlayerManager::GetPlayerManager().GetPlayer();
			auto ship = player ? player->GetCurrentSpaceShip().lock() : nullptr;
			return ship && ship->GetWorld() == &world ? ship : nullptr;
		};
		sas::AbilitySystemDebugView::effectDefinitions = []
		{
			List<sas::GameplayEffectSpec> definitions;
			for (const auto* definition : content::EffectContentCatalog::GetDefinitions())
			{
				if (!definition) continue;
				auto spec = sas::MakeGameplayEffectSpec(*definition);
				if (definition->sourceParameterized)
				{
					bool found = false;
					for (const auto* ability : content::AbilityContentCatalog::GetDefinitions())
					{
						for (const auto& parameters : ability->effectSpecs)
						{
							if (parameters.effectId.ToString() != definition->effectId) continue;
							spec.duration = parameters.duration.value_or(parameters.useAbilityDuration ? ability->duration : definition->duration);
							spec.maxStacks = parameters.maxStacks.value_or(definition->maxStacks);
							spec.modifiers = parameters.modifiers;
							spec.attributes = parameters.attributes;
							found = true;
							break;
						}
						if (found) break;
					}
				}
				definitions.push_back(std::move(spec));
			}
			return definitions;
		};
		vega::script::SetScriptRoot(ResourceRoot(!editor));
		vega::script::RegisterScriptTypes();
		vega::script::ScriptRuntime::Get().SetPlayerResolver([](World&) -> std::weak_ptr<Actor>
		{
			const auto* player = PlayerManager::GetPlayerManager().GetPlayer();
			return player ? player->GetCurrentSpaceShip() : std::weak_ptr<PlayerSpaceShip>{};
		});
		if (!GameContentBootstrap::Register())
		{
			LY_GAME_ERROR("Failed to register shipped game content; application will not start the game world");
			mApplication->QuitApplication();
			return;
		}

		RegisterGameSceneSettings();

		ly::perf::g_disableLights.store(false);

		mDevOptions = ParseDevOptions();
		try
		{
			const auto path = mProjectFile;
			if (std::filesystem::exists(path))
			{
				const auto project = ProjectSettings::Load(path);
				if (!mDevOptions.sceneSpecified) mDevOptions.scenePath = project.startupScene;
			}
		}
		catch (const std::exception& error)
		{
			LY_GAME_ERROR("Project: %s", error.what());
			mExitCode = 1;
			mApplication->QuitApplication();
			return;
		}
		PlayerMovementComponent::SetDevAutoPlay(mDevOptions.perfAutoPlay);
		InitializeVisualPipeline();
		if (mDevOptions.shipIndex >= 0)
		{
			const List<std::string> shipIds = content::ShipContentCatalog::GetPlayerShipIds();
			if (static_cast<std::size_t>(mDevOptions.shipIndex) < shipIds.size())
			{
				PlayerManager::GetPlayerManager().SetSelectedShipId(shipIds[static_cast<std::size_t>(mDevOptions.shipIndex)]);
			}
		}

		if (editor) return;

		if (mDevOptions.scenePath.empty()) mApplication->LoadWorld<ArenaTestLevel>();
		else
		{
			try
			{
				std::filesystem::path path{ mDevOptions.scenePath };
				if (path.is_relative()) path = ResourceRoot() / path;
				std::ifstream input{ path };
				if (!input) throw std::runtime_error("Cannot open scene: " + path.string());
				nlohmann::json scene;
				input >> scene;
				mApplication->LoadWorld(CreateLevel(*mApplication, scene, false));
				LY_GAME_INFO("SceneLevel: scene-loaded %s (play mode)", mDevOptions.scenePath.c_str());
			}
			catch (const std::exception& error)
			{
				LY_GAME_ERROR("Scene: %s", error.what());
				mExitCode = 1;
				mApplication->QuitApplication();
			}
		}
	}

	LightYearsGameModule::~LightYearsGameModule()
	{
		if (mApplication)
		{
			mApplication->SetWorldRenderPipeline(nullptr);
			mApplication->UnregisterGameModule();
		}
	}

	std::string LightYearsGameModule::SceneBackdropState() const
	{
		return mVisualPipeline ? mVisualPipeline->BackdropStopId() : std::string{};
	}

	void LightYearsGameModule::ApplySceneSettings(const nlohmann::json& settings)
	{
		const auto stop = SceneSettingsSchema::Get().Value(settings, "routeStop");
		if (!stop.is_number_integer() || stop.get<double>() < 0 || stop.get<double>() > std::numeric_limits<int>::max())
			throw std::runtime_error("routeStop must be a nonnegative integer");
		if (mVisualPipeline) mVisualPipeline->SetRouteStopIndex(static_cast<std::size_t>(stop.get<int>()));
	}

	LightYearsGameModule::DevOptions LightYearsGameModule::ParseDevOptions()
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
			else if ((arg == "--scene" || arg == "--editor-scene") && hasValue)
			{
				options.sceneSpecified = true;
				options.scenePath = __argv[++i];
			}
			else if (arg == "--capture-frames" && hasValue) options.captureFrames = std::max(1, std::atoi(__argv[++i]));
			else if (arg == "--visual-player-hull" && hasValue) options.playerHull = std::clamp(static_cast<float>(std::atof(__argv[++i])), 0.f, 1.f);
			else if (arg == "--visual-ship" && hasValue) options.shipIndex = std::atoi(__argv[++i]);
			else if (arg == "--visual-assets" && hasValue) options.visualAssets = __argv[++i];
			else if (arg == "--perf-autoplay") options.perfAutoPlay = true;
			else if (arg == "--galaxy-stop" && hasValue) options.galaxyStop = std::max(0, std::atoi(__argv[++i]));
			else if (arg == "--orbit-speed-steps" && hasValue) options.orbitSpeedSteps = std::atoi(__argv[++i]);
			else if (arg == "--warp" && hasValue) options.warpBeta = std::clamp(static_cast<float>(std::atof(__argv[++i])), 0.f, 0.99f);
		}
#endif
		return options;
	}

	void LightYearsGameModule::InitializeVisualPipeline()
	{
		// Order: --visual-assets, the packaged <exe dir>/visual copy (non-Debug builds), then the source tree so debug
		// builds hot-reload shader, palette and pipeline.json edits without a rebuild.
		std::filesystem::path assetsDir;
		const char* origin = "--visual-assets";
		if (!mDevOptions.visualAssets.empty()) assetsDir = mDevOptions.visualAssets;
		else
		{
			std::error_code error;
			assetsDir = (ResourceRoot() / ".." / "visual").lexically_normal();
			origin = "packaged copy";
			if (!std::filesystem::is_directory(assetsDir, error))
			{
				assetsDir = LY_VISUAL_ASSETS_DIR;
				origin = "source tree";
				if (std::filesystem::exists(mProjectFile))
				{
					const auto project = ProjectSettings::Load(mProjectFile);
					const auto visual = project.editor.value("visualAssets", std::string{});
					if (!visual.empty()) assetsDir = std::filesystem::weakly_canonical(mProjectFile.parent_path() / visual);
				}
			}
		}
		LY_GAME_INFO("Visual pipeline assets (%s): %s", origin, assetsDir.string().c_str());
		mVisualPipeline = std::make_unique<GameVisualPipeline>();
		mVisualPipeline->Initialize(mApplication->GetRenderWindow(), assetsDir, mDevOptions.colorScript);
		mVisualPipeline->SetEnabled(!mDevOptions.visualOff);
		mVisualPipeline->SetRouteStopIndex(static_cast<std::size_t>(mDevOptions.galaxyStop));
		if (mDevOptions.orbitSpeedSteps != 0) mVisualPipeline->StepOrbitSpeed(mDevOptions.orbitSpeedSteps);
		if (mDevOptions.warpBeta > 0.f)
		{
			// Screen up is the direction of travel for the dev warp; the ease starts from rest.
			mVisualPipeline->SetWarp(mDevOptions.warpBeta, sf::Vector2f(0.f, -1.f));
		}
		if (mDevOptions.playerHull >= 0.f)
		{
			mVisualPipeline->SetPlayerHullOverride(mDevOptions.playerHull);
		}
		mApplication->SetWorldRenderPipeline(mVisualPipeline.get());
	}

	void LightYearsGameModule::Tick(float deltaTime)
	{
		if (mVisualPipeline)
		{
			mVisualPipeline->Update(deltaTime);
		}
	}

	bool LightYearsGameModule::HandleEvent(const sf::Event& event)
	{
		if (!mVisualPipeline)
		{
			return false;
		}
		// F7 cycles the color script, F8 toggles the visual pipeline, F9 forces an asset reload, Ã– / Ã‡ cycle the galaxy route stop, + / - the orbit speed. The keys are consumed so levels never see them;
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
		else if (const auto* text = event.getIf<sf::Event::TextEntered>())
		{
			// O-umlaut (U+00F6/D6) / C-cedilla (U+00E7/C7) step back / forward along the galaxy route. Read as typed characters: on the Turkish layout these keys
			// have no sf::Keyboard::Key code of their own.
			if (text->unicode == 0xF6 || text->unicode == 0xD6)
			{
				mVisualPipeline->CycleRouteStop(-1);
				return true;
			}
			if (text->unicode == 0xE7 || text->unicode == 0xC7)
			{
				mVisualPipeline->CycleRouteStop(1);
				return true;
			}
			// + / - change the orbit speed of orbit stops. '*' and '=' also speed up: on the Turkish layout '+' needs Shift.
			if (text->unicode == '+' || text->unicode == '*' || text->unicode == '=')
			{
				mVisualPipeline->StepOrbitSpeed(1);
				return true;
			}
			if (text->unicode == '-')
			{
				mVisualPipeline->StepOrbitSpeed(-1);
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

	void LightYearsGameModule::AfterRender()
	{
		SaveCaptureIfDue();
	}

	void LightYearsGameModule::SaveCaptureIfDue()
	{
		if (mDevOptions.capturePath.empty() || ++mRenderedFrames < mDevOptions.captureFrames)
		{
			return;
		}
		// Runs before the window is presented, so the back buffer still holds the finished frame including the HUD.
		sf::RenderWindow& window = mApplication->GetRenderWindow();
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
		mApplication->QuitApplication();
	}

	sf::Vector2u GameProjectWindowSize(bool editor)
	{
		return ProjectWindowSize(editor);
	}

	void LightYearsGameModule::RegisterTypes()
	{
		RegisterGameTypes();
	}

	std::shared_ptr<World> LightYearsGameModule::CreateLevel(Application& application, const nlohmann::json& scene, bool edit)
	{
		return std::make_shared<SceneLevel>(&application, scene,
			[this](const nlohmann::json& settings) { ApplySceneSettings(settings); }, edit);
	}

	std::shared_ptr<Actor> LightYearsGameModule::PlayerActor(World& world)
	{
		const auto* player = PlayerManager::GetPlayerManager().GetPlayer();
		const auto ship = player ? player->GetCurrentSpaceShip().lock() : nullptr;
		return ship && ship->GetWorld() == &world ? ship : nullptr;
	}

	vega::ActorDisplayInfo LightYearsGameModule::DisplayInfo(const Actor& actor) const
	{
		const auto* start = dynamic_cast<const PlayerStart*>(&actor);
		return start ? vega::ActorDisplayInfo{ true, start->GetShip() } : vega::ActorDisplayInfo{};
	}
}

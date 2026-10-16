#include "presentation/visual/GameVisualPipeline.h"
#include "level/GameSceneSettings.h"

#include "framework/Actor.h"
#include "framework/AssetManager.h"
#include "framework/debug/Log.h"
#include "framework/debug/Profiler.h"

#include <cstdio>
#include <vector>

namespace ly
{
	bool GameVisualPipeline::Initialize(sf::RenderWindow& window, const std::filesystem::path& assetsDirectory, const std::string& colorScript)
	{
		mReady = false;
		mWindow = &window;
		LoadRoute();
		if (!sf::Shader::isAvailable())
		{
			LY_GAME_ERROR("Visual pipeline disabled: shaders are not available on this system");
			return false;
		}
		std::string error;
		if (!mAssets.Load(assetsDirectory, error))
		{
			LY_GAME_ERROR("Visual pipeline disabled: cannot load assets from %s: %s", assetsDirectory.string().c_str(), error.c_str());
			return false;
		}
		if (!mAssets.Scripts().SetActive(colorScript))
		{
			LY_GAME_WARN("Unknown visual color script \"%s\"; using \"%s\"", colorScript.c_str(), mAssets.Script().Name().c_str());
		}
		mPipeline = std::make_unique<visual::VisualPipeline>(mAssets);
		if (!mPipeline->Resize(window.getSize()))
		{
			LY_GAME_ERROR("Visual pipeline disabled: cannot create the layer textures");
			mPipeline.reset();
			return false;
		}
		LogReport(mAssets.LoadReport());
		mReady = true;
		LY_GAME_INFO("Visual pipeline ready (assets %s, world %ux%u)", assetsDirectory.string().c_str(), mPipeline->VirtualSize().x, mPipeline->VirtualSize().y);
		return true;
	}

	void GameVisualPipeline::LoadRoute()
	{
		if (mRouteAttempted)
		{
			return;
		}
		mRouteAttempted = true;
		const std::string root = AssetManager::GetAssetManager().GetAssetRootDirectory();
		const std::filesystem::path base = root.empty() ? std::filesystem::path("LightYearsGame/assets") : std::filesystem::path(root);
		std::string error;
		mRouteLoaded = mRoute.LoadFromFile(base / "content/data/galaxy_route.json", error) && !mRoute.Stops().empty();
		if (mRouteLoaded)
		{
			mRoute.AppendProceduralStops(RouteProceduralStopCount, RouteProceduralSeed);
			if (!mRoute.AppendStopsFromFile(base / "content/data/galaxy_centre_tour.json", error))
			{
				LY_GAME_WARN("Galactic Centre tour unavailable: %s", error.c_str());
			}
		}
		else
		{
			LY_GAME_WARN("Galaxy route unavailable, the backdrop stays at the Sun's position: %s", error.empty() ? "no stops" : error.c_str());
		}
		ApplyRouteStop();
	}

	void GameVisualPipeline::ApplyRouteStop()
	{
		if (!mRouteLoaded)
		{
			mBackdrop.SetGalacticPosition(visual::galaxy::RoutePosition(0.f));
			return;
		}
		const std::vector<RouteStop>& stops = mRoute.Stops();
		if (mRouteStopIndex >= stops.size())
		{
			LY_GAME_WARN("Galaxy route has %zu stops; stop %zu does not exist, using the last one", stops.size(), mRouteStopIndex);
			mRouteStopIndex = stops.size() - 1;
		}
		mBackdrop.SetRouteStop(stops[mRouteStopIndex]);
		if (!mStopLabel)
		{
			mStopLabel.emplace("");
			mStopLabel->SetTextSize(20);
		}
		mStopLabel->SetString("Stop: " + std::to_string(mRouteStopIndex) + "  " + stops[mRouteStopIndex].name);
	}

	void GameVisualPipeline::SetRouteStopIndex(std::size_t index)
	{
		mRouteStopIndex = index;
		ApplyRouteStop();
	}

	void GameVisualPipeline::StepOrbitSpeed(int step)
	{
		mBackdrop.StepOrbitSpeed(step);
		LY_GAME_INFO("Orbit speed x%g", mBackdrop.OrbitSpeedScale());
	}

	void GameVisualPipeline::CycleRouteStop(int step)
	{
		if (!mRouteLoaded)
		{
			return;
		}
		const std::vector<RouteStop>& stops = mRoute.Stops();
		const long long count = static_cast<long long>(stops.size());
		const long long next = ((static_cast<long long>(mRouteStopIndex) + step) % count + count) % count;
		SetRouteStopIndex(static_cast<std::size_t>(next));
		LY_GAME_INFO("Galaxy route stop %zu/%zu: %s", mRouteStopIndex, stops.size(), stops[mRouteStopIndex].name.c_str());
	}

	void GameVisualPipeline::LogReport(const visual::ReloadReport& report) const
	{
		for (const std::string& line : report.reloaded)
		{
			LY_GAME_INFO("Visual asset reloaded: %s", line.c_str());
		}
		for (const std::string& line : report.errors)
		{
			LY_GAME_WARN("Visual asset reload failed (previous version kept): %s", line.c_str());
		}
	}

	void GameVisualPipeline::Update(float deltaSeconds)
	{
		if (!mReady)
		{
			return;
		}
		mTime += deltaSeconds;
		LogReport(mAssets.Poll(deltaSeconds));
	}

	void GameVisualPipeline::ForceReload()
	{
		if (mReady)
		{
			LogReport(mAssets.Reload(true));
		}
	}

	void GameVisualPipeline::Toggle()
	{
		SetEnabled(!mEnabled);
	}

	void GameVisualPipeline::CycleColorScript()
	{
		if (mReady)
		{
			mAssets.Scripts().CycleActive();
			LY_GAME_INFO("Visual color script: %s", mAssets.Script().Name().c_str());
		}
	}

	void GameVisualPipeline::SetEnabled(bool enabled)
	{
		mEnabled = enabled;
		LY_GAME_INFO("Visual pipeline %s", IsActive() ? "ON" : (mReady ? "OFF" : "unavailable (assets failed to load)"));
	}

	void GameVisualPipeline::BeginFrame(const World& world, const sf::View& worldView)
	{
		if (mWindow->getSize() != mPipeline->OutputSize())
		{
			mPipeline->Resize(mWindow->getSize());
		}
		{
			LY_PROFILE_SCOPE("VP.PipelineBegin");
			mPipeline->BeginFrame();
		}
		// Full-resolution backdrop parallax follows the continuous camera. Pixel ships, sprites and their energy
		// glows share the snapped view so they stay aligned with each other.
		const sf::View snappedView = mPipeline->SnapWorldView(worldView);
		{
			LY_PROFILE_SCOPE("VP.Backdrop");
			mBackdrop.Draw(mAssets, *mPipeline, world, worldView, mSkyAuthoring ? 0.f : mTime);
		}
		mPipeline->Target(visual::Layer::Sprites).setView(snappedView);
		{
			LY_PROFILE_SCOPE("VP.Ships");
			mShips.Draw(world, snappedView, mAssets, *mPipeline, mTime);
		}
		{
			LY_PROFILE_SCOPE("VP.Lights");
			mLights.Submit(world, snappedView, mAssets, *mPipeline);
		}
	}

	sf::RenderTarget& GameVisualPipeline::GetTarget(RenderLayer layer)
	{
		// Background stays at full resolution; actors keep their sprite colors in the pixel-scaled sprite layer.
		return mPipeline->Target(layer == RenderLayer::Background ? visual::Layer::Background : visual::Layer::Sprites);
	}

	bool GameVisualPipeline::ReplacesLayer(RenderLayer layer) const
	{
		return layer == RenderLayer::Background;
	}

	void GameVisualPipeline::Compose(sf::RenderTarget& window)
	{
		mPipeline->Compose(window);
		DrawStopLabel(window);
	}

	void GameVisualPipeline::DrawStopLabel(sf::RenderTarget& window)
	{
		if (!mStopLabel)
		{
			return;
		}
		// Right-aligned on the same line as the frame-rate readout on the left.
		constexpr float Margin = 20.f;
		const float width = mStopLabel->GetBound().size.x;
		mStopLabel->SetWidgetLocation(sf::Vector2f(static_cast<float>(window.getSize().x) - width - Margin, 18.f));
		mStopLabel->NativeDraw(window);

		const std::optional<float> rate = mBackdrop.OrbitDegreesPerMinute();
		if (!rate)
		{
			return;
		}
		if (!mOrbitLabel)
		{
			mOrbitLabel.emplace("");
			mOrbitLabel->SetTextSize(16);
		}
		char text[64];
		std::snprintf(text, sizeof(text), "Orbit x%g  %.1f deg/min", mBackdrop.OrbitSpeedScale(), *rate);
		mOrbitLabel->SetString(text);
		const float orbitWidth = mOrbitLabel->GetBound().size.x;
		mOrbitLabel->SetWidgetLocation(sf::Vector2f(static_cast<float>(window.getSize().x) - orbitWidth - Margin, 18.f + mStopLabel->GetBound().size.y + 8.f));
		mOrbitLabel->NativeDraw(window);
	}
}

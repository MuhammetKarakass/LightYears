#include "presentation/visual/GameVisualPipeline.h"
#include "level/GameSceneSettings.h"

#include "VFX/ExplosionLights.h"
#include "framework/Actor.h"
#include "framework/AssetManager.h"
#include "framework/EngineContext.h"
#include "framework/debug/Log.h"
#include "framework/debug/Profiler.h"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace vega
{
	bool GameVisualPipeline::Initialize(sf::RenderWindow& window, const std::filesystem::path& assetsDirectory, const std::string& colorScript)
	{
		mReady = false;
		mWindow = &window;
		LoadRoute();
		if (!sf::Shader::isAvailable())
		{
			VEGA_GAME_ERROR("Visual pipeline disabled: shaders are not available on this system");
			return false;
		}
		std::string error;
		if (!mAssets.Load(assetsDirectory, error))
		{
			VEGA_GAME_ERROR("Visual pipeline disabled: cannot load assets from %s: %s", assetsDirectory.string().c_str(), error.c_str());
			return false;
		}
		if (!mAssets.Scripts().SetActive(colorScript))
		{
			VEGA_GAME_WARN("Unknown visual color script \"%s\"; using \"%s\"", colorScript.c_str(), mAssets.Script().Name().c_str());
		}
		mPipeline = std::make_unique<visual::VisualPipeline>(mAssets);
		if (!mPipeline->Resize(window.getSize()))
		{
			VEGA_GAME_ERROR("Visual pipeline disabled: cannot create the layer textures");
			mPipeline.reset();
			return false;
		}
		LogReport(mAssets.LoadReport());
		mReady = true;
		VEGA_GAME_INFO("Visual pipeline ready (assets %s, world %ux%u)", assetsDirectory.string().c_str(), mPipeline->VirtualSize().x, mPipeline->VirtualSize().y);
		return true;
	}

	void GameVisualPipeline::LoadRoute()
	{
		if (mRouteAttempted)
		{
			return;
		}
		mRouteAttempted = true;
		const std::string root = mContext.Assets().GetAssetRootDirectory();
		const std::filesystem::path base = root.empty() ? std::filesystem::path("LightYearsGame/assets") : std::filesystem::path(root);
		std::string error;
		mRouteLoaded = mRoute.LoadFromFile(base / "content/data/galaxy_route.json", error) && !mRoute.Stops().empty();
		if (mRouteLoaded)
		{
			mRoute.AppendProceduralStops(RouteProceduralStopCount, RouteProceduralSeed);
			if (!mRoute.AppendStopsFromFile(base / "content/data/galaxy_centre_tour.json", error))
			{
				VEGA_GAME_WARN("Galactic Centre tour unavailable: %s", error.c_str());
			}
		}
		else
		{
			VEGA_GAME_WARN("Galaxy route unavailable, the backdrop stays at the Sun's position: %s", error.empty() ? "no stops" : error.c_str());
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
			VEGA_GAME_WARN("Galaxy route has %zu stops; stop %zu does not exist, using the last one", stops.size(), mRouteStopIndex);
			mRouteStopIndex = stops.size() - 1;
		}
		mBackdrop.SetRouteStop(stops[mRouteStopIndex]);
		if (!mStopLabel)
		{
			mStopLabel.emplace(mContext, "");
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
		VEGA_GAME_INFO("Orbit speed x%g", mBackdrop.OrbitSpeedScale());
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
		VEGA_GAME_INFO("Galaxy route stop %zu/%zu: %s", mRouteStopIndex, stops.size(), stops[mRouteStopIndex].name.c_str());
	}

	void GameVisualPipeline::LogReport(const visual::ReloadReport& report) const
	{
		for (const std::string& line : report.reloaded)
		{
			VEGA_GAME_INFO("Visual asset reloaded: %s", line.c_str());
		}
		for (const std::string& line : report.errors)
		{
			VEGA_GAME_WARN("Visual asset reload failed (previous version kept): %s", line.c_str());
		}
	}

	void GameVisualPipeline::Update(float deltaSeconds)
	{
		// Flashes are aged even while the pipeline is unavailable, so they cannot pile up unsubmitted.
		ExplosionLights::Update(deltaSeconds);
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
			VEGA_GAME_INFO("Visual color script: %s", mAssets.Script().Name().c_str());
		}
	}

	void GameVisualPipeline::SetEnabled(bool enabled)
	{
		mEnabled = enabled;
		VEGA_GAME_INFO("Visual pipeline %s", IsActive() ? "ON" : (mReady ? "OFF" : "unavailable (assets failed to load)"));
	}

	void GameVisualPipeline::BeginFrame(const World& world, const sf::View& worldView)
	{
		if (mWindow->getSize() != mPipeline->OutputSize())
		{
			mPipeline->Resize(mWindow->getSize());
		}
		{
			VEGA_PROFILE_SCOPE("VP.PipelineBegin");
			mPipeline->BeginFrame();
		}
		// Full-resolution backdrop parallax follows the continuous camera. Pixel ships, sprites and their energy
		// glows share the snapped view so they stay aligned with each other.
		const sf::View snappedView = mPipeline->SnapWorldView(worldView);
		// The same frame the sprite layers use, kept for world-unit light submissions during World::Render.
		mFrameOrigin = snappedView.getCenter() - snappedView.getSize() * 0.5f;
		const float viewHeight = snappedView.getSize().y;
		mFrameScale = viewHeight > 0.f ? static_cast<float>(mPipeline->OutputSize().y) / viewHeight : 1.f;
		{
			VEGA_PROFILE_SCOPE("VP.Backdrop");
			mBackdrop.Draw(mAssets, *mPipeline, world, worldView, mSkyAuthoring ? 0.f : mTime);
		}
		// Explosion flashes are short-lived lights in world units; each fades to nothing over its lifetime.
		for (const ExplosionFlash& flash : ExplosionLights::Active())
		{
			if (flash.lifetime <= 0.f)
			{
				continue;
			}
			const float fade = 1.f - std::clamp(flash.age / flash.lifetime, 0.f, 1.f);
			sky::PointLight light;
			light.position = (flash.position - mFrameOrigin) * mFrameScale;
			light.radius = flash.radius * mFrameScale;
			light.color = { flash.color.r / 255.f, flash.color.g / 255.f, flash.color.b / 255.f };
			light.intensity = flash.intensity * fade * fade;
			mPipeline->SubmitPointLight(light);
		}
		mPipeline->Target(visual::Layer::Sprites).setView(snappedView);
		mPipeline->Target(visual::Layer::Lit).setView(snappedView);
		mPipeline->Target(visual::Layer::Decals).setView(snappedView);
		{
			VEGA_PROFILE_SCOPE("VP.Ships");
			mShips.Draw(world, snappedView, mAssets, *mPipeline, mTime);
		}
		{
			VEGA_PROFILE_SCOPE("VP.Lights");
			mLights.Submit(world, snappedView, mAssets, *mPipeline);
		}
	}

	sf::RenderTarget& GameVisualPipeline::GetTarget(RenderLayer layer)
	{
		// Background stays at full resolution. Ships and other world bodies are lit surfaces; telegraphs stay
		// readable under them; projectiles and effects are unlit for readability.
		using visual::Layer;
		switch (layer)
		{
		case RenderLayer::Background: return mPipeline->Target(Layer::Background);
		case RenderLayer::World: return mPipeline->Target(Layer::Lit);
		case RenderLayer::GroundDecal: return mPipeline->Target(Layer::Decals);
		case RenderLayer::WorldFront:
		case RenderLayer::WorldVfx:
		case RenderLayer::Foreground: return mPipeline->Target(Layer::Sprites);
		case RenderLayer::WorldEmissive: return mPipeline->Target(Layer::Emissive);
		default: return mPipeline->Target(Layer::Sprites);
		}
	}

	sf::RenderTarget* GameVisualPipeline::GetLightTarget(RenderLayer layer)
	{
		// An actor's own glows are emissive, not surfaces: drawn unlit under the ships, as they were drawn before the body.
		return layer == RenderLayer::World ? &mPipeline->Target(visual::Layer::Decals) : nullptr;
	}

	void GameVisualPipeline::SubmitLight(const SceneLight& light)
	{
		if (!mReady)
		{
			return;
		}
		// SceneLight is in world units; PointLight is in output pixels through the snapped frame BeginFrame kept.
		sky::PointLight point;
		point.position = (light.position - mFrameOrigin) * mFrameScale;
		point.radius = light.radius * mFrameScale;
		point.color = { light.color.r / 255.f, light.color.g / 255.f, light.color.b / 255.f };
		point.intensity = light.intensity;
		point.height = light.height;
		mPipeline->SubmitPointLight(point);
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
			mOrbitLabel.emplace(mContext, "");
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

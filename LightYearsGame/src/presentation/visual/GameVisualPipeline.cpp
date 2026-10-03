#include "presentation/visual/GameVisualPipeline.h"

#include "framework/Actor.h"
#include "framework/debug/Log.h"

namespace ly
{
	bool GameVisualPipeline::Initialize(sf::RenderWindow& window, const std::filesystem::path& assetsDirectory, const std::string& colorScript)
	{
		mReady = false;
		mWindow = &window;
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
		mPipeline->BeginFrame();
		// The camera snapped to the world-layer pixel grid. Sprites, backdrop parallax and energy glows all use it so
		// sub-pixel camera motion neither jitters sprites nor drifts glows off their ships. HUD and game positions
		// are untouched: the engine keeps using its own unsnapped view.
		const sf::View snappedView = mPipeline->SnapWorldView(worldView);
		mBackdrop.Draw(mAssets, *mPipeline, world, snappedView, mTime);
		// Actors draw with the game camera; the backdrop above used the pipeline's own pixel view.
		mPipeline->Target(visual::Layer::Sprites).setView(snappedView);
		mShips.Draw(world, snappedView, mAssets, *mPipeline, mTime);
		mLights.Submit(world, snappedView, mAssets, *mPipeline);
	}

	sf::RenderTarget& GameVisualPipeline::GetTarget(RenderLayer layer)
	{
		// Actors keep their sprite colors; only the procedural backdrop goes through the palette-dithered World layer.
		return mPipeline->Target(layer == RenderLayer::Background ? visual::Layer::Background : visual::Layer::Sprites);
	}

	bool GameVisualPipeline::ReplacesLayer(RenderLayer layer) const
	{
		return layer == RenderLayer::Background;
	}

	void GameVisualPipeline::Compose(sf::RenderWindow& window)
	{
		mPipeline->Compose(window);
	}
}

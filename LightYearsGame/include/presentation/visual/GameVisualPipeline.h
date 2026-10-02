#pragma once

#include "framework/WorldRenderPipeline.h"
#include "presentation/visual/EnergyLightPresenter.h"
#include "presentation/visual/ProceduralBackdrop.h"

#include <filesystem>
#include <memory>
#include <string>
#include <visual/VisualAssets.h>
#include <visual/VisualPipeline.h>

namespace ly
{
	// Game-side implementation of the engine's WorldRenderPipeline seam, built on LightYearsVisual.
	// Background layer -> procedural backdrop; every other render layer -> the undithered pixel sprite layer; energy glows are
	// submitted from a read-only look at the world. Owned by the application; HUD never goes through it.
	class GameVisualPipeline final : public WorldRenderPipeline
	{
	public:
		GameVisualPipeline() = default;

		// Loads shaders, palettes and settings. On failure the pipeline stays inactive and the game renders as before.
		// colorScript names the palette script to start with; an unknown name keeps the library default.
		bool Initialize(sf::RenderWindow& window, const std::filesystem::path& assetsDirectory, const std::string& colorScript);

		// Real-time frame hook: hot-reloads changed asset files.
		void Update(float deltaSeconds);
		void ForceReload();
		void Toggle();
		void CycleColorScript();
		void SetEnabled(bool enabled);
		bool IsEnabled() const { return mEnabled; }

		bool IsActive() const override { return mEnabled && mReady; }
		void BeginFrame(const World& world, const sf::View& worldView) override;
		sf::RenderTarget& GetTarget(RenderLayer layer) override;
		bool ReplacesLayer(RenderLayer layer) const override;
		void Compose(sf::RenderWindow& window) override;

	private:
		void LogReport(const visual::ReloadReport& report) const;

		sf::RenderWindow* mWindow = nullptr;
		visual::VisualAssets mAssets;
		std::unique_ptr<visual::VisualPipeline> mPipeline;
		ProceduralBackdrop mBackdrop;
		EnergyLightPresenter mLights;
		bool mReady = false;
		bool mEnabled = true;
		float mTime = 0.f;
	};
}

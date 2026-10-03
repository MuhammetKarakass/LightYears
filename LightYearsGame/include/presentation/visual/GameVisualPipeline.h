#pragma once

#include "framework/WorldRenderPipeline.h"
#include "presentation/visual/EnergyLightPresenter.h"
#include "presentation/visual/ProceduralBackdrop.h"
#include "presentation/visual/ShipVisualPresenter.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <visual/VisualAssets.h>
#include <visual/VisualPipeline.h>

namespace ly
{
	// Game-side implementation of the engine's WorldRenderPipeline seam, built on LightYearsVisual.
	// Background layer -> procedural backdrop; every other render layer -> the undithered pixel sprite layer; energy glows are
	// submitted from a read-only look at the world; the player ship is drawn procedurally and its sprite is skipped.
	// Owned by the application; HUD never goes through it.
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
		// Dev-only: forces the displayed hull fraction of the procedural player ship (presentation only).
		void SetPlayerHullOverride(std::optional<float> hullFraction) { mShips.SetHullOverride(hullFraction); }
		bool IsEnabled() const { return mEnabled; }

		bool IsActive() const override { return mEnabled && mReady; }
		void BeginFrame(const World& world, const sf::View& worldView) override;
		sf::RenderTarget& GetTarget(RenderLayer layer) override;
		bool ReplacesLayer(RenderLayer layer) const override;
		bool HidesActor(const Actor& actor) const override { return mShips.HidesActor(actor); }
		void Compose(sf::RenderWindow& window) override;

	private:
		void LogReport(const visual::ReloadReport& report) const;

		sf::RenderWindow* mWindow = nullptr;
		visual::VisualAssets mAssets;
		std::unique_ptr<visual::VisualPipeline> mPipeline;
		ProceduralBackdrop mBackdrop;
		EnergyLightPresenter mLights;
		ShipVisualPresenter mShips;
		bool mReady = false;
		bool mEnabled = true;
		float mTime = 0.f;
	};
}

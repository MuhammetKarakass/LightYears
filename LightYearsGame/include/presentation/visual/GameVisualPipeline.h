#pragma once

#include "framework/WorldRenderPipeline.h"
#include "gameplay/galaxy/GalaxyRoute.h"
#include "presentation/visual/EnergyLightPresenter.h"
#include "presentation/visual/ProceduralBackdrop.h"
#include "presentation/visual/ShipVisualPresenter.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <visual/VisualAssets.h>
#include <widget/TextWidget.h>
#include <visual/VisualPipeline.h>

namespace vega
{
	// Game-side implementation of the engine's WorldRenderPipeline seam, built on LightYearsVisual.
	// Background layer -> procedural backdrop; every other render layer -> the undithered pixel sprite layer; energy glows are
	// submitted from a read-only look at the world; the player ship is drawn procedurally and its sprite is skipped.
	// Owned by the application; HUD never goes through it.
	class GameVisualPipeline final : public WorldRenderPipeline
	{
	public:
		// The context supplies the asset root and the dev labels' fonts.
		explicit GameVisualPipeline(EngineContext& context) : mContext{ context } {}

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
		// Places the backdrop at a stop of the campaign route (0 = Sol). The index is clamped to the route.
		void SetRouteStopIndex(std::size_t index);
		const std::string& BackdropStopId() const { return mBackdrop.StopId(); }
		// Dev-only: moves to the next (+1) or previous (-1) route stop, wrapping around both ends.
		void CycleRouteStop(int step);
		// Dev-only: steps the orbit speed of orbit stops (+1 faster, -1 slower); shown under the stop label.
		void StepOrbitSpeed(int step);
		// Ship speed as a fraction of light speed and its on-screen direction (y down); eased by the backdrop.
		void SetWarp(float beta, sf::Vector2f direction) { mBackdrop.SetWarp(beta, direction); }
		bool IsEnabled() const { return mEnabled; }
		visual::VisualAssets& EditorAssets() { return mAssets; }
		const GalaxyRoute& Route() const { return mRoute; }
		ProceduralBackdrop& EditorBackdrop() { return mBackdrop; }
		void SetSkyAuthoring(bool enabled) { mSkyAuthoring = enabled; }

		bool IsActive() const override { return mEnabled && mReady; }
		void BeginFrame(const World& world, const sf::View& worldView) override;
		sf::RenderTarget& GetTarget(RenderLayer layer) override;
		sf::RenderTarget* GetLightTarget(RenderLayer layer) override;
		bool ReplacesLayer(RenderLayer layer) const override;
		bool HidesActor(const Actor& actor) const override { return mShips.HidesActor(actor); }
		// Engine lights arrive in world units; the frame's snapped view converts them to output pixels.
		void SubmitLight(const SceneLight& light) override;
		void Compose(sf::RenderTarget& window) override;

	private:
		EngineContext& mContext;
		void LogReport(const visual::ReloadReport& report) const;
		void LoadRoute();
		void ApplyRouteStop();
		void DrawStopLabel(sf::RenderTarget& window);

		sf::RenderWindow* mWindow = nullptr;
		visual::VisualAssets mAssets;
		std::unique_ptr<visual::VisualPipeline> mPipeline;
		ProceduralBackdrop mBackdrop;
		GalaxyRoute mRoute;
		bool mRouteAttempted = false;
		bool mRouteLoaded = false;
		std::size_t mRouteStopIndex = 0;
		// Route stop number and name, top right of the screen, so a backdrop can be named when tuning deepsky.json.
		std::optional<TextWidget> mStopLabel;
		// Orbit speed under the stop label, only at orbit stops.
		std::optional<TextWidget> mOrbitLabel;
		EnergyLightPresenter mLights;
		ShipVisualPresenter mShips;
		bool mReady = false;
		bool mEnabled = true;
		float mTime = 0.f;
		bool mSkyAuthoring = false;
		// The snapped view BeginFrame uses for the sprite/world layers: world position of output pixel (0, 0) and output
		// pixels per world unit. Engine light submissions during World::Render convert through these.
		sf::Vector2f mFrameOrigin;
		float mFrameScale = 1.f;
	};
}

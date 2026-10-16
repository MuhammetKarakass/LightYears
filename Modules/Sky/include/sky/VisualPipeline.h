#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "sky/LuminanceTier.h"
#include "sky/VisualAssets.h"

namespace vega::sky
{
	enum class Layer
	{
		Background,
		// Procedural pixel content; palette-dithered when settings.ditherEnabled. Drawn first.
		World,
		// Pixel-scaled content that keeps its own colors (sprite actors). Same virtual resolution, drawn over World.
		Sprites,
		Energy,
		// Soft nebula content, drawn into a transparent background-resolution target before composition.
		Nebula
	};

	struct GlowParams
	{
		sf::Vector2f position;
		float coreRadius = 3.f;
		float haloRadius = 28.f;
		sf::Color color = sf::Color::White;
		float intensity = 1.f;
		LuminanceTier tier = LuminanceTier::Decorative;
	};

	// Points run head first. Width tapers headWidth -> tailWidth and the light fades toward the tail.
	struct TrailParams
	{
		std::vector<sf::Vector2f> points;
		float headWidth = 12.f;
		float tailWidth = 1.f;
		sf::Color color = sf::Color::White;
		float intensity = 1.f;
		LuminanceTier tier = LuminanceTier::Decorative;
	};

	struct EnergyStats
	{
		int submitted = 0;
		int drawn = 0;
		int culledByBudget = 0;
		int clampedByTier = 0;
		float areaRatio = 0.f;
	};

	// Point-mass lens (Schwarzschild): a ray passing at impact parameter r from the lens on screen is bent so the image at
	// r shows the source at beta = r - thetaE^2 / r (lens equation). Inside the shadow (~2.6 Schwarzschild radii in
	// impact parameter) nothing escapes; just outside it the photon ring is brightened. Sgr A* itself is quiet (no
	// bright disk), so the effect is the distortion of the star field behind it.
	struct LensParams
	{
		sf::Vector2f center;          // output pixels
		float einsteinRadius = 120.f; // output pixels
		float shadowRadius = 30.f;    // output pixels
		float ringBrightness = 0.25f; // photon ring, fraction of the background cap
		bool enabled = false;
	};

	// Owns layer textures and composites them: background (gradient -> nebula -> stars, colour graded on composite)
	// -> world -> sprites -> energy (additive) -> bloom -> flash -> vignette. World uses nearest upscale and optional dither; sprites stay undithered.
	// Coordinates are output pixels unless a layer is given a game view (see SnapWorldView).
	class VisualPipeline
	{
	public:
		explicit VisualPipeline(VisualAssets& assets);

		bool Resize(sf::Vector2u outputSize);
		void BeginFrame();
		sf::RenderTarget& Target(Layer layer);
		// Premultiplied-composites the nebula texture over Background; unavailable or disabled targets are a no-op.
		void CompositeNebula();
		// Lenses the Background layer only, at Compose time. Persists until changed; disabled by default.
		void SetLens(const LensParams& lens);
		// Per-place look of the background on top of pipeline.json: a brighter ceiling and a stronger grade let a stop
		// carry its own character (for example a neon-warm hot zone). Defaults change nothing.
		struct BackgroundLook
		{
			float capScale = 1.f;
			float contrast = 0.f;   // 0 = keep backgroundGrade.contrast
			float saturation = 0.f; // 0 = keep backgroundGrade.saturation
		};
		void SetBackgroundLook(const BackgroundLook& look) { mLook = look; }

		// Energy draws are queued, tier-capped now and budget-culled in Compose (decorative first).
		void SubmitGlow(const GlowParams& glow);
		void SubmitTrail(const TrailParams& trail);
		// Custom additive energy draw (shield bubble, ring bursts): queued like glows, so it is budget-culled by tier.
		// area is the covered output area in pixels. The callback draws into the energy target (output pixels); the
		// caller keeps the brightness at or below TierBrightnessCap(tier).
		void SubmitEnergy(LuminanceTier tier, float area, std::function<void(sf::RenderTarget&)> draw);
		float TierBrightnessCap(LuminanceTier tier) const;
		// Full-screen additive pulse; scaled by settings.flashScale and capped.
		void SubmitFlash(sf::Color color, float strength);

		// target must have the size given to Resize.
		void Compose(sf::RenderTarget& target);

		const EnergyStats& Stats() const { return mStats; }
		sf::Vector2u OutputSize() const { return mOutputSize; }
		sf::Vector2u VirtualSize() const { return mVirtualSize; }
		// Output pixels per world-layer pixel.
		float WorldPixelScale() const { return mPixelScale; }

		// For a game camera view: the same view with its top-left corner snapped to the virtual pixel grid and its
		// width widened to the whole virtual texture. Set it on the World/Sprites targets so motion does not jitter,
		// and derive anything that must line up with those pixels (parallax, energy glows) from it, not from gameView.
		sf::View SnapWorldView(const sf::View& gameView) const;

	private:
		// Glows and trails are plain data so Compose can draw them in one batch each; everything else is a callback.
		// All energy draws are additive, so the batched order does not change the result.
		struct GlowInstance
		{
			sf::Vector2f position;
			float coreRadius = 0.f;
			float haloRadius = 0.f;
			sf::Glsl::Vec3 coreColor;
			sf::Glsl::Vec3 haloColor;
		};

		struct EnergyItem
		{
			enum class Kind { Custom, Glow, Trail };

			Kind kind = Kind::Custom;
			LuminanceTier tier = LuminanceTier::Decorative;
			float area = 0.f;
			GlowInstance glow;
			std::vector<sf::Vertex> triangles;
			std::function<void(sf::RenderTarget&)> draw;
		};

		struct BloomLevel
		{
			sf::RenderTexture ping;
			sf::RenderTexture pong;
		};

		bool EnsureTargets();
		void ResetWorldView();
		float TierScale(LuminanceTier tier, float peak);
		void FlushEnergy();
		void DrawGlowBatch(const std::vector<GlowInstance>& glows, std::size_t first, std::size_t count);
		const std::vector<sf::Glsl::Vec3>& DitherPalette();
		const std::vector<sf::Glsl::Vec3>& BackgroundPalette();
		void DrawBloom(sf::RenderTarget& target);
		void BlurLevel(BloomLevel& level);
		void DrawVignette(sf::RenderTarget& target);
		void LensBackground();

		VisualAssets& mAssets;
		sf::Vector2u mOutputSize;
		sf::Vector2u mVirtualSize;
		float mPixelScale = 1.f;
		int mBuiltVirtualHeight = 0;
		int mBuiltBloomLevels = 0;
		int mBuiltNebulaResolutionDivisor = 0;
		sf::Vector2u mBuiltOutputSize;
		sf::RenderTexture mBackground;
		// Scratch copy of the background for the lens; allocated only while a lens is enabled.
		std::unique_ptr<sf::RenderTexture> mLensScratch;
		LensParams mLens;
		BackgroundLook mLook;
		sf::RenderTexture mNebula;
		sf::RenderTexture mWorld;
		sf::RenderTexture mSprites;
		sf::RenderTexture mEnergy;
		std::vector<BloomLevel> mBloom;
		std::vector<EnergyItem> mEnergyQueue;
		// Scratch buffers reused every frame so the energy flush does not allocate.
		std::vector<GlowInstance> mGlowBatch;
		std::vector<sf::Vertex> mGlowVertices;
		std::vector<sf::Vertex> mTrailVertices;
		std::vector<sf::Glsl::Vec3> mDitherPalette;
		std::string mDitherPaletteScript;
		int mDitherPaletteAge = 0;
		// Palette of the stylized nebula composite (roles of backgroundStyle, capped to the background tier).
		std::vector<sf::Glsl::Vec3> mBackgroundPalette;
		std::string mBackgroundPaletteScript;
		std::vector<std::string> mBackgroundPaletteRoles;
		float mBackgroundPaletteCap = -1.f;
		int mBackgroundPaletteAge = 0;
		EnergyStats mStats;
		bool mNebulaTargetAvailable = false;
		bool mNebulaFrameReady = false;
		bool mNebulaAllocationWarningShown = false;
		sf::Color mFlashColor = sf::Color::White;
		float mFlash = 0.f;
	};
}

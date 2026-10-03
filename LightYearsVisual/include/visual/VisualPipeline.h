#pragma once

#include <functional>
#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/LuminanceTier.h"
#include "visual/VisualAssets.h"

namespace ly::visual
{
	enum class Layer
	{
		Background,
		// Procedural pixel content; palette-dithered when settings.ditherEnabled. Drawn first.
		World,
		// Pixel-scaled content that keeps its own colors (sprite actors). Same virtual resolution, drawn over World.
		Sprites,
		Energy
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

	// Owns the layer textures and composites them: background -> world (nearest upscale, optional dither)
	// -> sprites (nearest upscale, undithered) -> energy (additive) -> bloom -> flash -> vignette.
	// Coordinates are output pixels unless a layer is given a game view (see SnapWorldView).
	class VisualPipeline
	{
	public:
		explicit VisualPipeline(VisualAssets& assets);

		bool Resize(sf::Vector2u outputSize);
		void BeginFrame();
		sf::RenderTarget& Target(Layer layer);

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
		struct EnergyItem
		{
			LuminanceTier tier;
			float area;
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
		void DrawBloom(sf::RenderTarget& target);
		void BlurLevel(BloomLevel& level);
		void DrawVignette(sf::RenderTarget& target);

		VisualAssets& mAssets;
		sf::Vector2u mOutputSize;
		sf::Vector2u mVirtualSize;
		float mPixelScale = 1.f;
		int mBuiltVirtualHeight = 0;
		int mBuiltBloomLevels = 0;
		sf::Vector2u mBuiltOutputSize;
		sf::RenderTexture mBackground;
		sf::RenderTexture mWorld;
		sf::RenderTexture mSprites;
		sf::RenderTexture mEnergy;
		std::vector<BloomLevel> mBloom;
		std::vector<EnergyItem> mEnergyQueue;
		EnergyStats mStats;
		sf::Color mFlashColor = sf::Color::White;
		float mFlash = 0.f;
	};
}

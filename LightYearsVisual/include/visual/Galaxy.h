#pragma once

#include <SFML/Graphics.hpp>

#include "visual/Backdrop.h"
#include "visual/GalaxyModel.h"
#include "visual/PipelineSettings.h"
#include "visual/ShaderLibrary.h"

namespace ly::visual
{
	// The Milky Way as a long-exposure photograph sees it (galaxy.frag): starlight, resolved star dust, dust lanes and
	// H-alpha knots at full resolution. The band only moves by parallax, so it is rendered once per location into a
	// cache a little larger than the screen and then added onto the Background every frame.
	// Per-stop character of the band (deepsky.json "band"): its light families, its gas clouds and where the observer
	// looks. Defaults leave the band exactly as the physical model draws it.
	struct BandStyle
	{
		// 0 keeps the built-in colours, 1 uses the colours below.
		float tint = 0.f;
		sf::Glsl::Vec3 warm{1.f, 0.7f, 0.32f};      // old-star light along the spine
		sf::Glsl::Vec3 cool{0.35f, 0.58f, 1.f};     // young-star clouds on the flanks
		sf::Glsl::Vec3 cloud{1.f, 0.25f, 0.4f};     // the gas clouds' main line (H-alpha by default)
		sf::Glsl::Vec3 cloudAccent{0.25f, 0.92f, 0.85f}; // the hot hearts of the clouds ([O III] by default)
		float clouds = 1.f;       // amount of gas cloud in the band
		float cloudReach = 1.f;   // how far the clouds spread away from the plane, in band widths
		float bulge = 1.f;        // scale of the bulge swell (0 = looking away from the centre)
		float anticenter = 0.f;   // 1 = the whole band is the faint anticentre side
		// Shape and distance of the band, so it is not always the same diagonal seen from the same place:
		float curve = 1.f;        // arc of the band across the view (0 straight, 3 a strong arch)
		float taper = 0.f;        // perspective: < 0 narrows toward the centre end (the galaxy seen along its length),
		                          // > 0 toward the other end
		float starBoost = 1.f;    // resolved stars inside the band (deep in a star cloud there are many)

		bool operator==(const BandStyle& other) const;
		bool operator!=(const BandStyle& other) const { return !(*this == other); }
	};

	class GalaxyLayer
	{
	public:
		// Cache border on each side, in output pixels: the band's parallax drift is clamped to it.
		static constexpr unsigned CacheMargin = 64;

		struct Params
		{
			galaxy::SkyState sky;
			SkyPalette tint;
			MilkyWaySettings settings;
			BandStyle style;
			unsigned seed = 0;
			float maxBrightness = 0.6f;
			// Optional light rendered in the cache's own pixel frame (the stop's nebulae and haze, DeepSkyLayer): it is
			// added inside the band, so the band's dust lanes cross it. extraRevision changes whenever its content does.
			const sf::Texture* extraLight = nullptr;
			unsigned extraRevision = 0;
		};

		// scroll = camera motion in output pixels; settings.parallax is applied here.
		void Draw(ShaderLibrary& shaders, sf::RenderTarget& target, const Params& params, sf::Vector2f scroll);
		// Forces a re-render on the next Draw (for example after the galaxy shader was reloaded).
		void Invalidate() { mValid = false; }

	private:
		bool Render(sf::Shader& shader, const Params& params, sf::Vector2u outputSize);

		sf::RenderTexture mCache;
		bool mValid = false;
		sf::Vector2u mOutputSize;
		unsigned mSeed = 0;
		float mCap = 0.f;
		MilkyWaySettings mSettings;
		BandStyle mStyle;
		const sf::Texture* mExtraLight = nullptr;
		unsigned mExtraRevision = 0;
		const sf::Shader* mShader = nullptr;
	};

	// Sets galaxy.frag's band uniforms for params on a target of the given size (the cache, or a panorama); output height is the
	// screen's. Leaves the shader in the screen-band mode (uPanorama = 0).
	void SetBandUniforms(sf::Shader& shader, const GalaxyLayer::Params& params, sf::Vector2f size, float outputHeight);
}

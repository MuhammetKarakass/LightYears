#include "visual/Galaxy.h"

#include <algorithm>
#include <cmath>

#include "visual/Astro.h"
#include "visual/UniformWriter.h"

namespace ly::visual
{
	namespace
	{
		constexpr float kDegToRad = 3.14159265f / 180.f;
		constexpr unsigned kMargin = GalaxyLayer::CacheMargin;
		// How far the region colour script pulls the band's colours; the light itself stays physical.
		constexpr float kTintMix = 0.08f;

		sf::Glsl::Vec3 Display(astro::LinearRgb colour, float scale = 1.f)
		{
			const astro::Rgb8 rgb = astro::ToSrgb8(colour, scale);
			return {rgb.r / 255.f, rgb.g / 255.f, rgb.b / 255.f};
		}

		sf::Glsl::Vec3 Blackbody(float temperatureK, float scale = 1.f)
		{
			return Display(astro::BlackbodyColor(temperatureK), scale);
		}

		// Soft compression of a relative brightness: 1 stays about 0.8, large values approach 1.
		float Compress(float value)
		{
			return 1.f - std::exp(-1.6f * std::max(value, 0.f));
		}

		sf::Glsl::Vec3 Mix(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b, float k)
		{
			return {a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k, a.z + (b.z - a.z) * k};
		}

		// H-alpha with H-beta at 1/2.86 of its strength (Case B recombination): the rose pink of emission nebulae.
		sf::Glsl::Vec3 HAlphaPink()
		{
			const astro::LinearRgb a = astro::WavelengthColor(astro::HAlphaNm);
			const astro::LinearRgb b = astro::WavelengthColor(astro::HBetaNm);
			constexpr float kHBeta = 1.f / 2.86f;
			const astro::LinearRgb pink{a.r + b.r * kHBeta, a.g + b.g * kHBeta, a.b + b.b * kHBeta};
			const float peak = std::max({pink.r, pink.g, pink.b});
			return Display(pink, 1.f / std::max(peak, 1e-6f));
		}
	}

	bool BandStyle::operator==(const BandStyle& o) const
	{
		auto same = [](const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) { return a.x == b.x && a.y == b.y && a.z == b.z; };
		return tint == o.tint && same(warm, o.warm) && same(cool, o.cool) && same(cloud, o.cloud) && same(cloudAccent, o.cloudAccent)
			&& clouds == o.clouds && cloudReach == o.cloudReach && bulge == o.bulge && anticenter == o.anticenter && curve == o.curve
			&& taper == o.taper && starBoost == o.starBoost;
	}

	void SetBandUniforms(sf::Shader& shader, const GalaxyLayer::Params& params, sf::Vector2f size, float outputHeight)
	{
		const BandStyle& style = params.style;
		galaxy::SkyState sky = params.sky;
		// Looking away from the centre: no bulge, and both ends of the band are the faint anticentre light.
		sky.bulgeFraction *= std::max(style.bulge, 0.f);
		sky.bandCenterSide += (sky.bandAntiSide - sky.bandCenterSide) * std::clamp(style.anticenter, 0.f, 1.f);
		const MilkyWaySettings& settings = params.settings;

		const float angle = settings.angleDeg * kDegToRad;
		const sf::Vector2f normal(-std::sin(angle), std::cos(angle));
		const sf::Vector2f center = size * 0.5f + normal * (settings.offset * outputHeight);
		// In the bulge the band widens a little and the light per pixel drops, so stars carry it instead of a glow.
		const float width = settings.width * outputHeight * (1.f + 0.2f * sky.bulgeFraction);
		// Toward the centre the galaxy becomes the climax of the journey, so it brightens with the bulge instead of fading.
		const float gain = settings.intensity * (0.8f + 0.35f * sky.bulgeFraction * sky.bulgeFraction);
		// The model's dust is thin in the bulge, but the band must keep its lanes there, or it turns into a flat glow.
		const float dust = settings.dustLane * std::clamp(0.5f + 0.5f * sky.dustDensity, 0.85f, 1.3f);
		const float starDensity = std::clamp(1.f + 0.25f * std::log10(std::max(sky.starDensity, 1.f)), 1.f, 2.2f);

		// Integrated starlight colours: old disk light, young blue edges, red giants of the bulge, and the reddened
		// light at dust edges. The region palette pulls them a little toward the region's colours.
		const SkyPalette& tint = params.tint;
		// Art direction over the physics, the band's style is colour separation: gold and cream old-star light along the
		// spine, blue young-star clouds on the flanks, rose H-alpha and a little teal [O III] in the clouds.
		const float styleMix = std::clamp(style.tint, 0.f, 1.f);
		const sf::Glsl::Vec3 gold = Mix(Mix(sf::Glsl::Vec3(1.f, 0.7f, 0.32f), tint.lit, kTintMix), style.warm, styleMix);
		const sf::Glsl::Vec3 cream = Mix(Mix(sf::Glsl::Vec3(1.f, 0.95f, 0.88f), tint.lit, kTintMix), Mix(style.warm, sf::Glsl::Vec3(1.f, 1.f, 1.f), 0.65f), styleMix);
		const sf::Glsl::Vec3 blue = Mix(Mix(sf::Glsl::Vec3(0.35f, 0.58f, 1.f), tint.mid, kTintMix), style.cool, styleMix);
		const sf::Glsl::Vec3 teal = Mix(sf::Glsl::Vec3(0.25f, 0.92f, 0.85f), style.cloudAccent, styleMix);
		const sf::Glsl::Vec3 pink = Mix(HAlphaPink(), style.cloud, styleMix);

		{
			UniformWriter uniforms(shader);
			uniforms.Set("uSize", size);
			uniforms.Set("uCenter", center);
			uniforms.Set("uAngle", angle);
			uniforms.Set("uWidth", width);
			// Compress the model's band brightness (it grows by orders of magnitude toward the centre) into a range the
			// shader can shade without flattening into the cap.
			uniforms.Set("uCenterSide", gain * Compress(sky.bandCenterSide));
			uniforms.Set("uAntiSide", gain * Compress(sky.bandAntiSide));
			uniforms.Set("uBulge", sky.bulgeFraction);
			uniforms.Set("uArm", sky.armProximity);
			uniforms.Set("uDust", dust);
			uniforms.Set("uStarDensity", starDensity);
			uniforms.Set("uSeed", static_cast<float>(params.seed % 997u) * 0.731f);
			uniforms.Set("uCream", cream);
			uniforms.Set("uGold", gold);
			uniforms.Set("uTeal", teal);
			uniforms.Set("uBlue", blue);
			uniforms.Set("uPink", pink);
			uniforms.Set("uStyle", styleMix);
			uniforms.Set("uClouds", std::max(style.clouds, 0.f));
			uniforms.Set("uCloudReach", std::max(style.cloudReach, 0.1f));
			uniforms.Set("uExtraOn", params.extraLight != nullptr ? 1.f : 0.f);
			uniforms.Set("uCurve", style.curve);
			uniforms.Set("uTaper", std::clamp(style.taper, -0.95f, 0.95f));
			uniforms.Set("uStarBoost", std::max(style.starBoost, 0.f));
			uniforms.Set("uMaxBrightness", params.maxBrightness);
			uniforms.Set("uPanorama", 0.f);
		}
	}

	void GalaxyLayer::Draw(ShaderLibrary& shaders, sf::RenderTarget& target, const Params& params, sf::Vector2f scroll)
	{
		if (!params.settings.enabled)
		{
			return;
		}
		sf::Shader* shader = shaders.Find("galaxy");
		if (shader == nullptr)
		{
			return;
		}
		const sf::Vector2u outputSize = target.getSize();
		const bool stale = !mValid || mOutputSize != outputSize || mSeed != params.seed || mCap != params.maxBrightness
			|| mSettings != params.settings || mStyle != params.style || mExtraLight != params.extraLight
			|| mExtraRevision != params.extraRevision || mShader != shader;
		if (stale && !Render(*shader, params, outputSize))
		{
			return;
		}

		// The band is far away: it drifts by the parallax, clamped to the cache border.
		const float limit = static_cast<float>(kMargin);
		const sf::Vector2f drift(
			std::clamp(scroll.x * params.settings.parallax, -limit, limit),
			std::clamp(scroll.y * params.settings.parallax, -limit, limit));
		sf::Sprite sprite(mCache.getTexture());
		sprite.setPosition(sf::Vector2f(-limit, -limit) - drift);
		target.draw(sprite, sf::RenderStates(sf::BlendAdd));
	}

	bool GalaxyLayer::Render(sf::Shader& shader, const Params& params, sf::Vector2u outputSize)
	{
		const sf::Vector2u cacheSize(outputSize.x + 2 * kMargin, outputSize.y + 2 * kMargin);
		if (mCache.getSize() != cacheSize && !mCache.resize(cacheSize))
		{
			mValid = false;
			return false;
		}
		const sf::Vector2f size(cacheSize);
		SetBandUniforms(shader, params, size, static_cast<float>(outputSize.y));
		if (params.extraLight != nullptr)
		{
			// Bound once per location render, so the uniform's driver sync is not a per-frame cost.
			shader.setUniform("uExtra", *params.extraLight);
		}
		mCache.setView(sf::View(sf::FloatRect({0.f, 0.f}, size)));
		mCache.clear(sf::Color::Black);
		DrawShaderQuad(mCache, shader, sf::FloatRect({0.f, 0.f}, size), sf::BlendNone);
		mCache.display();

		mValid = true;
		mOutputSize = outputSize;
		mSeed = params.seed;
		mCap = params.maxBrightness;
		mSettings = params.settings;
		mStyle = params.style;
		mExtraLight = params.extraLight;
		mExtraRevision = params.extraRevision;
		mShader = &shader;
		return true;
	}
}

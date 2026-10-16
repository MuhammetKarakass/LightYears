#include "sky/VisualPipeline.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "sky/UniformWriter.h"

namespace vega::sky
{
	namespace
	{
		const sf::BlendMode Additive(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One);
		// Glows per draw call; must match MaxGlows in glow.frag (the index travels in the red vertex color channel).
		constexpr std::size_t GlowBatchSize = 32;

		// Order in which energy draws survive the budget: decorative goes first, the player never does.
		int SurvivalRank(LuminanceTier tier)
		{
			switch (tier)
			{
			case LuminanceTier::Player: return 0;
			case LuminanceTier::EnemyProjectile: return 1;
			case LuminanceTier::Environment: return 2;
			case LuminanceTier::Background: return 3;
			case LuminanceTier::Decorative: return 4;
			}
			return 4;
		}

		sf::Color ScaledColor(sf::Color color, float k)
		{
			auto channel = [k](std::uint8_t c) { return static_cast<std::uint8_t>(std::clamp(c * k, 0.f, 255.f)); };
			return sf::Color(channel(color.r), channel(color.g), channel(color.b), 255);
		}

		float Peak(sf::Color color, float intensity)
		{
			return std::max({color.r, color.g, color.b}) / 255.f * intensity;
		}

		void DrawTexture(sf::RenderTarget& target, const sf::Texture& texture, const sf::RenderStates& states)
		{
			sf::Sprite sprite(texture);
			sf::Vector2f textureSize(texture.getSize());
			sf::Vector2f targetSize(target.getSize());
			sprite.setScale({targetSize.x / textureSize.x, targetSize.y / textureSize.y});
			target.draw(sprite, states);
		}
	}

	VisualPipeline::VisualPipeline(VisualAssets& assets)
		: mAssets(assets)
	{
	}

	bool VisualPipeline::Resize(sf::Vector2u outputSize)
	{
		mOutputSize = outputSize;
		return EnsureTargets();
	}

	bool VisualPipeline::EnsureTargets()
	{
		if (mOutputSize.x == 0 || mOutputSize.y == 0)
		{
			mNebulaFrameReady = false;
			return false;
		}
		PipelineSettings& settings = mAssets.Settings();
		settings.nebulaResolutionDivisor = std::clamp(settings.nebulaResolutionDivisor, 1, 8);
		bool outputChanged = mBuiltOutputSize != mOutputSize;
		int virtualHeight = std::min(settings.virtualHeight, static_cast<int>(mOutputSize.y));
		if (outputChanged || settings.nebulaResolutionDivisor != mBuiltNebulaResolutionDivisor)
		{
			mNebulaTargetAvailable = false;
			mNebulaFrameReady = false;
			mBuiltNebulaResolutionDivisor = settings.nebulaResolutionDivisor;
			if (mBuiltNebulaResolutionDivisor > 1)
			{
				unsigned divisor = static_cast<unsigned>(mBuiltNebulaResolutionDivisor);
				sf::Vector2u nebulaSize{(mOutputSize.x - 1u) / divisor + 1u, (mOutputSize.y - 1u) / divisor + 1u};
				if (!mNebula.resize(nebulaSize))
				{
					if (!mNebulaAllocationWarningShown)
					{
						std::cerr << "VisualPipeline: cannot create nebula texture; using the background target\n";
						mNebulaAllocationWarningShown = true;
					}
				}
				else
				{
					mNebula.setSmooth(true);
					mNebulaTargetAvailable = true;
				}
			}
		}

		if (outputChanged)
		{
			if (!mBackground.resize(mOutputSize) || !mEnergy.resize(mOutputSize))
			{
				std::cerr << "VisualPipeline: cannot create layer textures\n";
				return false;
			}
			mEnergy.setSmooth(true);
		}
		if (outputChanged || virtualHeight != mBuiltVirtualHeight)
		{
			mPixelScale = static_cast<float>(mOutputSize.y) / static_cast<float>(virtualHeight);
			mVirtualSize = {static_cast<unsigned>(std::ceil(mOutputSize.x / mPixelScale)), static_cast<unsigned>(virtualHeight)};
			if (!mWorld.resize(mVirtualSize))
			{
				std::cerr << "VisualPipeline: cannot create the world texture\n";
				return false;
			}
			if (!mSprites.resize(mVirtualSize))
			{
				std::cerr << "VisualPipeline: cannot create the sprite texture\n";
				return false;
			}
			mWorld.setSmooth(false);
			mSprites.setSmooth(false);
			mBuiltVirtualHeight = virtualHeight;
		}
		int levels = settings.bloomEnabled ? settings.bloomLevels : 0;
		if (outputChanged || levels != mBuiltBloomLevels)
		{
			mBloom.clear();
			for (int i = 0; i < levels; ++i)
			{
				unsigned divisor = 2u << i;
				sf::Vector2u size(std::max(1u, mOutputSize.x / divisor), std::max(1u, mOutputSize.y / divisor));
				mBloom.emplace_back();
				if (!mBloom.back().ping.resize(size) || !mBloom.back().pong.resize(size))
				{
					std::cerr << "VisualPipeline: cannot create bloom textures\n";
					mBloom.clear();
					levels = 0;
					break;
				}
				mBloom.back().ping.setSmooth(true);
				mBloom.back().pong.setSmooth(true);
			}
			mBuiltBloomLevels = levels;
		}
		// The lens needs a same-size copy of the background; without a lens the memory is released.
		if (!mLens.enabled)
		{
			mLensScratch.reset();
		}
		else if (outputChanged || mLensScratch == nullptr)
		{
			mLensScratch = std::make_unique<sf::RenderTexture>();
			if (!mLensScratch->resize(mOutputSize))
			{
				std::cerr << "VisualPipeline: cannot create the lens texture\n";
				mLensScratch.reset();
			}
			else
			{
				mLensScratch->setSmooth(true);
			}
		}
		mBuiltOutputSize = mOutputSize;
		return true;
	}

	// The world layer is authored in output pixels; this view maps them onto the low-resolution texture.
	// A caller may replace the view during a frame (for example with a game camera); BeginFrame restores it.
	void VisualPipeline::ResetWorldView()
	{
		sf::Vector2f viewSize = sf::Vector2f(mVirtualSize) * mPixelScale;
		sf::View view(viewSize * 0.5f, viewSize);
		mWorld.setView(view);
		mSprites.setView(view);
	}

	sf::View VisualPipeline::SnapWorldView(const sf::View& gameView) const
	{
		const sf::Vector2f size = gameView.getSize();
		if (mVirtualSize.y == 0 || size.y <= 0.f || gameView.getRotation() != sf::degrees(0.f))
		{
			return gameView;
		}
		// game units per world-layer pixel
		const float unit = size.y / static_cast<float>(mVirtualSize.y);
		sf::Vector2f topLeft = gameView.getCenter() - size * 0.5f;
		topLeft = {std::round(topLeft.x / unit) * unit, std::round(topLeft.y / unit) * unit};
		const sf::Vector2f snappedSize = sf::Vector2f(mVirtualSize) * unit;
		return sf::View(topLeft + snappedSize * 0.5f, snappedSize);
	}

	void VisualPipeline::BeginFrame()
	{
		EnsureTargets();
		ResetWorldView();
		mBackground.clear(sf::Color::Black);
		mNebulaFrameReady = false;
		if (mOutputSize.x > 0 && mOutputSize.y > 0 && mBuiltNebulaResolutionDivisor > 1
			&& mNebulaTargetAvailable)
		{
			mNebula.setView(sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(mOutputSize))));
			mNebula.clear(sf::Color::Transparent);
			mNebulaFrameReady = true;
		}
		mWorld.clear(sf::Color::Transparent);
		mSprites.clear(sf::Color::Transparent);
		mEnergy.clear(sf::Color::Black);
		mEnergyQueue.clear();
		mStats = {};
		mFlash = 0.f;
	}

	sf::RenderTarget& VisualPipeline::Target(Layer layer)
	{
		switch (layer)
		{
		case Layer::Background: return mBackground;
		case Layer::World: return mWorld;
		case Layer::Sprites: return mSprites;
		case Layer::Energy: return mEnergy;
		case Layer::Nebula:
			if (mNebulaFrameReady && mNebulaTargetAvailable && mBuiltNebulaResolutionDivisor > 1)
			{
				return mNebula;
			}
			return mBackground;
		}
		return mWorld;
	}

	void VisualPipeline::CompositeNebula()
	{
		if (mBuiltNebulaResolutionDivisor <= 1 || !mNebulaTargetAvailable || !mNebulaFrameReady)
		{
			return;
		}
		mNebula.display();
		const sf::BlendMode premultiplied(
			sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha, sf::BlendMode::Equation::Add,
			sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha, sf::BlendMode::Equation::Add);
		const BackgroundStyleSettings& style = mAssets.Settings().backgroundStyle;
		sf::Shader* stylize = style.enabled ? mAssets.Shaders().Find("bg_stylize") : nullptr;
		if (stylize == nullptr)
		{
			DrawTexture(mBackground, mNebula.getTexture(), sf::RenderStates(premultiplied));
			return;
		}
		const std::vector<sf::Glsl::Vec3>& palette = BackgroundPalette();
		if (palette.size() < 2)
		{
			DrawTexture(mBackground, mNebula.getTexture(), sf::RenderStates(premultiplied));
			return;
		}
		// Style pixels follow the ship pixels unless the settings give their own size.
		const float pixel = std::max(style.pixelSize > 0.f ? style.pixelSize : mPixelScale, 1.f);
		stylize->setUniform("texture", sf::Shader::CurrentTexture);
		{
			UniformWriter uniforms(*stylize);
			uniforms.Set("uSize", sf::Vector2f(mOutputSize));
			uniforms.Set("uPixel", pixel);
			uniforms.Set("uLevels", static_cast<float>(style.toneLevels));
			uniforms.Set("uSpread", style.ditherSpread);
			uniforms.SetArray("uPalette", palette.data(), palette.size());
			uniforms.Set("uPaletteCount", static_cast<int>(palette.size()));
		}
		sf::RenderStates states(premultiplied);
		states.shader = stylize;
		DrawTexture(mBackground, mNebula.getTexture(), states);
	}

	void VisualPipeline::SetLens(const LensParams& lens)
	{
		mLens = lens;
		EnsureTargets();
	}

	// Remaps the finished background through the point-mass lens (lens.frag). The background is copied first, so the
	// shader reads the unlensed image and writes the result back into mBackground.
	void VisualPipeline::LensBackground()
	{
		sf::Shader* lens = mAssets.Shaders().Find("lens");
		if (lens == nullptr)
		{
			return;
		}
		sf::Vector2f outputSize(mOutputSize);
		sf::View fullView(sf::FloatRect({0.f, 0.f}, outputSize));
		mLensScratch->setView(fullView);
		DrawTexture(*mLensScratch, mBackground.getTexture(), sf::RenderStates(sf::BlendNone));
		mLensScratch->display();

		{
			UniformWriter uniforms(*lens);
			uniforms.Set("uSize", outputSize);
			// Render-texture draws are bottom-up (y flipped vs. top-down output pixels), so flip the lens centre's y.
				uniforms.Set("uCenter", sf::Vector2f(mLens.center.x, outputSize.y - mLens.center.y));
			uniforms.Set("uEinstein", mLens.einsteinRadius);
			uniforms.Set("uShadow", mLens.shadowRadius);
			uniforms.Set("uRingBrightness", mLens.ringBrightness);
			uniforms.Set("uCap", TierBrightnessCap(LuminanceTier::Background));
		}
		lens->setUniform("texture", sf::Shader::CurrentTexture);
		mBackground.setView(fullView);
		sf::RenderStates states(sf::BlendNone);
		states.shader = lens;
		DrawTexture(mBackground, mLensScratch->getTexture(), states);
		mBackground.display();
	}

	float VisualPipeline::TierScale(LuminanceTier tier, float peak)
	{
		float cap = mAssets.Settings().tierMaxBrightness[TierIndex(tier)];
		if (peak <= cap || peak <= 0.f)
		{
			return 1.f;
		}
		++mStats.clampedByTier;
		return cap / peak;
	}

	void VisualPipeline::SubmitGlow(const GlowParams& glow)
	{
		++mStats.submitted;
		sf::Color coreColor = LerpColor(glow.color, sf::Color::White, 0.55f);
		float scale = TierScale(glow.tier, Peak(coreColor, glow.intensity));
		sf::Color core = ScaledColor(coreColor, glow.intensity * scale);
		sf::Color halo = ScaledColor(glow.color, glow.intensity * scale);
		EnergyItem item;
		item.kind = EnergyItem::Kind::Glow;
		item.tier = glow.tier;
		item.area = 3.14159f * glow.haloRadius * glow.haloRadius;
		item.glow.position = glow.position;
		item.glow.coreRadius = glow.coreRadius;
		item.glow.haloRadius = glow.haloRadius;
		item.glow.coreColor = sf::Glsl::Vec3(core.r / 255.f, core.g / 255.f, core.b / 255.f);
		item.glow.haloColor = sf::Glsl::Vec3(halo.r / 255.f, halo.g / 255.f, halo.b / 255.f);
		mEnergyQueue.push_back(std::move(item));
	}

	void VisualPipeline::SubmitTrail(const TrailParams& trail)
	{
		size_t count = trail.points.size();
		if (count < 2)
		{
			return;
		}
		++mStats.submitted;
		sf::Color coreColor = LerpColor(trail.color, sf::Color::White, 0.35f);
		float scale = TierScale(trail.tier, Peak(coreColor, trail.intensity));
		float area = 0.f;

		// Three vertices per point (edge, center, edge): the center carries the light, the edges are dark.
		std::vector<sf::Vertex> ring;
		ring.reserve(count * 3);
		for (size_t i = 0; i < count; ++i)
		{
			float u = static_cast<float>(i) / static_cast<float>(count - 1);
			sf::Vector2f tangent = trail.points[std::min(i + 1, count - 1)] - trail.points[i == 0 ? 0 : i - 1];
			float length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
			sf::Vector2f normal = length > 0.f ? sf::Vector2f(-tangent.y / length, tangent.x / length) : sf::Vector2f(0.f, 1.f);
			float halfWidth = (trail.headWidth + (trail.tailWidth - trail.headWidth) * u) * 0.5f;
			float fade = std::pow(1.f - u, 1.5f);
			sf::Color lit = ScaledColor(coreColor, trail.intensity * scale * fade);
			sf::Vector2f p = trail.points[i];
			ring.push_back({p + normal * halfWidth, sf::Color::Black});
			ring.push_back({p, lit});
			ring.push_back({p - normal * halfWidth, sf::Color::Black});
			if (i > 0)
			{
				sf::Vector2f d = p - trail.points[i - 1];
				area += std::sqrt(d.x * d.x + d.y * d.y) * halfWidth * 2.f;
			}
		}
		std::vector<sf::Vertex> triangles;
		for (size_t i = 0; i + 1 < count; ++i)
		{
			const sf::Vertex* a = &ring[i * 3];
			const sf::Vertex* b = &ring[(i + 1) * 3];
			for (int side = 0; side < 2; ++side)
			{
				int edge = side * 2;
				triangles.push_back(a[edge]);
				triangles.push_back(a[1]);
				triangles.push_back(b[edge]);
				triangles.push_back(a[1]);
				triangles.push_back(b[1]);
				triangles.push_back(b[edge]);
			}
		}
		EnergyItem item;
		item.kind = EnergyItem::Kind::Trail;
		item.tier = trail.tier;
		item.area = area;
		item.triangles = std::move(triangles);
		mEnergyQueue.push_back(std::move(item));
	}

	void VisualPipeline::SubmitEnergy(LuminanceTier tier, float area, std::function<void(sf::RenderTarget&)> draw)
	{
		++mStats.submitted;
		EnergyItem item;
		item.tier = tier;
		item.area = area;
		item.draw = std::move(draw);
		mEnergyQueue.push_back(std::move(item));
	}

	float VisualPipeline::TierBrightnessCap(LuminanceTier tier) const
	{
		return mAssets.Settings().tierMaxBrightness[TierIndex(tier)];
	}

	void VisualPipeline::SubmitFlash(sf::Color color, float strength)
	{
		if (strength > mFlash)
		{
			mFlash = strength;
			mFlashColor = color;
		}
	}

	void VisualPipeline::FlushEnergy()
	{
		const PipelineSettings& settings = mAssets.Settings();
		std::stable_sort(mEnergyQueue.begin(), mEnergyQueue.end(), [](const EnergyItem& a, const EnergyItem& b)
		{
			return SurvivalRank(a.tier) < SurvivalRank(b.tier);
		});
		float screenArea = static_cast<float>(mOutputSize.x) * static_cast<float>(mOutputSize.y);
		float areaBudget = settings.maxEnergyAreaRatio * screenArea;
		float usedArea = 0.f;
		mGlowBatch.clear();
		mTrailVertices.clear();
		for (const EnergyItem& item : mEnergyQueue)
		{
			if (mStats.drawn >= settings.maxEnergyDraws || usedArea + item.area > areaBudget)
			{
				++mStats.culledByBudget;
				continue;
			}
			switch (item.kind)
			{
			case EnergyItem::Kind::Glow:
				mGlowBatch.push_back(item.glow);
				break;
			case EnergyItem::Kind::Trail:
				mTrailVertices.insert(mTrailVertices.end(), item.triangles.begin(), item.triangles.end());
				break;
			case EnergyItem::Kind::Custom:
				if (item.draw)
				{
					item.draw(mEnergy);
				}
				break;
			}
			usedArea += item.area;
			++mStats.drawn;
		}
		// Energy is additive, so batching glows and trails after the custom draws gives the same image with a
		// handful of draw calls instead of one per item.
		if (!mTrailVertices.empty())
		{
			mEnergy.draw(mTrailVertices.data(), mTrailVertices.size(), sf::PrimitiveType::Triangles, sf::RenderStates(Additive));
		}
		for (std::size_t first = 0; first < mGlowBatch.size(); first += GlowBatchSize)
		{
			DrawGlowBatch(mGlowBatch, first, std::min(GlowBatchSize, mGlowBatch.size() - first));
		}
		mStats.areaRatio = usedArea / screenArea;
		mEnergyQueue.clear();
	}

	// One draw call for up to GlowBatchSize glows: per-glow data sits in uniform arrays and every vertex carries its
	// glow index in the red color channel (see glow.frag).
	void VisualPipeline::DrawGlowBatch(const std::vector<GlowInstance>& glows, std::size_t first, std::size_t count)
	{
		sf::Shader* shader = mAssets.Shaders().Find("glow");
		if (shader == nullptr || count == 0)
		{
			return;
		}
		std::array<sf::Glsl::Vec4, GlowBatchSize> shapes{};
		std::array<sf::Glsl::Vec3, GlowBatchSize> coreColors{};
		std::array<sf::Glsl::Vec3, GlowBatchSize> haloColors{};
		mGlowVertices.clear();
		for (std::size_t i = 0; i < count; ++i)
		{
			const GlowInstance& glow = glows[first + i];
			shapes[i] = sf::Glsl::Vec4(glow.position.x, glow.position.y, glow.coreRadius, glow.haloRadius);
			coreColors[i] = glow.coreColor;
			haloColors[i] = glow.haloColor;
			const sf::Color index(static_cast<std::uint8_t>(i), 0, 0, 255);
			const float l = glow.position.x - glow.haloRadius;
			const float t = glow.position.y - glow.haloRadius;
			const float r = glow.position.x + glow.haloRadius;
			const float b = glow.position.y + glow.haloRadius;
			const sf::Vertex tl{{l, t}, index, {l, t}};
			const sf::Vertex tr{{r, t}, index, {r, t}};
			const sf::Vertex bl{{l, b}, index, {l, b}};
			const sf::Vertex br{{r, b}, index, {r, b}};
			for (const sf::Vertex& vertex : {tl, tr, bl, tr, br, bl})
			{
				mGlowVertices.push_back(vertex);
			}
		}
		{
			UniformWriter uniforms(*shader);
			uniforms.SetArray("uGlow", shapes.data(), GlowBatchSize);
			uniforms.SetArray("uCoreColor", coreColors.data(), GlowBatchSize);
			uniforms.SetArray("uHaloColor", haloColors.data(), GlowBatchSize);
		}
		sf::RenderStates states(Additive);
		states.shader = shader;
		mEnergy.draw(mGlowVertices.data(), mGlowVertices.size(), sf::PrimitiveType::Triangles, states);
	}

	// The dither palette is the same every frame unless the active script or its files change, so it is rebuilt about
	// once a second (and at once when the script switches) instead of on every composite.
	const std::vector<sf::Glsl::Vec3>& VisualPipeline::DitherPalette()
	{
		const std::string& script = mAssets.Script().Name();
		if (mDitherPalette.empty() || script != mDitherPaletteScript || ++mDitherPaletteAge >= 60)
		{
			mDitherPaletteScript = script;
			mDitherPaletteAge = 0;
			mDitherPalette.clear();
			// bg_ ramps belong to the nebula composite; the ships keep their own 64 colours.
			for (const sf::Color& c : mAssets.Script().Palette(64, "bg_"))
			{
				mDitherPalette.emplace_back(c.r / 255.f, c.g / 255.f, c.b / 255.f);
			}
		}
		return mDitherPalette;
	}

	// Same caching as DitherPalette, plus the settings it depends on: the role list and the background tier cap.
	// Every colour is limited to the cap, so the stylized nebula never exceeds the background tier.
	const std::vector<sf::Glsl::Vec3>& VisualPipeline::BackgroundPalette()
	{
		const ColorScript& script = mAssets.Script();
		const std::vector<std::string>& roles = mAssets.Settings().backgroundStyle.roles;
		const float cap = std::max(TierBrightnessCap(LuminanceTier::Background), 1e-4f);
		if (mBackgroundPalette.empty() || script.Name() != mBackgroundPaletteScript || roles != mBackgroundPaletteRoles
			|| cap != mBackgroundPaletteCap || ++mBackgroundPaletteAge >= 60)
		{
			mBackgroundPaletteScript = script.Name();
			mBackgroundPaletteRoles = roles;
			mBackgroundPaletteCap = cap;
			mBackgroundPaletteAge = 0;
			// The palest stop of each ramp (its last) is skipped: after the cap it turns neutral grey and washes out the hues.
			// Chroma is pushed out from luma by 1.5 and limited again, so the hue survives the brightness cap.
			auto boostChroma = [cap](sf::Color c)
			{
				const float luma = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
				auto channel = [luma](std::uint8_t v) { return static_cast<std::uint8_t>(std::clamp(luma + (v - luma) * 1.5f, 0.f, 255.f)); };
				return LimitBrightness(sf::Color(channel(c.r), channel(c.g), channel(c.b), c.a), cap);
			};
			std::vector<sf::Color> colors;
			for (const std::string& role : roles)
			{
				if (!script.HasRole(role))
				{
					continue;
				}
				const std::vector<ColorStop>& stops = script.Role(role).Stops();
				// A single-stop ramp keeps that stop so it still contributes.
				const std::size_t usable = stops.size() > 1 ? stops.size() - 1 : stops.size();
				for (std::size_t i = 0; i < usable; ++i)
				{
					const sf::Color c = boostChroma(LimitBrightness(stops[i].color, cap));
					if (std::find(colors.begin(), colors.end(), c) == colors.end())
					{
						colors.push_back(c);
					}
				}
			}
			// Same reduction as ColorScript::Palette: evenly spaced entries when there are too many.
			const size_t maxColors = 64;
			if (colors.size() > maxColors)
			{
				std::vector<sf::Color> reduced;
				for (size_t i = 0; i < maxColors; ++i)
				{
					reduced.push_back(colors[i * colors.size() / maxColors]);
				}
				colors = std::move(reduced);
			}
			mBackgroundPalette.clear();
			for (const sf::Color& c : colors)
			{
				mBackgroundPalette.emplace_back(c.r / 255.f, c.g / 255.f, c.b / 255.f);
			}
		}
		return mBackgroundPalette;
	}

	void VisualPipeline::Compose(sf::RenderTarget& target)
	{
		const PipelineSettings& settings = mAssets.Settings();
		FlushEnergy();
		mBackground.display();
		// The background is complete here (gradient, nebula, stars), so only it is lensed; world and sprites are not.
		if (mLens.enabled && mLensScratch != nullptr)
		{
			LensBackground();
		}
		mWorld.display();
		mSprites.display();
		mEnergy.display();

		sf::Vector2f outputSize(mOutputSize);
		target.setView(sf::View(sf::FloatRect({0.f, 0.f}, outputSize)));
		// The background is colour graded in this same draw. Its ceiling is the background tier cap.
		sf::RenderStates backgroundStates(sf::BlendNone);
		sf::Shader* grade = settings.backgroundGrade.enabled ? mAssets.Shaders().Find("bg_grade") : nullptr;
		if (grade != nullptr)
		{
			const BackgroundGradeSettings& g = settings.backgroundGrade;
			grade->setUniform("texture", sf::Shader::CurrentTexture);
			UniformWriter uniforms(*grade);
			uniforms.Set("uCap", std::max(TierBrightnessCap(LuminanceTier::Background) * mLook.capScale, 1e-4f));
			uniforms.Set("uContrast", mLook.contrast > 0.f ? mLook.contrast : g.contrast);
			uniforms.Set("uSaturation", mLook.saturation > 0.f ? mLook.saturation : g.saturation);
			uniforms.Set("uBlackPoint", g.blackPoint);
			uniforms.Set("uShadowTint", sf::Glsl::Vec3(g.shadowTint.x, g.shadowTint.y, g.shadowTint.z));
			uniforms.Set("uShadowStrength", g.shadowTintStrength);
			uniforms.Set("uHighlightTint", sf::Glsl::Vec3(g.highlightTint.x, g.highlightTint.y, g.highlightTint.z));
			uniforms.Set("uHighlightStrength", g.highlightTintStrength);
			backgroundStates.shader = grade;
		}
		DrawTexture(target, mBackground.getTexture(), backgroundStates);

		sf::RenderStates worldStates(sf::BlendAlpha);
		sf::Shader* dither = mAssets.Shaders().Find("world_dither");
		const std::vector<sf::Glsl::Vec3>& colors = settings.ditherEnabled && dither != nullptr ? DitherPalette() : mDitherPalette;
		if (settings.ditherEnabled && dither != nullptr && colors.size() >= 2)
		{
			dither->setUniform("texture", sf::Shader::CurrentTexture);
			UniformWriter uniforms(*dither);
			uniforms.Set("uTexSize", sf::Vector2f(mVirtualSize));
			uniforms.SetArray("uPalette", colors.data(), colors.size());
			uniforms.Set("uPaletteCount", static_cast<int>(colors.size()));
			uniforms.Set("uSpread", settings.ditherSpread);
			worldStates.shader = dither;
		}
		// The sprite is stretched by the integer-friendly world scale; the texture itself is sampled nearest.
		sf::Sprite world(mWorld.getTexture());
		world.setScale({mPixelScale, mPixelScale});
		target.draw(world, worldStates);

		// Sprites keep their own colors. The texture holds premultiplied color (SFML alpha blending onto a transparent
		// target), so it is composited premultiplied; plain alpha blending would darken soft edges twice.
		sf::Sprite sprites(mSprites.getTexture());
		sprites.setScale({mPixelScale, mPixelScale});
		target.draw(sprites, sf::RenderStates(sf::BlendMode(
			sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha, sf::BlendMode::Equation::Add,
			sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha, sf::BlendMode::Equation::Add)));

		// Through blit_scale so the black (unlit) majority of the energy layer is skipped; the result is the same
		// as a plain additive sprite.
		sf::Shader* energyBlit = mAssets.Shaders().Find("blit_scale");
		sf::RenderStates energyStates(Additive);
		if (energyBlit != nullptr)
		{
			energyBlit->setUniform("texture", sf::Shader::CurrentTexture);
			UniformWriter(*energyBlit).Set("uScale", 1.f);
			energyStates.shader = energyBlit;
		}
		DrawTexture(target, mEnergy.getTexture(), energyStates);
		DrawBloom(target);

		float flash = std::min(mFlash * settings.flashScale, 0.35f);
		if (flash > 0.f)
		{
			sf::RectangleShape rect(outputSize);
			rect.setFillColor(ScaledColor(mFlashColor, flash));
			target.draw(rect, sf::RenderStates(Additive));
		}
		DrawVignette(target);
	}

	void VisualPipeline::BlurLevel(BloomLevel& level)
	{
		sf::Shader* blur = mAssets.Shaders().Find("blur");
		if (blur == nullptr)
		{
			return;
		}
		sf::Vector2f size(level.ping.getSize());
		sf::RenderStates states(sf::BlendNone);
		states.shader = blur;
		blur->setUniform("texture", sf::Shader::CurrentTexture);

		UniformWriter(*blur).Set("uDir", sf::Vector2f(1.f / size.x, 0.f));
		level.pong.clear(sf::Color::Black);
		DrawTexture(level.pong, level.ping.getTexture(), states);
		level.pong.display();

		UniformWriter(*blur).Set("uDir", sf::Vector2f(0.f, 1.f / size.y));
		level.ping.clear(sf::Color::Black);
		DrawTexture(level.ping, level.pong.getTexture(), states);
		level.ping.display();
	}

	// Bloom comes from the energy layer only, so only intentional light blooms.
	void VisualPipeline::DrawBloom(sf::RenderTarget& target)
	{
		const PipelineSettings& settings = mAssets.Settings();
		sf::Shader* bright = mAssets.Shaders().Find("bright_pass");
		sf::Shader* blit = mAssets.Shaders().Find("blit_scale");
		if (mBloom.empty() || settings.bloomStrength <= 0.f || bright == nullptr || blit == nullptr)
		{
			return;
		}

		sf::RenderStates brightStates(sf::BlendNone);
		brightStates.shader = bright;
		bright->setUniform("texture", sf::Shader::CurrentTexture);
		UniformWriter(*bright).Set("uThreshold", settings.bloomThreshold);
		mBloom[0].ping.clear(sf::Color::Black);
		DrawTexture(mBloom[0].ping, mEnergy.getTexture(), brightStates);
		mBloom[0].ping.display();
		BlurLevel(mBloom[0]);

		sf::RenderStates copyStates(sf::BlendNone);
		copyStates.shader = blit;
		blit->setUniform("texture", sf::Shader::CurrentTexture);
		for (size_t i = 1; i < mBloom.size(); ++i)
		{
			UniformWriter(*blit).Set("uScale", 1.f);
			mBloom[i].ping.clear(sf::Color::Black);
			DrawTexture(mBloom[i].ping, mBloom[i - 1].ping.getTexture(), copyStates);
			mBloom[i].ping.display();
			BlurLevel(mBloom[i]);
		}

		sf::RenderStates addStates(Additive);
		addStates.shader = blit;
		for (size_t i = 0; i < mBloom.size(); ++i)
		{
			UniformWriter(*blit).Set("uScale", settings.bloomStrength / (1.f + 0.4f * static_cast<float>(i)));
			DrawTexture(target, mBloom[i].ping.getTexture(), addStates);
		}
	}

	void VisualPipeline::DrawVignette(sf::RenderTarget& target)
	{
		const PipelineSettings& settings = mAssets.Settings();
		sf::Shader* shader = mAssets.Shaders().Find("vignette");
		if (shader == nullptr || settings.vignetteStrength <= 0.f)
		{
			return;
		}
		sf::Vector2f size(mOutputSize);
		{
			UniformWriter uniforms(*shader);
			uniforms.Set("uResolution", size);
			uniforms.Set("uStrength", settings.vignetteStrength);
			uniforms.Set("uRadius", settings.vignetteRadius);
		}
		DrawShaderQuad(target, *shader, sf::FloatRect({0.f, 0.f}, size));
	}
}

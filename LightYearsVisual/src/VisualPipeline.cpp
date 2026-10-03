#include "visual/VisualPipeline.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace ly::visual
{
	namespace
	{
		const sf::BlendMode Additive(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One);

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
			return false;
		}
		const PipelineSettings& settings = mAssets.Settings();
		bool outputChanged = mBuiltOutputSize != mOutputSize;
		int virtualHeight = std::min(settings.virtualHeight, static_cast<int>(mOutputSize.y));

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
		}
		return mWorld;
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
		float area = 3.14159f * glow.haloRadius * glow.haloRadius;
		sf::Vector2f position = glow.position;
		float coreRadius = glow.coreRadius;
		float haloRadius = glow.haloRadius;

		mEnergyQueue.push_back({glow.tier, area, [this, position, coreRadius, haloRadius, core, halo](sf::RenderTarget& target)
		{
			sf::Shader* shader = mAssets.Shaders().Find("glow");
			if (shader == nullptr)
			{
				return;
			}
			shader->setUniform("uCenter", position);
			shader->setUniform("uCoreRadius", coreRadius);
			shader->setUniform("uHaloRadius", haloRadius);
			shader->setUniform("uCoreColor", sf::Glsl::Vec3(core.r / 255.f, core.g / 255.f, core.b / 255.f));
			shader->setUniform("uHaloColor", sf::Glsl::Vec3(halo.r / 255.f, halo.g / 255.f, halo.b / 255.f));
			sf::Vector2f extent(haloRadius, haloRadius);
			DrawShaderQuad(target, *shader, sf::FloatRect(position - extent, extent * 2.f), Additive);
		}});
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
		mEnergyQueue.push_back({trail.tier, area, [triangles = std::move(triangles)](sf::RenderTarget& target)
		{
			target.draw(triangles.data(), triangles.size(), sf::PrimitiveType::Triangles, sf::RenderStates(Additive));
		}});
	}

	void VisualPipeline::SubmitEnergy(LuminanceTier tier, float area, std::function<void(sf::RenderTarget&)> draw)
	{
		++mStats.submitted;
		mEnergyQueue.push_back({tier, area, std::move(draw)});
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
		for (const EnergyItem& item : mEnergyQueue)
		{
			if (mStats.drawn >= settings.maxEnergyDraws || usedArea + item.area > areaBudget)
			{
				++mStats.culledByBudget;
				continue;
			}
			item.draw(mEnergy);
			usedArea += item.area;
			++mStats.drawn;
		}
		mStats.areaRatio = usedArea / screenArea;
		mEnergyQueue.clear();
	}

	void VisualPipeline::Compose(sf::RenderTarget& target)
	{
		const PipelineSettings& settings = mAssets.Settings();
		FlushEnergy();
		mBackground.display();
		mWorld.display();
		mSprites.display();
		mEnergy.display();

		sf::Vector2f outputSize(mOutputSize);
		target.setView(sf::View(sf::FloatRect({0.f, 0.f}, outputSize)));
		DrawTexture(target, mBackground.getTexture(), sf::RenderStates(sf::BlendNone));

		sf::RenderStates worldStates(sf::BlendAlpha);
		sf::Shader* dither = mAssets.Shaders().Find("world_dither");
		std::vector<sf::Color> palette = mAssets.Script().Palette(64);
		if (settings.ditherEnabled && dither != nullptr && palette.size() >= 2)
		{
			std::vector<sf::Glsl::Vec3> colors;
			for (const sf::Color& c : palette)
			{
				colors.emplace_back(c.r / 255.f, c.g / 255.f, c.b / 255.f);
			}
			dither->setUniform("texture", sf::Shader::CurrentTexture);
			dither->setUniform("uTexSize", sf::Vector2f(mVirtualSize));
			dither->setUniformArray("uPalette", colors.data(), colors.size());
			dither->setUniform("uPaletteCount", static_cast<int>(colors.size()));
			dither->setUniform("uSpread", settings.ditherSpread);
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

		DrawTexture(target, mEnergy.getTexture(), sf::RenderStates(Additive));
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

		blur->setUniform("uDir", sf::Vector2f(1.f / size.x, 0.f));
		level.pong.clear(sf::Color::Black);
		DrawTexture(level.pong, level.ping.getTexture(), states);
		level.pong.display();

		blur->setUniform("uDir", sf::Vector2f(0.f, 1.f / size.y));
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
		bright->setUniform("uThreshold", settings.bloomThreshold);
		mBloom[0].ping.clear(sf::Color::Black);
		DrawTexture(mBloom[0].ping, mEnergy.getTexture(), brightStates);
		mBloom[0].ping.display();
		BlurLevel(mBloom[0]);

		sf::RenderStates copyStates(sf::BlendNone);
		copyStates.shader = blit;
		blit->setUniform("texture", sf::Shader::CurrentTexture);
		for (size_t i = 1; i < mBloom.size(); ++i)
		{
			blit->setUniform("uScale", 1.f);
			mBloom[i].ping.clear(sf::Color::Black);
			DrawTexture(mBloom[i].ping, mBloom[i - 1].ping.getTexture(), copyStates);
			mBloom[i].ping.display();
			BlurLevel(mBloom[i]);
		}

		sf::RenderStates addStates(Additive);
		addStates.shader = blit;
		for (size_t i = 0; i < mBloom.size(); ++i)
		{
			blit->setUniform("uScale", settings.bloomStrength / (1.f + 0.4f * static_cast<float>(i)));
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
		shader->setUniform("uResolution", size);
		shader->setUniform("uStrength", settings.vignetteStrength);
		shader->setUniform("uRadius", settings.vignetteRadius);
		DrawShaderQuad(target, *shader, sf::FloatRect({0.f, 0.f}, size));
	}
}

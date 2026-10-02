#include "presentation/visual/ProceduralBackdrop.h"

#include "framework/World.h"

#include <visual/LuminanceTier.h>

#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float NebulaFarParallax = 0.12f;
		constexpr float NebulaNearParallax = 0.25f;
		constexpr float StarScrollParallax = 0.5f;
	}

	ProceduralBackdrop::ProceduralBackdrop()
		: mStars(1337, 280)
	{
	}

	void ProceduralBackdrop::Draw(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const World& world, const sf::View& worldView, float timeSeconds)
	{
		const sf::Vector2f center = worldView.getCenter();
		if (mAnchoredWorld != &world)
		{
			mAnchoredWorld = &world;
			mCameraStart = center;
		}
		DrawBackground(assets, pipeline, center, timeSeconds);
		DrawPlanet(assets, pipeline, center, worldView.getSize().y, timeSeconds);
	}

	void ProceduralBackdrop::DrawBackground(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float timeSeconds)
	{
		using namespace visual;
		sf::RenderTarget& background = pipeline.Target(Layer::Background);
		const ColorScript& script = assets.Script();
		const float cap = assets.Settings().tierMaxBrightness[TierIndex(LuminanceTier::Background)];

		DrawGradient(background, script.Role("background").Sample(0.f), script.Role("background").Sample(0.85f), cap);

		NebulaParams nebulaFar;
		nebulaFar.scroll = cameraCenter * NebulaFarParallax;
		nebulaFar.scale = 0.0021f;
		nebulaFar.intensity = 0.8f;
		nebulaFar.seed = 1.f;
		nebulaFar.maxBrightness = cap;
		DrawNebula(assets.Shaders(), background, script.Role("ambient"), nebulaFar);

		NebulaParams nebulaNear = nebulaFar;
		nebulaNear.scroll = cameraCenter * NebulaNearParallax + sf::Vector2f(400.f, 150.f);
		nebulaNear.scale = 0.0032f;
		nebulaNear.intensity = 0.55f;
		nebulaNear.seed = 6.f;
		DrawNebula(assets.Shaders(), background, script.Role("ambient2"), nebulaNear);

		mStars.Draw(background, cameraCenter * StarScrollParallax, timeSeconds, script.Role("ambient"), cap);
	}

	void ProceduralBackdrop::DrawPlanet(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds)
	{
		const visual::PlanetSettings& settings = assets.Settings().planet;
		if (!settings.enabled || viewHeight <= 0.f)
		{
			return;
		}
		const sf::Vector2f output(pipeline.OutputSize());
		const float pixelScale = pipeline.WorldPixelScale();
		// output pixels per game unit, so the drift does not depend on the window size
		const float unitsToPixels = output.y / viewHeight;
		visual::PlanetParams planet;
		// Placed relative to the camera start and drifting opposite to the camera (a far layer moves less than a near
		// one). The center is snapped to the world-layer pixel grid so the limb does not shimmer.
		sf::Vector2f center = sf::Vector2f(output.x * settings.screenX, output.y * settings.screenY)
			- (cameraCenter - mCameraStart) * (settings.parallax * unitsToPixels);
		planet.center = { std::round(center.x / pixelScale) * pixelScale, std::round(center.y / pixelScale) * pixelScale };
		planet.radius = output.y * settings.radius;
		planet.brightness = settings.brightness;
		planet.saturation = settings.saturation;
		planet.dither = settings.dither;
		planet.pixelScale = pixelScale;
		planet.rotation = timeSeconds * settings.rotationSpeed;
		planet.light = { -0.7f, -0.45f, 0.55f };
		visual::DrawPlanet(assets.Shaders(), pipeline.Target(visual::Layer::World), assets.Script().Role("planet"), planet);
	}
}

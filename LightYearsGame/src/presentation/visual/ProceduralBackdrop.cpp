#include "presentation/visual/ProceduralBackdrop.h"

#include <visual/LuminanceTier.h>

namespace ly
{
	namespace
	{
		// World-space point the planet is anchored to (the center of the 6000x3000 test arena), and how much of the
		// camera motion it follows. Far layers move less than near ones, so the planet reads as very distant.
		const sf::Vector2f PlanetAnchor{ 3000.f, 1500.f };
		constexpr float PlanetParallax = 0.06f;
		// The planet is drawn in the pixel world layer, which has no tier cap, so it is dimmed to stay below gameplay.
		constexpr float PlanetBrightness = 0.7f;
		constexpr float NebulaFarParallax = 0.12f;
		constexpr float NebulaNearParallax = 0.25f;
		constexpr float StarScrollParallax = 0.5f;
	}

	ProceduralBackdrop::ProceduralBackdrop()
		: mStars(1337, 280)
	{
	}

	void ProceduralBackdrop::Draw(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const sf::View& worldView, float timeSeconds)
	{
		const sf::Vector2f center = worldView.getCenter();
		DrawBackground(assets, pipeline, center, timeSeconds);
		DrawPlanet(assets, pipeline, center, timeSeconds);
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

	void ProceduralBackdrop::DrawPlanet(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float timeSeconds)
	{
		const sf::Vector2f output(pipeline.OutputSize());
		visual::PlanetParams planet;
		// Upper right (the HUD lives at the bottom), cropped by the screen edges; drifts opposite to the camera.
		planet.center = sf::Vector2f(output.x * 0.88f, output.y * 0.16f) - (cameraCenter - PlanetAnchor) * PlanetParallax;
		planet.radius = output.y * 0.32f;
		planet.brightness = PlanetBrightness;
		planet.pixelScale = pipeline.WorldPixelScale();
		planet.rotation = timeSeconds * 0.04f;
		planet.light = { -0.7f, -0.45f, 0.55f };
		visual::DrawPlanet(assets.Shaders(), pipeline.Target(visual::Layer::World), assets.Script().Role("planet"), planet);
	}
}

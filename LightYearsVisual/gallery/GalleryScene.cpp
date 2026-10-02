#include "GalleryScene.h"

#include <cmath>
#include <random>

using namespace ly::visual;

namespace
{
	constexpr float ShipScale = 1.5f;

	// Ship hull in local units (nose up, 1 unit ~ 1 output pixel at 720p). Left side mirrors the right.
	const std::vector<sf::Vector2f> HullRight = {{0, -74}, {12, -44}, {16, -8}, {14, 40}, {8, 58}, {0, 58}};
	const std::vector<sf::Vector2f> WingRight = {{14, -6}, {54, 30}, {66, 64}, {40, 58}, {14, 40}};
	const std::vector<sf::Vector2f> Cockpit = {{0, -50}, {6, -30}, {0, -16}, {-6, -30}};
	const std::vector<sf::Vector2f> Nozzle = {{-9, 58}, {9, 58}, {7, 68}, {-7, 68}};

	std::vector<sf::Vector2f> Mirror(const std::vector<sf::Vector2f>& points)
	{
		std::vector<sf::Vector2f> mirrored;
		for (auto it = points.rbegin(); it != points.rend(); ++it)
		{
			mirrored.push_back({-it->x, it->y});
		}
		return mirrored;
	}
}

GalleryScene::GalleryScene(VisualAssets& assets, VisualPipeline& pipeline)
	: mAssets(assets)
	, mPipeline(pipeline)
	, mBrush(assets.Shaders())
	, mStars(1337, 280)
{
}

void GalleryScene::Update(float deltaSeconds)
{
	mTime += deltaSeconds;
	mScroll += sf::Vector2f(7.f, 3.f) * deltaSeconds;
}

void GalleryScene::Draw()
{
	DrawBackground();
	DrawWorld();
	DrawEnergy();
}

sf::Vector2f GalleryScene::ShipToScene(sf::Vector2f local) const
{
	sf::Vector2f size(mPipeline.OutputSize());
	float k = size.y / 720.f * ShipScale;
	float angle = 0.05f * std::sin(mTime * 0.9f);
	sf::Vector2f rotated(local.x * std::cos(angle) - local.y * std::sin(angle), local.x * std::sin(angle) + local.y * std::cos(angle));
	sf::Vector2f center(size.x * 0.56f, size.y * 0.40f + 7.f * k * std::sin(mTime * 1.3f));
	return center + rotated * k;
}

std::vector<sf::Vector2f> GalleryScene::ShipToScene(const std::vector<sf::Vector2f>& local) const
{
	std::vector<sf::Vector2f> scene;
	for (sf::Vector2f p : local)
	{
		scene.push_back(ShipToScene(p));
	}
	return scene;
}

void GalleryScene::DrawBackground()
{
	sf::RenderTarget& bg = mPipeline.Target(Layer::Background);
	const ColorScript& script = mAssets.Script();
	float cap = mAssets.Settings().tierMaxBrightness[TierIndex(LuminanceTier::Background)];

	DrawGradient(bg, script.Role("background").Sample(0.f), script.Role("background").Sample(0.85f), cap);

	NebulaParams far;
	far.scroll = mScroll * 0.25f;
	far.scale = 0.0021f;
	far.intensity = 0.8f;
	far.seed = 1.f;
	far.maxBrightness = cap;
	DrawNebula(mAssets.Shaders(), bg, script.Role("ambient"), far);

	NebulaParams near = far;
	near.scroll = mScroll * 0.55f + sf::Vector2f(400.f, 150.f);
	near.scale = 0.0032f;
	near.intensity = 0.55f;
	near.seed = 6.f;
	DrawNebula(mAssets.Shaders(), bg, script.Role("ambient2"), near);

	mStars.Draw(bg, mScroll, mTime, script.Role("ambient"), cap);
}

void GalleryScene::DrawWorld()
{
	sf::RenderTarget& world = mPipeline.Target(Layer::World);
	const ColorScript& script = mAssets.Script();
	sf::Vector2f size(mPipeline.OutputSize());
	float px = mPipeline.WorldPixelScale();

	// Off-screen-cropped giant ring arcs: scale reference and a hard-edged structure in the world layer.
	const ColorRamp& ambient = script.Role("ambient");
	ShapeStyle arc;
	arc.fill = ambient.Sample(0.5f);
	arc.outline = ambient.Sample(0.f);
	arc.outlineWidth = px;
	arc.hardEdges = true;
	sf::Vector2f arcCenter(size.x * 1.15f, size.y * 1.4f);
	mBrush.Arc(world, arcCenter, size.y * 1.25f, px * 5.f, 188.f, 74.f, arc);
	arc.fill = ambient.Sample(0.25f);
	mBrush.Arc(world, arcCenter, size.y * 1.25f + px * 14.f, px * 2.f, 205.f, 45.f, arc);

	// Procedural planet, lower left, cropped by nothing but its own limb.
	if (sf::Shader* planet = mAssets.Shaders().Find("planet"))
	{
		sf::Vector2f center(size.x * 0.2f, size.y * 0.72f);
		float radius = size.y * 0.27f;
		planet->setUniform("uCenter", center);
		planet->setUniform("uRadius", radius);
		planet->setUniform("uPixel", px);
		planet->setUniform("uLight", sf::Glsl::Vec3(0.75f, -0.45f, 0.5f));
		planet->setUniform("uRotation", mTime * 0.06f);
		planet->setUniform("uSeed", 3.f);
		planet->setUniform("uDither", 0.3f);
		SetRampUniform(*planet, "uRamp", script.Role("planet"), 5);
		DrawShaderQuad(world, *planet, sf::FloatRect(center - sf::Vector2f(radius, radius), sf::Vector2f(radius, radius) * 2.f));
	}

	// Ship: all outlines first so only the silhouette outline survives, then fills, then panel lines.
	const ColorRamp& hull = script.Role("hull");
	const ColorRamp& player = script.Role("player");
	sf::Color outline = hull.Sample(0.f);
	float outlineWidth = px * 2.f;
	std::vector<sf::Vector2f> hullR = ShipToScene(HullRight);
	std::vector<sf::Vector2f> hullL = ShipToScene(Mirror(HullRight));
	std::vector<sf::Vector2f> wingR = ShipToScene(WingRight);
	std::vector<sf::Vector2f> wingL = ShipToScene(Mirror(WingRight));
	for (const auto* part : {&hullR, &hullL, &wingR, &wingL})
	{
		mBrush.PolygonOutline(world, *part, outline, outlineWidth);
	}
	mBrush.PolygonOutline(world, ShipToScene(Nozzle), outline, outlineWidth);
	mBrush.Polygon(world, ShipToScene(Nozzle), hull.Sample(0.f));
	mBrush.Polygon(world, wingL, hull.Sample(0.25f));
	mBrush.Polygon(world, wingR, hull.Sample(0.5f));
	mBrush.Polygon(world, hullL, hull.Sample(0.5f));
	mBrush.Polygon(world, hullR, hull.Sample(0.75f));
	mBrush.Polygon(world, ShipToScene(Cockpit), player.Sample(0.85f));

	ShapeStyle panel;
	panel.fill = hull.Sample(0.f);
	panel.hardEdges = true;
	float lineRadius = px * 0.5f;
	mBrush.Capsule(world, ShipToScene({0.f, -12.f}), ShipToScene({0.f, 52.f}), lineRadius, panel);
	mBrush.Capsule(world, ShipToScene({22.f, 4.f}), ShipToScene({56.f, 40.f}), lineRadius, panel);
	mBrush.Capsule(world, ShipToScene({-22.f, 4.f}), ShipToScene({-56.f, 40.f}), lineRadius, panel);
	mBrush.Capsule(world, ShipToScene({9.f, -30.f}), ShipToScene({10.f, 30.f}), lineRadius, panel);
	mBrush.Capsule(world, ShipToScene({-9.f, -30.f}), ShipToScene({-10.f, 30.f}), lineRadius, panel);

	// Small SDF shape samples: filled circle with outline and a ring, both pixel-snapped.
	ShapeStyle dot;
	dot.fill = script.Role("combatWarm").Sample(0.45f);
	dot.outline = script.Role("combatWarm").Sample(0.f);
	dot.outlineWidth = px;
	dot.hardEdges = true;
	mBrush.Circle(world, {size.x * 0.88f, size.y * 0.82f}, size.y * 0.045f, dot);
	mBrush.Ring(world, {size.x * 0.88f, size.y * 0.82f}, size.y * 0.085f, px * 2.f, dot);
}

void GalleryScene::DrawEnergy()
{
	const ColorScript& script = mAssets.Script();
	sf::Vector2f size(mPipeline.OutputSize());
	float k = size.y / 720.f;
	float ks = k * ShipScale;

	const ColorRamp& player = script.Role("player");
	const ColorRamp& warm = script.Role("combatWarm");
	const ColorRamp& enemy = script.Role("enemy");
	const ColorRamp& ambient = script.Role("ambient");
	const ColorRamp& ambient2 = script.Role("ambient2");

	// Player engine: tapering trail plus the hottest glow on screen.
	TrailParams trail;
	for (int i = 0; i < 14; ++i)
	{
		float wobble = std::sin(mTime * 9.f + i * 0.8f) * 1.5f * static_cast<float>(i) / 13.f;
		trail.points.push_back(ShipToScene({wobble, 66.f + 12.f * static_cast<float>(i)}));
	}
	trail.headWidth = 20.f * ks;
	trail.tailWidth = 1.5f * ks;
	trail.color = player.Sample(0.8f);
	trail.intensity = 0.9f;
	trail.tier = LuminanceTier::Player;
	mPipeline.SubmitTrail(trail);

	GlowParams engine;
	engine.position = ShipToScene({0.f, 66.f});
	engine.coreRadius = 6.f * ks;
	engine.haloRadius = 48.f * ks;
	engine.color = player.Sample(0.8f);
	engine.tier = LuminanceTier::Player;
	mPipeline.SubmitGlow(engine);

	GlowParams light;
	light.coreRadius = 2.5f * ks;
	light.haloRadius = 16.f * ks;
	light.color = player.Sample(1.f);
	light.intensity = 0.8f;
	light.tier = LuminanceTier::Player;
	light.position = ShipToScene({64.f, 62.f});
	mPipeline.SubmitGlow(light);
	light.position = ShipToScene({-64.f, 62.f});
	mPipeline.SubmitGlow(light);

	// Enemy projectiles: warm, bright but never brighter than the player.
	for (int i = 0; i < 6; ++i)
	{
		float y = std::fmod(mTime * 150.f + static_cast<float>(i) * 137.f, size.y + 240.f) - 120.f;
		float x = size.x * (0.70f + 0.05f * static_cast<float>(i)) + 14.f * std::sin(mTime * 2.f + i);
		sf::Vector2f head(x, y);
		TrailParams tail;
		for (int p = 0; p < 7; ++p)
		{
			tail.points.push_back(head - sf::Vector2f(0.f, 11.f * k * static_cast<float>(p)));
		}
		tail.headWidth = 12.f * k;
		tail.tailWidth = 1.f * k;
		tail.color = (i % 2 == 0 ? warm : enemy).Sample(0.8f);
		tail.tier = LuminanceTier::EnemyProjectile;
		mPipeline.SubmitTrail(tail);

		GlowParams shot;
		shot.position = head;
		shot.coreRadius = 5.f * k;
		shot.haloRadius = 32.f * k;
		shot.color = tail.color;
		shot.tier = LuminanceTier::EnemyProjectile;
		mPipeline.SubmitGlow(shot);
	}

	// Decorative drifting motes, cool and low in the hierarchy.
	for (int i = 0; i < 12; ++i)
	{
		float a = mTime * (0.25f + 0.03f * static_cast<float>(i)) + static_cast<float>(i) * 1.7f;
		GlowParams mote;
		mote.position = {size.x * (0.5f + 0.42f * std::sin(a * 0.7f + i)), size.y * (0.5f + 0.38f * std::cos(a * 0.5f + i * 2.f))};
		mote.coreRadius = 2.5f * k;
		mote.haloRadius = 34.f * k;
		mote.color = (i % 2 == 0 ? ambient : ambient2).Sample(0.9f);
		mote.intensity = 0.9f;
		mote.tier = LuminanceTier::Decorative;
		mPipeline.SubmitGlow(mote);
	}

	// Environment light: faint warm rim where the planet catches the sun.
	GlowParams rim;
	rim.position = {size.x * 0.2f + size.y * 0.27f * 0.8f, size.y * 0.72f - size.y * 0.27f * 0.5f};
	rim.coreRadius = 1.f;
	rim.haloRadius = size.y * 0.16f;
	rim.color = script.Role("planet").Sample(0.85f);
	rim.intensity = 0.7f;
	rim.tier = LuminanceTier::Environment;
	mPipeline.SubmitGlow(rim);

	if (mStress)
	{
		// Budget demo: far more decorative glows than the energy budget allows. They are culled first.
		std::mt19937 rng(7);
		std::uniform_real_distribution<float> unit(0.f, 1.f);
		for (int i = 0; i < 160; ++i)
		{
			GlowParams spam;
			spam.position = {unit(rng) * size.x, unit(rng) * size.y};
			spam.coreRadius = 3.f * k;
			spam.haloRadius = 44.f * k;
			spam.color = ambient2.Sample(0.9f);
			spam.tier = LuminanceTier::Decorative;
			mPipeline.SubmitGlow(spam);
		}
	}
}

#include "GalleryScene.h"

#include <algorithm>
#include <cmath>
#include <random>

using namespace ly::visual;

GalleryScene::GalleryScene(VisualAssets& assets, VisualPipeline& pipeline)
	: mAssets(assets)
	, mPipeline(pipeline)
	, mBrush(assets.Shaders())
	, mShips(assets)
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
	// The showcase draws far more ships than a game frame does, so lift the energy budget it would otherwise hit.
	if (!mStress)
	{
		mAssets.Settings().maxEnergyDraws = std::max(mAssets.Settings().maxEnergyDraws, 400);
		mAssets.Settings().maxEnergyAreaRatio = std::max(mAssets.Settings().maxEnergyAreaRatio, 4.f);
	}
	DrawBackground();
	DrawWorld();
	DrawShips();
	DrawEnergy();
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

	// A hard-edged arc as scale reference, and the procedural planet, both far behind the ships.
	const ColorRamp& ambient = script.Role("ambient");
	ShapeStyle arc;
	arc.fill = ambient.Sample(0.5f);
	arc.outline = ambient.Sample(0.f);
	arc.outlineWidth = px;
	arc.hardEdges = true;
	sf::Vector2f arcCenter(size.x * 1.15f, size.y * 1.4f);
	mBrush.Arc(world, arcCenter, size.y * 1.25f, px * 5.f, 188.f, 74.f, arc);

	PlanetParams planet;
	planet.center = {size.x * 0.1f, size.y * 0.95f};
	planet.radius = size.y * 0.2f;
	planet.pixelScale = px;
	planet.rotation = mTime * 0.06f;
	DrawPlanet(mAssets.Shaders(), world, script.Role("planet"), planet);
}

namespace
{
	// state: 0 unequipped, 1 cooldown, 2 ready, 3 active
	ShipPortState Port(AbilityCategory category, int state, float progress = 0.f)
	{
		ShipPortState port;
		port.equipped = state != 0;
		port.category = category;
		port.isReady = state == 2;
		port.isActive = state == 3;
		port.cooldownFraction = state == 1 ? progress : 1.f;
		return port;
	}
}

void GalleryScene::DrawShips()
{
	sf::Vector2f size(mPipeline.OutputSize());
	const float k = size.y / 720.f;
	// Roster order first, then anything else the ships folder holds.
	std::vector<std::string> ids;
	for (const char* wanted : {"fighter", "breacher", "interceptor", "conductor", "aegis", "cryo_controller"})
	{
		if (mAssets.Ships().Find(wanted) != nullptr)
		{
			ids.push_back(wanted);
		}
	}
	for (const std::string& id : mAssets.Ships().Ids())
	{
		if (std::find(ids.begin(), ids.end(), id) == ids.end())
		{
			ids.push_back(id);
		}
	}
	if (ids.empty())
	{
		return;
	}
	const float fullScale = 0.95f * k;
	// The game draws a ship at about 0.5 output pixels per unit on a 1080p screen; the bottom strip shows that size.
	const float gameScale = 0.34f * k;

	auto baseState = [&](int index, int count, float rowY, float scale)
	{
		ShipVisualState state;
		state.position = {size.x * (static_cast<float>(index) + 0.5f) / static_cast<float>(count), size.y * rowY};
		state.scale = scale;
		state.time = mTime;
		state.seed = 1u + static_cast<unsigned>(index) * 7919u;
		state.shieldFraction = 1.f;
		return state;
	};
	auto draw = [&](const std::string& id, const ShipVisualState& state)
	{
		if (const ShipVisualDefinition* definition = mAssets.Ships().Find(id))
		{
			mShips.Draw(mPipeline, *definition, state);
		}
	};
	// A short hit now and then on one ship of the first row, so the flash and the shield ripple show up in captures.
	const float hitPhase = std::fmod(mTime, 1.6f);
	const float hit = hitPhase < 0.3f ? 1.f - hitPhase / 0.3f : 0.f;

	// Row 1: every ship definition, thrusting, shields up, one in afterburner.
	const int count = static_cast<int>(ids.size());
	for (int i = 0; i < count; ++i)
	{
		ShipVisualState state = baseState(i, count, 0.17f, fullScale);
		state.thrust = 0.35f + 0.3f * std::sin(mTime * 0.8f + static_cast<float>(i));
		state.afterburner = i == 1;
		state.shieldFraction = i == 3 ? 0.35f : 1.f;
		if (i == static_cast<int>(mTime / 1.6f) % count)
		{
			state.hitFlash = hit;
			state.shieldHit = hit;
			state.shieldHitAngle = 0.8f * static_cast<float>(i);
		}
		for (int p = 0; p < ShipPortCount; ++p)
		{
			state.ports[p] = Port(static_cast<AbilityCategory>((i + p) % 5), 2);
		}
		draw(ids[static_cast<size_t>(i)], state);
	}

	// Row 2: the first ship at each damage stage (no shield), then mixed port states on two more ships.
	const float hull[4] = {1.f, 0.65f, 0.4f, 0.15f};
	for (int i = 0; i < 4; ++i)
	{
		ShipVisualState state = baseState(i, 6, 0.45f, fullScale);
		state.hullFraction = hull[i];
		state.shieldFraction = 0.f;
		state.thrust = 0.4f;
		draw(ids[0], state);
	}
	if (ids.size() > 4)
	{
		ShipVisualState state = baseState(4, 6, 0.45f, fullScale);
		state.shieldFraction = 0.6f;
		state.ports[0] = Port(AbilityCategory::Offense, 0);
		state.ports[1] = Port(AbilityCategory::Defense, 1, std::fmod(mTime * 0.25f, 1.f));
		state.ports[2] = Port(AbilityCategory::Movement, 2);
		state.ports[3] = Port(AbilityCategory::Utility, 3);
		draw(ids[3], state);

		state = baseState(5, 6, 0.45f, fullScale);
		const float phase = std::fmod(mTime, 2.f);
		const float burst = phase < 0.4f ? 1.f - phase / 0.4f : 0.f;
		state.ports[0] = Port(AbilityCategory::Primary, 2);
		state.ports[1] = Port(AbilityCategory::Offense, 2);
		state.ports[2] = Port(AbilityCategory::Offense, 1, 0.8f);
		state.ports[3] = Port(AbilityCategory::Defense, 2);
		state.ports[1].justActivated = burst;
		state.ports[1].isActive = burst > 0.f;
		draw(ids[4], state);
	}

	// Row 3: game-size strip, slightly rotated, alternating damage stages.
	for (int i = 0; i < count; ++i)
	{
		ShipVisualState state = baseState(i, count, 0.78f, gameScale);
		state.rotationDegrees = 25.f * std::sin(mTime * 0.5f + static_cast<float>(i));
		state.thrust = 0.5f;
		state.hullFraction = i % 2 == 0 ? 1.f : 0.45f;
		for (int p = 0; p < ShipPortCount; ++p)
		{
			state.ports[p] = Port(static_cast<AbilityCategory>((i + p) % 5), p == 1 ? 1 : 2, 0.5f);
		}
		draw(ids[static_cast<size_t>(i)], state);
	}
}

void GalleryScene::DrawEnergy()
{
	const ColorScript& script = mAssets.Script();
	sf::Vector2f size(mPipeline.OutputSize());
	float k = size.y / 720.f;

	const ColorRamp& warm = script.Role("combatWarm");
	const ColorRamp& enemy = script.Role("enemy");
	const ColorRamp& ambient = script.Role("ambient");
	const ColorRamp& ambient2 = script.Role("ambient2");

	// Enemy projectiles on the edges: warm, bright but never brighter than the player.
	for (int i = 0; i < 4; ++i)
	{
		float y = std::fmod(mTime * 150.f + static_cast<float>(i) * 137.f, size.y + 240.f) - 120.f;
		float x = size.x * (0.015f + 0.012f * static_cast<float>(i)) + 6.f * std::sin(mTime * 2.f + i);
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
	for (int i = 0; i < 10; ++i)
	{
		float a = mTime * (0.25f + 0.03f * static_cast<float>(i)) + static_cast<float>(i) * 1.7f;
		GlowParams mote;
		mote.position = {size.x * (0.5f + 0.45f * std::sin(a * 0.7f + i)), size.y * (0.5f + 0.45f * std::cos(a * 0.5f + i * 2.f))};
		mote.coreRadius = 2.5f * k;
		mote.haloRadius = 34.f * k;
		mote.color = (i % 2 == 0 ? ambient : ambient2).Sample(0.9f);
		mote.intensity = 0.9f;
		mote.tier = LuminanceTier::Decorative;
		mPipeline.SubmitGlow(mote);
	}

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

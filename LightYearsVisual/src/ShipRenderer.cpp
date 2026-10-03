#include "visual/ShipRenderer.h"

#include <algorithm>
#include <cmath>

namespace ly::visual
{
	namespace
	{
		constexpr float Pi = 3.14159265358979f;
		const sf::BlendMode Additive(sf::BlendMode::Factor::One, sf::BlendMode::Factor::One);

		unsigned Mix(unsigned x)
		{
			x ^= x >> 16;
			x *= 0x7feb352dU;
			x ^= x >> 15;
			x *= 0x846ca68bU;
			x ^= x >> 16;
			return x;
		}

		// Deterministic 0..1 from two integers.
		float Rand01(unsigned a, unsigned b = 0)
		{
			return static_cast<float>(Mix(a * 747796405u + Mix(b + 0x9e3779b9u)) & 0xFFFFFFu) / 16777216.f;
		}

		float Length(sf::Vector2f v)
		{
			return std::sqrt(v.x * v.x + v.y * v.y);
		}

		sf::Color StopColor(const ColorRamp& ramp, int index)
		{
			const auto& stops = ramp.Stops();
			if (stops.empty())
			{
				return sf::Color::Magenta;
			}
			return stops[static_cast<size_t>(std::clamp(index, 0, static_cast<int>(stops.size()) - 1))].color;
		}

		int StopCount(const ColorRamp& ramp)
		{
			return static_cast<int>(ramp.Stops().size());
		}

		// Local ship units -> output pixels.
		struct Transform
		{
			sf::Vector2f origin;
			float c = 1.f;
			float s = 0.f;
			float k = 1.f;

			sf::Vector2f operator()(sf::Vector2f p) const
			{
				return origin + sf::Vector2f(p.x * c - p.y * s, p.x * s + p.y * c) * k;
			}

			sf::Vector2f Direction(sf::Vector2f d) const
			{
				return sf::Vector2f(d.x * c - d.y * s, d.x * s + d.y * c);
			}
		};

		sf::Color Scaled(sf::Color color, float k)
		{
			auto channel = [k](std::uint8_t v) { return static_cast<std::uint8_t>(std::clamp(v * k, 0.f, 255.f)); };
			return sf::Color(channel(color.r), channel(color.g), channel(color.b), color.a);
		}
	}

	int ShipRenderer::DamageStage(float hullFraction)
	{
		if (hullFraction > 0.75f) return 0;
		if (hullFraction > 0.5f) return 1;
		if (hullFraction > 0.25f) return 2;
		return 3;
	}

	ShipRenderer::ShipRenderer(VisualAssets& assets)
		: mAssets(assets)
		, mBrush(assets.Shaders())
	{
	}

	void ShipRenderer::Draw(VisualPipeline& pipeline, const ShipVisualDefinition& def, const ShipVisualState& state)
	{
		const ColorScript& script = mAssets.Script();
		const float px = pipeline.WorldPixelScale();
		const int stage = DamageStage(state.hullFraction);
		const float time = state.time;
		const unsigned seed = state.seed;

		const float angle = state.rotationDegrees * Pi / 180.f;
		Transform xf;
		xf.c = std::cos(angle);
		xf.s = std::sin(angle);
		xf.k = state.scale * def.scale;
		// Pixel-snapped center so the hull does not crawl against the virtual pixel grid.
		xf.origin = {std::round(state.position.x / px) * px, std::round(state.position.y / px) * px};

		const ColorRamp& hullRamp = script.Role(def.hullRole);
		const ColorRamp& plateRamp = script.Role(def.plateRole);
		const ColorRamp& accentRamp = script.Role(def.accentRole);
		const ColorRamp& glassRamp = script.Role(def.glassRole);
		const sf::Color outlineColor = StopColor(hullRamp, 0);
		// Level of detail by on-screen size (world-layer pixels across the larger dimension): small ships drop the
		// dark gaps and panel lines, which would eat a ship only 20 pixels wide.
		const float shipPixels = std::max(def.size.x, def.size.y) * xf.k / px;
		const bool showGaps = shipPixels >= 34.f;
		const bool showLines = shipPixels >= 28.f;

		// Hit flash lifts every stop; heavy damage darkens plates (and then hulls).
		const int flashBoost = state.hitFlash > 0.55f ? 3 : (state.hitFlash > 0.25f ? 1 : 0);
		auto partColor = [&](PartRole role, int shade, bool darkSide) -> sf::Color
		{
			int base = 0;
			const ColorRamp* ramp = &hullRamp;
			switch (role)
			{
			case PartRole::Hull: base = 2; break;
			case PartRole::Plate: base = 3; ramp = &plateRamp; break;
			case PartRole::Dark: base = 0; break;
			case PartRole::Accent: base = 2; ramp = &accentRamp; break;
			case PartRole::Trim: base = 3; ramp = &accentRamp; break;
			case PartRole::Glass: base = 3; ramp = &glassRamp; break;
			}
			int stop = base + shade + (role == PartRole::Dark ? 0 : flashBoost);
			if (role == PartRole::Plate && stage >= 1) stop -= 1;
			if (role == PartRole::Hull && stage >= 3) stop -= 1;
			if (darkSide && (role == PartRole::Hull || role == PartRole::Plate || role == PartRole::Accent)) stop -= 1;
			return StopColor(*ramp, std::clamp(stop, 0, StopCount(*ramp) - 1));
		};
		auto missing = [&](const ShipPart& part) { return part.detachStage > 0 && stage >= part.detachStage; };

		// Parts that are still on the ship, with their output-pixel polygons.
		struct Visible
		{
			const ShipPart* part;
			std::vector<sf::Vector2f> points;
			// A detached plate leaves a dark scar (a smaller hole) where it was.
			bool scar = false;
		};
		std::vector<Visible> visible;
		for (const ShipPart& part : def.parts)
		{
			const bool gone = missing(part);
			if (gone && !part.crackable)
			{
				continue;
			}
			Visible item{&part, {}, gone};
			for (sf::Vector2f p : part.points)
			{
				item.points.push_back(xf(gone ? part.centroid + (p - part.centroid) * 0.75f : p));
			}
			visible.push_back(std::move(item));
		}

		sf::RenderTarget& world = pipeline.Target(Layer::World);
		// Spawn protection: the hull flickers, everything else (engines, ports) stays.
		const bool hullShown = !state.ghost || std::fmod(time * 5.f, 1.f) < 0.6f;

		// Smoke first, under the hull, so it never hides the ship.
		if (stage >= 3)
		{
			ShapeStyle puff;
			puff.hardEdges = true;
			puff.fill = StopColor(hullRamp, 0);
			puff.outline = StopColor(hullRamp, 1);
			puff.outlineWidth = px;
			const int count = 3;
			for (int j = 0; j < count; ++j)
			{
				const float age = std::fmod(time * 0.5f + static_cast<float>(j) / count + Rand01(seed, j), 1.f);
				sf::Vector2f origin = def.smokeOrigin;
				if (!def.parts.empty())
				{
					const ShipPart& source = def.parts[Mix(seed + j * 31u) % def.parts.size()];
					if (!missing(source) && source.crackable)
					{
						origin = source.centroid;
					}
				}
				const sf::Vector2f local = origin + sf::Vector2f(std::sin(time * 1.3f + j * 2.1f) * 4.f * age, 6.f + age * 34.f);
				const float radius = px * (0.9f + 2.2f * age) * (1.f - 0.55f * age * age);
				mBrush.Circle(world, xf(local), radius, puff);
			}
		}

		if (hullShown)
		{
			// Silhouette outline first: only the outer half of each outline survives the fills.
			for (const Visible& item : visible)
			{
				if (!item.scar)
				{
					mBrush.PolygonOutline(world, item.points, outlineColor, px * 2.f);
				}
			}
			for (const Visible& item : visible)
			{
				const ShipPart& part = *item.part;
				if (item.scar)
				{
					mBrush.Polygon(world, item.points, outlineColor);
					continue;
				}
				if (part.gap && showGaps)
				{
					mBrush.PolygonOutline(world, item.points, outlineColor, px * 2.f);
				}
				const bool darkSide = part.paired && !part.mirrored;
				mBrush.Polygon(world, item.points, partColor(part.role, part.shade, darkSide));
			}

			// Panel lines follow the hull.
			ShapeStyle lineStyle;
			lineStyle.hardEdges = true;
			for (const ShipLine& line : def.lines)
			{
				if (!showLines)
				{
					break;
				}
				if (!line.part.empty())
				{
					auto owner = std::find_if(def.parts.begin(), def.parts.end(), [&](const ShipPart& p) { return p.name == line.part; });
					if (owner != def.parts.end() && missing(*owner))
					{
						continue;
					}
				}
				lineStyle.fill = partColor(line.role, 0, false);
				mBrush.Capsule(world, xf(line.a), xf(line.b), px * 0.5f, lineStyle);
			}

			// Cracks: deterministic per part seed and ship seed; more parts and more cracks per stage.
			if (stage >= 1)
			{
				static const float crackChance[4] = {0.f, 0.45f, 0.75f, 1.f};
				bool forcedFirst = false;
				lineStyle.fill = outlineColor;
				for (const Visible& item : visible)
				{
					const ShipPart& part = *item.part;
					if (item.scar)
					{
						continue;
					}
					sf::Vector2f lo = item.points.front();
					sf::Vector2f hi = lo;
					for (sf::Vector2f p : item.points)
					{
						lo = {std::min(lo.x, p.x), std::min(lo.y, p.y)};
						hi = {std::max(hi.x, p.x), std::max(hi.y, p.y)};
					}
					if (!part.crackable || Length(hi - lo) < px * 6.f)
					{
						continue;
					}
					const unsigned s = part.seed ^ (seed * 2654435761u);
					const bool crack = !forcedFirst || Rand01(s) < crackChance[stage];
					if (!crack)
					{
						continue;
					}
					forcedFirst = true;
					const int cracks = 1 + (stage >= 3 ? 1 : 0) + (stage >= 2 && Rand01(s, 7) > 0.5f ? 1 : 0);
					const size_t n = part.points.size();
					for (int c = 0; c < cracks; ++c)
					{
						const sf::Vector2f center = part.centroid;
						const sf::Vector2f dir = part.points[(Mix(s + c * 17u) % n)] - center;
						const sf::Vector2f perp(-dir.y, dir.x);
						const sf::Vector2f start = center + dir * 0.95f;
						const sf::Vector2f mid = center + dir * 0.55f + perp * ((Rand01(s, 10 + c) - 0.5f) * 0.5f);
						const sf::Vector2f end = center + dir * (0.05f + 0.25f * Rand01(s, 20 + c)) + perp * ((Rand01(s, 30 + c) - 0.5f) * 0.3f);
						mBrush.Capsule(world, xf(start), xf(mid), px * 0.5f, lineStyle);
						mBrush.Capsule(world, xf(mid), xf(end), px * 0.5f, lineStyle);
					}
				}
			}

			// Accent lights (hull pixels). Heavy damage makes them flicker.
			for (size_t i = 0; i < def.lights.size(); ++i)
			{
				const ShipLight& light = def.lights[i];
				bool on = light.blink <= 0.f || std::fmod(time * light.blink + Rand01(seed, static_cast<unsigned>(i)), 1.f) < 0.5f;
				if (stage >= 3 && Rand01(static_cast<unsigned>(time * 13.f) + static_cast<unsigned>(i) * 7u, seed) < 0.45f)
				{
					on = false;
				}
				ShapeStyle dot;
				dot.hardEdges = true;
				dot.fill = on ? StopColor(accentRamp, 4) : StopColor(accentRamp, 1);
				mBrush.Circle(world, xf(light.position), std::max(light.size * 0.5f * xf.k, px * 0.6f), dot);
			}
		}

		// Ability ports (hull sockets, lit by category).
		for (int i = 0; i < ShipPortCount; ++i)
		{
			const ShipPortState& port = state.ports[i];
			const sf::Vector2f center = xf(def.ports[i].position);
			const float radius = std::max(def.ports[i].size * 0.5f * xf.k, px * 1.f);
			ShapeStyle socket;
			socket.hardEdges = true;
			socket.outlineWidth = radius >= px * 1.8f ? px : 0.f;
			socket.fill = StopColor(hullRamp, 0);
			socket.outline = StopColor(hullRamp, 1);
			bool flickerOff = stage >= 3 && Rand01(static_cast<unsigned>(time * 9.f) + static_cast<unsigned>(i) * 5u, seed + 3u) < 0.4f;
			if (!port.equipped || flickerOff)
			{
				mBrush.Circle(world, center, radius, socket);
				continue;
			}
			const ColorRamp& ramp = script.Role(PortRoleName(port.category));
			const float pulse = 0.5f + 0.5f * std::sin(time * 3.4f + static_cast<float>(i) * 1.7f);
			if (port.isActive)
			{
				socket.fill = StopColor(ramp, 4);
				socket.outline = StopColor(ramp, 3);
				mBrush.Circle(world, center, radius * 1.15f, socket);
			}
			else if (port.isReady)
			{
				socket.fill = StopColor(ramp, pulse > 0.8f ? 4 : 3);
				socket.outline = StopColor(ramp, 2);
				mBrush.Circle(world, center, radius, socket);
			}
			else
			{
				// Cooling down: dim socket with a pie that fills clockwise from the top.
				socket.fill = StopColor(ramp, 0);
				socket.outline = StopColor(ramp, 1);
				mBrush.Circle(world, center, radius, socket);
				const float sweep = std::clamp(port.cooldownFraction, 0.f, 1.f) * 359.f;
				if (sweep > 4.f)
				{
					ShapeStyle pie;
					pie.hardEdges = true;
					pie.fill = StopColor(ramp, 2);
					mBrush.Arc(world, center, radius * 0.5f, radius, -90.f, sweep, pie);
				}
			}
		}

		// ---- Energy: engines, lights, ports, sparks, shield. ----
		const ColorRamp& playerRamp = script.Role("player");
		const ColorRamp& warmRamp = script.Role("combatWarm");
		const float thrust = std::clamp(state.thrust, 0.f, 1.f);
		const float stageFlicker = stage >= 3 ? 0.35f + 0.65f * Rand01(static_cast<unsigned>(time * 14.f), seed + 11u)
			: (stage == 2 && Rand01(static_cast<unsigned>(time * 10.f), seed + 11u) < 0.1f ? 0.5f : 1.f);

		sf::Color engineColor = playerRamp.Sample(0.8f);
		if (state.afterburner)
		{
			engineColor = LerpColor(engineColor, warmRamp.Sample(0.8f), 0.6f);
		}
		for (size_t m = 0; m < def.engines.size(); ++m)
		{
			const ShipEngineMount& mount = def.engines[m];
			const float size = mount.size * xf.k;
			const sf::Vector2f nozzle = xf(mount.position);
			const sf::Vector2f back = xf.Direction({0.f, 1.f});
			const sf::Vector2f side(-back.y, back.x);
			const float length = size * (1.1f + 3.f * thrust) * (state.afterburner ? 1.7f : 1.f);

			TrailParams trail;
			for (int p = 0; p < 8; ++p)
			{
				const float t = static_cast<float>(p) / 7.f;
				const float wobble = std::sin(time * 38.f + static_cast<float>(p) * 1.3f + static_cast<float>(m)) * size * 0.05f * t;
				trail.points.push_back(nozzle + back * (length * t) + side * wobble);
			}
			trail.headWidth = size * (state.afterburner ? 1.1f : 0.8f);
			trail.tailWidth = size * 0.08f;
			trail.color = engineColor;
			trail.intensity = (0.5f + 0.35f * thrust + (state.afterburner ? 0.15f : 0.f)) * stageFlicker;
			trail.tier = LuminanceTier::Player;
			pipeline.SubmitTrail(trail);

			GlowParams core;
			core.position = nozzle;
			core.coreRadius = std::max(size * 0.26f, px * 0.9f);
			core.haloRadius = size * (1.5f + 0.9f * thrust + (state.afterburner ? 0.8f : 0.f));
			core.color = engineColor;
			core.intensity = (0.85f + 0.15f * thrust) * stageFlicker;
			core.tier = LuminanceTier::Player;
			pipeline.SubmitGlow(core);
		}

		for (size_t i = 0; i < def.lights.size() && hullShown; ++i)
		{
			const ShipLight& light = def.lights[i];
			bool on = light.blink <= 0.f || std::fmod(time * light.blink + Rand01(seed, static_cast<unsigned>(i)), 1.f) < 0.5f;
			if (!on || (stage >= 3 && Rand01(static_cast<unsigned>(time * 13.f) + static_cast<unsigned>(i) * 7u, seed) < 0.45f))
			{
				continue;
			}
			GlowParams glow;
			glow.position = xf(light.position);
			glow.coreRadius = std::max(light.size * 0.3f * xf.k, px * 0.5f);
			glow.haloRadius = std::max(light.size * 3.2f * xf.k, px * 4.f);
			glow.color = accentRamp.Sample(0.85f);
			glow.intensity = 0.55f;
			glow.tier = LuminanceTier::Player;
			pipeline.SubmitGlow(glow);
		}

		for (int i = 0; i < ShipPortCount; ++i)
		{
			const ShipPortState& port = state.ports[i];
			if (!port.equipped)
			{
				continue;
			}
			if (stage >= 3 && Rand01(static_cast<unsigned>(time * 9.f) + static_cast<unsigned>(i) * 5u, seed + 3u) < 0.4f)
			{
				continue;
			}
			const sf::Vector2f center = xf(def.ports[i].position);
			const float radius = std::max(def.ports[i].size * 0.5f * xf.k, px * 1.f);
			const ColorRamp& ramp = script.Role(PortRoleName(port.category));
			const float pulse = 0.5f + 0.5f * std::sin(time * 3.4f + static_cast<float>(i) * 1.7f);
			float intensity;
			if (port.isActive)
			{
				intensity = 0.95f;
			}
			else if (port.isReady)
			{
				intensity = 0.5f + 0.2f * pulse;
			}
			else
			{
				intensity = 0.18f + 0.22f * std::clamp(port.cooldownFraction, 0.f, 1.f);
			}
			GlowParams glow;
			glow.position = center;
			glow.coreRadius = radius * 0.45f;
			glow.haloRadius = radius * (port.isActive ? 5.f : 3.2f);
			glow.color = ramp.Sample(0.8f);
			glow.intensity = intensity;
			glow.tier = LuminanceTier::Player;
			pipeline.SubmitGlow(glow);

			if (port.justActivated > 0.f)
			{
				const float t = std::clamp(port.justActivated, 0.f, 1.f);
				const float ringRadius = radius * (1.6f + 6.f * (1.f - t));
				const float thickness = std::max(px * 0.8f, radius * 0.7f * t);
				const float cap = pipeline.TierBrightnessCap(LuminanceTier::Player);
				const sf::Color color = LimitBrightness(ramp.Sample(0.9f), cap * 0.85f);
				ShapeBrush* brush = &mBrush;
				pipeline.SubmitEnergy(LuminanceTier::Player, Pi * ringRadius * ringRadius, [brush, center, ringRadius, thickness, color, t](sf::RenderTarget& target)
				{
					ShapeStyle ring;
					ring.additive = true;
					ring.fill = sf::Color(color.r, color.g, color.b, static_cast<std::uint8_t>(255.f * t));
					brush->Ring(target, center, ringRadius, thickness, ring);
				});
				GlowParams burst = glow;
				burst.coreRadius = radius * (0.8f + 0.9f * t);
				burst.haloRadius = radius * (5.f + 5.f * t);
				burst.color = ramp.Sample(0.95f);
				burst.intensity = 0.6f + 0.4f * t;
				pipeline.SubmitGlow(burst);
			}
		}

		// Sparks from damaged spots: small flickering warm lights on the hull.
		if (stage >= 2 && hullShown && !def.parts.empty())
		{
			const int sparks = stage >= 3 ? 4 : 2;
			for (int j = 0; j < sparks; ++j)
			{
				const ShipPart& source = def.parts[Mix(seed + 97u * static_cast<unsigned>(j) + 5u) % def.parts.size()];
				if (missing(source) || Rand01(static_cast<unsigned>(time * 17.f) + static_cast<unsigned>(j) * 3u, seed + 23u) < 0.55f)
				{
					continue;
				}
				const sf::Vector2f jitter(Rand01(static_cast<unsigned>(time * 17.f), seed + j) - 0.5f, Rand01(static_cast<unsigned>(time * 17.f), seed + j + 99u) - 0.5f);
				GlowParams spark;
				spark.position = xf(source.centroid + jitter * 10.f);
				spark.coreRadius = std::max(px * 0.8f, 1.5f);
				spark.haloRadius = px * 5.5f;
				spark.color = warmRamp.Sample(0.8f);
				spark.intensity = 0.8f;
				spark.tier = LuminanceTier::Decorative;
				pipeline.SubmitGlow(spark);
			}
		}

		// Shield bubble: faint shell that follows the shield fraction; nothing at zero.
		if (state.shieldFraction > 0.01f)
		{
			sf::Shader* shader = mAssets.Shaders().Find("shield");
			if (shader != nullptr)
			{
				const ColorRamp& shieldRamp = script.Role(PortRoleName(AbilityCategory::Defense));
				const float cap = std::min(0.5f, pipeline.TierBrightnessCap(LuminanceTier::Player));
				const sf::Color color = LimitBrightness(shieldRamp.Sample(0.75f), cap);
				float strength = std::clamp(state.shieldFraction, 0.f, 1.f);
				if (strength < 0.2f)
				{
					strength *= 0.4f + 0.6f * Rand01(static_cast<unsigned>(time * 12.f), seed + 41u);
				}
				const sf::Vector2f radii(def.shieldRadii.x * xf.k, def.shieldRadii.y * xf.k);
				const sf::Vector2f center = xf.origin;
				const sf::Vector2f axis(xf.c, xf.s);
				const float extent = std::max(radii.x, radii.y) * 1.2f;
				const float hit = std::clamp(state.shieldHit, 0.f, 1.f);
				const float hitAngle = state.shieldHitAngle;
				VisualAssets* assets = &mAssets;
				pipeline.SubmitEnergy(LuminanceTier::Player, 4.f * extent * extent, [assets, center, axis, radii, color, strength, hit, hitAngle, time, extent](sf::RenderTarget& target)
				{
					sf::Shader* shield = assets->Shaders().Find("shield");
					if (shield == nullptr)
					{
						return;
					}
					shield->setUniform("uCenter", center);
					shield->setUniform("uAxis", axis);
					shield->setUniform("uRadii", radii);
					shield->setUniform("uColor", sf::Glsl::Vec3(color.r / 255.f, color.g / 255.f, color.b / 255.f));
					shield->setUniform("uStrength", strength);
					shield->setUniform("uHit", hit);
					shield->setUniform("uHitAngle", hitAngle);
					shield->setUniform("uTime", time);
					const sf::Vector2f half(extent, extent);
					DrawShaderQuad(target, *shield, sf::FloatRect(center - half, half * 2.f), Additive);
				});
			}
		}
	}
}

#include "sky/DeepSky.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

#include <nlohmann/json.hpp>

#include "FileUtil.h"
#include "sky/UniformWriter.h"

namespace vega::sky
{
	namespace
	{
		constexpr float kDegToRad = 3.14159265f / 180.f;
		// The Milky Way cache's frame, so the light can be read inside the band at the same pixel.
		constexpr unsigned kMargin = GalaxyLayer::CacheMargin;
		constexpr float ColorMin = 0.f, ColorMax = 1.f;

		using Json = nlohmann::json;

		struct KindName
		{
			const char* name;
			DeepSkyKind kind;
		};

		constexpr KindName kKinds[] = {
			{"emission", DeepSkyKind::Emission},
			{"reflection", DeepSkyKind::Reflection},
			{"planetary", DeepSkyKind::Planetary},
			{"remnant", DeepSkyKind::Remnant},
			{"bubble", DeepSkyKind::Bubble},
			{"dark", DeepSkyKind::Dark},
			{"fog", DeepSkyKind::Fog},
			{"cluster", DeepSkyKind::Cluster},
			{"galaxy", DeepSkyKind::Galaxy},
			{"filaments", DeepSkyKind::Filaments},
			{"lobes", DeepSkyKind::Lobes},
			{"jet", DeepSkyKind::Jet},
			{"spiral", DeepSkyKind::Spiral},
			{"belt", DeepSkyKind::Belt},
			{"star", DeepSkyKind::Star},
		};

		float Bound(const char* path, float value)
		{
			static const auto fields = []
			{
				std::map<std::string_view, const SkyField*, std::less<>> result;
				for (const auto& field : DeepSkyFields()) result.emplace(field.path, &field);
				return result;
			}();
			const SkyField* field = fields.at(path);
			if (field->min) value = std::max(value, *field->min);
			if (field->max) value = std::min(value, *field->max);
			return value;
		}

		bool ParseKind(const std::string& name, DeepSkyKind& kind)
		{
			for (const KindName& entry : kKinds)
			{
				if (name == entry.name)
				{
					kind = entry.kind;
					return true;
				}
			}
			return false;
		}

		bool ReadColor(const Json& json, const char* key, sf::Glsl::Vec3& colour, const std::string& context, std::string& error)
		{
			const auto it = json.find(key);
			if (it == json.end())
			{
				return true;
			}
			if (!it->is_array() || it->size() != 3 || !std::all_of(it->begin(), it->end(), [](const Json& v) { return v.is_number(); }))
			{
				error = context + "." + key + " must be [r, g, b]";
				return false;
			}
			colour = {std::clamp((*it)[0].get<float>(), ColorMin, ColorMax), std::clamp((*it)[1].get<float>(), ColorMin, ColorMax), std::clamp((*it)[2].get<float>(), ColorMin, ColorMax)};
			return true;
		}

		std::optional<float> ReadOptional(const Json& json, const char* key)
		{
			const auto it = json.find(key);
			if (it == json.end() || !it->is_number())
			{
				return std::nullopt;
			}
			return it->get<float>();
		}

		// FNV-1a, stable across standard libraries: a preset used at two stops still looks different at each.
		float SeedOf(const std::string& text, std::size_t index)
		{
			std::uint32_t hash = 2166136261u;
			for (const char c : text)
			{
				hash ^= static_cast<std::uint8_t>(c);
				hash *= 16777619u;
			}
			return static_cast<float>(hash % 9973u) * 0.0131f + static_cast<float>(index) * 7.31f;
		}

		bool ParseObject(const Json& json, DeepSkyObject& object, const std::string& context, std::string& error)
		{
			if (!json.is_object())
			{
				error = context + " must be an object";
				return false;
			}
			const std::string kind = json.value("kind", std::string());
			if (!ParseKind(kind, object.kind))
			{
				error = context + ".kind '" + kind + "' is unknown";
				return false;
			}
			const auto at = json.find("at");
			if (at != json.end())
			{
				if (!at->is_array() || at->size() != 2 || !(*at)[0].is_number() || !(*at)[1].is_number())
				{
					error = context + ".at must be [x, y]";
					return false;
				}
				object.at = {(*at)[0].get<float>(), (*at)[1].get<float>()};
			}
			object.size = Bound("objects.*.size", json.value("size", object.size));
			object.aspect = Bound("objects.*.aspect", json.value("aspect", object.aspect));
			object.angleDeg = json.value("angleDeg", object.angleDeg);
			object.intensity = Bound("objects.*.intensity", json.value("intensity", object.intensity));
			object.variant = json.value("variant", object.variant);
			if (const auto seed = ReadOptional(json, "seed"))
			{
				object.seed = *seed;
			}
			else
			{
				object.seed = -1.f;
			}
			return ReadColor(json, "colorA", object.colorA, context, error) && ReadColor(json, "colorB", object.colorB, context, error)
				&& ReadColor(json, "colorC", object.colorC, context, error);
		}

		// Reads the fields of a stop entry into stop, adding to what a preset already put there.
		bool ParseStop(const Json& json, SkyAsset& stop, const std::string& context, std::string& error)
		{
			if (!json.is_object())
			{
				error = context + " must be an object";
				return false;
			}
			const auto objects = json.find("objects");
			if (objects != json.end())
			{
				if (!objects->is_array())
				{
					error = context + ".objects must be an array";
					return false;
				}
				for (std::size_t i = 0; i < objects->size(); ++i)
				{
					DeepSkyObject object;
					if (!ParseObject((*objects)[i], object, context + ".objects[" + std::to_string(i) + "]", error))
					{
						return false;
					}
					stop.objects.push_back(object);
				}
			}
			const auto band = json.find("band");
			if (band != json.end() && band->is_object())
			{
				if (auto v = ReadOptional(*band, "intensityScale")) stop.band.intensityScale = Bound("band.intensityScale", *v);
				if (auto v = ReadOptional(*band, "angleDeg")) stop.band.angleDeg = *v;
				if (auto v = ReadOptional(*band, "offset")) stop.band.offset = Bound("band.offset", *v);
				if (auto v = ReadOptional(*band, "width")) stop.band.width = Bound("band.width", *v);
				if (auto v = ReadOptional(*band, "dust")) stop.band.dust = Bound("band.dust", *v);
				BandStyle& style = stop.band.style;
				const std::string view = band->value("view", std::string());
				if (view == "anticenter")
				{
					// looking away from the centre: the faint, bluer outer disk, no bulge
					style.bulge = 0.f;
					style.anticenter = 1.f;
				}
				else if (view == "outward")
				{
					// looking out of the plane: the band sinks to the horizon, the bulge is far below
					style.bulge = 0.3f;
					style.anticenter = 0.5f;
				}
				else if (!view.empty() && view != "center")
				{
					error = context + ".band.view '" + view + "' is unknown (center, anticenter, outward)";
					return false;
				}
				const bool coloured = band->contains("warm") || band->contains("cool") || band->contains("cloud") || band->contains("cloudAccent");
				if (!ReadColor(*band, "warm", style.warm, context + ".band", error) || !ReadColor(*band, "cool", style.cool, context + ".band", error)
					|| !ReadColor(*band, "cloud", style.cloud, context + ".band", error) || !ReadColor(*band, "cloudAccent", style.cloudAccent, context + ".band", error))
				{
					return false;
				}
				style.tint = Bound("band.tint", band->value("tint", coloured ? 1.f : style.tint));
				if (auto v = ReadOptional(*band, "clouds")) style.clouds = Bound("band.clouds", *v);
				if (auto v = ReadOptional(*band, "cloudReach")) style.cloudReach = Bound("band.cloudReach", *v);
				if (auto v = ReadOptional(*band, "bulge")) style.bulge = Bound("band.bulge", *v);
				if (auto v = ReadOptional(*band, "anticenter")) style.anticenter = Bound("band.anticenter", *v);
				if (auto v = ReadOptional(*band, "curve")) style.curve = Bound("band.curve", *v);
				if (auto v = ReadOptional(*band, "taper")) style.taper = Bound("band.taper", *v);
				if (auto v = ReadOptional(*band, "starBoost")) style.starBoost = Bound("band.starBoost", *v);
			}
			if (json.contains("skyTint"))
			{
				sf::Glsl::Vec3 tint;
				if (!ReadColor(json, "skyTint", tint, context, error))
				{
					return false;
				}
				stop.skyTint = tint;
			}
			stop.skyTintStrength = Bound("skyTintStrength", json.value("skyTintStrength", stop.skyTintStrength));
			stop.followMirror = json.value("mirror", stop.followMirror);
			if (auto v = ReadOptional(json, "stars")) stop.starScale = Bound("stars", *v);
			if (auto v = ReadOptional(json, "motes")) stop.motes = Bound("motes", *v);
			const auto planet = json.find("planet");
			if (planet != json.end() && planet->is_object())
			{
				const auto at = planet->find("at");
				if (at != planet->end())
				{
					if (!at->is_array() || at->size() != 2 || !(*at)[0].is_number() || !(*at)[1].is_number())
					{
						error = context + ".planet.at must be [x, y]";
						return false;
					}
					stop.planet.at = sf::Vector2f((*at)[0].get<float>(), (*at)[1].get<float>());
				}
				if (auto v = ReadOptional(*planet, "scale")) stop.planet.scale = Bound("planet.scale", *v);
				stop.planet.hide = planet->value("hide", stop.planet.hide);
			}
			const auto grade = json.find("grade");
			if (grade != json.end() && grade->is_object())
			{
				stop.capScale = Bound("grade.capScale", grade->value("capScale", stop.capScale));
				stop.gradeContrast = Bound("grade.contrast", grade->value("contrast", stop.gradeContrast));
				stop.gradeSaturation = Bound("grade.saturation", grade->value("saturation", stop.gradeSaturation));
			}
			const auto nebula = json.find("nebula");
			if (nebula != json.end())
			{
				if (!nebula->is_array())
				{
					error = context + ".nebula must be an array of layers";
					return false;
				}
				for (std::size_t i = 0; i < nebula->size(); ++i)
				{
					const Json& entry = (*nebula)[i];
					const std::string where = context + ".nebula[" + std::to_string(i) + "]";
					const auto ramp = entry.find("ramp");
					if (!entry.is_object() || ramp == entry.end() || !ramp->is_array() || ramp->size() != 4)
					{
						error = where + ".ramp must be four [r, g, b] colours, dark to bright";
						return false;
					}
					DeepSkyNebulaLayer layer;
					for (std::size_t k = 0; k < 4; ++k)
					{
						Json holder = Json::object();
						holder["c"] = (*ramp)[k];
						if (!ReadColor(holder, "c", layer.ramp[k], where + ".ramp", error))
						{
							return false;
						}
					}
					layer.scale = Bound("nebula.*.scale", entry.value("scale", layer.scale));
					layer.intensity = Bound("nebula.*.intensity", entry.value("intensity", layer.intensity));
					layer.parallax = Bound("nebula.*.parallax", entry.value("parallax", layer.parallax));
					layer.seed = Bound("nebula.*.seed", entry.value("seed", layer.seed));
					stop.nebula.push_back(layer);
				}
			}
			const auto rocks = json.find("asteroids");
			if (rocks != json.end() && rocks->is_object())
			{
				AsteroidBeltSettings belt;
				belt.count = static_cast<int>(Bound("asteroids.count", static_cast<float>(rocks->value("count", belt.count))));
				belt.angleDeg = rocks->value("angleDeg", belt.angleDeg);
				belt.center = rocks->value("center", belt.center);
				belt.width = Bound("asteroids.width", rocks->value("width", belt.width));
				belt.speed = rocks->value("speed", belt.speed);
				belt.nearest = Bound("asteroids.nearest", rocks->value("nearest", belt.nearest));
				if (!ReadColor(*rocks, "dark", belt.dark, context + ".asteroids", error) || !ReadColor(*rocks, "mid", belt.mid, context + ".asteroids", error)
					|| !ReadColor(*rocks, "light", belt.light, context + ".asteroids", error) || !ReadColor(*rocks, "haze", belt.haze, context + ".asteroids", error))
				{
					return false;
				}
				stop.asteroids = belt;
			}
			const auto host = json.find("hostStar");
			if (host != json.end() && host->is_object())
			{
				const auto at = host->find("at");
				if (at != host->end())
				{
					if (!at->is_array() || at->size() != 2 || !(*at)[0].is_number() || !(*at)[1].is_number())
					{
						error = context + ".hostStar.at must be [x, y]";
						return false;
					}
					stop.hostStar.at = sf::Vector2f((*at)[0].get<float>(), (*at)[1].get<float>());
				}
				stop.hostStar.hide = host->value("hide", stop.hostStar.hide);
			}
			const auto paint = json.find("skyPaint");
			if (paint != json.end() && paint->is_object())
			{
				DeepSkyPaint& p = stop.skyPaint;
				p.enabled = paint->value("enabled", true);
				p.coverage = Bound("skyPaint.coverage", paint->value("coverage", p.coverage));
				p.intensity = Bound("skyPaint.intensity", paint->value("intensity", p.intensity));
				p.centerCalm = Bound("skyPaint.centerCalm", paint->value("centerCalm", p.centerCalm));
				const char* keys[] = {"shadow", "mid", "lit", "accent"};
				std::optional<sf::Glsl::Vec3>* slots[] = {&p.shadow, &p.mid, &p.lit, &p.accent};
				for (int k = 0; k < 4; ++k)
				{
					if (paint->contains(keys[k]))
					{
						sf::Glsl::Vec3 colour;
						if (!ReadColor(*paint, keys[k], colour, context + ".skyPaint", error))
						{
							return false;
						}
						*slots[k] = colour;
					}
				}
			}
			const auto orbit = json.find("orbit");
			if (orbit != json.end() && orbit->is_object())
			{
				// Starts from the stop's own orbit (a preset's, or an earlier value) so overrides merge like the other fields.
				OrbitSettings settings = stop.orbit.value_or(OrbitSettings{});
				settings.planet = std::max(orbit->value("planet", settings.planet), static_cast<int>(*FindSkyField("orbit.planet")->min));
				if (orbit->contains("kind"))
				{
					const std::optional<PlanetKind> kind = ParsePlanetKind(orbit->value("kind", std::string()));
					if (!kind)
					{
						error = context + ".orbit.kind must be one of terran, ocean, desert, lava, ice, gasGiant, hotJupiter, iceGiant";
						return false;
					}
					settings.kind = kind;
				}
				settings.horizonAngleDeg = orbit->value("horizonAngleDeg", settings.horizonAngleDeg);
				settings.coverage = Bound("orbit.coverage", orbit->value("coverage", settings.coverage));
				settings.radius = Bound("orbit.radius", orbit->value("radius", settings.radius));
				settings.atmosphere = Bound("orbit.atmosphere", orbit->value("atmosphere", settings.atmosphere));
				settings.detail = Bound("orbit.detail", orbit->value("detail", settings.detail));
				settings.brightness = Bound("orbit.brightness", orbit->value("brightness", settings.brightness));
				settings.spin = Bound("orbit.spin", orbit->value("spin", settings.spin));
				settings.fieldOfViewDeg = Bound("orbit.fieldOfViewDeg", orbit->value("fieldOfViewDeg", settings.fieldOfViewDeg));
				settings.degreesPerMinute = Bound("orbit.degreesPerMinute", orbit->value("degreesPerMinute", settings.degreesPerMinute));
				settings.startDeg = orbit->value("startDeg", settings.startDeg);
				settings.starDeg = orbit->value("starDeg", settings.starDeg);
				settings.starAcrossDeg = Bound("orbit.starAcrossDeg", orbit->value("starAcrossDeg", settings.starAcrossDeg));
				settings.bandTiltDeg = Bound("orbit.bandTiltDeg", orbit->value("bandTiltDeg", settings.bandTiltDeg));
				settings.bandNodeDeg = orbit->value("bandNodeDeg", settings.bandNodeDeg);
				settings.centerDeg = orbit->value("centerDeg", settings.centerDeg);
				if (orbit->contains("framing"))
				{
					const std::string framing = orbit->value("framing", std::string());
					if (framing != "horizon" && framing != "disk")
					{
						error = context + ".orbit.framing must be horizon or disk";
						return false;
					}
					settings.framing = framing == "disk" ? OrbitFraming::Disk : OrbitFraming::Horizon;
				}
				settings.diskRadius = Bound("orbit.diskRadius", orbit->value("diskRadius", settings.diskRadius));
				settings.moon = orbit->value("moon", settings.moon);
				const auto ring = orbit->find("ring");
				if (ring != orbit->end() && ring->is_object())
				{
					OrbitRing ringSettings = settings.ring.value_or(OrbitRing{});
					ringSettings.inner = Bound("orbit.ring.inner", ring->value("inner", ringSettings.inner));
					ringSettings.outer = std::max(ringSettings.inner + SkyRingRadiusGap, Bound("orbit.ring.outer", ring->value("outer", ringSettings.outer)));
					ringSettings.tiltDeg = Bound("orbit.ring.tiltDeg", ring->value("tiltDeg", ringSettings.tiltDeg));
					ringSettings.nodeDeg = ring->value("nodeDeg", ringSettings.nodeDeg);
					ringSettings.opacity = Bound("orbit.ring.opacity", ring->value("opacity", ringSettings.opacity));
					settings.ring = ringSettings;
				}
				stop.orbit = settings;
			}
			return true;
		}

		// Square quad around the object; every kind fades to zero inside its radius times this extent.
		float ExtentOf(DeepSkyKind kind)
		{
			switch (kind)
			{
			case DeepSkyKind::Lobes: return 1.1f;
			case DeepSkyKind::Fog: return 1.05f;
			default: return 1.02f;
			}
		}
	}

	const std::vector<SkyField>& DeepSkyFields()
	{
		static const std::vector<SkyField> fields = []
		{
			std::vector<SkyField> f;
			const auto number = [&](const char* path, float value, std::optional<float> min = {}, std::optional<float> max = {})
			{
				f.push_back({path, SkyFieldType::Number, value, min, max, {}});
			};
			const auto color = [&](const char* path, sf::Glsl::Vec3 value)
			{
				f.push_back({path, SkyFieldType::Color, {value.x, value.y, value.z}, ColorMin, ColorMax, {}});
			};
			const auto flag = [&](const char* path, bool value) { f.push_back({path, SkyFieldType::Bool, value, {}, {}, {}}); };
			const auto position = [&](const char* path, sf::Vector2f value) { f.push_back({path, SkyFieldType::Position, {value.x, value.y}, {}, {}, {}}); };
			const DeepSkyObject o;
			std::vector<std::string> kinds;
			for (const auto& kind : kKinds) kinds.emplace_back(kind.name);
			f.push_back({"objects.*.kind", SkyFieldType::Enum, "emission", {}, {}, kinds});
			position("objects.*.at", o.at);
			number("objects.*.size", o.size, 0.01f, 3.f);
			number("objects.*.aspect", o.aspect, 0.05f, 1.f);
			number("objects.*.angleDeg", o.angleDeg);
			color("objects.*.colorA", o.colorA); color("objects.*.colorB", o.colorB); color("objects.*.colorC", o.colorC);
			number("objects.*.intensity", o.intensity, 0.f, 1.5f);
			number("objects.*.variant", o.variant); number("objects.*.seed", o.seed);
			const BandStyle b;
			f.push_back({"band.view", SkyFieldType::Enum, "center", {}, {}, {"center", "anticenter", "outward"}});
			color("band.warm", b.warm); color("band.cool", b.cool); color("band.cloud", b.cloud); color("band.cloudAccent", b.cloudAccent);
			number("band.intensityScale", 1.f, 0.f, 2.f);
			number("band.angleDeg", MilkyWaySettings{}.angleDeg);
			number("band.offset", MilkyWaySettings{}.offset, -1.f, 1.f);
			number("band.width", MilkyWaySettings{}.width, 0.02f, 1.f);
			number("band.dust", 1.f, 0.f, 3.f);
			number("band.tint", b.tint, 0.f, 1.f);
			number("band.clouds", b.clouds, 0.f, 4.f);
			number("band.cloudReach", b.cloudReach, 0.2f, 6.f);
			number("band.bulge", b.bulge, 0.f, 2.f);
			number("band.anticenter", b.anticenter, 0.f, 1.f);
			number("band.curve", b.curve, -4.f, 4.f);
			number("band.taper", b.taper, -0.95f, 0.95f);
			number("band.starBoost", b.starBoost, 0.f, 10.f);
			color("skyTint", {1.f, 1.f, 1.f}); number("skyTintStrength", 0.f, 0.f, 1.f);
			flag("mirror", true);
			number("stars", 1.f, 0.f, 8.f); number("motes", 1.f, 0.f, 4.f);
			position("planet.at", {PlanetSettings{}.screenX, PlanetSettings{}.screenY});
			number("planet.scale", 1.f, 0.1f, 4.f); flag("planet.hide", false);
			position("hostStar.at", {HostStarSettings{}.screenX, HostStarSettings{}.screenY}); flag("hostStar.hide", false);
			number("grade.capScale", 1.f, 0.5f, 1.6f); number("grade.contrast", 0.f, 0.f, 3.f); number("grade.saturation", 0.f, 0.f, 3.f);
			const DeepSkyPaint p;
			flag("skyPaint.enabled", true);
			number("skyPaint.coverage", p.coverage, 0.f, 1.f); number("skyPaint.intensity", p.intensity, 0.f, 2.f); number("skyPaint.centerCalm", p.centerCalm, 0.f, 1.f);
			const auto palette = SkyPaletteFor(galaxy::SkyState{});
			color("skyPaint.shadow", palette.shadow); color("skyPaint.mid", palette.mid); color("skyPaint.lit", palette.lit); color("skyPaint.accent", palette.accent);
			const DeepSkyNebulaLayer n;
			number("nebula.*.scale", n.scale, 0.2f, 8.f); number("nebula.*.intensity", n.intensity, 0.f, 1.f);
			number("nebula.*.parallax", n.parallax, 0.f, 1.f); number("nebula.*.seed", n.seed, 0.f, 20.f);
			const AsteroidBeltSettings a;
			f.push_back({"asteroids.count", SkyFieldType::Integer, a.count, 0.f, 400.f, {}});
			number("asteroids.angleDeg", a.angleDeg); number("asteroids.center", a.center); number("asteroids.width", a.width, .01f, 2.f);
			number("asteroids.speed", a.speed); number("asteroids.nearest", a.nearest, .05f, 1.f);
			color("asteroids.dark", a.dark); color("asteroids.mid", a.mid); color("asteroids.light", a.light); color("asteroids.haze", a.haze);
			const OrbitSettings r;
			f.push_back({"orbit.planet", SkyFieldType::Integer, r.planet, 0.f, {}, {}});
			std::vector<std::string> planets;
			for (const char* name : {"terran", "ocean", "desert", "lava", "ice", "gasGiant", "hotJupiter", "iceGiant"})
				if (ParsePlanetKind(name)) planets.emplace_back(name);
			f.push_back({"orbit.kind", SkyFieldType::Enum, "terran", {}, {}, planets});
			f.push_back({"orbit.framing", SkyFieldType::Enum, "horizon", {}, {}, {"horizon", "disk"}});
			number("orbit.horizonAngleDeg", r.horizonAngleDeg);
			number("orbit.coverage", r.coverage, 0.05f, 0.7f); number("orbit.radius", r.radius, 0.6f, 20.f);
			number("orbit.atmosphere", r.atmosphere, 0.f, 0.2f); number("orbit.detail", r.detail, 0.25f, 8.f);
			number("orbit.brightness", r.brightness, 0.f, 3.f); number("orbit.spin", r.spin, -3.f, 3.f);
			number("orbit.fieldOfViewDeg", r.fieldOfViewDeg, 20.f, 140.f); number("orbit.degreesPerMinute", r.degreesPerMinute, -120.f, 120.f);
			number("orbit.startDeg", r.startDeg); number("orbit.starDeg", r.starDeg); number("orbit.starAcrossDeg", r.starAcrossDeg, -80.f, 80.f);
			number("orbit.bandTiltDeg", r.bandTiltDeg, 0.f, 90.f); number("orbit.bandNodeDeg", r.bandNodeDeg); number("orbit.centerDeg", r.centerDeg);
			number("orbit.diskRadius", r.diskRadius, 0.04f, 0.48f); flag("orbit.moon", r.moon);
			const OrbitRing ring;
			number("orbit.ring.inner", ring.inner, 1.02f, 4.f); number("orbit.ring.outer", ring.outer, 1.02f + SkyRingRadiusGap, 6.f);
			number("orbit.ring.tiltDeg", ring.tiltDeg, -89.f, 89.f); number("orbit.ring.nodeDeg", ring.nodeDeg); number("orbit.ring.opacity", ring.opacity, 0.f, 1.f);
			return f;
		}();
		return fields;
	}

	const SkyField* FindSkyField(const std::string& path)
	{
		std::string pattern;
		for (std::size_t i = 0; i < path.size();)
		{
			const auto end = path.find('.', i);
			const std::string part = path.substr(i, end == std::string::npos ? end : end - i);
			if (!pattern.empty()) pattern += '.';
			pattern += !part.empty() && std::all_of(part.begin(), part.end(), [](char c) { return c >= '0' && c <= '9'; }) ? "*" : part;
			if (end == std::string::npos) break;
			i = end + 1;
		}
		for (const auto& field : DeepSkyFields()) if (field.path == pattern) return &field;
		return nullptr;
	}

	std::string ValidateSkyField(const SkyField& field, const nlohmann::json& value)
	{
		const auto number = [&](const Json& v)
		{
			if (!v.is_number() || !std::isfinite(v.get<double>()) || !std::isfinite(v.get<float>())) return false;
			return (!field.min || v.get<double>() >= *field.min - 1e-6) && (!field.max || v.get<double>() <= *field.max + 1e-6);
		};
		bool valid = false;
		switch (field.type)
		{
		case SkyFieldType::Bool: valid = value.is_boolean(); break;
		case SkyFieldType::Number: valid = number(value); break;
		case SkyFieldType::Integer: valid = value.is_number_integer() && number(value); break;
		case SkyFieldType::Color:
		case SkyFieldType::Position: valid = value.is_array() && value.size() == (field.type == SkyFieldType::Color ? 3u : 2u) && std::all_of(value.begin(), value.end(), number); break;
		case SkyFieldType::Enum: valid = value.is_string() && std::find(field.options.begin(), field.options.end(), value.get<std::string>()) != field.options.end(); break;
		case SkyFieldType::Text: valid = value.is_string(); break;
		}
		return valid ? std::string{} : "Invalid type, enum or range for " + field.path;
	}

	bool DeepSkyCatalog::Load(const std::filesystem::path& file, std::string& error)
	{
		std::string text;
		if (!ReadTextFile(file, text))
		{
			error = "cannot read " + file.string();
			return false;
		}
		try
		{
			return Apply(Json::parse(text), error);
		}
		catch (const std::exception& exception)
		{
			error = exception.what();
			return false;
		}
	}

	bool DeepSkyCatalog::Apply(const nlohmann::json& document, std::string& error)
	{
		try
		{
			// Normalize the envelope only. Legacy collection IDs and seeded values
			// retain their exact parser path, so existing authored skies render identically.
			auto root = document;
			if (root.is_object() && root.contains("skies")) root["stops"] = root.at("skies");
			else if (root.is_object() && root.contains("sky"))
			{
				const auto id = root.value("id", std::string{});
				if (id.empty()) { error = "A sky asset requires a nonempty id"; return false; }
				root["stops"] = Json{{id, root.at("sky")}};
			}
			if (!root.is_object() || !root.contains("stops") || !root.at("stops").is_object())
			{
				error = "Expected skies collection, id/sky asset, or legacy stops collection";
				return false;
			}
			std::map<std::string, SkyAsset> presets;
			const auto presetsJson = root.find("presets");
			if (presetsJson != root.end() && presetsJson->is_object())
			{
				for (auto it = presetsJson->begin(); it != presetsJson->end(); ++it)
				{
					SkyAsset preset;
					if (!ParseStop(it.value(), preset, "presets." + it.key(), error))
					{
						return false;
					}
					presets[it.key()] = preset;
				}
			}
			std::map<std::string, SkyAsset> stops;
			const auto stopsJson = root.find("stops");
			if (stopsJson != root.end() && stopsJson->is_object())
			{
				for (auto it = stopsJson->begin(); it != stopsJson->end(); ++it)
				{
					const std::string context = "stops." + it.key();
					SkyAsset stop;
					if (it.value().is_string() || (it.value().is_object() && it.value().contains("use")))
					{
						const std::string use = it.value().is_string() ? it.value().get<std::string>() : it.value().value("use", std::string());
						const auto preset = presets.find(use);
						if (preset == presets.end())
						{
							error = context + " uses unknown preset '" + use + "'";
							return false;
						}
						stop = preset->second;
					}
					if (it.value().is_object() && !ParseStop(it.value(), stop, context, error))
					{
						return false;
					}
					for (std::size_t i = 0; i < stop.objects.size(); ++i)
					{
						if (stop.objects[i].seed < 0.f)
						{
							stop.objects[i].seed = SeedOf(it.key(), i);
						}
					}
					stops[it.key()] = std::move(stop);
				}
			}
			mStops = std::move(stops);
			mPresets = std::move(presets);
			++mVersion;
			error.clear();
			return true;
		}
		catch (const std::exception& exception)
		{
			error = exception.what();
			return false;
		}
	}

	bool DeepSkyCatalog::ApplyPatch(const nlohmann::json& fragment, const std::vector<std::string>& removed, std::string& error)
	{
		DeepSkyCatalog patch;
		if (!patch.Apply(fragment, error)) return false;
		for (auto& [id, stop] : patch.mStops) mStops[id] = std::move(stop);
		for (const auto& id : removed) mStops.erase(id);
		mPresets = std::move(patch.mPresets);
		++mVersion;
		return true;
	}

	const SkyAsset* DeepSkyCatalog::Find(const std::string& skyId) const
	{
		const auto it = mStops.find(skyId);
		return it == mStops.end() ? nullptr : &it->second;
	}

	const SkyAsset* DeepSkyCatalog::FindPreset(const std::string& name) const
	{
		const auto it = mPresets.find(name);
		return it == mPresets.end() ? nullptr : &it->second;
	}

	void DeepSkyLayer::Prepare(ShaderLibrary& shaders, sf::Vector2u outputSize, const Params& params)
	{
		if (params.sky == nullptr || params.sky->objects.empty())
		{
			if (mValid)
			{
				mValid = false;
				++mRevision;
			}
			mSkyId.clear();
			return;
		}
		sf::Shader* shader = shaders.Find("deepsky");
		if (shader == nullptr)
		{
			mValid = false;
			return;
		}
		const bool stale = !mValid || mOutputSize != outputSize || mSkyId != params.skyId || mMirrored != params.mirrored
			|| mVersion != params.catalogVersion || mCap != params.maxBrightness || mShader != shader;
		if (stale)
		{
			Render(*shader, params, outputSize);
			++mRevision;
		}
	}

	void DeepSkyLayer::DrawShade(sf::RenderTarget& target, sf::Vector2f scroll, float parallax) const
	{
		if (!mValid || (!mHasShade && !mHasFront))
		{
			return;
		}
		const float limit = static_cast<float>(kMargin);
		const sf::Vector2f drift(std::clamp(scroll.x * parallax, -limit, limit), std::clamp(scroll.y * parallax, -limit, limit));
		if (mHasShade)
		{
			sf::Sprite shade(mShade.getTexture());
			shade.setPosition(sf::Vector2f(-limit, -limit) - drift);
			target.draw(shade, sf::RenderStates(sf::BlendMultiply));
		}
		// Light in front of everything behind it: the occluders' own light (rim-lit dust, lit rock faces) on top of their
		// shadow, and giant stars, which must keep their colour instead of being reddened by the band's dust.
		sf::Sprite front(mFront.getTexture());
		front.setPosition(sf::Vector2f(-limit, -limit) - drift);
		target.draw(front, sf::RenderStates(sf::BlendAdd));
	}

	bool DeepSkyLayer::Render(sf::Shader& shader, const Params& params, sf::Vector2u outputSize)
	{
		const sf::Vector2u cacheSize(outputSize.x + 2 * kMargin, outputSize.y + 2 * kMargin);
		const SkyAsset& stop = *params.sky;
		const bool mirrored = params.mirrored && stop.followMirror;
		const auto occludes = [](const DeepSkyObject& o) { return o.kind == DeepSkyKind::Dark || o.kind == DeepSkyKind::Belt; };
		const auto inFront = [&](const DeepSkyObject& o) { return occludes(o) || o.kind == DeepSkyKind::Star; };
		mHasShade = std::any_of(stop.objects.begin(), stop.objects.end(), occludes);
		mHasFront = std::any_of(stop.objects.begin(), stop.objects.end(), inFront);
		mHasLight = std::any_of(stop.objects.begin(), stop.objects.end(), [&](const DeepSkyObject& o) { return !inFront(o); });
		if (mLight.getSize() != cacheSize && !mLight.resize(cacheSize))
		{
			mValid = false;
			return false;
		}
		if ((mHasShade && mShade.getSize() != cacheSize && !mShade.resize(cacheSize)) || (mHasFront && mFront.getSize() != cacheSize && !mFront.resize(cacheSize)))
		{
			mValid = false;
			return false;
		}
		const sf::Vector2f size(cacheSize);
		const float height = static_cast<float>(outputSize.y);
		const sf::View view(sf::FloatRect({0.f, 0.f}, size));
		mLight.setView(view);
		mLight.clear(sf::Color::Black);
		if (mHasShade)
		{
			mShade.setView(view);
			mShade.clear(sf::Color::White);
		}
		if (mHasFront)
		{
			mFront.setView(view);
			mFront.clear(sf::Color::Black);
		}

		for (const DeepSkyObject& object : stop.objects)
		{
			const float atX = mirrored ? 1.f - object.at.x : object.at.x;
			const float angleDeg = mirrored ? -object.angleDeg : object.angleDeg;
			const sf::Vector2f center(static_cast<float>(kMargin) + atX * static_cast<float>(outputSize.x), static_cast<float>(kMargin) + object.at.y * height);
			const float radius = object.size * height;
			const float half = radius * ExtentOf(object.kind);
			const sf::FloatRect rect(center - sf::Vector2f(half, half), sf::Vector2f(2.f * half, 2.f * half));
			const bool occluder = object.kind == DeepSkyKind::Dark || object.kind == DeepSkyKind::Belt;
			const int passes = occluder ? 2 : 1;
			for (int pass = 0; pass < passes; ++pass)
			{
				// pass 0 = light (additive), pass 1 = transmission of a dark object (multiplicative)
				{
					UniformWriter uniforms(shader);
					uniforms.Set("uCenter", center);
					uniforms.Set("uRadius", radius);
					uniforms.Set("uAngle", angleDeg * kDegToRad);
					uniforms.Set("uAspect", object.aspect);
					uniforms.Set("uKind", static_cast<float>(object.kind));
					uniforms.Set("uVariant", object.variant);
					uniforms.Set("uSeed", object.seed);
					uniforms.Set("uColorA", object.colorA);
					uniforms.Set("uColorB", object.colorB);
					uniforms.Set("uColorC", object.colorC);
					uniforms.Set("uIntensity", object.intensity);
					uniforms.Set("uPass", static_cast<float>(pass));
					uniforms.Set("uExtent", ExtentOf(object.kind));
					uniforms.Set("uMaxBrightness", params.maxBrightness);
				}
				if (pass == 0)
				{
					DrawShaderQuad(occluder || object.kind == DeepSkyKind::Star ? mFront : mLight, shader, rect, sf::BlendAdd);
				}
				else
				{
					DrawShaderQuad(mShade, shader, rect, sf::BlendMultiply);
				}
			}
		}
		mLight.display();
		if (mHasShade)
		{
			mShade.display();
		}
		if (mHasFront)
		{
			mFront.display();
		}

		mValid = true;
		mOutputSize = outputSize;
		mSkyId = params.skyId;
		mMirrored = params.mirrored;
		mVersion = params.catalogVersion;
		mCap = params.maxBrightness;
		mShader = &shader;
		return true;
	}
}

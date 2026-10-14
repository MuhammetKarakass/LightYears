#include "visual/DeepSky.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <nlohmann/json.hpp>

#include "FileUtil.h"
#include "visual/UniformWriter.h"

namespace ly::visual
{
	namespace
	{
		constexpr float kDegToRad = 3.14159265f / 180.f;
		// The Milky Way cache's frame, so the light can be read inside the band at the same pixel.
		constexpr unsigned kMargin = GalaxyLayer::CacheMargin;

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
			colour = {std::clamp((*it)[0].get<float>(), 0.f, 1.f), std::clamp((*it)[1].get<float>(), 0.f, 1.f), std::clamp((*it)[2].get<float>(), 0.f, 1.f)};
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
			object.size = std::clamp(json.value("size", object.size), 0.01f, 3.f);
			object.aspect = std::clamp(json.value("aspect", object.aspect), 0.05f, 1.f);
			object.angleDeg = json.value("angleDeg", object.angleDeg);
			object.intensity = std::clamp(json.value("intensity", object.intensity), 0.f, 1.5f);
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
		bool ParseStop(const Json& json, DeepSkyStop& stop, const std::string& context, std::string& error)
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
				if (auto v = ReadOptional(*band, "intensityScale")) stop.band.intensityScale = std::clamp(*v, 0.f, 2.f);
				if (auto v = ReadOptional(*band, "angleDeg")) stop.band.angleDeg = *v;
				if (auto v = ReadOptional(*band, "offset")) stop.band.offset = std::clamp(*v, -1.f, 1.f);
				if (auto v = ReadOptional(*band, "width")) stop.band.width = std::clamp(*v, 0.02f, 1.f);
				if (auto v = ReadOptional(*band, "dust")) stop.band.dust = std::clamp(*v, 0.f, 3.f);
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
				style.tint = std::clamp(band->value("tint", coloured ? 1.f : style.tint), 0.f, 1.f);
				if (auto v = ReadOptional(*band, "clouds")) style.clouds = std::clamp(*v, 0.f, 4.f);
				if (auto v = ReadOptional(*band, "cloudReach")) style.cloudReach = std::clamp(*v, 0.2f, 6.f);
				if (auto v = ReadOptional(*band, "bulge")) style.bulge = std::clamp(*v, 0.f, 2.f);
				if (auto v = ReadOptional(*band, "anticenter")) style.anticenter = std::clamp(*v, 0.f, 1.f);
				if (auto v = ReadOptional(*band, "curve")) style.curve = std::clamp(*v, -4.f, 4.f);
				if (auto v = ReadOptional(*band, "taper")) style.taper = std::clamp(*v, -0.95f, 0.95f);
				if (auto v = ReadOptional(*band, "starBoost")) style.starBoost = std::clamp(*v, 0.f, 10.f);
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
			stop.skyTintStrength = std::clamp(json.value("skyTintStrength", stop.skyTintStrength), 0.f, 1.f);
			stop.followMirror = json.value("mirror", stop.followMirror);
			if (auto v = ReadOptional(json, "stars")) stop.starScale = std::clamp(*v, 0.f, 8.f);
			if (auto v = ReadOptional(json, "motes")) stop.motes = std::clamp(*v, 0.f, 4.f);
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
				if (auto v = ReadOptional(*planet, "scale")) stop.planet.scale = std::clamp(*v, 0.1f, 4.f);
				stop.planet.hide = planet->value("hide", stop.planet.hide);
			}
			const auto grade = json.find("grade");
			if (grade != json.end() && grade->is_object())
			{
				stop.capScale = std::clamp(grade->value("capScale", stop.capScale), 0.5f, 1.6f);
				stop.gradeContrast = std::clamp(grade->value("contrast", stop.gradeContrast), 0.f, 3.f);
				stop.gradeSaturation = std::clamp(grade->value("saturation", stop.gradeSaturation), 0.f, 3.f);
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
					layer.scale = std::clamp(entry.value("scale", layer.scale), 0.2f, 8.f);
					layer.intensity = std::clamp(entry.value("intensity", layer.intensity), 0.f, 1.f);
					layer.parallax = std::clamp(entry.value("parallax", layer.parallax), 0.f, 1.f);
					layer.seed = std::clamp(entry.value("seed", layer.seed), 0.f, 20.f);
					stop.nebula.push_back(layer);
				}
			}
			const auto rocks = json.find("asteroids");
			if (rocks != json.end() && rocks->is_object())
			{
				AsteroidBeltSettings belt;
				belt.count = std::clamp(rocks->value("count", belt.count), 0, 400);
				belt.angleDeg = rocks->value("angleDeg", belt.angleDeg);
				belt.center = rocks->value("center", belt.center);
				belt.width = std::clamp(rocks->value("width", belt.width), 0.01f, 2.f);
				belt.speed = rocks->value("speed", belt.speed);
				belt.nearest = std::clamp(rocks->value("nearest", belt.nearest), 0.05f, 1.f);
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
				p.coverage = std::clamp(paint->value("coverage", p.coverage), 0.f, 1.f);
				p.intensity = std::clamp(paint->value("intensity", p.intensity), 0.f, 2.f);
				p.centerCalm = std::clamp(paint->value("centerCalm", p.centerCalm), 0.f, 1.f);
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
				settings.planet = std::max(orbit->value("planet", settings.planet), 0);
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
				settings.coverage = std::clamp(orbit->value("coverage", settings.coverage), 0.05f, 0.7f);
				settings.radius = std::clamp(orbit->value("radius", settings.radius), 0.6f, 20.f);
				settings.atmosphere = std::clamp(orbit->value("atmosphere", settings.atmosphere), 0.f, 0.2f);
				settings.detail = std::clamp(orbit->value("detail", settings.detail), 0.25f, 8.f);
				settings.brightness = std::clamp(orbit->value("brightness", settings.brightness), 0.f, 3.f);
				settings.spin = std::clamp(orbit->value("spin", settings.spin), -3.f, 3.f);
				settings.fieldOfViewDeg = std::clamp(orbit->value("fieldOfViewDeg", settings.fieldOfViewDeg), 20.f, 140.f);
				settings.degreesPerMinute = std::clamp(orbit->value("degreesPerMinute", settings.degreesPerMinute), -120.f, 120.f);
				settings.startDeg = orbit->value("startDeg", settings.startDeg);
				settings.starDeg = orbit->value("starDeg", settings.starDeg);
				settings.starAcrossDeg = std::clamp(orbit->value("starAcrossDeg", settings.starAcrossDeg), -80.f, 80.f);
				settings.bandTiltDeg = std::clamp(orbit->value("bandTiltDeg", settings.bandTiltDeg), 0.f, 90.f);
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
				settings.diskRadius = std::clamp(orbit->value("diskRadius", settings.diskRadius), 0.04f, 0.48f);
				settings.moon = orbit->value("moon", settings.moon);
				const auto ring = orbit->find("ring");
				if (ring != orbit->end() && ring->is_object())
				{
					OrbitRing ringSettings = settings.ring.value_or(OrbitRing{});
					ringSettings.inner = std::clamp(ring->value("inner", ringSettings.inner), 1.02f, 4.f);
					ringSettings.outer = std::clamp(ring->value("outer", ringSettings.outer), ringSettings.inner + 0.05f, 6.f);
					ringSettings.tiltDeg = std::clamp(ring->value("tiltDeg", ringSettings.tiltDeg), -89.f, 89.f);
					ringSettings.nodeDeg = ring->value("nodeDeg", ringSettings.nodeDeg);
					ringSettings.opacity = std::clamp(ring->value("opacity", ringSettings.opacity), 0.f, 1.f);
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
			const Json root = Json::parse(text);
			std::map<std::string, DeepSkyStop> presets;
			const auto presetsJson = root.find("presets");
			if (presetsJson != root.end() && presetsJson->is_object())
			{
				for (auto it = presetsJson->begin(); it != presetsJson->end(); ++it)
				{
					DeepSkyStop preset;
					if (!ParseStop(it.value(), preset, "presets." + it.key(), error))
					{
						return false;
					}
					presets[it.key()] = preset;
				}
			}
			std::map<std::string, DeepSkyStop> stops;
			const auto stopsJson = root.find("stops");
			if (stopsJson != root.end() && stopsJson->is_object())
			{
				for (auto it = stopsJson->begin(); it != stopsJson->end(); ++it)
				{
					const std::string context = "stops." + it.key();
					DeepSkyStop stop;
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
			++mVersion;
			return true;
		}
		catch (const std::exception& exception)
		{
			error = exception.what();
			return false;
		}
	}

	const DeepSkyStop* DeepSkyCatalog::Find(const std::string& stopId) const
	{
		const auto it = mStops.find(stopId);
		return it == mStops.end() ? nullptr : &it->second;
	}

	void DeepSkyLayer::Prepare(ShaderLibrary& shaders, sf::Vector2u outputSize, const Params& params)
	{
		if (params.stop == nullptr || params.stop->objects.empty())
		{
			if (mValid)
			{
				mValid = false;
				++mRevision;
			}
			mStopId.clear();
			return;
		}
		sf::Shader* shader = shaders.Find("deepsky");
		if (shader == nullptr)
		{
			mValid = false;
			return;
		}
		const bool stale = !mValid || mOutputSize != outputSize || mStopId != params.stopId || mMirrored != params.mirrored
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
		const DeepSkyStop& stop = *params.stop;
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
		mStopId = params.stopId;
		mMirrored = params.mirrored;
		mVersion = params.catalogVersion;
		mCap = params.maxBrightness;
		mShader = &shader;
		return true;
	}
}

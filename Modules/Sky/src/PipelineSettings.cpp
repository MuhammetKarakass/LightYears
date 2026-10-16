#include "sky/PipelineSettings.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "FileUtil.h"

namespace vega::sky
{
	namespace
	{
		const nlohmann::json& Section(const nlohmann::json& root, const char* name)
		{
			static const nlohmann::json empty = nlohmann::json::object();
			auto it = root.find(name);
			return it != root.end() && it->is_object() ? *it : empty;
		}

		// A colour multiplier: an array of three numbers, each clamped to 0..2. A missing key keeps the default.
		bool ReadTint(const nlohmann::json& section, const char* key, sf::Vector3f& tint, std::string& error)
		{
			auto it = section.find(key);
			if (it == section.end())
			{
				return true;
			}
			const bool numbers = std::all_of(it->begin(), it->end(), [](const nlohmann::json& v) { return v.is_number(); });
			if (!it->is_array() || it->size() != 3 || !numbers)
			{
				error = std::string("backgroundGrade.") + key + " must be an array of 3 numbers";
				return false;
			}
			tint = sf::Vector3f(std::clamp(it->at(0).get<float>(), 0.f, 2.f), std::clamp(it->at(1).get<float>(), 0.f, 2.f),
				std::clamp(it->at(2).get<float>(), 0.f, 2.f));
			return true;
		}
	}

	bool operator==(const BackgroundStyleSettings& a, const BackgroundStyleSettings& b)
	{
		return a.enabled == b.enabled && a.pixelSize == b.pixelSize && a.toneLevels == b.toneLevels
			&& a.ditherSpread == b.ditherSpread && a.roles == b.roles;
	}

	bool operator!=(const BackgroundStyleSettings& a, const BackgroundStyleSettings& b)
	{
		return !(a == b);
	}

	bool operator==(const NearFieldSettings& a, const NearFieldSettings& b)
	{
		return a.enabled == b.enabled;
	}

	bool operator!=(const NearFieldSettings& a, const NearFieldSettings& b)
	{
		return !(a == b);
	}

	bool operator==(const BackgroundGradeSettings& a, const BackgroundGradeSettings& b)
	{
		return a.enabled == b.enabled && a.contrast == b.contrast && a.saturation == b.saturation
			&& a.blackPoint == b.blackPoint && a.shadowTint == b.shadowTint && a.shadowTintStrength == b.shadowTintStrength
			&& a.highlightTint == b.highlightTint && a.highlightTintStrength == b.highlightTintStrength;
	}

	bool operator!=(const BackgroundGradeSettings& a, const BackgroundGradeSettings& b)
	{
		return !(a == b);
	}

	bool operator==(const NebulaSettings& a, const NebulaSettings& b)
	{
		return a.enabled == b.enabled && a.emissionRadius == b.emissionRadius && a.emissionDensity == b.emissionDensity
			&& a.dustOpticalDepth == b.dustOpticalDepth && a.dustSize == b.dustSize && a.reflection == b.reflection
			&& a.parallax == b.parallax && a.palette == b.palette && a.saturation == b.saturation;
	}

	bool operator!=(const NebulaSettings& a, const NebulaSettings& b)
	{
		return !(a == b);
	}

	bool operator==(const AnomalySettings& a, const AnomalySettings& b)
	{
		return a.pulsarChance == b.pulsarChance && a.shatteredMoonChance == b.shatteredMoonChance && a.cometChance == b.cometChance
			&& a.lensEinsteinRadius == b.lensEinsteinRadius && a.lensShadowRadius == b.lensShadowRadius
			&& a.lensRingBrightness == b.lensRingBrightness;
	}

	bool operator!=(const AnomalySettings& a, const AnomalySettings& b)
	{
		return !(a == b);
	}

	bool operator==(const WarpSettings& a, const WarpSettings& b)
	{
		return a.maxBeta == b.maxBeta && a.rampSeconds == b.rampSeconds && a.fieldOfViewRad == b.fieldOfViewRad;
	}

	bool operator!=(const WarpSettings& a, const WarpSettings& b)
	{
		return !(a == b);
	}

	bool operator==(const HostStarSettings& a, const HostStarSettings& b)
	{
		return a.enabled == b.enabled && a.screenX == b.screenX && a.screenY == b.screenY && a.radiusAt1AU == b.radiusAt1AU
			&& a.glowRadius == b.glowRadius && a.intensity == b.intensity && a.parallax == b.parallax;
	}

	bool operator!=(const HostStarSettings& a, const HostStarSettings& b)
	{
		return !(a == b);
	}

	bool operator==(const StarFieldSettings& a, const StarFieldSettings& b)
	{
		return a.baseCount == b.baseCount && a.maxCount == b.maxCount
			&& a.brightestMagnitude == b.brightestMagnitude && a.faintestMagnitude == b.faintestMagnitude
			&& a.magnitudeSlope == b.magnitudeSlope && a.minLevel == b.minLevel && a.levelGamma == b.levelGamma
			&& a.spikeMagnitude == b.spikeMagnitude && a.variableFraction == b.variableFraction
			&& a.bandFraction == b.bandFraction && a.nearFraction == b.nearFraction
			&& a.farParallax == b.farParallax && a.nearParallax == b.nearParallax;
	}

	bool operator!=(const StarFieldSettings& a, const StarFieldSettings& b)
	{
		return !(a == b);
	}

	bool operator==(const MilkyWaySettings& a, const MilkyWaySettings& b)
	{
		return a.enabled == b.enabled && a.angleDeg == b.angleDeg && a.offset == b.offset && a.width == b.width
			&& a.intensity == b.intensity && a.dustLane == b.dustLane && a.parallax == b.parallax
			&& a.glowIntensity == b.glowIntensity;
	}

	bool operator!=(const MilkyWaySettings& a, const MilkyWaySettings& b)
	{
		return !(a == b);
	}

	bool operator==(const SkyPaintSettings& a, const SkyPaintSettings& b)
	{
		return a.enabled == b.enabled && a.coverage == b.coverage && a.intensity == b.intensity && a.centerCalm == b.centerCalm
			&& a.farParallax == b.farParallax && a.midParallax == b.midParallax && a.nearParallax == b.nearParallax;
	}

	bool operator!=(const SkyPaintSettings& a, const SkyPaintSettings& b)
	{
		return !(a == b);
	}

	bool LoadPipelineSettings(const std::filesystem::path& file, PipelineSettings& out, std::string& error)
	{
		std::string text;
		if (!ReadTextFile(file, text))
		{
			error = "cannot read " + file.u8string();
			return false;
		}
		try
		{
			nlohmann::json root = nlohmann::json::parse(text);
			PipelineSettings s;

			const nlohmann::json& world = Section(root, "world");
			s.virtualHeight = std::clamp(world.value("virtualHeight", s.virtualHeight), 45, 2160);
			s.nebulaResolutionDivisor = std::clamp(Section(root, "nebula").value("resolutionDivisor", s.nebulaResolutionDivisor), 1, 8);
			s.ditherEnabled = world.value("dither", s.ditherEnabled);
			s.ditherSpread = std::clamp(world.value("ditherSpread", s.ditherSpread), 0.f, 1.f);

			const nlohmann::json& bloom = Section(root, "bloom");
			s.bloomEnabled = bloom.value("enabled", s.bloomEnabled);
			s.bloomThreshold = std::clamp(bloom.value("threshold", s.bloomThreshold), 0.f, 0.99f);
			s.bloomStrength = std::clamp(bloom.value("strength", s.bloomStrength), 0.f, 4.f);
			s.bloomLevels = std::clamp(bloom.value("levels", s.bloomLevels), 1, 5);

			const nlohmann::json& vignette = Section(root, "vignette");
			s.vignetteStrength = std::clamp(vignette.value("strength", s.vignetteStrength), 0.f, 1.f);
			s.vignetteRadius = std::clamp(vignette.value("radius", s.vignetteRadius), 0.1f, 1.5f);

			s.flashScale = std::clamp(Section(root, "flash").value("scale", s.flashScale), 0.f, 1.f);

			const nlohmann::json& planet = Section(root, "planet");
			PlanetSettings& p = s.planet;
			p.enabled = planet.value("enabled", p.enabled);
			p.screenX = std::clamp(planet.value("screenX", p.screenX), -0.5f, 1.5f);
			p.screenY = std::clamp(planet.value("screenY", p.screenY), -0.5f, 1.5f);
			p.radius = std::clamp(planet.value("radius", p.radius), 0.01f, 1.5f);
			p.parallax = std::clamp(planet.value("parallax", p.parallax), 0.f, 1.f);
			p.brightness = std::clamp(planet.value("brightness", p.brightness), 0.f, 1.f);
			p.saturation = std::clamp(planet.value("saturation", p.saturation), 0.f, 1.f);
			p.opacity = std::clamp(planet.value("opacity", p.opacity), 0.f, 1.f);
			p.rotationSpeed = std::clamp(planet.value("rotationSpeed", p.rotationSpeed), -1.f, 1.f);
			p.dither = std::clamp(planet.value("dither", p.dither), 0.f, 1.f);

			const nlohmann::json& hostStar = Section(root, "hostStar");
			HostStarSettings& h = s.hostStar;
			h.enabled = hostStar.value("enabled", h.enabled);
			h.screenX = std::clamp(hostStar.value("screenX", h.screenX), -0.5f, 1.5f);
			h.screenY = std::clamp(hostStar.value("screenY", h.screenY), -0.5f, 1.5f);
			h.radiusAt1AU = std::clamp(hostStar.value("radiusAt1AU", h.radiusAt1AU), 0.0005f, 0.2f);
			h.glowRadius = std::clamp(hostStar.value("glowRadius", h.glowRadius), 0.01f, 0.5f);
			h.intensity = std::clamp(hostStar.value("intensity", h.intensity), 0.f, 1.f);
			h.parallax = std::clamp(hostStar.value("parallax", h.parallax), 0.f, 1.f);

			const nlohmann::json& starField = Section(root, "starField");
			StarFieldSettings& f = s.starField;
			f.baseCount = std::clamp(starField.value("baseCount", f.baseCount), 0, 20000);
			f.maxCount = std::clamp(starField.value("maxCount", f.maxCount), 0, 20000);
			f.brightestMagnitude = std::clamp(starField.value("brightestMagnitude", f.brightestMagnitude), -5.f, 12.f);
			f.faintestMagnitude = std::clamp(starField.value("faintestMagnitude", f.faintestMagnitude), -5.f, 12.f);
			f.magnitudeSlope = std::clamp(starField.value("magnitudeSlope", f.magnitudeSlope), 0.f, 2.f);
			f.minLevel = std::clamp(starField.value("minLevel", f.minLevel), 0.f, 1.f);
			f.levelGamma = std::clamp(starField.value("levelGamma", f.levelGamma), 0.1f, 5.f);
			f.spikeMagnitude = std::clamp(starField.value("spikeMagnitude", f.spikeMagnitude), -5.f, 12.f);
			f.variableFraction = std::clamp(starField.value("variableFraction", f.variableFraction), 0.f, 1.f);
			f.bandFraction = std::clamp(starField.value("bandFraction", f.bandFraction), 0.f, 1.f);
			f.nearFraction = std::clamp(starField.value("nearFraction", f.nearFraction), 0.f, 1.f);
			f.farParallax = std::clamp(starField.value("farParallax", f.farParallax), 0.f, 1.f);
			f.nearParallax = std::clamp(starField.value("nearParallax", f.nearParallax), 0.f, 1.f);
			if (f.faintestMagnitude <= f.brightestMagnitude + 0.5f)
			{
				error = "starField.faintestMagnitude must be more than brightestMagnitude + 0.5";
				return false;
			}
			if (f.maxCount < f.baseCount)
			{
				error = "starField.maxCount must not be less than baseCount";
				return false;
			}

			const nlohmann::json& milkyWay = Section(root, "milkyWay");
			MilkyWaySettings& m = s.milkyWay;
			m.enabled = milkyWay.value("enabled", m.enabled);
			m.angleDeg = std::clamp(milkyWay.value("angleDeg", m.angleDeg), -360.f, 360.f);
			m.offset = std::clamp(milkyWay.value("offset", m.offset), -1.f, 1.f);
			m.width = std::clamp(milkyWay.value("width", m.width), 0.01f, 1.f);
			m.intensity = std::clamp(milkyWay.value("intensity", m.intensity), 0.f, 1.f);
			m.dustLane = std::clamp(milkyWay.value("dustLane", m.dustLane), 0.f, 1.f);
			m.parallax = std::clamp(milkyWay.value("parallax", m.parallax), 0.f, 1.f);
			m.glowIntensity = std::clamp(milkyWay.value("glowIntensity", m.glowIntensity), 0.f, 1.f);

			const nlohmann::json& skyPaint = Section(root, "skyPaint");
			SkyPaintSettings& sp = s.skyPaint;
			sp.enabled = skyPaint.value("enabled", sp.enabled);
			sp.coverage = std::clamp(skyPaint.value("coverage", sp.coverage), 0.f, 1.f);
			sp.intensity = std::clamp(skyPaint.value("intensity", sp.intensity), 0.f, 2.f);
			sp.centerCalm = std::clamp(skyPaint.value("centerCalm", sp.centerCalm), 0.f, 1.f);
			sp.farParallax = std::clamp(skyPaint.value("farParallax", sp.farParallax), 0.f, 1.f);
			sp.midParallax = std::clamp(skyPaint.value("midParallax", sp.midParallax), 0.f, 1.f);
			sp.nearParallax = std::clamp(skyPaint.value("nearParallax", sp.nearParallax), 0.f, 1.f);

			const nlohmann::json& physical = Section(root, "physicalNebula");
			NebulaSettings& n = s.physicalNebula;
			n.enabled = physical.value("enabled", n.enabled);
			n.emissionRadius = std::clamp(physical.value("emissionRadius", n.emissionRadius), 0.05f, 1.f);
			n.emissionDensity = std::clamp(physical.value("emissionDensity", n.emissionDensity), 0.f, 4.f);
			n.dustOpticalDepth = std::clamp(physical.value("dustOpticalDepth", n.dustOpticalDepth), 0.f, 8.f);
			n.dustSize = std::clamp(physical.value("dustSize", n.dustSize), 0.05f, 1.5f);
			n.reflection = std::clamp(physical.value("reflection", n.reflection), 0.f, 1.f);
			n.parallax = std::clamp(physical.value("parallax", n.parallax), 0.f, 1.f);
			n.saturation = std::clamp(physical.value("saturation", n.saturation), 0.f, 3.f);
			if (physical.contains("palette"))
			{
				const std::string palette = physical.at("palette").get<std::string>();
				if (palette == "trueColor")
				{
					n.palette = NebulaPalette::TrueColor;
				}
				else if (palette == "narrowband")
				{
					n.palette = NebulaPalette::Narrowband;
				}
				else
				{
					error = "physicalNebula.palette must be trueColor or narrowband";
					return false;
				}
			}

			const nlohmann::json& anomalies = Section(root, "anomalies");
			AnomalySettings& a = s.anomalies;
			a.pulsarChance = std::clamp(anomalies.value("pulsarChance", a.pulsarChance), 0.f, 1.f);
			a.shatteredMoonChance = std::clamp(anomalies.value("shatteredMoonChance", a.shatteredMoonChance), 0.f, 1.f);
			a.cometChance = std::clamp(anomalies.value("cometChance", a.cometChance), 0.f, 1.f);
			a.lensEinsteinRadius = std::clamp(anomalies.value("lensEinsteinRadius", a.lensEinsteinRadius), 0.f, 0.5f);
			a.lensShadowRadius = std::clamp(anomalies.value("lensShadowRadius", a.lensShadowRadius), 0.f, 0.5f);
			a.lensRingBrightness = std::clamp(anomalies.value("lensRingBrightness", a.lensRingBrightness), 0.f, 1.f);

			const nlohmann::json& warp = Section(root, "warp");
			WarpSettings& w = s.warp;
			w.maxBeta = std::clamp(warp.value("maxBeta", w.maxBeta), 0.f, 0.99f);
			w.rampSeconds = std::clamp(warp.value("rampSeconds", w.rampSeconds), 0.05f, 10.f);
			w.fieldOfViewRad = std::clamp(warp.value("fieldOfViewRad", w.fieldOfViewRad), 0.2f, 3.f);

			const nlohmann::json& grade = Section(root, "backgroundGrade");
			BackgroundGradeSettings& g = s.backgroundGrade;
			g.enabled = grade.value("enabled", g.enabled);
			g.contrast = std::clamp(grade.value("contrast", g.contrast), 0.5f, 3.f);
			g.saturation = std::clamp(grade.value("saturation", g.saturation), 0.f, 3.f);
			g.blackPoint = std::clamp(grade.value("blackPoint", g.blackPoint), 0.f, 0.2f);
			g.shadowTintStrength = std::clamp(grade.value("shadowTintStrength", g.shadowTintStrength), 0.f, 1.f);
			g.highlightTintStrength = std::clamp(grade.value("highlightTintStrength", g.highlightTintStrength), 0.f, 1.f);
			if (!ReadTint(grade, "shadowTint", g.shadowTint, error) || !ReadTint(grade, "highlightTint", g.highlightTint, error))
			{
				return false;
			}

			const nlohmann::json& style = Section(root, "backgroundStyle");
			BackgroundStyleSettings& bs = s.backgroundStyle;
			bs.enabled = style.value("enabled", bs.enabled);
			bs.pixelSize = std::clamp(style.value("pixelSize", bs.pixelSize), 0.f, 8.f);
			bs.toneLevels = std::clamp(style.value("toneLevels", bs.toneLevels), 2, 8);
			bs.ditherSpread = std::clamp(style.value("ditherSpread", bs.ditherSpread), 0.f, 0.5f);
			if (style.contains("roles"))
			{
				const nlohmann::json& roles = style.at("roles");
				if (!roles.is_array() || !std::all_of(roles.begin(), roles.end(), [](const nlohmann::json& r) { return r.is_string(); }))
				{
					error = "backgroundStyle.roles must be an array of role names";
					return false;
				}
				bs.roles.clear();
				for (const nlohmann::json& role : roles)
				{
					bs.roles.push_back(role.get<std::string>());
				}
			}

			const nlohmann::json& nearField = Section(root, "nearField");
			s.nearField.enabled = nearField.value("enabled", s.nearField.enabled);

			const nlohmann::json& energy = Section(root, "energy");
			s.maxEnergyDraws = std::clamp(energy.value("maxDraws", s.maxEnergyDraws), 1, 4096);
			s.maxEnergyAreaRatio = std::clamp(energy.value("maxAreaRatio", s.maxEnergyAreaRatio), 0.05f, 32.f);
			const nlohmann::json& caps = Section(energy, "tierMaxBrightness");
			for (int i = 0; i < LuminanceTierCount; ++i)
			{
				const char* name = TierName(static_cast<LuminanceTier>(i));
				s.tierMaxBrightness[i] = std::clamp(caps.value(name, s.tierMaxBrightness[i]), 0.f, 1.f);
			}
			for (int i = 1; i < LuminanceTierCount; ++i)
			{
				if (s.tierMaxBrightness[i] < s.tierMaxBrightness[i - 1])
				{
					error = std::string("tierMaxBrightness must not decrease: ") + TierName(static_cast<LuminanceTier>(i))
						+ " is dimmer than " + TierName(static_cast<LuminanceTier>(i - 1));
					return false;
				}
			}
			out = s;
			return true;
		}
		catch (const nlohmann::json::exception& e)
		{
			error = e.what();
			return false;
		}
	}
}

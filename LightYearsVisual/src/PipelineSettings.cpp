#include "visual/PipelineSettings.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "FileUtil.h"

namespace ly::visual
{
	namespace
	{
		const nlohmann::json& Section(const nlohmann::json& root, const char* name)
		{
			static const nlohmann::json empty = nlohmann::json::object();
			auto it = root.find(name);
			return it != root.end() && it->is_object() ? *it : empty;
		}
	}

	bool LoadPipelineSettings(const std::filesystem::path& file, PipelineSettings& out, std::string& error)
	{
		std::string text;
		if (!ReadTextFile(file, text))
		{
			error = "cannot read " + file.string();
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
			p.rotationSpeed = std::clamp(planet.value("rotationSpeed", p.rotationSpeed), -1.f, 1.f);
			p.dither = std::clamp(planet.value("dither", p.dither), 0.f, 1.f);

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

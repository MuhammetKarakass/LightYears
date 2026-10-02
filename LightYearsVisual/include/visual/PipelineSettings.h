#pragma once

#include <array>
#include <filesystem>
#include <string>

#include "visual/LuminanceTier.h"

namespace ly::visual
{
	// Every runtime knob of the render pipeline. Loaded from pipeline.json, editable while running.
	struct PipelineSettings
	{
		int virtualHeight = 240;
		bool ditherEnabled = true;
		float ditherSpread = 0.1f;

		bool bloomEnabled = true;
		float bloomThreshold = 0.55f;
		float bloomStrength = 0.9f;
		int bloomLevels = 3;

		float vignetteStrength = 0.4f;
		float vignetteRadius = 0.7f;
		float flashScale = 1.f;

		int maxEnergyDraws = 96;
		float maxEnergyAreaRatio = 1.5f;
		std::array<float, LuminanceTierCount> tierMaxBrightness = {0.35f, 0.5f, 0.65f, 0.85f, 1.f};
	};

	// Starts from defaults, applies the file, validates. On failure out is left untouched.
	bool LoadPipelineSettings(const std::filesystem::path& file, PipelineSettings& out, std::string& error);
}

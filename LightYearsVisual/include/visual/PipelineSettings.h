#pragma once

#include <array>
#include <filesystem>
#include <string>

#include "visual/LuminanceTier.h"

namespace ly::visual
{
	// The backdrop planet (drawn by the game into the dithered World layer). Positions are fractions of the output
	// size at the camera start; the planet then drifts against camera motion by `parallax`.
	struct PlanetSettings
	{
		bool enabled = true;
		float screenX = 0.95f;
		float screenY = 0.14f;
		// fraction of the output height
		float radius = 0.15f;
		// output pixels of drift per output pixel of camera motion (0 = fixed to the screen)
		float parallax = 0.025f;
		float brightness = 0.45f;
		float saturation = 0.55f;
		float rotationSpeed = 0.03f;
		float dither = 0.3f;
	};

	// Every runtime knob of the render pipeline. Loaded from pipeline.json, editable while running.
	struct PipelineSettings
	{
		int virtualHeight = 240;
		int nebulaResolutionDivisor = 1;
		bool ditherEnabled = true;
		float ditherSpread = 0.1f;

		bool bloomEnabled = true;
		float bloomThreshold = 0.55f;
		float bloomStrength = 0.9f;
		int bloomLevels = 3;

		float vignetteStrength = 0.4f;
		float vignetteRadius = 0.7f;
		float flashScale = 1.f;

		PlanetSettings planet;

		int maxEnergyDraws = 96;
		float maxEnergyAreaRatio = 1.5f;
		std::array<float, LuminanceTierCount> tierMaxBrightness = {0.35f, 0.5f, 0.65f, 0.85f, 1.f};
	};

	// Starts from defaults, applies the file, validates. On failure out is left untouched.
	bool LoadPipelineSettings(const std::filesystem::path& file, PipelineSettings& out, std::string& error);
}

#pragma once

#include <SFML/Graphics.hpp>

#include "visual/ShaderLibrary.h"

namespace ly::visual
{
	// TrueColor: the eye's colours of the lines (H-alpha red/pink, [O III] teal). Narrowband: the "Hubble palette" (SHO),
	// a real false-colour mapping used for HST images: [S II] -> red, H-alpha -> green, [O III] -> blue.
	enum class NebulaPalette { TrueColor, Narrowband };

	// HII region around a hot ionizing star. Colours are emission lines (Case B recombination: H-alpha/H-beta = 2.86);
	// [O III] needs ~35 eV photons so it glows only in the inner zone and only around hot (O-type) stars; [S II] and
	// [N II] peak at the ionization front at the edge (Stromgren radius).
	struct EmissionNebulaParams
	{
		sf::Vector2f center;          // ionizing star, output pixels
		float radius = 300.f;         // Stromgren radius, output pixels
		sf::Vector2f scroll;          // camera motion, output pixels (parallax applied inside)
		float parallax = 0.03f;
		float ionizingTemperatureK = 38000.f; // sets the [O III] share: 0 below ~30 000 K, full above ~45 000 K
		float density = 1.f;          // overall brightness scale
		float seed = 0.f;
		float maxBrightness = 0.35f;
		NebulaPalette palette = NebulaPalette::TrueColor;
		float saturation = 1.35f;     // chroma boost, luma preserved
		float timeSeconds = 0.f;      // slow drift of the filaments
	};
	void DrawEmissionNebula(ShaderLibrary& shaders, sf::RenderTarget& nebulaTarget, const EmissionNebulaParams& params);

	// Cold dust cloud: absorbs light behind it (extinction grows toward the cloud core, filamentary edges, dark globules)
	// and scatters the light of a nearby star (reflection nebula; small grains scatter blue more strongly).
	struct DustCloudParams
	{
		sf::Vector2f center;          // output pixels
		sf::Vector2f size{600.f, 300.f};
		float angleRad = 0.f;
		sf::Vector2f scroll;
		float parallax = 0.03f;
		float opticalDepth = 1.5f;    // peak tau; transmitted fraction = exp(-tau)
		float reflection = 0.f;       // 0..1 strength of scattered starlight
		float illuminatorTemperatureK = 15000.f;
		sf::Vector2f illuminator;     // star lighting the dust, output pixels
		float seed = 0.f;
		float maxBrightness = 0.35f;
		float pixelSize = 3.f;        // output px per style pixel of the dust grid (the world pixel scale)
		bool stylize = false;         // banded style pixels (dust.frag); false draws smooth values
	};
	// Two passes on the Background target: multiplicative extinction (darkens what is already drawn: gradient, band,
	// emission), then additive reflected light.
	void DrawDustCloud(ShaderLibrary& shaders, sf::RenderTarget& backgroundTarget, const DustCloudParams& params);

	// Data-driven defaults for the backdrop; wired into pipeline.json later by the integration agent.
	struct NebulaSettings
	{
		bool enabled = true;
		// Emission is a distant HII region, not the sky: a small, dimmer cloud keeps it from covering the view.
		float emissionRadius = 0.18f;     // fraction of output height
		float emissionDensity = 0.6f;
		float dustOpticalDepth = 0.9f;
		float dustSize = 0.35f;           // fraction of output height
		float reflection = 0.5f;
		float parallax = 0.03f;
		NebulaPalette palette = NebulaPalette::TrueColor;
		float saturation = 1.35f;
	};
}

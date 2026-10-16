#pragma once

#include "sky/Astro.h"

// A compact Milky Way model for deciding what the sky looks like from a point in the galaxy.
// Structure (published, rounded values): exponential thin + thick disk, a central bulge/bar, a nuclear star cluster
// around Sgr A*, logarithmic spiral arms and a thin dust disk. Densities are relative to the solar neighbourhood, so
// the Sun's position returns ~1. Pure math, no SFML; cheap enough to call once per stop or per second, not per pixel.
namespace vega::sky::galaxy
{
	constexpr float SunRadiusKpc = 8.2f;
	constexpr float SunHeightKpc = 0.02f;
	constexpr float LightYearsPerKpc = 3261.56f;

	// Galactocentric cylindrical coordinates. azimuth is measured from the Sun-centre line (Sun at 0) in radians.
	struct GalacticPosition
	{
		float radiusKpc = SunRadiusKpc;
		float heightKpc = SunHeightKpc;
		float azimuthRad = 0.f;
	};

	enum class Arm
	{
		None,
		Norma3Kpc,
		ScutumCentaurus,
		SagittariusCarina,
		LocalOrion,
		Perseus,
		Outer
	};

	const char* ArmName(Arm arm);

	// Everything the backdrop needs about the sky at one position.
	struct SkyState
	{
		// Local stellar density, Sun = 1. Grows by orders of magnitude toward the centre.
		float starDensity = 1.f;
		// Mix of visible star classes: young hot stars in arms, old red giants in the bulge.
		astro::StellarMix stellarMix;
		// 0 = between arms, 1 = on an arm's ridge (star-forming regions, emission nebulae, young blue stars).
		float armProximity = 0.f;
		Arm nearestArm = Arm::None;
		// 0 = disk population only, 1 = deep inside the bulge.
		float bulgeFraction = 0.f;
		// Local dust density, Sun = 1.
		float dustDensity = 1.f;
		// [Fe/H] in dex, Sun = 0. Higher toward the centre (more rocky/giant planets), lower outward.
		float metallicity = 0.f;
		// Surface brightness of the Milky Way band looking along the plane toward the centre / away from it,
		// integrated along the line of sight with dust extinction. Normalized so the Sun's average of the two is 1.
		float bandCenterSide = 1.f;
		float bandAntiSide = 1.f;
		// Diffuse glow of the whole sky from unresolved stars, Sun ~0 (dark sky), approaching 1 deep in the bulge.
		float skyGlow = 0.f;
	};

	// Stellar mass density relative to the solar neighbourhood (thin + thick disk + bulge + nuclear cluster).
	float StellarDensity(const GalacticPosition& position);
	// Dust density relative to the solar neighbourhood (thin exponential dust disk, enhanced on arms).
	float DustDensity(const GalacticPosition& position);
	// 0..1 closeness to the nearest arm ridge; writes that arm to nearest when non-null.
	float ArmProximity(const GalacticPosition& position, Arm* nearest = nullptr);
	// [Fe/H]: about -0.06 dex/kpc radial gradient through the Sun, flattening to a mildly super-solar bulge.
	float Metallicity(const GalacticPosition& position);

	SkyState ComputeSky(const GalacticPosition& position);

	// A sample radial path from the Sun (progress 0) to Sgr A* (progress 1) along azimuth 0, settling into
	// the plane. progress is clamped to [0,1]; radius never reaches exactly 0.
	GalacticPosition RoutePosition(float progress);
	// Distance to the galactic centre in light years.
	float DistanceToCenterLy(const GalacticPosition& position);
}

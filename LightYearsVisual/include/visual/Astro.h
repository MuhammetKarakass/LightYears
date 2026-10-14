#pragma once

#include <cstdint>

// Small astrophysics helpers for the backdrop: blackbody color, magnitudes and star temperature sampling.
// Pure math, no SFML; everything is deterministic for the same inputs.
namespace ly::visual::astro
{
	// Linear (not gamma-encoded) sRGB.
	struct LinearRgb
	{
		float r = 0.f;
		float g = 0.f;
		float b = 0.f;
	};

	struct Rgb8
	{
		std::uint8_t r = 0;
		std::uint8_t g = 0;
		std::uint8_t b = 0;
	};

	constexpr float MinStarTemperatureK = 1000.f;
	constexpr float MaxStarTemperatureK = 40000.f;

	// Chromaticity of a blackbody at temperatureK (clamped to [MinStarTemperatureK, MaxStarTemperatureK]):
	// Planck spectrum integrated against the CIE 1931 2-degree observer, XYZ -> linear sRGB (D65), negative channels
	// clipped, then scaled so the brightest channel is 1. ~6500 K is near white, cooler is orange, hotter is blue-white.
	LinearRgb BlackbodyColor(float temperatureK);

	// Linear channel (0..1, clamped) -> 8-bit sRGB with the standard transfer curve.
	std::uint8_t LinearToSrgb8(float linear);
	// color * scale per channel, then LinearToSrgb8.
	Rgb8 ToSrgb8(LinearRgb color, float scale = 1.f);

	// Relative flux of an apparent magnitude: 10^(-0.4 m). Magnitude 0 -> 1, each +5 mag -> 1/100.
	float MagnitudeToFlux(float magnitude);
	// Inverse of MagnitudeToFlux; flux must be > 0.
	float FluxToMagnitude(float flux);
	// Distance modulus: m = M + 5 log10(d / 10 pc). distanceParsec must be > 0.
	float ApparentMagnitude(float absoluteMagnitude, float distanceParsec);

	// Star counts grow by 10^slope per magnitude fainter (slope ~0.5 for a disk population seen from inside).
	// Maps uniform u in [0,1) to a magnitude in [brightest, faintest] with N(<m) proportional to 10^(slope m),
	// so most samples are near `faintest` and only a few are near `brightest`.
	float SampleMagnitude(float u, float brightest, float faintest, float slope);

	// Visible-star mix by broad class. Weights need not sum to 1; they are normalized when sampling.
	struct StellarMix
	{
		// O/B main sequence, 10 000 - 35 000 K: young, found in spiral arms and star-forming regions.
		float hot = 0.f;
		// A/F, 6 000 - 10 000 K.
		float white = 0.f;
		// G/K dwarfs and subgiants, 4 500 - 6 000 K.
		float yellow = 0.f;
		// K/M giants, 3 000 - 4 500 K: old, dominate the bulge.
		float red = 0.f;
	};

	// Picks a class with uClass and a temperature inside that class with uTemp (both uniform in [0,1)).
	// Hot temperatures are skewed toward the cool end of their range (hotter stars are rarer).
	float SampleStarTemperature(const StellarMix& mix, float uClass, float uTemp);

	// Emission line wavelengths in nm (air).
	constexpr float HAlphaNm = 656.28f;
	constexpr float HBetaNm = 486.13f;
	constexpr float OIIINm = 500.68f;   // [O III] 500.7
	constexpr float NIINm = 658.35f;    // [N II]
	constexpr float SIINm = 671.6f;     // [S II] 6716/6731 blend

	// Colour of monochromatic light: CIE 1931 2-degree response at nm -> XYZ -> linear sRGB. Spectral colours are outside
	// the sRGB gamut, so the result is desaturated toward white just enough to make every channel >= 0, then scaled so
	// the brightest channel is 1. Valid 380..780 nm (clamped).
	LinearRgb WavelengthColor(float nm);
}

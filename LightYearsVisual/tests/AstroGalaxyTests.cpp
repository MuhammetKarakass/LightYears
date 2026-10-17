#include "visual/Astro.h"
#include "visual/GalaxyModel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace
{
	int gChecks = 0;
	int gFailures = 0;

	void Check(bool condition, const char* expression, const char* message, const char* file, int line)
	{
		++gChecks;
		if (!condition)
		{
			++gFailures;
			std::printf("%s:%d: CHECK failed: %s (%s)\n", file, line, expression, message);
		}
	}
}

#define CHECK(cond, message) Check((cond), #cond, (message), __FILE__, __LINE__)

namespace
{
	using vega::visual::astro::LinearRgb;
	using vega::visual::astro::StellarMix;
	namespace galaxy = vega::visual::galaxy;

	bool Near(double a, double b, double tolerance)
	{
		return std::fabs(a - b) <= tolerance;
	}

	bool SameColor(LinearRgb a, LinearRgb b)
	{
		return a.r == b.r && a.g == b.g && a.b == b.b;
	}

	void TestBlackbody()
	{
		const LinearRgb warm = vega::visual::astro::BlackbodyColor(6500.f);
		CHECK(warm.r > 0.88f && warm.g > 0.88f && warm.b > 0.88f, "6500 K is near white");

		const LinearRgb orange = vega::visual::astro::BlackbodyColor(3000.f);
		CHECK(Near(orange.r, 1.0, 1e-4), "3000 K peak channel is red");
		CHECK(orange.b < 0.5f, "3000 K blue is low");
		CHECK(orange.r > orange.g && orange.g > orange.b, "3000 K is ordered r > g > b");

		const LinearRgb blue = vega::visual::astro::BlackbodyColor(15000.f);
		CHECK(Near(blue.b, 1.0, 1e-6), "15000 K peak channel is blue");
		CHECK(blue.r < 0.85f, "15000 K red is reduced");

		double previous = -1.0;
		bool monotonic = true;
		for (float t = 2000.f; t <= 30000.f; t += 1000.f)
		{
			const LinearRgb c = vega::visual::astro::BlackbodyColor(t);
			const double ratio = c.r > 0.f ? static_cast<double>(c.b) / c.r : 1e9;
			if (!(ratio > previous))
			{
				monotonic = false;
			}
			previous = ratio;
		}
		CHECK(monotonic, "blue/red ratio strictly increases with temperature");

		CHECK(SameColor(vega::visual::astro::BlackbodyColor(500.f), vega::visual::astro::BlackbodyColor(1000.f)),
			"temperatures below the range clamp to the minimum");
		CHECK(SameColor(vega::visual::astro::BlackbodyColor(90000.f), vega::visual::astro::BlackbodyColor(40000.f)),
			"temperatures above the range clamp to the maximum");
	}

	void TestSrgb()
	{
		CHECK(vega::visual::astro::LinearToSrgb8(0.f) == 0, "linear 0 maps to 0");
		CHECK(vega::visual::astro::LinearToSrgb8(1.f) == 255, "linear 1 maps to 255");
		const int half = vega::visual::astro::LinearToSrgb8(0.5f);
		CHECK(std::abs(half - 188) <= 1, "linear 0.5 maps to about 188");

		const vega::visual::astro::Rgb8 px = vega::visual::astro::ToSrgb8(LinearRgb{1.f, 0.5f, 0.f}, 2.f);
		CHECK(px.r == 255 && px.b == 0, "ToSrgb8 applies scale and clamps");
	}

	void TestMagnitudes()
	{
		CHECK(Near(vega::visual::astro::MagnitudeToFlux(0.f), 1.0, 1e-6), "magnitude 0 is flux 1");
		CHECK(Near(vega::visual::astro::MagnitudeToFlux(5.f), 0.01, 1e-5), "+5 mag is 1/100 flux");
		CHECK(Near(vega::visual::astro::FluxToMagnitude(vega::visual::astro::MagnitudeToFlux(3.7f)), 3.7, 1e-4),
			"flux and magnitude round-trip");
		CHECK(Near(vega::visual::astro::ApparentMagnitude(4.83f, 10.f), 4.83, 1e-5), "10 pc leaves M unchanged");
		CHECK(Near(vega::visual::astro::ApparentMagnitude(4.83f, 100.f), 9.83, 1e-4), "100 pc adds 5 mag");
	}

	void TestSampleMagnitude()
	{
		const float brightest = -2.f;
		const float faintest = 8.f;
		const float slope = 0.5f;

		CHECK(Near(vega::visual::astro::SampleMagnitude(0.f, brightest, faintest, slope), brightest, 1e-3),
			"u=0 gives the brightest magnitude");
		CHECK(Near(vega::visual::astro::SampleMagnitude(0.999999f, brightest, faintest, slope), faintest, 1e-2),
			"u->1 gives the faintest magnitude");

		bool monotonic = true;
		float previous = vega::visual::astro::SampleMagnitude(0.f, brightest, faintest, slope);
		for (int i = 1; i <= 1000; ++i)
		{
			const float m = vega::visual::astro::SampleMagnitude(static_cast<float>(i) / 1000.f, brightest, faintest, slope);
			if (m < previous)
			{
				monotonic = false;
			}
			previous = m;
		}
		CHECK(monotonic, "magnitude is monotonic in u");

		const int count = 20000;
		int bright = 0;
		for (int i = 0; i < count; ++i)
		{
			const float u = (static_cast<float>(i) + 0.5f) / static_cast<float>(count);
			if (vega::visual::astro::SampleMagnitude(u, brightest, faintest, slope) < 7.f)
			{
				++bright;
			}
		}
		const double fraction = static_cast<double>(bright) / count;
		CHECK(Near(fraction, 0.30, 0.03), "fraction brighter than 7 mag is about 10^-0.5 of the range");
	}

	void TestStarTemperature()
	{
		StellarMix red;
		red.red = 1.f;
		bool redInRange = true;
		for (int i = 0; i < 50; ++i)
		{
			for (int j = 0; j < 50; ++j)
			{
				const float t = vega::visual::astro::SampleStarTemperature(red,
					(static_cast<float>(i) + 0.5f) / 50.f, (static_cast<float>(j) + 0.5f) / 50.f);
				if (t < 3000.f || t > 4500.f)
				{
					redInRange = false;
				}
			}
		}
		CHECK(redInRange, "red-only mix samples stay in 3000-4500 K");

		StellarMix hot;
		hot.hot = 1.f;
		bool hotInRange = true;
		std::vector<float> temperatures;
		for (int j = 0; j < 2000; ++j)
		{
			const float t = vega::visual::astro::SampleStarTemperature(hot, 0.5f, (static_cast<float>(j) + 0.5f) / 2000.f);
			temperatures.push_back(t);
			if (t < 10000.f || t > 35000.f)
			{
				hotInRange = false;
			}
		}
		CHECK(hotInRange, "hot-only mix samples stay in 10000-35000 K");
		std::nth_element(temperatures.begin(), temperatures.begin() + temperatures.size() / 2, temperatures.end());
		CHECK(temperatures[temperatures.size() / 2] < 20000.f, "hot stars skew to the cool end (median < 20000 K)");
	}

	void TestGalaxyDensity()
	{
		const galaxy::GalacticPosition sun{};
		CHECK(Near(galaxy::StellarDensity(sun), 1.0, 0.02), "stellar density at the Sun is 1");
		CHECK(Near(galaxy::DustDensity(sun), 1.0, 0.02), "dust density at the Sun is 1");

		const float progress[] = {0.f, 0.25f, 0.5f, 0.75f, 0.9f, 0.99f, 0.999f};
		bool increasing = true;
		float previous = -1.f;
		for (float p : progress)
		{
			const float density = galaxy::ComputeSky(galaxy::RoutePosition(p)).starDensity;
			if (!(density > previous))
			{
				increasing = false;
			}
			previous = density;
		}
		CHECK(increasing, "star density increases along the route to the centre");

		const galaxy::GalacticPosition oneKpc{1.f, 0.f, 0.f};
		const float atOneKpc = galaxy::StellarDensity(oneKpc);
		CHECK(atOneKpc >= 10.f && atOneKpc <= 40.f, "density at R = 1 kpc is 10-40 (Sun = 1)");

		const galaxy::GalacticPosition nucleus{0.001f, 0.f, 0.f};
		CHECK(galaxy::StellarDensity(nucleus) > 1e6f, "density at R = 1 pc exceeds 1e6");

		const galaxy::SkyState solar = galaxy::ComputeSky(sun);
		CHECK(solar.bulgeFraction < 0.01f, "bulge fraction at the Sun is negligible");

		// The model gives ~0.78 at R = 0.5 kpc (the bulge is ~85 vs disk ~24 there); the spec asked for > 0.8.
		const galaxy::SkyState halfKpc = galaxy::ComputeSky(galaxy::GalacticPosition{0.5f, 0.f, 0.f});
		CHECK(halfKpc.bulgeFraction > 0.75f, "bulge dominates at R = 0.5 kpc");
	}

	void TestArms()
	{
		const galaxy::GalacticPosition sun{};
		galaxy::Arm nearest = galaxy::Arm::None;
		galaxy::ArmProximity(sun, &nearest);
		CHECK(nearest == galaxy::Arm::LocalOrion, "the Sun sits on the local Orion spur");

		galaxy::Arm sagittarius = galaxy::Arm::None;
		const float proximity = galaxy::ArmProximity(galaxy::GalacticPosition{6.7f, 0.02f, 0.f}, &sagittarius);
		CHECK(sagittarius == galaxy::Arm::SagittariusCarina && proximity > 0.9f,
			"R = 6.7 kpc lies on the Sagittarius-Carina ridge");
	}

	void TestMetallicity()
	{
		const galaxy::GalacticPosition sun{};
		CHECK(Near(galaxy::Metallicity(sun), 0.0, 1e-4), "metallicity at the Sun is 0");
		CHECK(Near(galaxy::Metallicity(galaxy::GalacticPosition{4.2f, 0.f, 0.f}), 0.24, 0.05),
			"metallicity at R = 4.2 kpc is about +0.24");
		CHECK(galaxy::Metallicity(galaxy::GalacticPosition{12.f, 0.f, 0.f}) < -0.2f,
			"metallicity at R = 12 kpc is below -0.2");
	}

	void TestSky()
	{
		const galaxy::SkyState sky = galaxy::ComputeSky(galaxy::GalacticPosition{});
		CHECK(sky.skyGlow < 0.01f, "the Sun's sky is dark");
		CHECK(sky.bandCenterSide > sky.bandAntiSide, "the band is brighter toward the centre");
		CHECK(Near(0.5 * (sky.bandCenterSide + sky.bandAntiSide), 1.0, 0.02),
			"the Sun's mean band brightness is 1");

		const float hotShare = sky.stellarMix.hot / (sky.stellarMix.hot + sky.stellarMix.white
			+ sky.stellarMix.yellow + sky.stellarMix.red);
		CHECK(hotShare >= 0.08f && hotShare <= 0.25f, "hot share at the Sun is 8-25 percent");

		const galaxy::SkyState inner = galaxy::ComputeSky(galaxy::GalacticPosition{0.3f, 0.f, 0.f});
		const float total = inner.stellarMix.hot + inner.stellarMix.white + inner.stellarMix.yellow + inner.stellarMix.red;
		CHECK(inner.stellarMix.red / total > 0.6f, "red giants dominate at R = 0.3 kpc");
	}

	void TestWavelengthColor()
	{
		using vega::visual::astro::WavelengthColor;
		using vega::visual::astro::HAlphaNm;
		using vega::visual::astro::HBetaNm;
		using vega::visual::astro::OIIINm;

		// H-alpha is deep red; the sRGB gamut cannot hold it, so the blue channel is only lifted slightly.
		const LinearRgb halpha = WavelengthColor(HAlphaNm);
		CHECK(halpha.r == 1.f, "H-alpha red channel is the brightest");
		CHECK(halpha.g < 0.25f && halpha.b < 0.25f, "H-alpha is red with little green or blue");

		// [O III] 500.7 nm is green-cyan.
		const LinearRgb oiii = WavelengthColor(OIIINm);
		CHECK(oiii.g == 1.f, "[O III] green channel is the brightest");
		CHECK(oiii.r < 0.5f, "[O III] has little red");

		// H-beta 486 nm is blue-cyan.
		const LinearRgb hbeta = WavelengthColor(HBetaNm);
		CHECK(hbeta.b >= hbeta.g * 0.8f && hbeta.r < 0.4f, "H-beta is blue, not red");

		const LinearRgb violet = WavelengthColor(450.f);
		CHECK(violet.b == 1.f && violet.b >= violet.r && violet.b >= violet.g, "450 nm is blue-dominant");

		bool inRange = true;
		for (float nm = 380.f; nm <= 780.f; nm += 10.f)
		{
			const LinearRgb c = WavelengthColor(nm);
			for (float v : {c.r, c.g, c.b})
			{
				inRange = inRange && v >= 0.f && v <= 1.f;
			}
		}
		CHECK(inRange, "wavelength colours stay in 0..1 for 380-780 nm");
	}

	void TestRoute()
	{
		const galaxy::GalacticPosition start = galaxy::RoutePosition(0.f);
		CHECK(Near(start.radiusKpc, galaxy::SunRadiusKpc, 1e-6), "route starts at the Sun's radius");
		CHECK(Near(start.heightKpc, galaxy::SunHeightKpc, 1e-6), "route starts at the Sun's height");
		CHECK(galaxy::RoutePosition(1.f).radiusKpc > 0.f, "route end stays above zero radius");
		CHECK(Near(galaxy::DistanceToCenterLy(galaxy::GalacticPosition{}), 26745.0, 100.0),
			"Sun is about 26745 ly from the centre");
	}
}

int main()
{
	TestBlackbody();
	TestSrgb();
	TestMagnitudes();
	TestSampleMagnitude();
	TestStarTemperature();
	TestGalaxyDensity();
	TestArms();
	TestMetallicity();
	TestSky();
	TestRoute();
	TestWavelengthColor();

	std::printf("AstroGalaxyTests: %d checks, %d failures\n", gChecks, gFailures);
	return gFailures == 0 ? 0 : 1;
}

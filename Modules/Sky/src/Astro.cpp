#include "sky/Astro.h"

#include <algorithm>
#include <cmath>

namespace vega::sky::astro
{
	namespace
	{
		// SI constants for Planck's law. The absolute scale cancels in the max-channel normalization.
		constexpr double PlanckConstant = 6.62607015e-34;
		constexpr double LightSpeed = 299792458.0;
		constexpr double BoltzmannConstant = 1.380649e-23;
		constexpr double HcOverK = PlanckConstant * LightSpeed / BoltzmannConstant;
		constexpr double TwoHc2 = 2.0 * PlanckConstant * LightSpeed * LightSpeed;

		// 380..780 nm inclusive, 5 nm steps, matching the range of the CIE fit below.
		constexpr double SpectrumMinNm = 380.0;
		constexpr double SpectrumStepNm = 5.0;
		constexpr int SpectrumSamples = 81;

		// Largest float below 1; keeps u in [0,1) after clamping.
		constexpr double UpperUnitD = 0.999999999;
		constexpr float UpperUnitF = 0.99999994f;

		// Direct integration is ~80 samples, so no temperature table is cached.
		constexpr double BlackbodyFloorK = MinStarTemperatureK;
		constexpr double BlackbodyCeilK = MaxStarTemperatureK;

		// Multi-lobe Gaussian fit of the CIE 1931 2-degree colour matching functions, lambda in nm.
		// Wyman, Sloan & Shirley 2013 (JCGT, "Simple Analytic Approximations to the CIE XYZ Color Matching Functions").
		double Lobe(double lambda, double mu, double sigmaBelow, double sigmaAbove)
		{
			const double sigma = lambda < mu ? sigmaBelow : sigmaAbove;
			const double t = (lambda - mu) / sigma;
			return std::exp(-0.5 * t * t);
		}

		double CieX(double lambda)
		{
			return 1.056 * Lobe(lambda, 599.8, 37.9, 31.0)
				+ 0.362 * Lobe(lambda, 442.0, 16.0, 26.7)
				- 0.065 * Lobe(lambda, 501.1, 20.4, 26.2);
		}

		double CieY(double lambda)
		{
			return 0.821 * Lobe(lambda, 568.8, 46.9, 40.5)
				+ 0.286 * Lobe(lambda, 530.9, 16.3, 31.1);
		}

		double CieZ(double lambda)
		{
			return 1.217 * Lobe(lambda, 437.0, 11.8, 36.0)
				+ 0.681 * Lobe(lambda, 459.0, 26.0, 13.8);
		}

		// Planck spectral radiance B(lambda, T), lambda in nm.
		double PlanckRadiance(double lambdaNm, double temperatureK)
		{
			const double lambda = lambdaNm * 1e-9;
			const double exponent = HcOverK / (lambda * temperatureK);
			return TwoHc2 / (std::pow(lambda, 5.0) * std::expm1(exponent));
		}

		double LogUniform(double low, double high, double u)
		{
			return low * std::pow(high / low, u);
		}

		struct TemperatureRange
		{
			double low;
			double high;
		};

		constexpr TemperatureRange HotRange{10000.0, 35000.0};
		constexpr TemperatureRange WhiteRange{6000.0, 10000.0};
		constexpr TemperatureRange YellowRange{4500.0, 6000.0};
		constexpr TemperatureRange RedRange{3000.0, 4500.0};
		// Hot-star temperatures are skewed to the cool end: T = 10000 * 3.5^(u^2).
		constexpr double HotSpan = 3.5;
	}

	LinearRgb BlackbodyColor(float temperatureK)
	{
		const double temperature = std::clamp(static_cast<double>(temperatureK), BlackbodyFloorK, BlackbodyCeilK);

		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		for (int i = 0; i < SpectrumSamples; ++i)
		{
			const double lambda = SpectrumMinNm + SpectrumStepNm * i;
			const double spectrum = PlanckRadiance(lambda, temperature);
			x += spectrum * CieX(lambda);
			y += spectrum * CieY(lambda);
			z += spectrum * CieZ(lambda);
		}

		// XYZ -> linear sRGB, D65 white.
		double r = 3.2406 * x - 1.5372 * y - 0.4986 * z;
		double g = -0.9689 * x + 1.8758 * y + 0.0415 * z;
		double b = 0.0557 * x - 0.2040 * y + 1.0570 * z;
		r = std::max(r, 0.0);
		g = std::max(g, 0.0);
		b = std::max(b, 0.0);

		const double peak = std::max({r, g, b});
		if (peak <= 0.0)
		{
			return LinearRgb{1.f, 1.f, 1.f};
		}
		return LinearRgb{static_cast<float>(r / peak), static_cast<float>(g / peak), static_cast<float>(b / peak)};
	}

	std::uint8_t LinearToSrgb8(float linear)
	{
		if (!(linear > 0.f))
		{
			return 0;
		}
		const double x = std::min(static_cast<double>(linear), 1.0);
		const double encoded = x <= 0.0031308 ? 12.92 * x : 1.055 * std::pow(x, 1.0 / 2.4) - 0.055;
		return static_cast<std::uint8_t>(std::lround(std::clamp(encoded, 0.0, 1.0) * 255.0));
	}

	Rgb8 ToSrgb8(LinearRgb color, float scale)
	{
		return Rgb8{
			LinearToSrgb8(color.r * scale),
			LinearToSrgb8(color.g * scale),
			LinearToSrgb8(color.b * scale)};
	}

	float MagnitudeToFlux(float magnitude)
	{
		return std::pow(10.f, -0.4f * magnitude);
	}

	float FluxToMagnitude(float flux)
	{
		return -2.5f * std::log10(flux);
	}

	float ApparentMagnitude(float absoluteMagnitude, float distanceParsec)
	{
		return absoluteMagnitude + 5.f * std::log10(distanceParsec / 10.f);
	}

	float SampleMagnitude(float u, float brightest, float faintest, float slope)
	{
		const float low = std::min(brightest, faintest);
		const float high = std::max(brightest, faintest);
		const float uc = std::clamp(u, 0.f, UpperUnitF);

		if (!(slope > 0.f))
		{
			return std::clamp(brightest + uc * (faintest - brightest), low, high);
		}

		// Cumulative count N(<m) ~ 10^(slope m) inverted in closed form.
		const double s = slope;
		const double a = std::pow(10.0, s * brightest);
		const double b = std::pow(10.0, s * faintest);
		const double m = std::log10(a + static_cast<double>(uc) * (b - a)) / s;
		return std::clamp(static_cast<float>(m), low, high);
	}

	float SampleStarTemperature(const StellarMix& mix, float uClass, float uTemp)
	{
		const double weights[4] = {
			std::max(0.0, static_cast<double>(mix.hot)),
			std::max(0.0, static_cast<double>(mix.white)),
			std::max(0.0, static_cast<double>(mix.yellow)),
			std::max(0.0, static_cast<double>(mix.red))};
		const double total = weights[0] + weights[1] + weights[2] + weights[3];

		// Nothing weighted: fall back to yellow.
		int index = 2;
		if (total > 0.0)
		{
			const double pick = std::clamp(static_cast<double>(uClass), 0.0, UpperUnitD) * total;
			double cumulative = 0.0;
			for (int i = 0; i < 4; ++i)
			{
				if (weights[i] <= 0.0)
				{
					continue;
				}
				index = i;
				cumulative += weights[i];
				if (pick < cumulative)
				{
					break;
				}
			}
		}

		const double t = std::clamp(static_cast<double>(uTemp), 0.0, UpperUnitD);
		double temperature = 0.0;
		TemperatureRange range = YellowRange;
		switch (index)
		{
		case 0:
			range = HotRange;
			temperature = HotRange.low * std::pow(HotSpan, t * t);
			break;
		case 1:
			range = WhiteRange;
			temperature = LogUniform(WhiteRange.low, WhiteRange.high, t);
			break;
		case 3:
			range = RedRange;
			temperature = LogUniform(RedRange.low, RedRange.high, t);
			break;
		default:
			range = YellowRange;
			temperature = LogUniform(YellowRange.low, YellowRange.high, t);
			break;
		}
		return static_cast<float>(std::clamp(temperature, range.low, range.high));
	}

	LinearRgb WavelengthColor(float nm)
	{
		const double lambda = std::clamp(static_cast<double>(nm), SpectrumMinNm, SpectrumMinNm + SpectrumStepNm * (SpectrumSamples - 1));
		const double x = CieX(lambda);
		const double y = CieY(lambda);
		const double z = CieZ(lambda);

		// XYZ -> linear sRGB, D65 white (same matrix as BlackbodyColor; D65 maps to (1, 1, 1)).
		double c[3] = {
			3.2406 * x - 1.5372 * y - 0.4986 * z,
			-0.9689 * x + 1.8758 * y + 0.0415 * z,
			0.0557 * x - 0.2040 * y + 1.0570 * z};

		// Mixing toward white by t keeps the sum but lifts a negative channel c_i to c_i + t (1 - c_i); pick the
		// smallest t that makes every channel non-negative.
		double t = 0.0;
		for (double value : c)
		{
			if (value < 0.0)
			{
				t = std::max(t, -value / (1.0 - value));
			}
		}
		t = std::min(t, 1.0);
		for (double& value : c)
		{
			value = std::max(value + t * (1.0 - value), 0.0);
		}

		const double peak = std::max({c[0], c[1], c[2]});
		if (peak <= 0.0)
		{
			return LinearRgb{1.f, 1.f, 1.f};
		}
		return LinearRgb{static_cast<float>(c[0] / peak), static_cast<float>(c[1] / peak), static_cast<float>(c[2] / peak)};
	}
}

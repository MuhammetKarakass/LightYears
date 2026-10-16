#pragma once

#include <algorithm>
#include <cmath>

// Special-relativity helpers for the star field: aberration, Doppler shift and beaming of starlight seen by an observer
// moving at beta = v/c. Header-only. Angles are passed as cosines so no trigonometry is needed per star.
namespace vega::sky::relativity
{
	// Keeps gamma finite; beta = 1 would be a division by zero.
	constexpr float kMaxBeta = 0.999f;

	// Lorentz factor 1 / sqrt(1 - beta^2).
	inline float Gamma(float beta)
	{
		const float b = std::clamp(beta, 0.f, kMaxBeta);
		return 1.f / std::sqrt(1.f - b * b);
	}

	// Aberration: a source at angle theta from the direction of motion in the rest frame appears at theta' with
	// cos(theta') = (cos(theta) + beta) / (1 + beta cos(theta)). Stars crowd toward the direction of motion.
	inline float AberratedCos(float cosTheta, float beta)
	{
		const float b = std::clamp(beta, 0.f, kMaxBeta);
		const float c = std::clamp(cosTheta, -1.f, 1.f);
		return (c + b) / (1.f + b * c);
	}

	// Doppler factor D = nu_observed / nu_emitted for light arriving from the OBSERVED angle theta' (cosine measured from
	// the direction of motion): D = 1 / (gamma (1 - beta cos theta')). Ahead (cos = 1) gives sqrt((1+b)/(1-b)) > 1,
	// blueshift. Substituting the aberrated angle, 1 - beta cos theta' = (1 - beta^2) / (1 + beta cos theta), so this
	// equals gamma (1 + beta cos theta) with the REST-frame angle theta. Pass the aberrated cosine from AberratedCos.
	inline float DopplerFactor(float cosThetaObserved, float beta)
	{
		const float b = std::clamp(beta, 0.f, kMaxBeta);
		const float c = std::clamp(cosThetaObserved, -1.f, 1.f);
		return 1.f / (Gamma(b) * (1.f - b * c));
	}

	// Bolometric flux of a moving source scales with D^4 (relativistic beaming), so the magnitude changes by
	// -2.5 log10(D^4) = -10 log10(D). Positive D (brighter) gives a negative delta.
	inline float BeamingMagnitudeDelta(float doppler)
	{
		return -10.f * std::log10(std::max(doppler, 1e-6f));
	}
}

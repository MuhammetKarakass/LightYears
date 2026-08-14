#pragma once

#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	// These states are intentionally specific to circular area telegraphs. They
	// describe the small runtime state needed by the typed visual profile and do
	// not form a universal presentation variant.
	enum class AreaTelegraphAnchorMode : std::uint8_t
	{
		FixedLocation,
		FollowActor
	};

	enum class AreaTelegraphProgressDriver : std::uint8_t
	{
		Timed,
		External
	};

	enum class AreaTelegraphPhase : std::uint8_t
	{
		Countdown,
		CompletionFeedback
	};

	struct AreaTelegraphRuntimeState
	{
		AreaTelegraphPhase phase = AreaTelegraphPhase::Countdown;
		float age = 0.f;
		float countdownProgress = 0.f;
		float completionProgress = 1.f;
		float completionAge = 0.f;
	};

	struct AreaTelegraphVisualState
	{
		sf::Color fillColor = sf::Color::Transparent;
		sf::Color outlineColor = sf::Color::Transparent;
		float fillScale = 1.f;
		float countdownRingScale = 1.f;
		float outlineScale = 1.f;
	};

	inline float SaturateAreaTelegraphProgress(float value)
	{
		return std::clamp(value, 0.f, 1.f);
	}

	inline float ResolveAreaTelegraphRadialProgress(
		const AreaTelegraphVisualDefinition& definition,
		float progress
	)
	{
		const float normalizedProgress = SaturateAreaTelegraphProgress(progress);
		const float logStrength = std::max(0.f, definition.radialGrowthLogStrength);
		const float phaseEnd = std::clamp(definition.radialGrowthPrimaryPhaseEnd, 0.f, 1.f);
		const float phaseFill = std::clamp(definition.radialGrowthPrimaryPhaseFill, 0.f, 1.f);
		const auto ResolveCurve = [logStrength](float phaseProgress)
		{
			const float normalizedPhaseProgress = SaturateAreaTelegraphProgress(phaseProgress);
			if (logStrength <= 0.f)
			{
				return normalizedPhaseProgress;
			}
			return SaturateAreaTelegraphProgress(
				std::log1p(logStrength * normalizedPhaseProgress) /
				std::log1p(logStrength)
			);
		};

		if (phaseEnd >= 1.f)
		{
			return ResolveCurve(normalizedProgress);
		}

		if (phaseEnd <= 0.f)
		{
			return phaseFill + (1.f - phaseFill) * ResolveCurve(normalizedProgress);
		}

		if (normalizedProgress <= phaseEnd)
		{
			return phaseFill * ResolveCurve(normalizedProgress / phaseEnd);
		}

		return phaseFill + (1.f - phaseFill) * ResolveCurve(
			(normalizedProgress - phaseEnd) / (1.f - phaseEnd)
		);
	}

	inline sf::Color BlendAreaTelegraphColor(
		const sf::Color& from,
		const sf::Color& to,
		float alpha
	)
	{
		const float t = SaturateAreaTelegraphProgress(alpha);
		const auto LerpChannel = [t](std::uint8_t first, std::uint8_t second)
		{
			return static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(first) + (static_cast<float>(second) - first) * t,
				0.f,
				255.f
			));
		};
		return {
			LerpChannel(from.r, to.r),
			LerpChannel(from.g, to.g),
			LerpChannel(from.b, to.b),
			LerpChannel(from.a, to.a)
		};
	}

	inline AreaTelegraphVisualState ResolveAreaTelegraphVisualState(
		const AreaTelegraphVisualDefinition& definition,
		const AreaTelegraphRuntimeState& runtime
	)
	{
		const bool completion = runtime.phase == AreaTelegraphPhase::CompletionFeedback;
		const float rawProgress = SaturateAreaTelegraphProgress(
			completion ? runtime.completionProgress : runtime.countdownProgress
		);
		const float completionDuration = std::max(0.001f, definition.completionFeedbackDuration);
		const float completionProgress = completion
			? SaturateAreaTelegraphProgress(runtime.completionAge / completionDuration)
			: 0.f;
		const float easedProgress = std::pow(
			rawProgress,
			std::max(1.f, definition.countdownEaseExponent)
		);
		const float radialProgress = definition.fillMode == AreaTelegraphFillMode::RadialProgress
			? ResolveAreaTelegraphRadialProgress(definition, rawProgress)
			: 1.f;
		const float sinePulse = (std::sin(runtime.age * definition.pulseSpeed) + 1.f) * 0.5f;
		const float intensity = definition.minimumPulse
			+ (definition.maximumPulse - definition.minimumPulse) * sinePulse;
		const float baseScale = completion
			? definition.completionStartScale
				+ (definition.completionEndScale - definition.completionStartScale) * completionProgress
			: definition.countdownStartScale
				+ (definition.countdownEndScale - definition.countdownStartScale) * easedProgress;
		const float pulseScale = 1.f + definition.pulseScaleAmount * sinePulse;
		const float minimumFillRadiusRatio = definition.fillMode == AreaTelegraphFillMode::RadialProgress
			? SaturateAreaTelegraphProgress(definition.minimumFillRadiusRatio)
			: 0.f;
		const float chargedRadiusProgress = minimumFillRadiusRatio
			+ (1.f - minimumFillRadiusRatio) * radialProgress;
		const float completionScale = completion ? baseScale : 1.f;
		const float completionRadiusScale = completion ? chargedRadiusProgress : 1.f;

		AreaTelegraphVisualState state;
		state.fillColor = BlendAreaTelegraphColor(
			definition.fillColor,
			definition.dangerFillColor,
			easedProgress
		);
		state.outlineColor = BlendAreaTelegraphColor(
			definition.outlineColor,
			definition.dangerOutlineColor,
			easedProgress
		);
		if (completion)
		{
			state.fillColor = definition.completionFillColor;
			state.outlineColor = definition.completionOutlineColor;
		}
		state.fillColor.a = static_cast<std::uint8_t>(std::clamp(
			state.fillColor.a * intensity,
			0.f,
			255.f
		));
		state.outlineColor.a = static_cast<std::uint8_t>(std::clamp(
			state.outlineColor.a * intensity,
			0.f,
			255.f
		));
		if (completion)
		{
			state.fillColor.a = static_cast<std::uint8_t>(state.fillColor.a * (1.f - completionProgress));
			state.outlineColor.a = static_cast<std::uint8_t>(state.outlineColor.a * (1.f - completionProgress));
		}
		if (!definition.drawInteriorFill)
		{
			state.fillColor.a = 0;
		}

		state.fillScale = (definition.fillMode == AreaTelegraphFillMode::RadialProgress
			? chargedRadiusProgress
			: 1.f) * completionScale * pulseScale;
		state.countdownRingScale = baseScale;
		state.outlineScale = completionRadiusScale * completionScale * pulseScale;
		return state;
	}
}

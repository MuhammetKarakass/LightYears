#include "gameplay/ability/actors/sunBeam/SunBeamVisual.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		float Saturate(float value)
		{
			return std::clamp(value, 0.f, 1.f);
		}

		sf::Color WithIntensity(const sf::Color& color, float intensity)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * Saturate(intensity),
				0.f,
				255.f
			));
			return result;
		}
	}

	void SunBeamVisual::Configure(
		const SunBeamVisualDefinition& definition,
		float beamWidth,
		float beamLength,
		float impactRadius
	)
	{
		mDefinition = definition;
		mBeamWidth = std::max(1.f, beamWidth);
		mVisibleBeamLength = std::max(1.f, beamLength * std::max(0.f, definition.visibleLengthScale));
		mImpactRadius = std::max(1.f, impactRadius);

		mOuterColumn.setPointCount(4);
		mCoreColumn.setPointCount(4);
		ConfigureColumnGeometry(definition.arrivalStartWidthScale);

		mGroundGlow.setRadius(mImpactRadius);
		mGroundGlow.setOrigin({ mImpactRadius, mImpactRadius });

		mImpactRing.setRadius(mImpactRadius);
		mImpactRing.setOrigin({ mImpactRadius, mImpactRadius });
		mImpactRing.setFillColor(sf::Color::Transparent);
		mImpactRing.setOutlineThickness(std::max(0.f, definition.impactRingThickness));

		const float overheadOuterRadius = std::max(2.f, mBeamWidth * 0.75f);
		const float overheadCoreRadius = std::max(1.f, mBeamWidth * 0.28f);
		mOverheadOuter.setRadius(overheadOuterRadius);
		mOverheadOuter.setOrigin({ overheadOuterRadius, overheadOuterRadius });
		mOverheadCore.setRadius(overheadCoreRadius);
		mOverheadCore.setOrigin({ overheadCoreRadius, overheadCoreRadius });

		mConvergenceHalo.setRadius(mImpactRadius);
		mConvergenceHalo.setOrigin({ mImpactRadius, mImpactRadius });
		mConvergenceHalo.setFillColor(sf::Color::Transparent);
		mConvergenceHalo.setOutlineThickness(std::max(1.f, definition.impactRingThickness * 0.5f));

		const float glareLength = std::max(
			mBeamWidth,
			mImpactRadius * std::max(1.f, definition.impactGlareLengthScale)
		);
		const float glareThickness = std::max(2.f, mBeamWidth * 0.12f);
		mHorizontalGlare.setSize({ glareLength, glareThickness });
		mHorizontalGlare.setOrigin({ glareLength * 0.5f, glareThickness * 0.5f });
		mVerticalGlare.setSize({ glareThickness, glareLength });
		mVerticalGlare.setOrigin({ glareThickness * 0.5f, glareLength * 0.5f });

	}

	void SunBeamVisual::Draw(
		sf::RenderTarget& target,
		const sf::Vector2f& impactLocation,
		const SunBeamVisualFrame& frame,
		float age
	)
	{
		const float pulse = 0.75f + 0.25f * std::sin(age * std::max(0.f, mDefinition.pulseSpeed));
		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;

		if (frame.groundGlow)
		{
			const SunBeamGroundGlowVisualState& state = *frame.groundGlow;
			const float scale = std::max(0.f, state.scale);
			mGroundGlow.setPosition(impactLocation);
			mGroundGlow.setScale({ scale, scale });
			mGroundGlow.setFillColor(WithIntensity(mDefinition.groundGlowColor, state.intensity * pulse));
			target.draw(mGroundGlow, additiveStates);
		}

		if (frame.radialPulse)
		{
			const SunBeamRadialPulseVisualState& state = *frame.radialPulse;
			const float progress = Saturate(state.progress);
			const float scale = 0.55f + (std::max(0.55f, mDefinition.impactRingEndScale) - 0.55f) * progress;
			mImpactRing.setPosition(impactLocation);
			mImpactRing.setScale({ scale, scale });
			mImpactRing.setOutlineColor(WithIntensity(
				mDefinition.impactRingColor,
				state.intensity * (1.f - progress)
			));
			target.draw(mImpactRing, additiveStates);
		}

		if (frame.overhead)
		{
			const SunBeamOverheadVisualState& state = *frame.overhead;
			const float progress = Saturate(state.progress);
			const bool isImpact = state.stage == SunBeamOverheadVisualStage::Impact;
			const float arrivalScale = mDefinition.overheadStartScale
				+ (mDefinition.overheadEndScale - mDefinition.overheadStartScale) * progress;
			const float discScale = isImpact
				? mDefinition.overheadEndScale
					+ (std::max(mDefinition.overheadEndScale, mDefinition.impactFlashScale)
						- mDefinition.overheadEndScale) * (1.f - progress)
				: arrivalScale;
			const float haloScale = isImpact
				? mDefinition.convergenceHaloEndScale + 0.45f * progress
				: mDefinition.convergenceHaloStartScale
					+ (mDefinition.convergenceHaloEndScale - mDefinition.convergenceHaloStartScale) * progress;
			const float visibility = isImpact
				? state.intensity * (1.f - 0.85f * progress)
				: state.intensity * (0.2f + 0.8f * progress);
			const sf::Vector2f sourceLocation = impactLocation + mDefinition.perspectiveSourceOffset;

			mConvergenceHalo.setPosition(impactLocation);
			mConvergenceHalo.setScale({ haloScale, haloScale });
			mConvergenceHalo.setOutlineColor(WithIntensity(
				mDefinition.impactRingColor,
				visibility * (isImpact ? 1.f - progress : 0.35f + 0.65f * progress)
			));
			target.draw(mConvergenceHalo, additiveStates);

			if (isImpact)
			{
				const float columnWidthScale = mDefinition.impactColumnWidthScale
					* (1.f + 0.12f * (1.f - progress));
				ConfigurePerspectiveShaftGeometry(sourceLocation, impactLocation, columnWidthScale);
				const sf::Color outerBottomColor = WithIntensity(
					mDefinition.outerColor,
					visibility * mDefinition.perspectiveShaftIntensity
				);
				const sf::Color coreBottomColor = WithIntensity(
					mDefinition.coreColor,
					visibility * mDefinition.perspectiveShaftIntensity * 0.85f
				);
				sf::Color transparentOuter = outerBottomColor;
				sf::Color transparentCore = coreBottomColor;
				transparentOuter.a = 0;
				transparentCore.a = 0;
				mPerspectiveOuterShaft[0].color = transparentOuter;
				mPerspectiveOuterShaft[1].color = transparentOuter;
				mPerspectiveOuterShaft[2].color = outerBottomColor;
				mPerspectiveOuterShaft[3].color = outerBottomColor;
				mPerspectiveCoreShaft[0].color = transparentCore;
				mPerspectiveCoreShaft[1].color = transparentCore;
				mPerspectiveCoreShaft[2].color = coreBottomColor;
				mPerspectiveCoreShaft[3].color = coreBottomColor;
				target.draw(mPerspectiveOuterShaft, additiveStates);
				target.draw(mPerspectiveCoreShaft, additiveStates);
			}

			if (isImpact)
			{
				const float glareIntensity = state.intensity * (1.f - progress);
				mHorizontalGlare.setPosition(impactLocation);
				mVerticalGlare.setPosition(impactLocation);
				mHorizontalGlare.setFillColor(WithIntensity(mDefinition.coreColor, glareIntensity * 0.65f));
				mVerticalGlare.setFillColor(WithIntensity(mDefinition.coreColor, glareIntensity * 0.65f));
				target.draw(mHorizontalGlare, additiveStates);
				target.draw(mVerticalGlare, additiveStates);
			}

			mOverheadOuter.setPosition(impactLocation);
			mOverheadCore.setPosition(impactLocation);
			mOverheadOuter.setScale({ discScale, discScale });
			mOverheadCore.setScale({ discScale, discScale });
			mOverheadOuter.setFillColor(WithIntensity(mDefinition.outerColor, visibility * pulse));
			mOverheadCore.setFillColor(WithIntensity(mDefinition.coreColor, visibility));
			target.draw(mOverheadOuter, additiveStates);
			target.draw(mOverheadCore, additiveStates);
		}

		if (frame.column)
		{
			const SunBeamColumnVisualState& state = *frame.column;
			ConfigureColumnGeometry(state.topWidthScale);
			mOuterColumn.setPosition(impactLocation);
			mCoreColumn.setPosition(impactLocation);
			mOuterColumn.setFillColor(WithIntensity(mDefinition.outerColor, state.intensity * pulse));
			mCoreColumn.setFillColor(WithIntensity(mDefinition.coreColor, state.intensity));
			target.draw(mOuterColumn, additiveStates);
			target.draw(mCoreColumn, additiveStates);
		}
	}

	void SunBeamVisual::ConfigureColumnGeometry(float topWidthScale)
	{
		const float clampedTopWidthScale = std::clamp(topWidthScale, 0.02f, 1.f);
		const float outerTopWidth = std::max(2.f, mBeamWidth * clampedTopWidthScale);
		const float coreWidth = std::max(2.f, mBeamWidth * 0.3f);
		const float coreTopWidth = std::max(1.f, coreWidth * clampedTopWidthScale);

		mOuterColumn.setPoint(0, { -outerTopWidth * 0.5f, -mVisibleBeamLength });
		mOuterColumn.setPoint(1, { outerTopWidth * 0.5f, -mVisibleBeamLength });
		mOuterColumn.setPoint(2, { mBeamWidth * 0.5f, 0.f });
		mOuterColumn.setPoint(3, { -mBeamWidth * 0.5f, 0.f });

		mCoreColumn.setPoint(0, { -coreTopWidth * 0.5f, -mVisibleBeamLength });
		mCoreColumn.setPoint(1, { coreTopWidth * 0.5f, -mVisibleBeamLength });
		mCoreColumn.setPoint(2, { coreWidth * 0.5f, 0.f });
		mCoreColumn.setPoint(3, { -coreWidth * 0.5f, 0.f });
	}

	void SunBeamVisual::ConfigurePerspectiveShaftGeometry(
		const sf::Vector2f& sourceLocation,
		const sf::Vector2f& headLocation,
		float columnWidthScale
	)
	{
		const sf::Vector2f shaftVector = headLocation - sourceLocation;
		const float shaftLength = std::sqrt(
			shaftVector.x * shaftVector.x + shaftVector.y * shaftVector.y
		);
		if (shaftLength <= 0.001f)
		{
			return;
		}

		const sf::Vector2f perpendicular{
			-shaftVector.y / shaftLength,
			shaftVector.x / shaftLength
		};
		const sf::Vector2f localHead = shaftVector;
		const float outerHeadHalfWidth = std::max(
			2.f,
			mBeamWidth * std::max(0.05f, columnWidthScale) * 0.5f
		);
		const float outerTopHalfWidth = outerHeadHalfWidth;
		const float coreScale = std::clamp(mDefinition.perspectiveCoreWidthScale, 0.05f, 1.f);
		const float coreTopHalfWidth = std::max(0.75f, outerTopHalfWidth * coreScale);
		const float coreHeadHalfWidth = std::max(1.f, outerHeadHalfWidth * coreScale);

		mPerspectiveOuterShaft[0].position = sourceLocation - perpendicular * outerTopHalfWidth;
		mPerspectiveOuterShaft[1].position = sourceLocation + perpendicular * outerTopHalfWidth;
		mPerspectiveOuterShaft[2].position = sourceLocation + localHead - perpendicular * outerHeadHalfWidth;
		mPerspectiveOuterShaft[3].position = sourceLocation + localHead + perpendicular * outerHeadHalfWidth;

		mPerspectiveCoreShaft[0].position = sourceLocation - perpendicular * coreTopHalfWidth;
		mPerspectiveCoreShaft[1].position = sourceLocation + perpendicular * coreTopHalfWidth;
		mPerspectiveCoreShaft[2].position = sourceLocation + localHead - perpendicular * coreHeadHalfWidth;
		mPerspectiveCoreShaft[3].position = sourceLocation + localHead + perpendicular * coreHeadHalfWidth;
	}
}

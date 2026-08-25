#include "gameplay/ability/glacialPressure/GlacialPressureTelegraphActor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		constexpr float Pi = 3.14159265358979323846f;

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * std::clamp(multiplier, 0.f, 1.f),
				0.f,
				255.f
			));
			return result;
		}
	}

	GlacialPressureTelegraphActor::GlacialPressureTelegraphActor(
		World* world,
		Actor* owner,
		const GlacialPressurePresentationProfile& profile
	)
		: Actor{ world },
		mOwner{ owner },
		mProfile{ profile }
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
		if (mOwner)
		{
			SetActorLocation(mOwner->GetActorLocation());
		}
		BuildGeometry();
	}

	void GlacialPressureTelegraphActor::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mAge += safeDeltaTime;
		SetActorLocation(mOwner->GetActorLocation());
		if (mCompletionFeedback)
		{
			mCompletionAge += safeDeltaTime;
			if (mCompletionAge >= mProfile.completionFeedbackDuration)
			{
				Destroy();
				return;
			}
		}
		BuildGeometry();
		Actor::Tick(deltaTime);
	}

	void GlacialPressureTelegraphActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mBands, additive);
		window.draw(mSeparators, additive);
	}

	void GlacialPressureTelegraphActor::SetExternalProgress(float normalizedProgress)
	{
		if (GetIsPendingDestroy() || mCompletionFeedback)
		{
			return;
		}
		mProgress = std::clamp(normalizedProgress, 0.f, 1.f);
		BuildGeometry();
	}

	void GlacialPressureTelegraphActor::Complete(float normalizedProgress)
	{
		if (GetIsPendingDestroy() || mCompletionFeedback)
		{
			return;
		}
		mProgress = std::clamp(normalizedProgress, 0.f, 1.f);
		mCompletionProgress = mProgress;
		mCompletionAge = 0.f;
		mCompletionFeedback = mProfile.completionFeedbackDuration > 0.f;
		if (!mCompletionFeedback)
		{
			Destroy();
			return;
		}
		BuildGeometry();
	}

	bool GlacialPressureTelegraphActor::IsInCompletionFeedback() const
	{
		return mCompletionFeedback && !GetIsPendingDestroy();
	}

	void GlacialPressureTelegraphActor::BuildGeometry()
	{
		if (!mOwner)
		{
			return;
		}

		mBands.clear();
		mSeparators.clear();
		const int segmentCount = std::clamp(mProfile.segmentCount, 1, 5);
		const float range = std::max(1.f, mProfile.range);
		const float segmentLength = range / static_cast<float>(segmentCount);
		const sf::Vector2f origin = mOwner->GetActorLocation();
		const sf::Vector2f forward = mOwner->GetActorForwardDirection();
		const float forwardAngle = std::atan2(forward.y, forward.x);
		const float halfAngle = mProfile.halfAngleDegrees * Pi / 180.f;
		const float pulse = 0.82f + 0.18f *
			(0.5f + 0.5f * std::sin(mAge * mProfile.pulseSpeed));
		const float completionPulse = mCompletionFeedback
			? 1.f + 0.20f * std::max(
				0.f,
				1.f - mCompletionAge /
					std::max(0.001f, mProfile.completionFeedbackDuration)
			)
			: 1.f;
		const float effectiveProgress = mCompletionFeedback
			? mCompletionProgress
			: mProgress;

		constexpr int ArcSegmentsPerBand = 16;
		const auto PointOnCone = [origin](float angle, float radius)
		{
			return origin + sf::Vector2f{
				std::cos(angle) * radius,
				std::sin(angle) * radius
			};
		};
		for (int segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
		{
			const float segmentProgress = std::clamp(
				effectiveProgress * static_cast<float>(segmentCount) -
					static_cast<float>(segmentIndex),
				0.f,
				1.f
			);
			if (segmentProgress <= 0.f)
			{
				continue;
			}

			const float innerRadius = segmentLength * static_cast<float>(segmentIndex);
			const float outerRadius = innerRadius + segmentLength * segmentProgress;
			sf::Color color = mCompletionFeedback
				? mProfile.completionColor
				: mProfile.segmentColors[static_cast<std::size_t>(segmentIndex)];
			color = WithAlpha(color, mCompletionFeedback ? completionPulse : pulse);

			for (int arcIndex = 0; arcIndex < ArcSegmentsPerBand; ++arcIndex)
			{
				const float startRatio = static_cast<float>(arcIndex) /
					static_cast<float>(ArcSegmentsPerBand);
				const float endRatio = static_cast<float>(arcIndex + 1) /
					static_cast<float>(ArcSegmentsPerBand);
				// Each band spans the whole cone. Segment index changes radius only,
				// matching the gameplay falloff from the ship outward; it must not
				// split the cone into five angular wedges.
				const float startAngle = forwardAngle - halfAngle +
					startRatio * halfAngle * 2.f;
				const float endAngle = forwardAngle - halfAngle +
					endRatio * halfAngle * 2.f;

				const sf::Vector2f outerStart = PointOnCone(startAngle, outerRadius);
				const sf::Vector2f outerEnd = PointOnCone(endAngle, outerRadius);
				const sf::Vector2f innerStart = PointOnCone(startAngle, innerRadius);
				const sf::Vector2f innerEnd = PointOnCone(endAngle, innerRadius);

				mBands.append(sf::Vertex{ outerStart, color });
				mBands.append(sf::Vertex{ outerEnd, color });
				mBands.append(sf::Vertex{ innerEnd, color });
				mBands.append(sf::Vertex{ outerStart, color });
				mBands.append(sf::Vertex{ innerEnd, color });
				mBands.append(sf::Vertex{ innerStart, color });
			}
		}

		const sf::Color separatorColor = WithAlpha(
			mProfile.separatorColor,
			mCompletionFeedback ? completionPulse : pulse
		);
		const float visibleRadius = range * effectiveProgress;
		const auto AppendArc = [&](float radius)
		{
			for (int arcIndex = 0; arcIndex < ArcSegmentsPerBand; ++arcIndex)
			{
				const float startRatio = static_cast<float>(arcIndex) /
					static_cast<float>(ArcSegmentsPerBand);
				const float endRatio = static_cast<float>(arcIndex + 1) /
					static_cast<float>(ArcSegmentsPerBand);
				const float startAngle = forwardAngle - halfAngle +
					startRatio * halfAngle * 2.f;
				const float endAngle = forwardAngle - halfAngle +
					endRatio * halfAngle * 2.f;
				mSeparators.append(sf::Vertex{
					PointOnCone(startAngle, radius), separatorColor
				});
				mSeparators.append(sf::Vertex{
					PointOnCone(endAngle, radius), separatorColor
				});
			}
		};

		if (visibleRadius > 0.f)
		{
			// Only the cone's two outside edges originate at the ship. The inner
			// segment dividers are arcs at fixed distances from it.
			mSeparators.append(sf::Vertex{
				origin, separatorColor
			});
			mSeparators.append(sf::Vertex{
				PointOnCone(forwardAngle - halfAngle, visibleRadius), separatorColor
			});
			mSeparators.append(sf::Vertex{
				origin, separatorColor
			});
			mSeparators.append(sf::Vertex{
				PointOnCone(forwardAngle + halfAngle, visibleRadius), separatorColor
			});

			for (int boundaryIndex = 1;
				boundaryIndex < segmentCount;
				++boundaryIndex)
			{
				const float boundaryRadius = segmentLength *
					static_cast<float>(boundaryIndex);
				if (boundaryRadius <= visibleRadius)
				{
					AppendArc(boundaryRadius);
				}
			}
			AppendArc(visibleRadius);
		}
	}
}

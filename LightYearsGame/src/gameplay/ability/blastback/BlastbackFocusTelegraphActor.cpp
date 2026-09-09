#include "gameplay/ability/blastback/BlastbackFocusTelegraphActor.h"

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

	BlastbackFocusTelegraphActor::BlastbackFocusTelegraphActor(
		World* world,
		Actor* owner,
		const BlastbackPresentationProfile& profile
	)
		: Actor{ world }
		, mOwner{ owner }
		, mProfile{ profile }
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
		BuildGeometry();
	}

	void BlastbackFocusTelegraphActor::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}
		mAge += std::max(0.f, deltaTime);
		SetActorLocation(mOwner->GetActorLocation());
		BuildGeometry();
		Actor::Tick(deltaTime);
	}

	void BlastbackFocusTelegraphActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mGeometry, additive);
	}

	void BlastbackFocusTelegraphActor::SetProgress(float normalizedProgress)
	{
		mProgress = std::clamp(normalizedProgress, 0.f, 1.f);
		BuildGeometry();
	}

	void BlastbackFocusTelegraphActor::BuildGeometry()
	{
		mGeometry.clear();
		if (!mOwner || mProgress <= 0.f)
		{
			return;
		}

		const sf::Vector2f origin = mOwner->GetActorLocation();
		const sf::Vector2f forward = mOwner->GetActorForwardDirection();
		const float forwardAngle = std::atan2(forward.y, forward.x);
		const float halfAngle = mProfile.halfAngleDegrees * Pi / 180.f;
		const float maximumRange = std::max(1.f, mProfile.range);
		const float emitterInnerRadius = std::min(
			std::max(0.f, mProfile.emitterInnerRadius),
			maximumRange
		);
		// The inner arc stays fixed at the owner; only the outer arc grows during
		// focus. This creates the annular-sector shape instead of an inflating cone.
		const float visibleRange = std::max(
			emitterInnerRadius,
			maximumRange * mProgress
		);
		const float visibleInnerRange = std::min(
			visibleRange,
			std::max(0.f, mProfile.innerRange)
		);
		const float pulse = 0.72f + 0.28f * (
			0.5f + 0.5f * std::sin(mAge * mProfile.focusPulseSpeed)
		);
		const auto point = [origin](float angle, float radius)
		{
			return origin + sf::Vector2f{
				std::cos(angle) * radius,
				std::sin(angle) * radius
			};
		};

		constexpr int ArcSegments = 20;
		const auto appendBand = [&](float innerRadius, float outerRadius, sf::Color color)
		{
			if (outerRadius <= innerRadius)
			{
				return;
			}
			for (int index = 0; index < ArcSegments; ++index)
			{
				const float startRatio = static_cast<float>(index) / ArcSegments;
				const float endRatio = static_cast<float>(index + 1) / ArcSegments;
				const float startAngle = forwardAngle - halfAngle + startRatio * halfAngle * 2.f;
				const float endAngle = forwardAngle - halfAngle + endRatio * halfAngle * 2.f;
				const sf::Vector2f outerStart = point(startAngle, outerRadius);
				const sf::Vector2f outerEnd = point(endAngle, outerRadius);
				const sf::Vector2f innerStart = point(startAngle, innerRadius);
				const sf::Vector2f innerEnd = point(endAngle, innerRadius);
				mGeometry.append(sf::Vertex{ outerStart, color });
				mGeometry.append(sf::Vertex{ outerEnd, color });
				mGeometry.append(sf::Vertex{ innerEnd, color });
				mGeometry.append(sf::Vertex{ outerStart, color });
				mGeometry.append(sf::Vertex{ innerEnd, color });
				mGeometry.append(sf::Vertex{ innerStart, color });
			}
		};

		appendBand(
			emitterInnerRadius,
			visibleInnerRange,
			WithAlpha(mProfile.innerColor, pulse)
		);
		appendBand(
			visibleInnerRange,
			visibleRange,
			WithAlpha(mProfile.outerColor, pulse)
		);
	}
}

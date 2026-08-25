#include "gameplay/ability/directionalBarrier/DirectionalBarrierVisualActor.h"
#include "gameplay/portal/PortalTransferParticipant.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
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

		void BuildOuterEdge(
			sf::ConvexShape& edge,
			const sf::Vector2f& origin,
			const sf::Vector2f& forward,
			float radius,
			float halfAngleDegrees,
			float thickness,
			const sf::Color& color
		)
		{
			constexpr std::size_t ArcSegments = 24;
			const float safeRadius = std::max(1.f, radius);
			const float safeThickness = std::clamp(thickness, 1.f, safeRadius - 1.f);
			const float innerRadius = std::max(1.f, safeRadius - safeThickness);
			const float forwardAngle = std::atan2(forward.y, forward.x);
			const float halfAngle = halfAngleDegrees * 0.01745329251994329577f;

			// Only the annular outer edge is filled. The cone interior remains
			// transparent so gameplay remains visible behind the barrier.
			edge.setPointCount((ArcSegments + 1) * 2);
			for (std::size_t index = 0; index <= ArcSegments; ++index)
			{
				const float normalized = static_cast<float>(index) /
					static_cast<float>(ArcSegments);
				const float angle = forwardAngle - halfAngle + normalized * halfAngle * 2.f;
				edge.setPoint(
					index,
					origin + sf::Vector2f{
						std::cos(angle) * safeRadius,
						std::sin(angle) * safeRadius
					}
				);
			}
			for (std::size_t index = 0; index <= ArcSegments; ++index)
			{
				const float normalized = static_cast<float>(ArcSegments - index) /
					static_cast<float>(ArcSegments);
				const float angle = forwardAngle - halfAngle + normalized * halfAngle * 2.f;
				edge.setPoint(
					ArcSegments + 1 + index,
					origin + sf::Vector2f{
						std::cos(angle) * innerRadius,
						std::sin(angle) * innerRadius
					}
				);
			}
			edge.setFillColor(color);
		}

		void BuildSideEdges(
			sf::VertexArray& sideEdges,
			const sf::Vector2f& origin,
			const sf::Vector2f& forward,
			float radius,
			float halfAngleDegrees,
			const sf::Color& color
		)
		{
			const float forwardAngle = std::atan2(forward.y, forward.x);
			const float halfAngle = halfAngleDegrees * 0.01745329251994329577f;
			const float safeRadius = std::max(1.f, radius);
			const sf::Vector2f leftEndpoint = origin + sf::Vector2f{
				std::cos(forwardAngle - halfAngle) * safeRadius,
				std::sin(forwardAngle - halfAngle) * safeRadius
			};
			const sf::Vector2f rightEndpoint = origin + sf::Vector2f{
				std::cos(forwardAngle + halfAngle) * safeRadius,
				std::sin(forwardAngle + halfAngle) * safeRadius
			};
			sideEdges[0] = sf::Vertex{ origin, color };
			sideEdges[1] = sf::Vertex{ leftEndpoint, color };
			sideEdges[2] = sf::Vertex{ origin, color };
			sideEdges[3] = sf::Vertex{ rightEndpoint, color };
		}
	}

	DirectionalBarrierVisualActor::DirectionalBarrierVisualActor(
		World* world,
		Actor* owner,
		const DirectionalBarrierPresentationProfile& profile
	)
		: Actor{ world },
		mOwner{ owner },
		mProfile{ profile }
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
		UpdateVisuals();
	}

	void DirectionalBarrierVisualActor::Tick(float deltaTime)
	{
		if (!mOwner || mOwner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}
		if (const auto* participant = dynamic_cast<const PortalTransferParticipant*>(mOwner);
			participant && participant->IsInPortalTransit())
		{
			SetRenderEnabled(false);
			return;
		}

		SetRenderEnabled(true);
		mAge += std::max(0.f, deltaTime);
		SetActorLocation(mOwner->GetActorLocation());
		UpdateVisuals();
		Actor::Tick(deltaTime);
	}

	void DirectionalBarrierVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || !IsRenderEnabled())
		{
			return;
		}

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mOuterPanel, additive);
		window.draw(mSideEdges, additive);
	}

	void DirectionalBarrierVisualActor::UpdateVisuals()
	{
		if (!mOwner)
		{
			return;
		}

		const float pulse = 0.75f + 0.25f *
			(0.5f + 0.5f * std::sin(mAge * mProfile.pulseSpeed));
		const sf::Vector2f origin = mOwner->GetActorLocation();
		const sf::Vector2f forward = mOwner->GetActorForwardDirection();
		const sf::Color edgeColor = WithAlpha(mProfile.edgeColor, pulse);
		BuildOuterEdge(
			mOuterPanel,
			origin,
			forward,
			mProfile.radius,
			mProfile.halfAngleDegrees,
			mProfile.edgeThickness,
			edgeColor
		);
		BuildSideEdges(
			mSideEdges,
			origin,
			forward,
			mProfile.radius,
			mProfile.halfAngleDegrees,
			edgeColor
		);
	}
}

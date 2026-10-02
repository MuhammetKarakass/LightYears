#include "gameplay/weapon/visuals/ConeBlastVisualActor.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float ConeBlastLifetime = 0.12f;
		constexpr std::size_t ConeArcSegments = 12;
	}

	ConeBlastVisualActor::ConeBlastVisualActor(
		World* world,
		const sf::Vector2f& apex,
		const sf::Vector2f& direction,
		float range,
		float halfAngleDegrees,
		const sf::Color& color
	)
		: Actor(world)
		, mApex(apex)
		, mDirection(direction)
		, mRange(range)
		, mHalfAngleDegrees(halfAngleDegrees)
		, mColor(color)
		, mRemainingLifetime(ConeBlastLifetime)
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		mFan.resize(ConeArcSegments + 2);
		RebuildGeometry();
	}

	void ConeBlastVisualActor::Tick(float deltaTime)
	{
		mRemainingLifetime -= std::max(0.f, deltaTime);
		if (mRemainingLifetime <= 0.f)
		{
			Destroy();
			return;
		}
		RebuildGeometry();
	}

	void ConeBlastVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;
		window.draw(mFan, additiveStates);
	}

	void ConeBlastVisualActor::RebuildGeometry()
	{
		const float fade = std::clamp(mRemainingLifetime / ConeBlastLifetime, 0.f, 1.f);
		sf::Color apexColor = mColor;
		apexColor.a = static_cast<std::uint8_t>(200.f * fade);
		sf::Color edgeColor = mColor;
		edgeColor.a = static_cast<std::uint8_t>(40.f * fade);

		// The flash expands quickly toward full range while it fades.
		const float reach = mRange * (0.55f + 0.45f * (1.f - fade));
		const float baseAngle = std::atan2(mDirection.y, mDirection.x);
		const float halfAngle = mHalfAngleDegrees * 3.14159265f / 180.f;
		mFan[0].position = mApex;
		mFan[0].color = apexColor;
		for (std::size_t index = 0; index <= ConeArcSegments; ++index)
		{
			const float progress = static_cast<float>(index) / static_cast<float>(ConeArcSegments);
			const float angle = baseAngle - halfAngle + 2.f * halfAngle * progress;
			mFan[index + 1].position = mApex + sf::Vector2f{ std::cos(angle), std::sin(angle) } * reach;
			mFan[index + 1].color = edgeColor;
		}
	}
}

#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr size_t BoltPointCount = 9;

		sf::Color WithAlpha(const sf::Color& color, float alpha)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
			return result;
		}
	}

	ElectricArcVisualActor::ElectricArcVisualActor(
		World* world,
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		const sf::Color& color
	)
		: Actor(world)
		, mStart(start)
		, mEnd(end)
		, mColor(color)
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		mOuterBolt.resize(BoltPointCount);
		mCoreBolt.resize(BoltPointCount);
		RebuildGeometry();
	}

	void ElectricArcVisualActor::Tick(float deltaTime)
	{
		mAge += std::max(0.f, deltaTime);
		mRemainingLifetime -= std::max(0.f, deltaTime);
		if (mRemainingLifetime <= 0.f)
		{
			Destroy();
			return;
		}
		RebuildGeometry();
	}

	void ElectricArcVisualActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;
		window.draw(mOuterBolt, additiveStates);
		window.draw(mCoreBolt, additiveStates);
	}

	void ElectricArcVisualActor::RebuildGeometry()
	{
		const sf::Vector2f direction = mEnd - mStart;
		const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
		if (length <= 0.001f)
		{
			return;
		}

		const sf::Vector2f normal{ -direction.y / length, direction.x / length };
		const float fade = std::clamp(mRemainingLifetime / 0.10f, 0.f, 1.f);
		for (size_t index = 0; index < BoltPointCount; ++index)
		{
			const float progress = static_cast<float>(index) / static_cast<float>(BoltPointCount - 1);
			const float edgeFade = std::sin(progress * 3.14159265f);
			const float jitter = index == 0 || index + 1 == BoltPointCount
				? 0.f
				: std::sin(mAge * 95.f + static_cast<float>(index) * 2.13f) * std::min(18.f, length * 0.08f);
			const sf::Vector2f position = mStart + direction * progress + normal * jitter;
			mOuterBolt[index].position = position;
			mCoreBolt[index].position = position;
			mOuterBolt[index].color = WithAlpha(mColor, fade * (0.45f + edgeFade * 0.55f));
			mCoreBolt[index].color = WithAlpha(sf::Color{ 235, 250, 255, 255 }, fade * edgeFade);
		}
	}
}

#include "presentation/ability/ionComet/IonCometVisualActor.h"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <algorithm>

namespace vega
{
	IonCometVisualActor::IonCometVisualActor(World* world, weak_ptr<Actor> owner, const IonCometPresentationProfile& profile) : Actor{ world }, mOwner{ std::move(owner) }, mProfile{ profile }
	{
		SetEnablePhysics(false);
		SetRenderLayer(RenderLayer::WorldVfx);
	}
	void IonCometVisualActor::Tick(float deltaSeconds)
	{
		const auto owner = mOwner.lock();
		if (!owner || owner->GetIsPendingDestroy()) { Destroy(); return; }
		SetActorLocation(owner->GetActorLocation());
		mBurstRemaining = std::max(0.f, mBurstRemaining - std::max(0.f, deltaSeconds));
	}
	void IonCometVisualActor::Burst(const sf::Vector2f& origin, float radius)
	{
		mBurstOrigin = origin;
		mBurstRadius = radius;
		mBurstRemaining = mProfile.burstSeconds;
	}
	void IonCometVisualActor::Render(sf::RenderTarget& target)
	{
		const auto owner = mOwner.lock();
		if (!owner || owner->GetIsPendingDestroy()) return;
		if (mFieldRadius > 0.f)
		{
			sf::CircleShape field{ mFieldRadius, 96 };
			field.setOrigin({ mFieldRadius, mFieldRadius });
			field.setPosition(owner->GetActorLocation());
			field.setFillColor(mProfile.fieldFill);
			field.setOutlineColor(mProfile.fieldEdge);
			field.setOutlineThickness(mProfile.fieldThickness);
			target.draw(field);
		}
		if (mBurstRemaining > 0.f)
		{
			const float alpha = mBurstRemaining / mProfile.burstSeconds;
			const float radius = mBurstRadius * (1.f - .15f * alpha);
			sf::CircleShape burst{ radius, 96 };
			burst.setOrigin({ radius, radius });
			burst.setPosition(mBurstOrigin);
			burst.setFillColor(sf::Color::Transparent);
			auto color = mProfile.burstEdge;
			color.a = static_cast<std::uint8_t>(color.a * alpha);
			burst.setOutlineColor(color);
			burst.setOutlineThickness(mProfile.burstThickness);
			target.draw(burst);
		}
	}
}

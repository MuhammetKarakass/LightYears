#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisual.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	GravityAnomalyEffectVisual::GravityAnomalyEffectVisual(
		World* world,
		Actor* owner,
		const GravityAnomalyEffectVisualDefinition& definition
	)
		: GameplayEffectVisual(world, owner)
		, mDefinition(definition)
		, mCore(std::max(1.f, definition.radius * 0.52f), 24)
		, mRing(std::max(1.f, definition.radius), 36)
	{
		const float coreRadius = mCore.getRadius();
		const float ringRadius = mRing.getRadius();
		mCore.setOrigin({ coreRadius, coreRadius });
		mRing.setOrigin({ ringRadius, ringRadius });
		mRing.setFillColor(sf::Color::Transparent);
		mRing.setOutlineThickness(std::max(0.f, definition.ringThickness));
	}

	void GravityAnomalyEffectVisual::SynchronizeState(const GameplayEffectVisualStateView& state)
	{
		(void)state;
	}

	void GravityAnomalyEffectVisual::TickVisual(float deltaTime)
	{
		const shared_ptr<Actor> owner = GetVisualOwner();
		if (!owner)
		{
			return;
		}
		mAge += std::max(0.f, deltaTime);
		SetActorLocation(owner->GetActorLocation());
		const float pulse = 0.82f + 0.18f * std::sin(mAge * 8.f);
		mCore.setPosition(GetActorLocation());
		mCore.setFillColor(mDefinition.coreColor);
		mCore.setScale({ pulse, pulse });
		mRing.setPosition(GetActorLocation());
		mRing.setOutlineColor(mDefinition.ringColor);
		mRing.setRotation(sf::degrees(mAge * mDefinition.rotationSpeed));
	}

	void GravityAnomalyEffectVisual::Render(sf::RenderWindow& window)
	{
		Actor::Render(window);
		if (GetIsPendingDestroy())
		{
			return;
		}
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mCore, additive);
		window.draw(mRing, additive);
	}
}

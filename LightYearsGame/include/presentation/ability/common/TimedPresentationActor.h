#pragma once

#include "framework/Actor.h"

namespace vega
{
	// Shared lifecycle for one-shot presentation actors that live for a fixed
	// duration without collision or physics. Effect-specific drawing stays in
	// UpdateVisuals; this base only owns the age/duration bookkeeping.
	class TimedPresentationActor : public Actor
	{
	public:
		TimedPresentationActor(World* world, const sf::Vector2f& worldLocation, float duration, RenderLayer renderLayer = RenderLayer::WorldVfx);

		void Tick(float deltaTime) final override;

	protected:
		virtual void UpdateVisuals() = 0;

		float GetAge() const { return mAge; }
		float GetDuration() const { return mDuration; }

	private:
		float mDuration = 0.f;
		float mAge = 0.f;
	};
}

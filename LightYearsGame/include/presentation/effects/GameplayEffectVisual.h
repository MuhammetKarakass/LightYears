#pragma once

#include "framework/Actor.h"
#include "attributes/GameplayAttribute.h"

namespace ly
{
	struct GameplayEffectVisualStateView
	{
		float remainingDuration = 0.f;
		float totalDuration = 0.f;
		int stackCount = 1;
		const sas::GameplayAttributeList& runtimeAttributes;
	};

	class GameplayEffectVisual : public Actor
	{
	public:
		GameplayEffectVisual(World* world, Actor* owner, const std::string& texturePath = "");

		void Tick(float deltaTime) final override;
		virtual void SynchronizeState(const GameplayEffectVisualStateView& state) = 0;

	protected:
		shared_ptr<Actor> GetVisualOwner() const { return mOwner.lock(); }
		virtual void TickVisual(float deltaTime) = 0;

	private:
		weak_ptr<Actor> mOwner;
	};
}

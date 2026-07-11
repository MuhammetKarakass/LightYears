#pragma once

#include "framework/Actor.h"
#include "gameplay/attributes/GameplayAttribute.h"

namespace ly
{
	struct GameplayEffectVisualStateView
	{
		float remainingDuration = 0.f;
		float totalDuration = 0.f;
		int stackCount = 1;
		const GameplayAttributeList& runtimeAttributes;
	};

	class GameplayEffectVisual : public Actor
	{
	public:
		GameplayEffectVisual(World* world, Actor* owner, const std::string& texturePath = "");

		void Tick(float deltaTime) final override;
		virtual void SynchronizeState(const GameplayEffectVisualStateView& state) = 0;

	protected:
		Actor* GetVisualOwner() const { return mOwner; }
		virtual void TickVisual(float deltaTime) = 0;

	private:
		Actor* mOwner = nullptr;
	};
}

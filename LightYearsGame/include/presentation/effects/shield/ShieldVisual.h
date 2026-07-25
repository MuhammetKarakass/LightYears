#pragma once

#include "presentation/effects/GameplayEffectVisual.h"
#include "presentation/effects/shield/ShieldVisualDefinition.h"

namespace ly
{
	class ShieldVisual final : public GameplayEffectVisual
	{
	public:
		ShieldVisual(World* world, Actor* owner, const ShieldVisualDefinition& definition);

		void BeginPlay() override;
		void SynchronizeState(const GameplayEffectVisualStateView& state) override;

	protected:
		void TickVisual(float deltaTime) override;

	private:
		ShieldVisualDefinition mDefinition;
		float mPulseTime = 0.f;
		float mIntegrityRatio = 1.f;
		float mRemainingDurationRatio = 1.f;
	};

}

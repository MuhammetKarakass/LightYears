#include "presentation/effects/shield/ShieldVisual.h"

#include "framework/World.h"
#include "gameConfigs/EffectStructs.h"
#include "gameConfigs/VisualConfig.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	ShieldVisual::ShieldVisual(
		World* world,
		Actor* owner,
		const ShieldVisualDefinition& definition
	)
		: GameplayEffectVisual(world, owner, definition.texturePath),
		mDefinition(definition)
	{
	}

	void ShieldVisual::BeginPlay()
	{
		Actor::BeginPlay();
		if (GetSprite())
		{
			GetSprite()->setColor(mDefinition.color);
			GetSprite()->setScale({ mDefinition.baseScale, mDefinition.baseScale });
		}
	}

	void ShieldVisual::SynchronizeState(const GameplayEffectVisualStateView& state)
	{
		mRemainingDurationRatio = state.totalDuration > 0.f
			? std::clamp(state.remainingDuration / state.totalDuration, 0.f, 1.f)
			: 1.f;

		const GameplayAttribute* capacity = FindGameplayAttribute(
			state.runtimeAttributes,
			BarrierEffectSchema::Capacity
		);
		mIntegrityRatio = capacity && capacity->baseValue > 0.f
			? std::clamp(capacity->currentValue / capacity->baseValue, 0.f, 1.f)
			: 1.f;
	}

	void ShieldVisual::TickVisual(float deltaTime)
	{
		Actor* owner = GetVisualOwner();
		if (!owner)
		{
			return;
		}

		SetActorLocation(owner->GetActorLocation() + mDefinition.localOffset);
		AddActorRotationOffset(mDefinition.rotationSpeed * deltaTime);
		mPulseTime += deltaTime;

		if (!GetSprite())
		{
			return;
		}

		const float integrityLoss = 1.f - mIntegrityRatio;
		const float pulseSpeed = mDefinition.pulseSpeed * (
			1.f + integrityLoss * (std::max(1.f, mDefinition.lowIntegrityPulseMultiplier) - 1.f)
		);
		const float pulse = (std::sin(mPulseTime * pulseSpeed) + 1.f) * 0.5f;
		const float scale = mDefinition.baseScale + pulse * mDefinition.pulseScaleAmount;
		GetSprite()->setScale({ scale, scale });

		const float baseAlpha = mDefinition.minimumAlpha
			+ pulse * (mDefinition.maximumAlpha - mDefinition.minimumAlpha);
		const float integrityAlpha = mDefinition.lowIntegrityAlphaMultiplier
			+ (1.f - mDefinition.lowIntegrityAlphaMultiplier) * mIntegrityRatio;
		const float durationFade = std::clamp(mRemainingDurationRatio / 0.15f, 0.f, 1.f);
		sf::Color color = mDefinition.color;
		color.a = static_cast<std::uint8_t>(std::clamp(
			baseAlpha * integrityAlpha * durationFade,
			0.f,
			255.f
		));
		GetSprite()->setColor(color);
	}

	bool RegisterShieldVisuals()
	{
		static const bool registered = GameplayEffectVisualRegistry::RegisterFactory(
			VisualData::Shield_Basic.visualId,
			[](Actor& owner) -> weak_ptr<GameplayEffectVisual>
			{
				World* world = owner.GetWorld();
				return world
					? world->SpawnActor<ShieldVisual>(&owner, VisualData::Shield_Basic)
					: weak_ptr<GameplayEffectVisual>{};
			}
		);
		return registered;
	}
}

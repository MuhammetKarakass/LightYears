#include "attributes/AttributeSystem.h"
#include "presentation/effects/shield/ShieldVisual.h"
#include <framework/SpriteComponent.h>

#include "gameConfigs/combat/EffectStructs.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace vega
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
		if (SpriteComponent* spriteComponent = FindComponent<SpriteComponent>(); spriteComponent && spriteComponent->HasSprite())
		{
			spriteComponent->GetSprite()->setColor(mDefinition.color);
			spriteComponent->GetSprite()->setScale({ mDefinition.baseScale, mDefinition.baseScale });
		}
	}

	void ShieldVisual::SynchronizeState(const GameplayEffectVisualStateView& state)
	{
		mRemainingDurationRatio = state.totalDuration > 0.f
			? std::clamp(state.remainingDuration / state.totalDuration, 0.f, 1.f)
			: 1.f;

		const sas::GameplayAttribute* capacity = sas::FindAttribute(
			state.runtimeAttributes,
			BarrierEffectSchema::Capacity
		);
		mIntegrityRatio = capacity && capacity->baseValue > 0.f
			? std::clamp(capacity->currentValue / capacity->baseValue, 0.f, 1.f)
			: 1.f;
	}

	void ShieldVisual::TickVisual(float deltaTime)
	{
		const shared_ptr<Actor> owner = GetVisualOwner();
		if (!owner)
		{
			return;
		}

		SetActorLocation(owner->GetActorLocation() + mDefinition.localOffset);
		AddActorRotationOffset(mDefinition.rotationSpeed * deltaTime);
		mPulseTime += deltaTime;

		SpriteComponent* spriteComponent = FindComponent<SpriteComponent>();
		if (!spriteComponent || !spriteComponent->HasSprite())
		{
			return;
		}
		std::optional<sf::Sprite>& sprite = spriteComponent->GetSprite();

		const float integrityLoss = 1.f - mIntegrityRatio;
		const float pulseSpeed = mDefinition.pulseSpeed * (
			1.f + integrityLoss * (std::max(1.f, mDefinition.lowIntegrityPulseMultiplier) - 1.f)
		);
		const float pulse = (std::sin(mPulseTime * pulseSpeed) + 1.f) * 0.5f;
		const float scale = mDefinition.baseScale + pulse * mDefinition.pulseScaleAmount;
		sprite->setScale({ scale, scale });

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
		sprite->setColor(color);
	}

}

#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeSystem.h"
namespace ly
{
	enum class GameplayEffectDurationPolicy
	{
		Instant,
		Duration,
		Infinite
	};

	enum class GameplayEffectStackingPolicy
	{
		None,
		RefreshDuration,
		Stack
	};

	struct GameplayEffectDefinition
	{
		std::string effectId;
		GameplayTag behaviorTag;
		GameplayEffectDurationPolicy durationPolicy = GameplayEffectDurationPolicy::Instant;
		GameplayEffectStackingPolicy stackingPolicy = GameplayEffectStackingPolicy::None;
		float duration = 0.f;
		int maxStacks = 1;
		List<GameplayTag> grantedTags;
		List<AttributeModifier> modifiers;
		GameplayAttributeList attributes;
		std::string activeVisualId;
		List<GameplayTag> applicationRequiredTags;
		List<GameplayTag> applicationBlockedTags;
		// Most effects intentionally coalesce by effect ID. Area effects can opt
		// into source-scoped applications so separate world sources own and clean
		// up their own instance without disturbing one another.
		bool sourceScopedApplication = false;
	};
}

struct BarrierEffectSchema
{
	inline static const ly::GameplayTag BehaviorId{ "EffectBehavior.Barrier" };
	inline static const ly::GameplayTag Capacity{ "Attribute.Effect.BarrierCapacity" };
	inline static const ly::GameplayTag AbsorptionRatio{ "Attribute.Effect.BarrierAbsorptionRatio" };
	inline static const ly::GameplayTag RegenerationPerSecond{ "Attribute.Effect.BarrierRegenerationPerSecond" };
	inline static const ly::GameplayTag RegenerationDelay{ "Attribute.Effect.BarrierRegenerationDelay" };
	inline static const ly::GameplayTag RegenerationDelayRemaining{ "Attribute.Effect.BarrierRegenerationDelayRemaining" };
	inline static const ly::GameplayTag BrokenEventId{ "Event.Owner.BarrierBroken" };
};

namespace EffectData
{
	const ly::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId);
}

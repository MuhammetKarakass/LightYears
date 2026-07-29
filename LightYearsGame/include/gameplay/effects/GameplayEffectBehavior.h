#pragma once

#include "gameplay/effects/ActiveGameplayEffect.h"
#include "gameplay/damage/DamageContext.h"

namespace ly
{
	class Actor;

	struct GameplayEffectBehaviorEvent
	{
		GameplayTag eventTag;
		float magnitude = 0.f;
	};

	struct GameplayEffectBehaviorResult
	{
		bool changed = false;
		bool removeEffect = false;
		List<GameplayEffectBehaviorEvent> events;
	};

	enum class IncomingDamagePhase
	{
		PreMitigation,
		Standard
	};

	namespace GameplayEffectBehavior
	{
		using AddStackHandler = void (*)(ActiveGameplayEffect& effect);
		using TickHandler = GameplayEffectBehaviorResult (*)(
			ActiveGameplayEffect& effect,
			Actor& owner,
			float deltaTime
		);
		using IncomingDamageHandler = GameplayEffectBehaviorResult (*)(
			ActiveGameplayEffect& effect,
			DamageContext& context
		);

		struct Hooks
		{
			AddStackHandler addStack = nullptr;
			TickHandler tick = nullptr;
			IncomingDamagePhase incomingDamagePhase = IncomingDamagePhase::Standard;
			IncomingDamageHandler processIncomingDamage = nullptr;
		};

		bool RegisterBehavior(const GameplayTag& behaviorTag, const Hooks& hooks);
		bool IsBehaviorRegistered(const GameplayTag& behaviorTag);
		bool RegisterTickHandler(const GameplayTag& behaviorTag, TickHandler handler);
		void Initialize(ActiveGameplayEffect& effect);
		void Refresh(ActiveGameplayEffect& effect);
		void AddStack(ActiveGameplayEffect& effect);
		GameplayEffectBehaviorResult Tick(ActiveGameplayEffect& effect, Actor& owner, float deltaTime);
		IncomingDamagePhase GetIncomingDamagePhase(const ActiveGameplayEffect& effect);
		GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context);
	}
}

#pragma once

#include "effects/GameplayEffectBehaviorRegistry.h"
#include "effects/GameplayEffectBehaviorResult.h"

namespace sas
{
	template <
		typename ActiveEffect,
		typename TickContext,
		typename EventContext,
		typename EventPhase
	>
	class GameplayEffectBehaviorRuntime
	{
	public:
		using AddStackHandler = void (*)(ActiveEffect&);
		using TickHandler = GameplayEffectBehaviorResult (*)(
			ActiveEffect&,
			TickContext&,
			float
		);
		using EventHandler = GameplayEffectBehaviorResult (*)(
			ActiveEffect&,
			EventContext&
		);

		struct Hooks
		{
			AddStackHandler addStack = nullptr;
			TickHandler tick = nullptr;
			EventPhase eventPhase{};
			EventHandler processEvent = nullptr;
		};

		bool Register(const GameplayEffectBehaviorKey& behaviorKey, const Hooks& hooks)
		{
			if (!behaviorKey.IsValid() ||
				(!hooks.addStack && !hooks.tick && !hooks.processEvent))
			{
				return false;
			}
			return mRegistry.Register(behaviorKey, hooks);
		}

		bool IsRegistered(const GameplayEffectBehaviorKey& behaviorKey) const
		{
			return mRegistry.IsRegistered(behaviorKey);
		}

		bool AddStack(ActiveEffect& effect) const
		{
			const Hooks* hooks = Find(effect);
			if (hooks && hooks->addStack)
			{
				hooks->addStack(effect);
				return true;
			}
			return false;
		}

		GameplayEffectBehaviorResult Tick(
			ActiveEffect& effect,
			TickContext& context,
			float deltaTime
		) const
		{
			const Hooks* hooks = Find(effect);
			return hooks && hooks->tick
				? hooks->tick(effect, context, deltaTime)
				: GameplayEffectBehaviorResult{};
		}

		EventPhase GetEventPhase(
			const ActiveEffect& effect,
			EventPhase defaultPhase
		) const
		{
			const Hooks* hooks = Find(effect);
			return hooks ? hooks->eventPhase : defaultPhase;
		}

		GameplayEffectBehaviorResult ProcessEvent(
			ActiveEffect& effect,
			EventContext& context
		) const
		{
			const Hooks* hooks = Find(effect);
			return hooks && hooks->processEvent
				? hooks->processEvent(effect, context)
				: GameplayEffectBehaviorResult{};
		}

	private:
		const Hooks* Find(const ActiveEffect& effect) const
		{
			return mRegistry.Find(effect.spec.definition.behaviorKey);
		}

		GameplayEffectBehaviorRegistry<Hooks> mRegistry;
	};

}

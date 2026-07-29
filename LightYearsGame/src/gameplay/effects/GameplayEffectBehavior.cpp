#include "gameplay/effects/GameplayEffectBehavior.h"

namespace ly
{
	namespace
	{
		using BehaviorHookMap = Dictionary<
			GameplayTag,
			GameplayEffectBehavior::Hooks,
			GameplayTagHash
		>;

		BehaviorHookMap& GetBehaviorHooks()
		{
			static BehaviorHookMap handlers;
			return handlers;
		}

		void ResetRuntimeAttributes(ActiveGameplayEffect& effect)
		{
			effect.runtimeAttributes = effect.spec.attributes;
			if (effect.runtimeAttributes.empty())
			{
				return;
			}
			for (GameplayAttribute& attribute : effect.runtimeAttributes)
			{
				attribute.currentValue = attribute.baseValue;
			}
		}

	}

	namespace GameplayEffectBehavior
	{
		bool RegisterBehavior(const GameplayTag& behaviorTag, const Hooks& hooks)
		{
			if (!behaviorTag.IsValid() ||
				(!hooks.addStack && !hooks.tick && !hooks.processIncomingDamage))
			{
				return false;
			}
			return GetBehaviorHooks().emplace(behaviorTag, hooks).second;
		}

		bool IsBehaviorRegistered(const GameplayTag& behaviorTag)
		{
			return GetBehaviorHooks().find(behaviorTag) != GetBehaviorHooks().end();
		}

		bool RegisterTickHandler(const GameplayTag& behaviorTag, TickHandler handler)
		{
			if (!behaviorTag.IsValid() || handler == nullptr)
			{
				return false;
			}
			Hooks hooks;
			hooks.tick = handler;
			return RegisterBehavior(behaviorTag, hooks);
		}

		void Initialize(ActiveGameplayEffect& effect)
		{
			ResetRuntimeAttributes(effect);
		}

		void Refresh(ActiveGameplayEffect& effect)
		{
			ResetRuntimeAttributes(effect);
		}

		void AddStack(ActiveGameplayEffect& effect)
		{
			const auto behavior = GetBehaviorHooks().find(effect.spec.definition.behaviorTag);
			if (behavior != GetBehaviorHooks().end() && behavior->second.addStack)
			{
				behavior->second.addStack(effect);
			}
		}

		GameplayEffectBehaviorResult Tick(
			ActiveGameplayEffect& effect,
			Actor& owner,
			float deltaTime
		)
		{
			const auto behavior = GetBehaviorHooks().find(effect.spec.definition.behaviorTag);
			if (behavior != GetBehaviorHooks().end() && behavior->second.tick)
			{
				return behavior->second.tick(effect, owner, deltaTime);
			}
			return {};
		}

		IncomingDamagePhase GetIncomingDamagePhase(const ActiveGameplayEffect& effect)
		{
			const auto behavior = GetBehaviorHooks().find(effect.spec.definition.behaviorTag);
			return behavior != GetBehaviorHooks().end()
				? behavior->second.incomingDamagePhase
				: IncomingDamagePhase::Standard;
		}

		GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context)
		{
			const auto behavior = GetBehaviorHooks().find(effect.spec.definition.behaviorTag);
			if (behavior != GetBehaviorHooks().end() && behavior->second.processIncomingDamage)
			{
				return behavior->second.processIncomingDamage(effect, context);
			}
			return {};
		}
	}
}

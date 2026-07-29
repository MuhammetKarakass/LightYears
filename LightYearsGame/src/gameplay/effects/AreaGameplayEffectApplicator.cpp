#include "gameplay/effects/AreaGameplayEffectApplicator.h"

#include "framework/Actor.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/effects/GameplayEffectSystem.h"

#include <algorithm>

namespace ly
{
	void AreaGameplayEffectApplicator::Update(
		const List<shared_ptr<Actor>>& targetsInside,
		const GameplayEffectSpec& spec,
		Actor& source,
		const void* sourceScope,
		const RuntimeContextFactory& createRuntimeContext,
		const RuntimeContextCallback& onStay,
		const RuntimeContextCallback& onExit
	)
	{
		for (size_t index = 0; index < mTrackedEffects.size();)
		{
			TrackedEffect& tracked = mTrackedEffects[index];
			const shared_ptr<Actor> target = tracked.target.lock();
			const bool stillInside = target && std::any_of(
				targetsInside.begin(),
				targetsInside.end(),
				[&target](const shared_ptr<Actor>& candidate)
				{
					return candidate.get() == target.get();
				}
			);
			auto* combatant = target
				? dynamic_cast<Combatant*>(target.get())
				: nullptr;
			GameplayEffectSystem* effects = combatant
				? &combatant->GetCombatRuntime().GetEffects()
				: nullptr;
			const bool effectStillActive =
				effects && effects->FindEffect(tracked.handle) != nullptr;

			if (stillInside && effectStillActive)
			{
				if (tracked.runtimeContext && onStay)
				{
					onStay(*tracked.runtimeContext);
				}
				effects->RefreshEffectDuration(tracked.handle);
				++index;
				continue;
			}

			if (tracked.runtimeContext && onExit)
			{
				onExit(*tracked.runtimeContext);
			}
			mTrackedEffects.erase(
				mTrackedEffects.begin() + static_cast<std::ptrdiff_t>(index)
			);
		}

		for (const shared_ptr<Actor>& target : targetsInside)
		{
			if (!target)
			{
				continue;
			}
			const bool alreadyTracked = std::any_of(
				mTrackedEffects.begin(),
				mTrackedEffects.end(),
				[&target](const TrackedEffect& tracked)
				{
					const shared_ptr<Actor> existing = tracked.target.lock();
					return existing && existing.get() == target.get();
				}
			);
			if (alreadyTracked)
			{
				continue;
			}

			auto* combatant = dynamic_cast<Combatant*>(target.get());
			if (!combatant)
			{
				continue;
			}
			std::shared_ptr<GameplayEffectRuntimeContext> runtimeContext =
				createRuntimeContext ? createRuntimeContext(*target) : nullptr;
			const GameplayEffectHandle handle =
				combatant->GetCombatRuntime().GetEffects().ApplyEffect(
					spec,
					GameplayEffectApplicationContext{
						&source,
						sourceScope,
						runtimeContext
					}
				);
			if (handle.IsValid())
			{
				mTrackedEffects.push_back(TrackedEffect{
					target,
					handle,
					std::move(runtimeContext)
				});
			}
		}
	}

	void AreaGameplayEffectApplicator::Clear(
		const RuntimeContextCallback& onExit
	)
	{
		for (TrackedEffect& tracked : mTrackedEffects)
		{
			if (tracked.runtimeContext && onExit)
			{
				onExit(*tracked.runtimeContext);
			}
		}
		mTrackedEffects.clear();
	}
}

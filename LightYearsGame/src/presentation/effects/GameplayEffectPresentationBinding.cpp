#include "presentation/effects/GameplayEffectPresentationBinding.h"

#include "framework/Actor.h"
#include "presentation/effects/GameplayEffectVisual.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"

namespace ly
{
	GameplayEffectPresentationBinding::GameplayEffectPresentationBinding(
		Actor& owner
	)
		: mOwner{ &owner }
	{
	}

	void GameplayEffectPresentationBinding::Activate(
		sas::ActiveGameplayEffect& effect
	)
	{
		if (!mOwner || effect.spec.definition.activeVisualId.empty())
		{
			return;
		}
		mVisuals[effect.handle.id] = GameplayEffectVisualRegistry::Spawn(
			effect.spec.definition.activeVisualId,
			*mOwner
		);
		Synchronize(effect);
	}

	void GameplayEffectPresentationBinding::Synchronize(
		sas::ActiveGameplayEffect& effect
	)
	{
		const auto found = mVisuals.find(effect.handle.id);
		if (found == mVisuals.end())
		{
			return;
		}
		if (auto visual = found->second.lock())
		{
			visual->SynchronizeState(GameplayEffectVisualStateView{
				effect.remainingDuration,
				effect.totalDuration,
				effect.stackCount,
				effect.runtimeAttributes
			});
		}
	}

	void GameplayEffectPresentationBinding::Remove(
		sas::ActiveGameplayEffect& effect
	)
	{
		const auto found = mVisuals.find(effect.handle.id);
		if (found == mVisuals.end())
		{
			return;
		}
		if (auto visual = found->second.lock())
		{
			visual->Destroy();
		}
		mVisuals.erase(found);
	}

	void GameplayEffectPresentationBinding::Clear()
	{
		for (auto& [handle, visualWeak] : mVisuals)
		{
			if (auto visual = visualWeak.lock())
			{
				visual->Destroy();
			}
		}
		mVisuals.clear();
	}
}

#include "presentation/effects/GameplayEffectPresentationBinding.h"

#include "framework/Actor.h"
#include "presentation/effects/GameplayEffectVisual.h"
#include "presentation/effects/GameplayEffectVisualRegistry.h"

#include <exception>
#include <vector>

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
		const unsigned int handle = effect.handle.id;
		const auto found = mVisuals.find(handle);
		if (found == mVisuals.end())
		{
			return;
		}
		const weak_ptr<GameplayEffectVisual> ownedVisual = found->second;
		if (auto visual = ownedVisual.lock())
		{
			visual->Destroy();
		}
		const auto current = mVisuals.find(handle);
		if (current != mVisuals.end() &&
			!current->second.owner_before(ownedVisual) &&
			!ownedVisual.owner_before(current->second))
		{
			mVisuals.erase(current);
		}
	}

	void GameplayEffectPresentationBinding::Clear()
	{
		std::vector<unsigned int> handles;
		handles.reserve(mVisuals.size());
		for (const auto& [handle, visualWeak] : mVisuals)
		{
			(void)visualWeak;
			handles.push_back(handle);
		}

		std::exception_ptr error;
		for (const unsigned int handle : handles)
		{
			auto found = mVisuals.find(handle);
			if (found == mVisuals.end()) continue;
			const weak_ptr<GameplayEffectVisual> ownedVisual = found->second;
			try
			{
				if (auto visual = ownedVisual.lock()) visual->Destroy();
			}
			catch (...) { if (!error) error = std::current_exception(); continue; }

			found = mVisuals.find(handle);
			if (found != mVisuals.end() &&
				!found->second.owner_before(ownedVisual) &&
				!ownedVisual.owner_before(found->second))
			{
				mVisuals.erase(found);
			}
		}
		if (error) std::rethrow_exception(error);
	}
}

#pragma once

#include "effects/ActiveGameplayEffect.h"

#include <memory>
#include <unordered_map>

namespace ly
{
	class Actor;
	class GameplayEffectVisual;

	class GameplayEffectPresentationBinding
	{
	public:
		explicit GameplayEffectPresentationBinding(Actor& owner);

		void Activate(sas::ActiveGameplayEffect& effect);
		void Synchronize(sas::ActiveGameplayEffect& effect);
		void Remove(sas::ActiveGameplayEffect& effect);
		void Clear();

	private:
		Actor* mOwner = nullptr;
		std::unordered_map<unsigned int, weak_ptr<GameplayEffectVisual>> mVisuals;
	};
}

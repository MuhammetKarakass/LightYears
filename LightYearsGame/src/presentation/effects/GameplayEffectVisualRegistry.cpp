#include "presentation/effects/GameplayEffectVisualRegistry.h"

#include "presentation/effects/GameplayEffectVisual.h"

#include <utility>

namespace ly
{
	namespace
	{
		Dictionary<std::string, GameplayEffectVisualFactory>& GetFactories()
		{
			static Dictionary<std::string, GameplayEffectVisualFactory> factories;
			return factories;
		}
	}

	bool GameplayEffectVisualRegistry::RegisterFactory(
		const std::string& visualId,
		GameplayEffectVisualFactory factory
	)
	{
		if (visualId.empty() || !factory)
		{
			return false;
		}
		return GetFactories().emplace(visualId, std::move(factory)).second;
	}

	weak_ptr<GameplayEffectVisual> GameplayEffectVisualRegistry::Spawn(
		const std::string& visualId,
		Actor& owner
	)
	{
		const auto found = GetFactories().find(visualId);
		return found != GetFactories().end()
			? found->second(owner)
			: weak_ptr<GameplayEffectVisual>{};
	}
}

#include "presentation/effects/GameplayEffectVisualRegistry.h"

#include "gameplay/content/ContentIdSchema.h"
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
		if (!content::ContentIdSchema::ValidateGameplayEffectVisualId(visualId) || !factory)
		{
			return false;
		}
		return GetFactories().emplace(visualId, std::move(factory)).second;
	}

	bool GameplayEffectVisualRegistry::IsRegistered(const std::string& visualId)
	{
		return GetFactories().find(visualId) != GetFactories().end();
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

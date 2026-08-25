#include "gameplay/content/AbilityDefinitionMaterializer.h"

#include <stdexcept>

namespace ly::content
{
	namespace
	{
		const GameAbilityDefinition* FindFallback(
			const List<const GameAbilityDefinition*>& fallbackDefinitions,
			const std::string& abilityId
		)
		{
			for (const GameAbilityDefinition* definition : fallbackDefinitions)
			{
				if (definition && definition->abilityId == abilityId)
				{
					return definition;
				}
			}
			return nullptr;
		}
	}

	AbilityLoader::LoadedDefinition MaterializeAbilityDefinition(
		const std::string& abilityId,
		const std::string& fallbackAbilityId,
		const List<const GameAbilityDefinition*>& fallbackDefinitions
	)
	{
		const GameAbilityDefinition* fallback = FindFallback(
			fallbackDefinitions,
			fallbackAbilityId
		);
		if (!fallback)
		{
			throw std::runtime_error(
				"No C++ behavior base exists for ability '" + fallbackAbilityId + "'"
			);
		}

		AbilityLoader::LoadedDefinition loaded;
		loaded.id = abilityId;
		loaded.definition = *fallback;
		loaded.definition.abilityId = abilityId;
		loaded.definition.scalingRules.clear();
		loaded.definition.levelProgression.clear();
		loaded.definition.levelUpgradeScrapCosts.clear();
		loaded.definition.effectSpecs.clear();
		loaded.definition.attributes.clear();
		return loaded;
	}
}

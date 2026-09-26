#pragma once

#include "framework/JsonDocumentLoader.h"
#include "gameplay/content/AbilityLoader.h"

namespace ly::content::ability_loader_detail
{
	using Json = JsonDocumentLoader::Json;

	std::string RequiredString(const Json& object, const char* fieldName);

	AbilityLoader::LoadedDefinition ParseAbility(
		const Json& object,
		const List<const GameAbilityDefinition*>& fallbackDefinitions,
		const List<const AbilityActorDefinition*>& fallbackActorDefinitions,
		const std::string& fallbackAbilityId
	);
}

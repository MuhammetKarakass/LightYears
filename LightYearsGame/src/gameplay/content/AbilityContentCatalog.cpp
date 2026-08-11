#include "gameplay/content/AbilityContentCatalog.h"

#include "gameplay/content/AbilityLoader.h"

#include <map>

namespace ly::content
{
	namespace
	{
		List<GameAbilityDefinition>& GetDefinitionsStorage()
		{
			static List<GameAbilityDefinition> definitions;
			return definitions;
		}

		List<const GameAbilityDefinition*>& GetDefinitionPointers()
		{
			static List<const GameAbilityDefinition*> definitions;
			return definitions;
		}

		std::map<std::string, std::map<std::string, float>>& GetSettingsStorage()
		{
			static std::map<std::string, std::map<std::string, float>> settings;
			return settings;
		}

		List<AbilityActorDefinition>& GetActorDefinitionsStorage()
		{
			static List<AbilityActorDefinition> definitions;
			return definitions;
		}

		List<const AbilityActorDefinition*>& GetActorDefinitionPointers()
		{
			static List<const AbilityActorDefinition*> definitions;
			return definitions;
		}

		bool& GetLoadedState()
		{
			static bool loaded = false;
			return loaded;
		}

		bool Fail(std::string* failureReason, const std::string& message)
		{
			if (failureReason)
			{
				*failureReason = message;
			}
			return false;
		}
	}

	bool AbilityContentCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		const List<const GameAbilityDefinition*>& fallbackDefinitions,
		const List<const AbilityActorDefinition*>& fallbackActorDefinitions,
		std::string* failureReason
	)
	{
		const AbilityLoader::Result loaded = AbilityLoader::LoadFromFile(
			filePath,
			fallbackDefinitions,
			fallbackActorDefinitions
		);
		if (!loaded.Succeeded())
		{
			return Fail(failureReason, loaded.error);
		}

		GetDefinitionsStorage().clear();
		GetDefinitionPointers().clear();
		GetSettingsStorage().clear();
		GetActorDefinitionsStorage().clear();
		GetActorDefinitionPointers().clear();
		GetDefinitionsStorage().reserve(loaded.definitions.size());
		size_t actorDefinitionCount = 0;
		for (const AbilityLoader::LoadedDefinition& definition : loaded.definitions)
		{
			actorDefinitionCount += definition.actorDefinitions.size();
		}
		GetActorDefinitionsStorage().reserve(actorDefinitionCount);
		for (const AbilityLoader::LoadedDefinition& definition : loaded.definitions)
		{
			GetDefinitionsStorage().push_back(definition.definition);
			GetDefinitionPointers().push_back(&GetDefinitionsStorage().back());
			GetSettingsStorage()[definition.id] = definition.numericSettings;
			for (const AbilityActorDefinition& actorDefinition : definition.actorDefinitions)
			{
				GetActorDefinitionsStorage().push_back(actorDefinition);
				GetActorDefinitionPointers().push_back(&GetActorDefinitionsStorage().back());
			}
		}
		GetLoadedState() = true;
		return true;
	}

	const GameAbilityDefinition* AbilityContentCatalog::FindById(
		const std::string& abilityId
	)
	{
		for (const GameAbilityDefinition* definition : GetDefinitionPointers())
		{
			if (definition && definition->abilityId == abilityId)
			{
				return definition;
			}
		}
		return nullptr;
	}

	const AbilityActorDefinition* AbilityContentCatalog::FindActorById(
		const std::string& actorDefinitionId
	)
	{
		for (const AbilityActorDefinition* definition : GetActorDefinitionPointers())
		{
			if (definition && definition->actorDefinitionId.ToString() == actorDefinitionId)
			{
				return definition;
			}
		}
		return nullptr;
	}

	const List<const GameAbilityDefinition*>& AbilityContentCatalog::GetDefinitions()
	{
		return GetDefinitionPointers();
	}

	std::optional<float> AbilityContentCatalog::FindNumericSetting(
		const std::string& abilityId,
		const std::string& settingName
	)
	{
		const auto abilityIt = GetSettingsStorage().find(abilityId);
		if (abilityIt == GetSettingsStorage().end())
		{
			return std::nullopt;
		}
		const auto settingIt = abilityIt->second.find(settingName);
		return settingIt == abilityIt->second.end()
			? std::nullopt
			: std::optional<float>{ settingIt->second };
	}

	bool AbilityContentCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}

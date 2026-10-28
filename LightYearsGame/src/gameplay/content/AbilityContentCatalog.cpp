#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/RetainedContent.h"

#include "gameplay/content/AbilityLoader.h"

#include <map>

namespace vega::content
{
	namespace
	{
		thread_local const GameAbilityDefinition* scopedDefinition = nullptr;
		thread_local const List<AbilityLoader::LoadedDefinition>* validationDefinitions = nullptr;
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

	ScopedAbilityDefinitionView::ScopedAbilityDefinitionView(const GameAbilityDefinition& definition) : mPrevious{ scopedDefinition } { scopedDefinition = definition.configurationContent ? &definition : nullptr; }
	ScopedAbilityDefinitionView::~ScopedAbilityDefinitionView() { scopedDefinition = mPrevious; }

	ScopedAbilityValidationView::ScopedAbilityValidationView(const List<AbilityLoader::LoadedDefinition>& definitions)
		: mPrevious{ validationDefinitions }
	{
		validationDefinitions = &definitions;
	}
	ScopedAbilityValidationView::~ScopedAbilityValidationView() { validationDefinitions = mPrevious; }

	bool AbilityContentCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		const List<const GameAbilityDefinition*>& fallbackDefinitions,
		const List<const AbilityActorDefinition*>& fallbackActorDefinitions,
		std::string* failureReason, bool preserveExisting
	)
	{

		if (!preserveExisting)
		{
			GetLoadedState() = false;
			RetainContent(GetDefinitionsStorage()); GetDefinitionPointers().clear(); GetSettingsStorage().clear(); RetainContent(GetActorDefinitionsStorage()); GetActorDefinitionPointers().clear();
		}

		const AbilityLoader::Result loaded = AbilityLoader::LoadFromFile(
			filePath,
			fallbackDefinitions,
			fallbackActorDefinitions
		);
		if (!loaded.Succeeded())
		{
			return Fail(failureReason, loaded.error);
		}

		RetainContent(GetDefinitionsStorage());
		GetDefinitionPointers().clear();
		GetSettingsStorage().clear();
		RetainContent(GetActorDefinitionsStorage());
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

	const GameAbilityDefinition* AbilityContentCatalog::FindById(const std::string& abilityId)
	{
		if (scopedDefinition && scopedDefinition->abilityId == abilityId) return scopedDefinition;
		if (validationDefinitions) for (const auto& loaded : *validationDefinitions)
			if (loaded.id == abilityId) return &loaded.definition;
		for (const GameAbilityDefinition* definition : GetDefinitionPointers())
		{
			if (definition && definition->abilityId == abilityId)
			{
				return definition;
			}
		}
		return nullptr;
	}

	const AbilityActorDefinition* AbilityContentCatalog::FindActorById(const std::string& actorDefinitionId)
	{
		if (scopedDefinition && scopedDefinition->configurationContent)
			for (const auto& actor : scopedDefinition->configurationContent->actors) if (actor.actorDefinitionId.ToString() == actorDefinitionId) return &actor;
		if (validationDefinitions) for (const auto& loaded : *validationDefinitions)
			for (const auto& actor : loaded.actorDefinitions) if (actor.actorDefinitionId.ToString() == actorDefinitionId) return &actor;
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

	std::optional<float> AbilityContentCatalog::FindNumericSetting(const std::string& abilityId, const std::string& settingName)
	{
		if (scopedDefinition && scopedDefinition->abilityId == abilityId && scopedDefinition->configurationContent)
		{
			const auto& settings = scopedDefinition->configurationContent->numericSettings;
			const auto setting = settings.find(settingName);
			return setting == settings.end() ? std::nullopt : std::optional<float>{ setting->second };
		}
		if (validationDefinitions) for (const auto& loaded : *validationDefinitions) if (loaded.id == abilityId)
		{
			const auto setting = loaded.numericSettings.find(settingName);
			return setting == loaded.numericSettings.end() ? std::nullopt : std::optional<float>{ setting->second };
		}
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

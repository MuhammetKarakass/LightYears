#include "gameplay/content/EffectContentCatalog.h"

#include "gameplay/content/EffectLoader.h"


namespace ly::content
{
	namespace
	{
		List<sas::GameplayEffectDefinition>& GetDefinitionsStorage()
		{
			static List<sas::GameplayEffectDefinition> definitions;
			return definitions;
		}

		List<const sas::GameplayEffectDefinition*>& GetDefinitionPointers()
		{
			static List<const sas::GameplayEffectDefinition*> definitions;
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

	bool EffectContentCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		const List<const sas::GameplayEffectDefinition*>& fallbackDefinitions,
		std::string* failureReason
	)
	{
		const EffectLoader::Result loaded = EffectLoader::LoadFromFile(
			filePath,
			fallbackDefinitions
		);
		if (!loaded.Succeeded())
		{
			return Fail(failureReason, loaded.error);
		}

		GetDefinitionsStorage().clear();
		GetDefinitionPointers().clear();
		// The fallback list is used only by EffectLoader to recover the typed
		// behavior/policy skeleton for each JSON record. It must never add records
		// that are absent from the authoritative JSON catalog.
		GetDefinitionsStorage().reserve(loaded.definitions.size());
		for (const EffectLoader::LoadedDefinition& definition : loaded.definitions)
		{
			GetDefinitionsStorage().push_back(definition.definition);
		}
		GetDefinitionPointers().reserve(GetDefinitionsStorage().size());
		for (const sas::GameplayEffectDefinition& definition : GetDefinitionsStorage())
		{
			GetDefinitionPointers().push_back(&definition);
		}
		GetLoadedState() = true;
		return true;
	}

	const sas::GameplayEffectDefinition* EffectContentCatalog::FindById(
		const std::string& effectId
	)
	{
		for (const sas::GameplayEffectDefinition* definition : GetDefinitionPointers())
		{
			if (definition && definition->effectId == effectId)
			{
				return definition;
			}
		}
		return nullptr;
	}

	const List<const sas::GameplayEffectDefinition*>&
	EffectContentCatalog::GetDefinitions()
	{
		return GetDefinitionPointers();
	}

	bool EffectContentCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}

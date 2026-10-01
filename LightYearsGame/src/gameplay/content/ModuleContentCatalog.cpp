#include "gameplay/content/ModuleContentCatalog.h"

#include "gameplay/content/ModuleLoader.h"

#include <utility>

namespace ly::content
{
	namespace
	{
		List<ModuleDefinition>& MutableDefinitions()
		{
			static List<ModuleDefinition> definitions;
			return definitions;
		}

		ModuleZoneTable& MutableZoneTable()
		{
			static ModuleZoneTable zoneTable;
			return zoneTable;
		}

		bool& GetLoadedState()
		{
			static bool loaded = false;
			return loaded;
		}
	}

	bool ModuleContentCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		std::string* failureReason)
	{
		GetLoadedState() = false;
		MutableDefinitions().clear();
		MutableZoneTable() = ModuleZoneTable{};

		ModuleLoader::Result loaded = ModuleLoader::LoadFromFile(filePath);
		if (!loaded.Succeeded())
		{
			if (failureReason)
			{
				*failureReason = loaded.error;
			}
			return false;
		}

		MutableDefinitions() = std::move(loaded.definitions);
		MutableZoneTable() = std::move(loaded.zoneTable);
		GetLoadedState() = true;
		return true;
	}

	const ModuleDefinition* ModuleContentCatalog::FindById(const sas::ContentId& moduleId)
	{
		for (const ModuleDefinition& definition : MutableDefinitions())
		{
			if (definition.moduleId == moduleId)
			{
				return &definition;
			}
		}
		return nullptr;
	}

	const List<ModuleDefinition>& ModuleContentCatalog::GetDefinitions()
	{
		return MutableDefinitions();
	}

	const ModuleZoneTable& ModuleContentCatalog::GetZoneTable()
	{
		return MutableZoneTable();
	}

	bool ModuleContentCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}

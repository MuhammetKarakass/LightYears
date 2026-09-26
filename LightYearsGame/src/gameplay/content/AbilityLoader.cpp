#include "gameplay/content/AbilityLoader.h"
#include "AbilityDefinitionJsonParser.h"

#include "framework/JsonDocumentLoader.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/content/GameAbilitySettingContracts.h"

#include <functional>
#include <map>
#include <set>
#include <stdexcept>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		Json MergeJsonObjects(Json base, const Json& overrides)
		{
			if (!base.is_object() || !overrides.is_object())
			{
				return overrides;
			}

			for (const auto& [name, value] : overrides.items())
			{
				if (base.contains(name) && base.at(name).is_object() && value.is_object())
				{
					base[name] = MergeJsonObjects(base.at(name), value);
				}
				else
				{
					base[name] = value;
				}
			}
			return base;
		}
	}

	AbilityLoader::Result AbilityLoader::LoadFromFile(
		const std::filesystem::path& filePath,
		const List<const GameAbilityDefinition*>& fallbackDefinitions,
		const List<const AbilityActorDefinition*>& fallbackActorDefinitions
	)
	{
		const JsonDocumentLoader::Result documentResult =
			JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded())
		{
			return Result{ {}, documentResult.error };
		}

		try
		{
			std::string contractFailure;
			if (!RegisterGameAbilitySettingContracts(&contractFailure))
			{
				return Result{ {}, contractFailure };
			}

			const Json& root = *documentResult.document;
			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, "Unsupported ability schema version" };
			}

			Result result;
			std::map<std::string, const Json*> sourceAbilities;
			std::set<std::string> abilityIds;
			std::set<std::string> actorIds;
			for (const Json& ability : root.at("abilities"))
			{
				const std::string abilityId = ability_loader_detail::RequiredString(ability, "id");
				std::string idFailure;
				if (!ContentIdSchema::ValidateAbilityId(abilityId, &idFailure))
				{
					throw std::runtime_error(
						"Invalid ability ID '" + abilityId + "': " + idFailure
					);
				}
				if (ability.contains("baseId"))
				{
					const std::string baseId = ability.at("baseId").get<std::string>();
					if (!ContentIdSchema::ValidateAbilityId(baseId, &idFailure))
					{
						throw std::runtime_error(
							"Invalid ability baseId '" + baseId + "': " + idFailure
						);
					}
				}
				if (!abilityIds.insert(abilityId).second)
				{
					throw std::runtime_error("Duplicate ability ID: " + abilityId);
				}
				sourceAbilities.emplace(abilityId, &ability);
			}

			std::map<std::string, Json> materializedAbilities;
			std::map<std::string, std::string> cppBaseIds;
			std::set<std::string> resolvingAbilities;
			std::function<void(const std::string&)> materializeAbility;
			materializeAbility = [&](const std::string& abilityId)
			{
				if (materializedAbilities.find(abilityId) != materializedAbilities.end())
				{
					return;
				}
				if (!resolvingAbilities.insert(abilityId).second)
				{
					throw std::runtime_error(
						"Cyclic ability baseId reference involving '" + abilityId + "'"
					);
				}

				const auto source = sourceAbilities.find(abilityId);
				if (source == sourceAbilities.end())
				{
					throw std::runtime_error(
						"Ability baseId references missing ability '" + abilityId + "'"
					);
				}

				const Json& object = *source->second;
				const std::string baseId = object.value("baseId", abilityId);
				if (baseId == abilityId)
				{
					materializedAbilities.emplace(abilityId, object);
					cppBaseIds.emplace(abilityId, abilityId);
				}
				else
				{
					materializeAbility(baseId);
					const Json& baseObject = materializedAbilities.at(baseId);
					Json materialized = MergeJsonObjects(baseObject, object);
					// Actor definitions are global runtime records. A value-only
					// variant reuses the base ability's actors; inheriting them into
					// the variant would create duplicate global actor IDs.
					if (!object.contains("actors"))
					{
						materialized.erase("actors");
					}
					materialized["id"] = abilityId;
					materialized.erase("baseId");
					materializedAbilities.emplace(abilityId, std::move(materialized));
					cppBaseIds.emplace(abilityId, cppBaseIds.at(baseId));
				}
				resolvingAbilities.erase(abilityId);
			};

			for (const Json& ability : root.at("abilities"))
			{
				const std::string abilityId = ability_loader_detail::RequiredString(ability, "id");
				materializeAbility(abilityId);
				LoadedDefinition definition = ability_loader_detail::ParseAbility(
					materializedAbilities.at(abilityId),
					fallbackDefinitions,
					fallbackActorDefinitions,
					cppBaseIds.at(abilityId)
				);
				for (const AbilityActorDefinition& actor : definition.actorDefinitions)
				{
					if (!actorIds.insert(actor.actorDefinitionId.ToString()).second)
					{
						throw std::runtime_error(
							"Duplicate ability actor ID: " +
							actor.actorDefinitionId.ToString()
						);
					}
				}
				result.definitions.push_back(std::move(definition));
			}
			if (result.definitions.empty())
			{
				return Result{ {}, "Ability catalog is empty" };
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				"Failed to load abilities from '" + filePath.string() + "': " + exception.what()
			};
		}
	}
}

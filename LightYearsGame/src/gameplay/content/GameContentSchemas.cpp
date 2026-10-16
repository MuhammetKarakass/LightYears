#include <gas/GasContentSchemas.h>
#include "gameplay/content/GameContentSchemas.h"
#include "framework/content/ContentSchema.h"
#include "gameplay/content/AbilityLoader.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/EffectLoader.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ShipLoader.h"
#include "gameplay/content/WeaponLoader.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/module/ShipSchematicValidator.h"
#include "gameConfigs/ship/ShipConfig.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"
#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/EffectConfig.h"
#include <algorithm>

namespace ly::content
{
	namespace
	{
		bool EffectValid(const sas::GameplayEffectDefinition& definition, std::string* error)
		{
			return GameplayEffectDefinitionValidator::Validate(definition, [](const auto& key)
			{
				return GetEffectBehaviorRuntime().IsRegistered(key);
			}, error);
		}

		void Add(ContentTypeSchema& schema, const std::string& path, const char* example, bool curve = false)
		{
			if (std::any_of(schema.fields.begin(), schema.fields.end(), [&](const auto& f) { return f.path == path; })) return;
			ContentField field = sas::MakeContentField(path, example, curve);
			const auto key = path.substr(path.find_last_of('/') + 1);
			const auto& value = field.defaultValue;
			field.category = path.find("/progression") == 0 ? "Progression" : path.find("/settings") == 0 ? "Behavior" :
				path.find("/attributes") == 0 || path.find("/modifiers") == 0 ? "Attributes" : "Definition";
			field.tooltip = "Authored " + path + ". Validated by the game content loader and definition validators.";
			if (path == "/invocationOutputAttributes/*") field.kind = FieldKind::Attribute;
			if (key == "moduleId" || path == "/conflictsWith/*") field.referenceType = "module";
			if (key == "weaponId" || key == "primaryWeaponId") field.referenceType = "weapon";
			if (!field.referenceType.empty()) field.kind = FieldKind::Reference;
			if (key == "family" || key == "systemCategory" || key == "zone" || key == "typeTag" ||
				(key == "*" && (path.find("Tags/") != std::string::npos || path.find("tags/") != std::string::npos))) field.kind = FieldKind::Tag;
			if (value.is_array() && (key == "damageFraction" || key == "bonusSeconds")) field.kind = FieldKind::Curve;
			if (key == "disposition") field.options = { "Neutral", "Beneficial", "Harmful" };
			if (key == "rarity") field.options = { "Common", "Uncommon", "Rare", "Epic", "Legendary" };
			if (!field.options.empty()) field.kind = FieldKind::Enum;
			schema.fields.push_back(std::move(field));
		}
	}

	void RegisterGameContentSchemas()
	{
		auto& registry = ContentSchemaRegistry::Get();
		if (registry.Find("ability")) return;
		registry.isTag = [](const std::string& value) { return GameplayTagSchema::Validate(GameplayTag{ value }, GameplayTagKind::Any); };
		registry.isAttribute = [](const std::string& value) { return AttributeIdSchema::Validate(sas::AttributeId{ value }); };
		registry.tags = { GameplayTagSchema::AbilityPrimary.name, GameplayTagSchema::AbilityOffense.name, GameplayTagSchema::AbilityDefense.name,
			GameplayTagSchema::AbilityMovement.name, GameplayTagSchema::AbilityUtility.name, GameplayTagSchema::BlockAbilityActivation.name,
			GameplayTagSchema::BlockMovementInput.name, GameplayTagSchema::BlockPrimaryWeaponFire.name, GameplayTagSchema::BlockExternalMovement.name };
		for (const auto* system : GameplayTagSchema::GetZonedRequiredModuleSystems()) registry.tags.push_back(system->name);
		for (const auto& pair : std::vector<std::pair<std::string, std::string>>{ {"ability", "abilities"}, {"effect", "effects"}, {"module", "modules"}, {"ship", "ships"}, {"weapon", "weapons"} })
		{
			ContentTypeSchema schema;
			schema.type = pair.first;
			schema.collection = pair.second;
			schema.file = "content/data/" + pair.second + ".json";
#include "GameContentFields.inc"
			if (schema.type == "ability")
			{
				Add(schema, "/baseId", "\"\"");
				schema.validateId = [](const auto& id, auto& error) { return ContentIdSchema::ValidateAbilityId(id, &error); };
				schema.duplicate = [](const auto& source, const auto& id)
				{
					auto result = source;
					result["id"] = id;
					result["baseId"] = source.value("baseId", source.at("id"));
					// Actors are global records; variants share the base's actors.
					result.erase("actors");
					return result;
				};
				schema.validate = [](const ContentDocuments& documents, const std::string& id)
				{
					const auto loaded = AbilityLoader::LoadFromJson(documents.at("ability"), AbilityData::GetBuiltinShippedAbilityDefinitions(), AbilityData::GetBuiltinAbilityActorDefinitions(), id);
					if (!loaded.Succeeded()) return loaded.error;
					const ScopedAbilityValidationView validationView{ loaded.definitions };
					std::string error;
					const auto effects = EffectLoader::LoadFromJson(documents.at("effect"), EffectData::GetBuiltinGameplayEffectDefinitions());
					if (!effects.Succeeded()) return effects.error;
					const AbilityActionValidator::EffectLookup findEffect = [&](const std::string& effectId) -> const sas::GameplayEffectDefinition*
					{
						for (const auto& effect : effects.definitions) if (effect.id == effectId) return &effect.definition;
						return nullptr;
					};
					for (const auto& definition : loaded.definitions)
						if (!GameAbilityDefinitionValidator::Validate(definition.definition, EffectValid, &error, findEffect)) return error.empty() ? std::string{ "Invalid ability definition" } : error;
					return error;
				};
			}
			if (schema.type == "effect")
			{
				for (const auto* key : { "duration", "maxStacks" }) Add(schema, std::string{"/"} + key, "1");
				Add(schema, "/behaviorKey", "\"\"");
				Add(schema, "/modifiers", "[]");
				Add(schema, "/modifiers/*", "{}");
				Add(schema, "/modifiers/*/attributeId", "\"Owner.AttackPower\"");
				Add(schema, "/modifiers/*/operation", "\"Add\"");
				Add(schema, "/modifiers/*/magnitude", "0.0");
				Add(schema, "/attributes", "[]");
				Add(schema, "/attributes/*", "{}");
				Add(schema, "/attributes/*/id", "\"Owner.AttackPower\"");
				Add(schema, "/attributes/*/baseValue", "0.0");
				Add(schema, "/attributes/*/minValue", "0.0");
				Add(schema, "/attributes/*/maxValue", "1000000.0");
				schema.templates["TimedModifier"] = AuthoringJson::parse(R"({"id":"Effect.Authoring.Modifier.Basic","behaviorKey":"","durationPolicy":"Duration","stackingPolicy":"RefreshDuration","sourceParameterized":false,"duration":2.0,"maxStacks":1,"modifiers":[{"attributeId":"Owner.AttackPower","operation":"Add","magnitude":5.0}]})");
				schema.validateId = [](const auto& id, auto& error) { return ContentIdSchema::ValidateEffectId(id, &error); };
				schema.duplicate = [](const auto& source, const auto& id)
				{
					auto result = source; result["id"] = id;
					if (!result.contains("behaviorKey")) result["behaviorKey"] = "";
					return result;
				};
				schema.validate = [](const ContentDocuments& documents, const std::string& id)
				{
					const auto loaded = EffectLoader::LoadFromJson(documents.at("effect"), EffectData::GetBuiltinGameplayEffectDefinitions(), id);
					if (!loaded.Succeeded()) return loaded.error;
					std::string error;
					for (const auto& definition : loaded.definitions)
						if (!EffectValid(definition.definition, &error)) return error.empty() ? std::string{ "Invalid effect definition" } : error;
					return error;
				};
			}
			if (schema.type == "module")
			{
				schema.validateId = [](const auto& id, auto& error) { return ContentIdSchema::ValidateModuleId(id, &error); };
				schema.validate = [](const ContentDocuments& documents, const std::string& id) { return ModuleLoader::LoadFromJson(documents.at("module"), id).error; };
			}
			if (schema.type == "ship")
			{
				schema.validateId = [](const auto& id, auto& error) { return ContentIdSchema::ValidateShipId(id, &error); };
				schema.validate = [](const ContentDocuments& documents, const std::string& id)
				{
					const auto loaded = ShipLoader::LoadFromJson(documents.at("ship"), ShipData::Ship_Player_Fighter, id);
					if (!loaded.Succeeded()) return loaded.error;
					const auto modules = ModuleLoader::LoadFromJson(documents.at("module"));
					if (!modules.Succeeded()) return modules.error;
					const auto weapons = WeaponLoader::LoadFromJson(documents.at("weapon"));
					if (!weapons.Succeeded()) return weapons.error;
					for (const auto& ship : loaded.definitions)
						if (!ship.definition.primaryWeaponId.empty() && std::none_of(weapons.definitions.begin(), weapons.definitions.end(),
							[&](const auto& weapon) { return weapon.weaponId == ship.definition.primaryWeaponId; }))
							return std::string{ "Unknown primary weapon: " } + ship.definition.primaryWeaponId;
					std::string error;
					for (const auto& ship : loaded.definitions) if (ship.definition.schematic && !ShipSchematicValidator::Validate(*ship.definition.schematic, modules.definitions, modules.zoneTable, &error)) return error;
					return error;
				};
			}
			if (schema.type == "weapon")
			{
				schema.validateId = [](const auto& id, auto& error) { return ContentIdSchema::ValidateWeaponId(id, &error); };
				schema.validate = [](const ContentDocuments& documents, const std::string& id)
				{
					const auto loaded = WeaponLoader::LoadFromJson(documents.at("weapon"), id);
					if (!loaded.Succeeded()) return loaded.error;
					for (const auto& weapon : loaded.definitions)
					{
						const auto result = PrimaryWeaponExecutionSystem::ValidateDefinition(weapon);
						if (!result.isValid) return result.reason;
					}
					return std::string{};
				};
			}
			registry.Register(std::move(schema));
		}
	}
}

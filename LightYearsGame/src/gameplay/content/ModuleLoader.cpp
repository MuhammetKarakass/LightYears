#include "gameplay/content/ModuleLoader.h"

#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/content/AttributeJsonParser.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include "framework/JsonDocumentLoader.h"

#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		float ParseFiniteFloat(const Json& value, const std::string& context)
		{
			if (!value.is_number())
			{
				throw std::runtime_error(context + " must be a number");
			}
			const float number = value.get<float>();
			if (!std::isfinite(number))
			{
				throw std::runtime_error(context + " must be finite");
			}
			return number;
		}

		GameplayTag ParseTag(const Json& value, GameplayTagKind kind, const std::string& context)
		{
			if (!value.is_string())
			{
				throw std::runtime_error(context + " must be a string");
			}
			const GameplayTag tag{ value.get<std::string>() };
			std::string failure;
			if (!GameplayTagSchema::Validate(tag, kind, &failure))
			{
				throw std::runtime_error(context + " '" + tag.name + "': " + failure);
			}
			return tag;
		}

		ModuleAttributeTarget ParseTarget(const std::string& value)
		{
			if (value == "Owner") return ModuleAttributeTarget::Owner;
			if (value == "Ship") return ModuleAttributeTarget::Ship;
			if (value == "PrimaryWeapon") return ModuleAttributeTarget::PrimaryWeapon;
			throw std::runtime_error("Unknown module attribute target: " + value);
		}

		sas::AttributeModifier ParseModifier(const Json& object, const std::string& context)
		{
			if (!object.is_object())
			{
				throw std::runtime_error(context + " must be an object");
			}
			sas::AttributeModifier modifier{
				sas::AttributeId{ object.at("attributeId").get<std::string>() },
				AttributeJsonParser::ParseOperation(object.at("operation").get<std::string>(), context),
				ParseFiniteFloat(object.at("magnitude"), context + ".magnitude"),
				object.value("priority", 0)
			};
			std::string failure;
			if (!AttributeIdSchema::Validate(modifier.attributeId, &failure))
			{
				throw std::runtime_error(context + ": " + failure);
			}
			return modifier;
		}

		ModuleShape ParseShape(const Json& value, const std::string& context)
		{
			if (!value.is_array() || value.empty())
			{
				throw std::runtime_error(context + " must be a non-empty array of [x, y] cells");
			}
			ModuleShape shape;
			std::set<std::pair<int, int>> seen;
			for (const Json& cell : value)
			{
				if (!cell.is_array() || cell.size() != 2 ||
					!cell.at(0).is_number_integer() || !cell.at(1).is_number_integer())
				{
					throw std::runtime_error(context + " cells must be [x, y] integer pairs");
				}
				const int x = cell.at(0).get<int>();
				const int y = cell.at(1).get<int>();
				if (x < 0 || y < 0)
				{
					throw std::runtime_error(context + " cell offsets must not be negative");
				}
				if (!seen.insert({ x, y }).second)
				{
					throw std::runtime_error(context + " contains a duplicate cell");
				}
				shape.cells.push_back(sf::Vector2i{ x, y });
			}
			return shape;
		}

		ModuleSizeVariant ParseVariant(const Json& object, const std::string& context)
		{
			ModuleSizeVariant variant;
			variant.shape = ParseShape(object.at("shape"), context + ".shape");
			std::size_t index = 0;
			for (const Json& entry : object.value("attributeModifiers", Json::array()))
			{
				const std::string entryContext = context + ".attributeModifiers[" + std::to_string(index++) + "]";
				ModuleAttributeModifier modifier;
				modifier.target = ParseTarget(entry.at("target").get<std::string>());
				modifier.modifier = ParseModifier(entry, entryContext);
				modifier.affectedByZoneBonus = entry.value("affectedByZoneBonus", true);
				variant.attributeModifiers.push_back(std::move(modifier));
			}
			index = 0;
			for (const Json& entry : object.value("abilityModifiers", Json::array()))
			{
				const std::string entryContext = context + ".abilityModifiers[" + std::to_string(index++) + "]";
				ModuleAbilityModifier modifier;
				modifier.modifier = ParseModifier(entry, entryContext);
				modifier.affectedByZoneBonus = entry.value("affectedByZoneBonus", true);
				variant.abilityModifiers.push_back(std::move(modifier));
			}
			return variant;
		}

		ModuleDefinition ParseModule(const Json& object)
		{
			ModuleDefinition definition;
			const std::string id = object.at("id").get<std::string>();
			definition.moduleId = sas::ContentId{ id };
			ParsedModuleId parsedId;
			std::string failure;
			if (!ContentIdSchema::ParseModuleId(id, parsedId, &failure))
			{
				throw std::runtime_error(failure);
			}
			const std::string context = "Module " + id;
			definition.displayName = object.value("displayName", id);
			definition.family = ParseTag(object.at("family"), GameplayTagKind::AbilityCategory, context + " family");
			if (definition.family.name != parsedId.family)
			{
				throw std::runtime_error(context + " family does not match its ID family segment");
			}
			if (object.contains("systemCategory"))
			{
				definition.systemCategory = ParseTag(
					object.at("systemCategory"),
					GameplayTagKind::ModuleSystem,
					context + " systemCategory"
				);
			}
			const Json& variants = object.at("sizeVariants");
			if (!variants.is_array() || variants.empty())
			{
				throw std::runtime_error(context + " must define at least one size variant");
			}
			std::size_t index = 0;
			for (const Json& variant : variants)
			{
				definition.sizeVariants.push_back(
					ParseVariant(variant, context + ".sizeVariants[" + std::to_string(index++) + "]")
				);
			}
			return definition;
		}

		ModuleZoneTable ParseZoneTable(const Json& root)
		{
			ModuleZoneTable table;
			for (const Json& entry : root.at("zones"))
			{
				ModuleZoneEntry zone;
				zone.zone = ParseTag(entry.at("zone"), GameplayTagKind::ModuleZone, "Zone");
				if (table.FindZone(zone.zone))
				{
					throw std::runtime_error("Duplicate module zone: " + zone.zone.name);
				}
				const Json& family = entry.at("family");
				if (family.is_string() && family.get<std::string>() == "*")
				{
					zone.anyFamily = true;
				}
				else
				{
					zone.family = ParseTag(family, GameplayTagKind::AbilityCategory, "Zone family");
				}
				zone.bonus = ParseFiniteFloat(entry.at("bonus"), "Zone bonus");
				if (zone.bonus < 0.f)
				{
					throw std::runtime_error("Zone bonus must not be negative");
				}
				table.zones.push_back(std::move(zone));
			}
			const GameplayTag* requiredZones[] = {
				&GameplayTags::Module::Zone::Nose, &GameplayTags::Module::Zone::Wings,
				&GameplayTags::Module::Zone::Hull, &GameplayTags::Module::Zone::Core,
				&GameplayTags::Module::Zone::Engine, &GameplayTags::Module::Zone::Sensor
			};
			for (const GameplayTag* required : requiredZones)
			{
				if (!table.FindZone(*required))
				{
					throw std::runtime_error("Module zone table is missing " + required->name);
				}
			}

			const Json& systemZones = root.at("systemZones");
			if (!systemZones.is_object())
			{
				throw std::runtime_error("systemZones must be an object");
			}
			for (auto it = systemZones.begin(); it != systemZones.end(); ++it)
			{
				ModuleSystemZoneEntry entry;
				entry.system = ParseTag(Json(it.key()), GameplayTagKind::ModuleSystem, "System zone key");
				entry.zone = ParseTag(it.value(), GameplayTagKind::ModuleZone, "System zone");
				if (!table.FindZone(entry.zone))
				{
					throw std::runtime_error("System zone is missing from the zone table: " + entry.zone.name);
				}
				table.systemZones.push_back(std::move(entry));
			}
			const GameplayTag* requiredSystems[] = {
				&GameplayTags::Module::System::Shield, &GameplayTags::Module::System::Afterburner,
				&GameplayTags::Module::System::HealthReadout, &GameplayTags::Module::System::DamageReadout
			};
			for (const GameplayTag* required : requiredSystems)
			{
				if (!table.FindSystemZone(*required))
				{
					throw std::runtime_error("systemZones is missing " + required->name);
				}
			}

			table.rebootDurationSeconds = ParseFiniteFloat(root.at("rebootDurationSeconds"), "rebootDurationSeconds");
			table.combatWindowSeconds = ParseFiniteFloat(root.at("combatWindowSeconds"), "combatWindowSeconds");
			if (table.rebootDurationSeconds < 0.f || table.combatWindowSeconds <= 0.f)
			{
				throw std::runtime_error("Module reboot/combat durations are out of range");
			}
			return table;
		}
	}

	ModuleLoader::Result ModuleLoader::LoadFromFile(const std::filesystem::path& filePath)
	{
		const JsonDocumentLoader::Result documentResult = JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded())
		{
			return Result{ {}, {}, documentResult.error };
		}

		try
		{
			const Json& root = *documentResult.document;
			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, {}, "Unsupported module schema version" };
			}

			Result result;
			result.zoneTable = ParseZoneTable(root);
			std::set<std::string> moduleIds;
			for (const Json& module : root.at("modules"))
			{
				ModuleDefinition definition = ParseModule(module);
				if (!moduleIds.insert(definition.moduleId.ToString()).second)
				{
					throw std::runtime_error("Duplicate module ID: " + definition.moduleId.ToString());
				}
				result.definitions.push_back(std::move(definition));
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				{},
				"Failed to load modules from '" + filePath.string() + "': " + exception.what()
			};
		}
	}
}

#include "ModuleLoaderParsing.h"

#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/module/ModuleDescriptionValues.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <set>
#include <utility>

namespace vega::content
{
	namespace module_loader_detail
	{
		void ValidateZoneEffects(const ModuleDefinition& definition, const ModuleZoneTable& zoneTable, const std::string& context)
		{
			for (std::size_t index = 0; index < definition.sizeVariants.size(); ++index)
			{
				std::string failure;
				if (!ModuleLoader::ValidateRulePackageZoneEffects(definition.sizeVariants[index], zoneTable,
					context + ".sizeVariants[" + std::to_string(index) + "]", &failure)) throw std::runtime_error(failure);
			}
		}

		ModuleDefinition ParseModule(const Json& object, const ModuleZoneTable& zoneTable)
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
			if (parsedId.abilityModule)
			{
				for (const char* forbidden : { "rarity", "levelCount", "family", "systemCategory", "sideFeatures", "sharedPools", "sizeLadder" })
					if (object.contains(forbidden)) throw std::runtime_error(context + " ability modules cannot declare " + forbidden);
				definition.rarity = ModuleRarity::None;
				definition.levelCount = 1; // Neutral internal placement level; no progression is exposed.
				definition.abilityModule = ParseAbilityModuleRules(object.at("abilityModule"), id);
			}
			else if (object.contains("abilityModule")) throw std::runtime_error(context + " abilityModule requires an ability module ID");
			if (parsedId.unique)
			{
				for (const char* forbidden : { "rarity", "levelCount", "family", "systemCategory", "sideFeatures", "sharedPools", "sizeLadder", "legendary", "abilityModule" })
					if (object.contains(forbidden)) throw std::runtime_error(context + " Unique cannot declare " + forbidden);
				definition.rarity = ModuleRarity::None;
				definition.levelCount = 1;
				const Json& rule = object.at("unique");
				if (!rule.is_object()) throw std::runtime_error(context + " unique must be an object");
				UniqueModuleRule values;
				const std::string name = id.substr(std::string{ "Module.Unique." }.size());
				std::set<std::string> allowed;
				if (name == "Amplifier") { values.kind = UniqueModuleKind::Amplifier; allowed = { "levelBonus" }; values.levelBonus = ParsePositiveInt(rule.at("levelBonus"), context); }
				else if (name == "Relay") values.kind = UniqueModuleKind::Relay;
				else if (name == "Echo") { values.kind = UniqueModuleKind::Echo; allowed = { "echoDelaySeconds", "echoPowerFraction" }; values.echoDelaySeconds = ParseFiniteFloat(rule.at("echoDelaySeconds"), context); values.echoPowerFraction = ParseFiniteFloat(rule.at("echoPowerFraction"), context); if (values.echoDelaySeconds <= 0.f || values.echoPowerFraction <= 0.f || values.echoPowerFraction > 1.f) throw std::runtime_error(context + " invalid echo values"); }
				else if (name == "Mirror") values.kind = UniqueModuleKind::Mirror;
				else if (name == "Pulsar") { values.kind = UniqueModuleKind::Pulsar; allowed = { "pulsarIntervalSeconds" }; values.pulsarIntervalSeconds = ParseFiniteFloat(rule.at("pulsarIntervalSeconds"), context); if (values.pulsarIntervalSeconds <= 0.f) throw std::runtime_error(context + " invalid interval"); }
				else if (name == "Spark") values.kind = UniqueModuleKind::Spark;
				else if (name == "ProfitShare") { values.kind = UniqueModuleKind::ProfitShare; allowed = { "goldPerTrigger", "cooldownSeconds" }; values.goldPerTrigger = ParsePositiveInt(rule.at("goldPerTrigger"), context); values.cooldownSeconds = ParseFiniteFloat(rule.at("cooldownSeconds"), context); if (values.cooldownSeconds <= 0.f) throw std::runtime_error(context + " invalid cooldown"); }
				else throw std::runtime_error(context + " unknown Unique kind");
				for (const auto& field : rule.items()) if (!allowed.count(field.key())) throw std::runtime_error(context + " unknown Unique field " + field.key());
				definition.unique = values;
			}
			else if (object.contains("unique")) throw std::runtime_error(context + " unique requires a Unique ID");
			definition.displayName = object.value("displayName", id);
			if (object.contains("rarity"))
			{
				if (!object.at("rarity").is_string()) throw std::runtime_error(context + " rarity must be a string");
				definition.rarity = ParseRarity(object.at("rarity").get<std::string>());
			}
			if (object.contains("levelCount"))
				definition.levelCount = ParsePositiveInt(object.at("levelCount"), context + " levelCount");
			if (object.contains("description"))
			{
				if (!object.at("description").is_string()) throw std::runtime_error(context + " description must be a string");
				definition.description = object.at("description").get<std::string>();
			}
			if (!parsedId.abilityModule && !parsedId.unique) definition.family = ParseTag(object.at("family"), GameplayTagKind::AbilityCategory, context + " family");
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
			if (object.contains("sideFeatures"))
			{
				const Json& sideFeatures = object.at("sideFeatures");
				if (!sideFeatures.is_array()) throw std::runtime_error(context + " sideFeatures must be an array");
				std::set<std::string> seenFeatures;
				for (const Json& feature : sideFeatures)
				{
					const sas::ContentId featureId = ParseContentId(feature, context + " sideFeatures");
					if (!seenFeatures.insert(featureId.ToString()).second)
						throw std::runtime_error(context + " contains duplicate sideFeature " + featureId.ToString());
					definition.sideFeatures.push_back(featureId);
				}
			}
			if (object.contains("sharedPools"))
			{
				const Json& sharedPools = object.at("sharedPools");
				if (!sharedPools.is_array()) throw std::runtime_error(context + " sharedPools must be an array");
				std::set<std::string> seenPools;
				std::set<std::string> seenSharedFeatures;
				for (const Json& pool : sharedPools)
				{
					if (!pool.is_object()) throw std::runtime_error(context + " sharedPools entries must be objects");
					ModuleSharedPoolPermission permission;
					permission.poolId = ParseContentId(pool.at("poolId"), context + " sharedPools.poolId");
					if (!seenPools.insert(permission.poolId.ToString()).second)
						throw std::runtime_error(context + " contains duplicate shared pool " + permission.poolId.ToString());
					const Json& allowedFeatures = pool.at("allowedFeatureIds");
					if (!allowedFeatures.is_array() || allowedFeatures.empty())
						throw std::runtime_error(context + " shared pool must list allowedFeatureIds");
					std::set<std::string> seenAllowedFeatures;
					for (const Json& feature : allowedFeatures)
					{
						const sas::ContentId featureId = ParseContentId(feature, context + " sharedPools.allowedFeatureIds");
						if (!seenAllowedFeatures.insert(featureId.ToString()).second)
							throw std::runtime_error(context + " shared pool repeats allowed feature " + featureId.ToString());
						if (!seenSharedFeatures.insert(featureId.ToString()).second)
							throw std::runtime_error(context + " shares one side feature across multiple pools");
						for (const sas::ContentId& privateId : definition.sideFeatures)
							if (privateId == featureId) throw std::runtime_error(context + " feature cannot be both private and shared");
						permission.allowedFeatureIds.push_back(featureId);
					}
					definition.sharedPools.push_back(std::move(permission));
				}
			}
			if (object.contains("conflictsWith"))
			{
				const Json& conflicts = object.at("conflictsWith");
				if (!conflicts.is_array()) throw std::runtime_error(context + " conflictsWith must be an array");
				std::set<std::string> seenConflicts;
				for (const Json& conflict : conflicts)
				{
					const sas::ContentId conflictId = ParseContentId(conflict, context + " conflictsWith");
					if (conflictId == definition.moduleId) throw std::runtime_error(context + " cannot conflict with itself");
					if (!seenConflicts.insert(conflictId.ToString()).second)
						throw std::runtime_error(context + " contains duplicate conflict " + conflictId.ToString());
					definition.conflictsWith.push_back(conflictId);
				}
			}
			const Json& variants = object.at("sizeVariants");
			if (parsedId.abilityModule || parsedId.unique)
			{
				if (!variants.is_array() || variants.size() != 1) throw std::runtime_error(context + " ability modules require exactly one size variant");
				for (const auto& field : variants.front().items())
					if (field.key() != "shape") throw std::runtime_error(context + " ability modules use fixed abilityModule rules only");
			}
			if (!variants.is_array() || variants.empty())
			{
				throw std::runtime_error(context + " must define at least one size variant");
			}
			std::size_t index = 0;
			for (const Json& variant : variants)
			{
				const std::string variantContext = context + ".sizeVariants[" + std::to_string(index++) + "]";
				module_loader_detail::RejectZoneRules(variant, variantContext);
				definition.sizeVariants.push_back(
					ParseVariant(variant, variantContext, definition.levelCount, true, false)
				);
			}
			if (object.contains("sizeLadder"))
			{
				const Json& ladder = object.at("sizeLadder");
				if (!ladder.is_array() || ladder.size() != definition.sizeVariants.size())
					throw std::runtime_error(context + " sizeLadder must declare every size variant");
				int previousCellCount = 0;
				for (std::size_t ladderIndex = 0; ladderIndex < ladder.size(); ++ladderIndex)
				{
					const int declaredCellCount = ParsePositiveInt(ladder[ladderIndex], context + ".sizeLadder[" + std::to_string(ladderIndex) + "]");
					const int actualCellCount = static_cast<int>(definition.sizeVariants[ladderIndex].shape.cells.size());
					if (declaredCellCount != actualCellCount || declaredCellCount <= previousCellCount)
						throw std::runtime_error(context + " sizeLadder must match shapes in strictly increasing cell count");
					definition.sizeLadderCellCounts.push_back(declaredCellCount);
					previousCellCount = declaredCellCount;
				}
			}
			if (definition.sizeVariants.size() == 1 && (!definition.sideFeatures.empty() || !definition.sharedPools.empty()))
				throw std::runtime_error(context + " single-size modules cannot declare side features or shared pools");
			if ((!definition.sideFeatures.empty() || !definition.sharedPools.empty()) &&
				definition.sizeLadderCellCounts.size() != definition.sizeVariants.size())
				throw std::runtime_error(context + " side-feature modules require an explicit sizeLadder");
			if ((parsedId.abilityModule || parsedId.unique) && definition.sizeVariants.front().shape.cells.size() != 1) throw std::runtime_error(context + " ability modules must occupy exactly 1 cell");
			if (definition.rarity == ModuleRarity::Legendary)
			{
				if (definition.sizeVariants.size() != 1) throw std::runtime_error(context + " Legendary modules require exactly one size variant");
				if (!definition.sideFeatures.empty() || !definition.sharedPools.empty()) throw std::runtime_error(context + " Legendary modules cannot declare side features or shared pools");
				if (definition.levelCount != 3 || definition.systemCategory.IsValid() || definition.abilityModule) throw std::runtime_error(context + " Legendary modules must be normal L1-L3 main modules");
				definition.legendary = ParseLegendaryRule(object.at("legendary"), definition.family, context, definition.levelCount);
			}
			else if (object.contains("legendary")) throw std::runtime_error(context + " legendary rules require Legendary rarity");
			ValidateDescription(definition, context);
			ValidateZoneEffects(definition, zoneTable, context);
			if (definition.systemCategory.IsValid())
			{
				if (GameplayTagSchema::IsOneCellModuleSystem(definition.systemCategory))
				{
					for (const ModuleSizeVariant& variant : definition.sizeVariants)
					{
						if (variant.shape.cells.size() != 1) throw std::runtime_error(context + " must occupy exactly 1 cell in every size variant");
					}
				}
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
				const Json& levelBonus = entry.at("levelBonus");
				if (!levelBonus.is_number_integer()) throw std::runtime_error("Zone levelBonus must be an integer");
				zone.levelBonus = levelBonus.get<int>();
				if (zone.levelBonus < 0 || zone.levelBonus > kMaxZoneLevelBonus)
				{
					throw std::runtime_error("Zone levelBonus must be in [0, 2]");
				}
				table.zones.push_back(std::move(zone));
			}
			const GameplayTag* requiredZones[] = {
				&GameplayTags::Module::Zone::Nose, &GameplayTags::Module::Zone::Wings,
				&GameplayTags::Module::Zone::Hull, &GameplayTags::Module::Zone::Core,
				&GameplayTags::Module::Zone::Engine, &GameplayTags::Module::Zone::Sensor,
				&GameplayTags::Module::Zone::AbilityBay
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
				if (GameplayTagSchema::IsModuleAbilitySlotSystem(entry.system) != (entry.zone == GameplayTags::Module::Zone::AbilityBay))
				{
					throw std::runtime_error("Only ability slot systems belong in the ability bay, and they belong nowhere else: " + entry.system.name);
				}
				if (!table.FindZone(entry.zone))
				{
					throw std::runtime_error("System zone is missing from the zone table: " + entry.zone.name);
				}
				table.systemZones.push_back(std::move(entry));
			}
			for (const GameplayTag* required : GameplayTagSchema::GetZonedRequiredModuleSystems())
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

	ModuleRulePackage ModuleLoader::ParseRulePackage(const nlohmann::json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus, bool sideFeature)
	{
		if (!object.is_object()) throw std::runtime_error(context + " must be an object");
		if (object.contains("shape")) throw std::runtime_error(context + " rule package must not define a shape");
		module_loader_detail::RejectZoneRules(object, context);
		nlohmann::json shapedObject = object;
		nlohmann::json shape = nlohmann::json::array();
		shape.push_back(nlohmann::json::array({ 0, 0 }));
		shapedObject["shape"] = std::move(shape);
		ModuleSizeVariant parsed = module_loader_detail::ParseVariant(shapedObject, context, minimumLevelCount, defaultAffectedByZoneBonus, sideFeature);
		return static_cast<const ModuleRulePackage&>(parsed);
	}

	bool ModuleLoader::ValidateRulePackageLevelCount(const ModuleRulePackage& rules, int levelCount, const std::string& context, std::string* failureReason)
	{
		if (levelCount <= 0)
		{
			if (failureReason) *failureReason = context + " levelCount must be positive";
			return false;
		}
		const std::size_t required = static_cast<std::size_t>(levelCount);
		return VisitModuleRuleLevelTables(rules, [&](const char* path, std::size_t index, const auto& table)
		{
			if (table.values.empty() || table.values.size() >= required) return true;
			if (failureReason) *failureReason = context + " " + path + " (rule " + std::to_string(index) + ") level table is shorter than levelCount";
			return false;
		});
	}

	bool ModuleLoader::ValidateRulePackageZoneEffects(const ModuleRulePackage& rules, const ModuleZoneTable& zoneTable, const std::string& context, std::string* failureReason)
	{
		for (const ModuleZoneEffect& effect : rules.zoneEffects)
		{
			const ModuleZoneEntry* zone = zoneTable.FindZone(effect.zone);
			if (!zone)
			{
				if (failureReason) *failureReason = context + " zone effect references an unknown zone: " + effect.zone.name;
				return false;
			}
			if (effect.scope == ModuleCooldownScope::FamilyOfZone && (zone->anyFamily || !zone->family.IsValid()))
			{
				if (failureReason) *failureReason = context + " FamilyOfZone cooldown requires a zone with one family: " + effect.zone.name;
				return false;
			}
			if (effect.kind == ModuleZoneEffectKind::PrimaryHitCooldownSeconds &&
				((effect.scope == ModuleCooldownScope::FamilyOfZone && zone->family == GameplayTags::Ability::Primary) ||
				(effect.scope == ModuleCooldownScope::AllActiveNonPrimary && !zone->anyFamily)))
			{
				if (failureReason) *failureReason = context + " hit refunds require a non-Primary family or an all-family zone";
				return false;
			}
		}
		return true;
	}

	ModuleLoader::Result ModuleLoader::LoadFromFile(const std::filesystem::path& filePath)
	{
		const JsonDocumentLoader::Result documentResult = JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded())
		{
			return Result{ {}, {}, documentResult.error };
		}

		return LoadFromJson(*documentResult.document);
	}

	ModuleLoader::Result ModuleLoader::LoadFromJson(const nlohmann::json& root, const std::string& selectedId)
	{
		try
		{

			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, {}, "Unsupported module schema version" };
			}

			Result result;
			result.zoneTable = module_loader_detail::ParseZoneTable(root);
			std::set<std::string> moduleIds;
			for (const module_loader_detail::Json& module : root.at("modules"))
			{
				if (!selectedId.empty() && module.value("id", std::string{}) != selectedId) continue;
				ModuleDefinition definition = module_loader_detail::ParseModule(module, result.zoneTable);
				if (!moduleIds.insert(definition.moduleId.ToString()).second)
				{
					throw std::runtime_error("Duplicate module ID: " + definition.moduleId.ToString());
				}
				result.definitions.push_back(std::move(definition));
			}
			for (const module_loader_detail::Json& module : root.at("modules")) moduleIds.insert(module.at("id").get<std::string>());
			for (const ModuleDefinition& definition : result.definitions)
			{
				for (const sas::ContentId& conflictId : definition.conflictsWith)
				{
					if (!moduleIds.count(conflictId.ToString()))
						throw std::runtime_error("Module " + definition.moduleId.ToString() + " conflictsWith unknown module " + conflictId.ToString());
				}
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				{},
				"Failed to load modules from '" + std::string{ "content document" } + "': " + exception.what()
			};
		}
	}
}

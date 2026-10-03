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

		ModuleTriggerKind ParseTriggerKind(const std::string& value)
		{
			if (value == "None") return ModuleTriggerKind::None;
			if (value == "AbilityActivated") return ModuleTriggerKind::AbilityActivated;
			if (value == "ShieldBroken") return ModuleTriggerKind::ShieldBroken;
			if (value == "DamageTaken") return ModuleTriggerKind::DamageTaken;
			if (value == "SourceDamageDealt") return ModuleTriggerKind::SourceDamageDealt;
			throw std::runtime_error("Unknown module trigger kind: " + value);
		}

		ModuleDamageCondition ParseDamageCondition(const std::string& value)
		{
			if (value == "PrimaryShotOnly") return ModuleDamageCondition::PrimaryShotOnly;
			if (value == "TargetHealthAtMost") return ModuleDamageCondition::TargetHealthAtMost;
			throw std::runtime_error("Unknown module damage condition: " + value);
		}

		ModuleCooldownScope ParseCooldownScope(const std::string& value)
		{
			if (value == "FamilyOfZone") return ModuleCooldownScope::FamilyOfZone;
			if (value == "AllActiveNonPrimary") return ModuleCooldownScope::AllActiveNonPrimary;
			throw std::runtime_error("Unknown module cooldown scope: " + value);
		}

		DamageCriticalPolicy ParseCriticalPolicy(const std::string& value)
		{
			if (value == "Random") return DamageCriticalPolicy::Random;
			if (value == "Guaranteed") return DamageCriticalPolicy::Guaranteed;
			if (value == "Disabled") return DamageCriticalPolicy::Disabled;
			throw std::runtime_error("Unknown module bonus critical policy: " + value);
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

		ModuleAttributeModifier ParseAttributeModifier(const Json& object, const std::string& context, bool defaultAffectedByZoneBonus)
		{
			ModuleAttributeModifier modifier;
			modifier.target = ParseTarget(object.at("target").get<std::string>());
			modifier.modifier = ParseModifier(object, context);
			modifier.affectedByZoneBonus = object.value("affectedByZoneBonus", defaultAffectedByZoneBonus);
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
				variant.attributeModifiers.push_back(ParseAttributeModifier(entry, entryContext, true));
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
			index = 0;
			const Json triggerBuffs = object.value("triggerBuffs", Json::array());
			if (!triggerBuffs.is_array()) throw std::runtime_error(context + ".triggerBuffs must be an array");
			for (const Json& entry : triggerBuffs)
			{
				const std::string entryContext = context + ".triggerBuffs[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				ModuleTriggerBuff buff;
				buff.trigger = ParseTriggerKind(entry.at("trigger").get<std::string>());
				if (buff.trigger == ModuleTriggerKind::None) throw std::runtime_error(entryContext + " trigger must not be None");
				buff.durationSeconds = ParseFiniteFloat(entry.at("durationSeconds"), entryContext + ".durationSeconds");
				if (buff.durationSeconds <= 0.f) throw std::runtime_error(entryContext + " durationSeconds must be positive");
				buff.movementSpeedMultiplier = ParseFiniteFloat(entry.value("movementSpeedMultiplier", Json(1.f)), entryContext + ".movementSpeedMultiplier");
				if (buff.movementSpeedMultiplier <= 0.f) throw std::runtime_error(entryContext + " movementSpeedMultiplier must be positive");
				const Json tags = entry.value("requiredEventTags", Json::array());
				if (!tags.is_array()) throw std::runtime_error(entryContext + ".requiredEventTags must be an array");
				std::set<std::string> seenTags;
				for (const Json& tagValue : tags)
				{
					const GameplayTagKind kind = buff.trigger == ModuleTriggerKind::AbilityActivated
						? GameplayTagKind::AbilityCategory
						: GameplayTagKind::DamageType;
					if (buff.trigger != ModuleTriggerKind::AbilityActivated && buff.trigger != ModuleTriggerKind::SourceDamageDealt)
						throw std::runtime_error(entryContext + " does not accept requiredEventTags for this trigger");
					GameplayTag tag = ParseTag(tagValue, kind, entryContext + ".requiredEventTags");
					if (!seenTags.insert(tag.name).second) throw std::runtime_error(entryContext + " has a duplicate required event tag: " + tag.name);
					buff.requiredEventTags.push_back(std::move(tag));
				}
				const Json modifiers = entry.value("modifiers", Json::array());
				if (!modifiers.is_array()) throw std::runtime_error(entryContext + ".modifiers must be an array");
				std::size_t modifierIndex = 0;
				for (const Json& modifier : modifiers)
				{
					const std::string modifierContext = entryContext + ".modifiers[" + std::to_string(modifierIndex++) + "]";
					buff.modifiers.push_back(ParseAttributeModifier(modifier, modifierContext, false));
				}
				if (buff.modifiers.empty() && buff.movementSpeedMultiplier == 1.f)
					throw std::runtime_error(entryContext + " must define a modifier or movementSpeedMultiplier");
				variant.triggerBuffs.push_back(std::move(buff));
			}
			index = 0;
			const Json damageRules = object.value("damageRules", Json::array());
			if (!damageRules.is_array()) throw std::runtime_error(context + ".damageRules must be an array");
			for (const Json& entry : damageRules)
			{
				const std::string entryContext = context + ".damageRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				ModuleDamageRule rule;
				rule.condition = ParseDamageCondition(entry.at("condition").get<std::string>());
				if (entry.contains("bonusCriticalPolicy"))
					rule.bonusCriticalPolicy = ParseCriticalPolicy(entry.at("bonusCriticalPolicy").get<std::string>());
				rule.conditionValue = entry.contains("conditionValue") ? ParseFiniteFloat(entry.at("conditionValue"), entryContext + ".conditionValue") : 0.f;
				rule.damageMultiplier = entry.contains("damageMultiplier") ? ParseFiniteFloat(entry.at("damageMultiplier"), entryContext + ".damageMultiplier") : 1.f;
				rule.bonusDamageFraction = entry.contains("bonusDamageFraction") ? ParseFiniteFloat(entry.at("bonusDamageFraction"), entryContext + ".bonusDamageFraction") : 0.f;
				if (rule.damageMultiplier <= 0.f) throw std::runtime_error(entryContext + " damageMultiplier must be positive");
				if (rule.bonusDamageFraction < 0.f || rule.bonusDamageFraction > 1.f)
					throw std::runtime_error(entryContext + " bonusDamageFraction must be between 0 and 1");
				if (rule.condition == ModuleDamageCondition::TargetHealthAtMost)
				{
					if (!entry.contains("conditionValue") || rule.conditionValue < 0.f || rule.conditionValue > 1.f)
						throw std::runtime_error(entryContext + " TargetHealthAtMost requires conditionValue in 0..1");
				}
				else if (entry.contains("conditionValue") && rule.conditionValue != 0.f)
				{
					throw std::runtime_error(entryContext + " PrimaryShotOnly does not accept conditionValue");
				}
				if (rule.bonusDamageFraction > 0.f)
				{
					if (!entry.contains("bonusDamageType")) throw std::runtime_error(entryContext + " bonusDamageFraction requires bonusDamageType");
					rule.bonusDamageType = ParseTag(entry.at("bonusDamageType"), GameplayTagKind::DamageType, entryContext + ".bonusDamageType");
					if (rule.bonusCriticalPolicy != DamageCriticalPolicy::Disabled)
						throw std::runtime_error(entryContext + " bonus damage critical policy must be Disabled");
				}
				else if (entry.contains("bonusDamageType") || entry.contains("bonusCriticalPolicy"))
				{
					throw std::runtime_error(entryContext + " bonus damage fields require bonusDamageFraction");
				}
				if (entry.contains("activeWhileTrigger"))
					rule.activeWhileTrigger = ParseTriggerKind(entry.at("activeWhileTrigger").get<std::string>());
				if (rule.activeWhileTrigger != ModuleTriggerKind::None)
				{
					if (!entry.contains("activeDurationSeconds")) throw std::runtime_error(entryContext + " active trigger requires activeDurationSeconds");
					rule.activeDurationSeconds = ParseFiniteFloat(entry.at("activeDurationSeconds"), entryContext + ".activeDurationSeconds");
					if (rule.activeDurationSeconds <= 0.f) throw std::runtime_error(entryContext + " activeDurationSeconds must be positive");
				}
				else if (entry.contains("activeDurationSeconds"))
				{
					throw std::runtime_error(entryContext + " activeDurationSeconds requires activeWhileTrigger");
				}
				if (rule.damageMultiplier == 1.f && rule.bonusDamageFraction == 0.f)
					throw std::runtime_error(entryContext + " must change outgoing damage");
				variant.damageRules.push_back(std::move(rule));
			}
			index = 0;
			std::set<std::string> seenZones;
			const Json zoneEffects = object.value("zoneEffects", Json::array());
			if (!zoneEffects.is_array()) throw std::runtime_error(context + ".zoneEffects must be an array");
			for (const Json& entry : zoneEffects)
			{
				const std::string entryContext = context + ".zoneEffects[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				ModuleZoneEffect effect;
				effect.zone = ParseTag(entry.at("zone"), GameplayTagKind::ModuleZone, entryContext + ".zone");
				if (!seenZones.insert(effect.zone.name).second) throw std::runtime_error(entryContext + " duplicates zone effect: " + effect.zone.name);
				effect.scope = ParseCooldownScope(entry.at("scope").get<std::string>());
				effect.multiplier = ParseFiniteFloat(entry.at("multiplier"), entryContext + ".multiplier");
				if (effect.multiplier <= 0.f || effect.multiplier > 1.f)
					throw std::runtime_error(entryContext + " multiplier must be in (0, 1]");
				variant.zoneEffects.push_back(std::move(effect));
			}
			return variant;
		}

		bool HasAttribute(const ModuleSizeVariant& variant, const std::string& attributeName)
		{
			for (const ModuleAttributeModifier& modifier : variant.attributeModifiers)
				if (modifier.modifier.attributeId.GetName() == attributeName) return true;
			for (const ModuleAbilityModifier& modifier : variant.abilityModifiers)
				if (modifier.modifier.attributeId.GetName() == attributeName) return true;
			for (const ModuleTriggerBuff& buff : variant.triggerBuffs)
				for (const ModuleAttributeModifier& modifier : buff.modifiers)
					if (modifier.modifier.attributeId.GetName() == attributeName) return true;
			return false;
		}

		bool HasDescriptionToken(const ModuleSizeVariant& variant, const std::string& token)
		{
			if (HasAttribute(variant, token)) return true;
			for (std::size_t i = 0; i < variant.triggerBuffs.size(); ++i)
			{
				const std::string prefix = "trigger." + std::to_string(i) + ".";
				if (token == prefix + "durationSeconds" || token == prefix + "movementSpeedMultiplier") return true;
			}
			for (std::size_t i = 0; i < variant.damageRules.size(); ++i)
			{
				const ModuleDamageRule& rule = variant.damageRules[i];
				const std::string prefix = "damage." + std::to_string(i) + ".";
				if (token == prefix + "damageMultiplier") return true;
				if (token == prefix + "conditionPercent" && rule.condition == ModuleDamageCondition::TargetHealthAtMost) return true;
				if (token == prefix + "bonusDamagePercent" && rule.bonusDamageFraction > 0.f) return true;
				if (token == prefix + "activeDurationSeconds" && rule.activeWhileTrigger != ModuleTriggerKind::None) return true;
			}
			for (const ModuleZoneEffect& effect : variant.zoneEffects)
				if (token == "cooldown." + effect.zone.name + ".multiplier") return true;
			return false;
		}

		void ValidateDescription(const ModuleDefinition& definition, const std::string& context)
		{
			std::size_t position = 0;
			while (true)
			{
				const std::size_t open = definition.description.find('{', position);
				const std::size_t strayClose = definition.description.find('}', position);
				if (open == std::string::npos)
				{
					if (strayClose != std::string::npos) throw std::runtime_error(context + " description has an unmatched closing brace");
					return;
				}
				if (strayClose != std::string::npos && strayClose < open)
					throw std::runtime_error(context + " description has an unmatched closing brace");

				const std::size_t close = definition.description.find('}', open + 1);
				if (close == std::string::npos || close == open + 1)
					throw std::runtime_error(context + " description has an invalid attribute placeholder");

				const std::string token = definition.description.substr(open + 1, close - open - 1);
				for (const ModuleSizeVariant& variant : definition.sizeVariants)
					if (!HasDescriptionToken(variant, token))
						throw std::runtime_error(context + " description references a missing numeric value: " + token);
				position = close + 1;
			}
		}

		void ValidateZoneEffects(const ModuleDefinition& definition, const ModuleZoneTable& zoneTable, const std::string& context)
		{
			for (const ModuleSizeVariant& variant : definition.sizeVariants)
			{
				for (const ModuleZoneEffect& effect : variant.zoneEffects)
				{
					const ModuleZoneEntry* zone = zoneTable.FindZone(effect.zone);
					if (!zone) throw std::runtime_error(context + " zone effect references an unknown zone: " + effect.zone.name);
					if (effect.scope == ModuleCooldownScope::FamilyOfZone && (zone->anyFamily || !zone->family.IsValid()))
						throw std::runtime_error(context + " FamilyOfZone cooldown requires a zone with one family: " + effect.zone.name);
				}
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
			definition.displayName = object.value("displayName", id);
			if (object.contains("description"))
			{
				if (!object.at("description").is_string()) throw std::runtime_error(context + " description must be a string");
				definition.description = object.at("description").get<std::string>();
			}
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
			ValidateDescription(definition, context);
			ValidateZoneEffects(definition, zoneTable, context);
			if (definition.systemCategory.IsValid())
			{
				if (GameplayTagSchema::IsTwoCellModuleSystem(definition.systemCategory))
				{
					for (const ModuleSizeVariant& variant : definition.sizeVariants)
					{
						if (variant.shape.cells.size() != 2)
						{
							throw std::runtime_error(context + " must occupy exactly 2 cells in every size variant");
						}
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
				if (GameplayTagSchema::IsModuleAbilitySlotSystem(entry.system))
				{
					throw std::runtime_error("Ability slot systems must be installable in any module zone: " + entry.system.name);
				}
				entry.zone = ParseTag(it.value(), GameplayTagKind::ModuleZone, "System zone");
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
				ModuleDefinition definition = ParseModule(module, result.zoneTable);
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

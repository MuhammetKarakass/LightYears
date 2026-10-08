#include "gameplay/module/ModuleDescriptionValues.h"
#include "gameplay/content/ModuleLoader.h"

#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/AttributeJsonParser.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include "framework/JsonDocumentLoader.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <initializer_list>
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

		template<typename T, typename ParseValue>
		T ParseLevelValue(const Json& value, const std::string& context, int minimumLevelCount, ModuleLevelTable<T>& table, ParseValue parseValue)
		{
			if (!value.is_array()) return parseValue(value, context);
			if (value.empty()) throw std::runtime_error(context + " level table must not be empty");
			for (std::size_t index = 0; index < value.size(); ++index)
			{
				table.values.push_back(parseValue(value[index], context + "[" + std::to_string(index) + "]"));
			}
			if (minimumLevelCount > 0 && table.values.size() < static_cast<std::size_t>(minimumLevelCount))
			{
				throw std::runtime_error(context + " level table is shorter than levelCount");
			}
			return table.values.front();
		}

		float ParseLevelFloat(const Json& value, const std::string& context, int minimumLevelCount, ModuleLevelTable<float>& table)
		{
			return ParseLevelValue<float>(value, context, minimumLevelCount, table,
				[](const Json& entry, const std::string& entryContext) { return ParseFiniteFloat(entry, entryContext); });
		}

		int ParsePositiveLevelInt(const Json& value, const std::string& context, int minimumLevelCount, ModuleLevelTable<int>& table)
		{
			return ParseLevelValue<int>(value, context, minimumLevelCount, table,
				[](const Json& entry, const std::string& entryContext)
				{
					std::uint64_t parsed = 0;
					if (entry.is_number_unsigned())
					{
						parsed = entry.get<std::uint64_t>();
					}
					else if (entry.is_number_integer())
					{
						const std::int64_t signedValue = entry.get<std::int64_t>();
						if (signedValue <= 0) throw std::runtime_error(entryContext + " must be positive");
						parsed = static_cast<std::uint64_t>(signedValue);
					}
					else
					{
						throw std::runtime_error(entryContext + " must be an integer");
					}
					if (parsed == 0 || parsed > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
						throw std::runtime_error(entryContext + " must be a positive 32-bit integer");
					return static_cast<int>(parsed);
				});
		}

		int ParsePositiveInt(const Json& value, const std::string& context)
		{
			if (value.is_array()) throw std::runtime_error(context + " must be a scalar integer");
			ModuleLevelTable<int> unused;
			return ParsePositiveLevelInt(value, context, 0, unused);
		}

		template<typename T, typename Predicate>
		void ValidateTable(const ModuleLevelTable<T>& table, const std::string& context, Predicate valid, const char* message)
		{
			for (const T& value : table.values)
				if (!valid(value)) throw std::runtime_error(context + " " + message);
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

		ModuleRarity ParseRarity(const std::string& value)
		{
			if (value == "Common") return ModuleRarity::Common;
			if (value == "Uncommon") return ModuleRarity::Uncommon;
			if (value == "Rare") return ModuleRarity::Rare;
			if (value == "Epic") return ModuleRarity::Epic;
			if (value == "Legendary") return ModuleRarity::Legendary;
			throw std::runtime_error("Unknown module rarity: " + value);
		}

		sas::ContentId ParseContentId(const Json& value, const std::string& context)
		{
			if (!value.is_string()) throw std::runtime_error(context + " must be a string");
			const std::string id = value.get<std::string>();
			std::string failure;
			if (!ContentIdSchema::ValidateContentId(id, &failure)) throw std::runtime_error(context + ": " + failure);
			return sas::ContentId{ id };
		}

		ModuleTriggerKind ParseTriggerKind(const std::string& value)
		{
			if (value == "KillConfirmed") return ModuleTriggerKind::KillConfirmed;
			if (value == "CriticalHit") return ModuleTriggerKind::CriticalHit;
			if (value == "None") return ModuleTriggerKind::None;
			if (value == "AbilityActivated") return ModuleTriggerKind::AbilityActivated;
			if (value == "ShieldBroken") return ModuleTriggerKind::ShieldBroken;
			if (value == "DamageTaken") return ModuleTriggerKind::DamageTaken;
			if (value == "SourceDamageDealt") return ModuleTriggerKind::SourceDamageDealt;
			throw std::runtime_error("Unknown module trigger kind: " + value);
		}

		List<GameplayTag> ParseRequiredEventTags(const Json& tags, ModuleTriggerKind trigger, const std::string& context)
		{
			if (!tags.is_array()) throw std::runtime_error(context + " must be an array");
			List<GameplayTag> parsed;
			std::set<std::string> seenTags;
			for (const Json& tagValue : tags)
			{
				if (trigger != ModuleTriggerKind::AbilityActivated && trigger != ModuleTriggerKind::SourceDamageDealt && trigger != ModuleTriggerKind::CriticalHit)
					throw std::runtime_error(context + " is not accepted for this trigger");
				const GameplayTagKind kind = trigger == ModuleTriggerKind::AbilityActivated
					? GameplayTagKind::AbilityCategory
					: GameplayTagKind::DamageType;
				GameplayTag tag = ParseTag(tagValue, kind, context);
				if (!seenTags.insert(tag.name).second) throw std::runtime_error(context + " has a duplicate tag: " + tag.name);
				parsed.push_back(std::move(tag));
			}
			return parsed;
		}

		ModuleDamageCondition ParseDamageCondition(const std::string& value)
		{
			if (value == "OwnerHealthAtMost") return ModuleDamageCondition::OwnerHealthAtMost;
			if (value == "TargetWithoutStatus") return ModuleDamageCondition::TargetWithoutStatus;
			if (value == "TargetThermalAndCryo") return ModuleDamageCondition::TargetThermalAndCryo;
			if (value == "SourceHasStatus") return ModuleDamageCondition::SourceHasStatus;
			if (value == "AdjacentAbilityDirectHit") return ModuleDamageCondition::AdjacentAbilityDirectHit;
			if (value == "PrimaryShotOnly") return ModuleDamageCondition::PrimaryShotOnly;
			if (value == "TargetHealthAtMost") return ModuleDamageCondition::TargetHealthAtMost;
			if (value == "FullMagazineShot") return ModuleDamageCondition::FullMagazineShot;
			if (value == "FirstTargetHit") return ModuleDamageCondition::FirstTargetHit;
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

		sas::AttributeModifier ParseModifier(const Json& object, const std::string& context, int minimumLevelCount, ModuleLevelTable<float>& magnitudeByLevel)
		{
			if (!object.is_object())
			{
				throw std::runtime_error(context + " must be an object");
			}
			sas::AttributeModifier modifier{
				sas::AttributeId{ object.at("attributeId").get<std::string>() },
				AttributeJsonParser::ParseOperation(object.at("operation").get<std::string>(), context),
				ParseLevelFloat(object.at("magnitude"), context + ".magnitude", minimumLevelCount, magnitudeByLevel),
				object.value("priority", 0)
			};
			std::string failure;
			if (!AttributeIdSchema::Validate(modifier.attributeId, &failure))
			{
				throw std::runtime_error(context + ": " + failure);
			}
			return modifier;
		}

		ModuleAttributeModifier ParseAttributeModifier(const Json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus)
		{
			ModuleAttributeModifier modifier;
			modifier.target = ParseTarget(object.at("target").get<std::string>());
			modifier.modifier = ParseModifier(object, context, minimumLevelCount, modifier.magnitudeByLevel);
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

		ModuleSizeVariant ParseVariant(const Json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus)
		{
			ModuleSizeVariant variant;
			variant.shape = ParseShape(object.at("shape"), context + ".shape");
			std::size_t index = 0;
			for (const Json& entry : object.value("attributeModifiers", Json::array()))
			{
				const std::string entryContext = context + ".attributeModifiers[" + std::to_string(index++) + "]";
				variant.attributeModifiers.push_back(ParseAttributeModifier(entry, entryContext, minimumLevelCount, defaultAffectedByZoneBonus));
			}
			index = 0;
			for (const Json& entry : object.value("abilityModifiers", Json::array()))
			{
				const std::string entryContext = context + ".abilityModifiers[" + std::to_string(index++) + "]";
				ModuleAbilityModifier modifier;
				modifier.modifier = ParseModifier(entry, entryContext, minimumLevelCount, modifier.magnitudeByLevel);
				modifier.affectedByZoneBonus = entry.value("affectedByZoneBonus", defaultAffectedByZoneBonus);
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
				buff.durationSeconds = ParseLevelFloat(
					entry.at("durationSeconds"), entryContext + ".durationSeconds", minimumLevelCount, buff.durationSecondsByLevel);
				if (buff.durationSeconds <= 0.f) throw std::runtime_error(entryContext + " durationSeconds must be positive");
				ValidateTable(buff.durationSecondsByLevel, entryContext + ".durationSeconds", [](float value) { return value > 0.f; }, "must be positive");
				buff.movementSpeedMultiplier = entry.contains("movementSpeedMultiplier")
					? ParseLevelFloat(entry.at("movementSpeedMultiplier"), entryContext + ".movementSpeedMultiplier", minimumLevelCount,
						buff.movementSpeedMultiplierByLevel)
					: 1.f;
				if (buff.movementSpeedMultiplier <= 0.f) throw std::runtime_error(entryContext + " movementSpeedMultiplier must be positive");
				ValidateTable(buff.movementSpeedMultiplierByLevel, entryContext + ".movementSpeedMultiplier", [](float value) { return value > 0.f; }, "must be positive");
				buff.requiredEventTags = ParseRequiredEventTags(
					entry.value("requiredEventTags", Json::array()), buff.trigger, entryContext + ".requiredEventTags");
				const Json modifiers = entry.value("modifiers", Json::array());
				if (!modifiers.is_array()) throw std::runtime_error(entryContext + ".modifiers must be an array");
				std::size_t modifierIndex = 0;
				for (const Json& modifier : modifiers)
				{
					const std::string modifierContext = entryContext + ".modifiers[" + std::to_string(modifierIndex++) + "]";
					buff.modifiers.push_back(ParseAttributeModifier(modifier, modifierContext, minimumLevelCount, false));
				}
				bool changesMovementSpeed = buff.movementSpeedMultiplier != 1.f;
				for (float value : buff.movementSpeedMultiplierByLevel.values) changesMovementSpeed = changesMovementSpeed || value != 1.f;
				if (buff.modifiers.empty() && !changesMovementSpeed)
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
				rule.incoming = entry.value("incoming", false);
				rule.condition = ParseDamageCondition(entry.at("condition").get<std::string>());
				if (entry.contains("openingShotCount"))
				{
					if (rule.condition != ModuleDamageCondition::FullMagazineShot)
						throw std::runtime_error(entryContext + " openingShotCount requires FullMagazineShot and an integer");
					rule.openingShotCount = ParsePositiveLevelInt(
						entry.at("openingShotCount"), entryContext + ".openingShotCount", minimumLevelCount, rule.openingShotCountByLevel);
				}
				if (entry.contains("bonusCriticalPolicy"))
					rule.bonusCriticalPolicy = ParseCriticalPolicy(entry.at("bonusCriticalPolicy").get<std::string>());
				rule.conditionValue = entry.contains("conditionValue")
					? ParseLevelFloat(entry.at("conditionValue"), entryContext + ".conditionValue", minimumLevelCount, rule.conditionValueByLevel)
					: 0.f;
				rule.damageMultiplier = entry.contains("damageMultiplier")
					? ParseLevelFloat(entry.at("damageMultiplier"), entryContext + ".damageMultiplier", minimumLevelCount, rule.damageMultiplierByLevel)
					: 1.f;
				rule.bonusDamageFraction = entry.contains("bonusDamageFraction")
					? ParseLevelFloat(entry.at("bonusDamageFraction"), entryContext + ".bonusDamageFraction", minimumLevelCount,
						rule.bonusDamageFractionByLevel)
					: 0.f;
				if (rule.damageMultiplier <= 0.f) throw std::runtime_error(entryContext + " damageMultiplier must be positive");
				ValidateTable(rule.damageMultiplierByLevel, entryContext + ".damageMultiplier",
					[](float value) { return value > 0.f; }, "must be positive");
				if (rule.bonusDamageFraction < 0.f || rule.bonusDamageFraction > 1.f)
					throw std::runtime_error(entryContext + " bonusDamageFraction must be between 0 and 1");
				ValidateTable(rule.bonusDamageFractionByLevel, entryContext + ".bonusDamageFraction",
					[](float value) { return value >= 0.f && value <= 1.f; }, "must be between 0 and 1");
				bool hasBonusDamage = rule.bonusDamageFraction > 0.f;
				if (entry.contains("maxHealthDamageFraction"))
				{
					if (rule.condition != ModuleDamageCondition::AdjacentAbilityDirectHit || rule.incoming)
						throw std::runtime_error(entryContext + " MaxHealth damage requires outgoing adjacency");
					rule.maxHealthDamageFraction = ParseLevelFloat(entry.at("maxHealthDamageFraction"), entryContext, minimumLevelCount, rule.maxHealthDamageFractionByLevel);
					const auto positive = [](float value) { return value > 0.f; };
					if (!positive(rule.maxHealthDamageFraction)) throw std::runtime_error(entryContext + " invalid MaxHealth damage fraction");
					ValidateTable(rule.maxHealthDamageFractionByLevel, entryContext, positive, "invalid MaxHealth damage fraction");
				}
				else if (rule.condition == ModuleDamageCondition::AdjacentAbilityDirectHit)
					throw std::runtime_error(entryContext + " adjacency requires maxHealthDamageFraction");
				bool changesDamageMultiplier = rule.damageMultiplier != 1.f;
				for (float value : rule.bonusDamageFractionByLevel.values) hasBonusDamage = hasBonusDamage || value > 0.f;
				for (float value : rule.damageMultiplierByLevel.values) changesDamageMultiplier = changesDamageMultiplier || value != 1.f;
				if (GetModuleDamageConditionPhase(rule.condition) == ModuleDamageRulePhase::Fire && hasBonusDamage)
					throw std::runtime_error(entryContext + " fire-time conditions support damageMultiplier only");
				if (rule.condition == ModuleDamageCondition::TargetHealthAtMost || rule.condition == ModuleDamageCondition::OwnerHealthAtMost)
				{
					if (!entry.contains("conditionValue") || rule.conditionValue < 0.f || rule.conditionValue > 1.f)
						throw std::runtime_error(entryContext + " TargetHealthAtMost requires conditionValue in 0..1");
					ValidateTable(rule.conditionValueByLevel, entryContext + ".conditionValue",
						[](float value) { return value >= 0.f && value <= 1.f; }, "must be between 0 and 1");
				}
				else if (entry.contains("conditionValue"))
				{
					throw std::runtime_error(entryContext + " this damage condition does not accept conditionValue");
				}
				if (hasBonusDamage)
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
					rule.activeDurationSeconds = ParseLevelFloat(
						entry.at("activeDurationSeconds"), entryContext + ".activeDurationSeconds", minimumLevelCount,
						rule.activeDurationSecondsByLevel);
					if (rule.activeDurationSeconds <= 0.f) throw std::runtime_error(entryContext + " activeDurationSeconds must be positive");
					ValidateTable(rule.activeDurationSecondsByLevel, entryContext + ".activeDurationSeconds",
						[](float value) { return value > 0.f; }, "must be positive");
					rule.activeWhileEventTags = ParseRequiredEventTags(
						entry.value("activeWhileEventTags", Json::array()), rule.activeWhileTrigger, entryContext + ".activeWhileEventTags");
				}
				else if (entry.contains("activeDurationSeconds") || entry.contains("activeWhileEventTags"))
				{
					throw std::runtime_error(entryContext + " activeDurationSeconds and activeWhileEventTags require activeWhileTrigger");
				}
				if (!changesDamageMultiplier && !hasBonusDamage && rule.maxHealthDamageFraction <= 0.f)
					throw std::runtime_error(entryContext + " must change outgoing damage");
				if (rule.incoming && ((rule.condition != ModuleDamageCondition::OwnerHealthAtMost && rule.condition != ModuleDamageCondition::SourceHasStatus) || hasBonusDamage || rule.activeWhileTrigger != ModuleTriggerKind::None))
					throw std::runtime_error(entryContext + " incoming rules require an owner-health/source-status condition and a multiplier only");
				if (!rule.incoming && rule.condition == ModuleDamageCondition::SourceHasStatus)
					throw std::runtime_error(entryContext + " SourceHasStatus is an incoming condition");
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
				effect.multiplier = ParseLevelFloat(entry.at("multiplier"), entryContext + ".multiplier", minimumLevelCount, effect.multiplierByLevel);
				if (effect.multiplier <= 0.f || effect.multiplier > 1.f)
					throw std::runtime_error(entryContext + " multiplier must be in (0, 1]");
				ValidateTable(effect.multiplierByLevel, entryContext + ".multiplier",
					[](float value) { return value > 0.f && value <= 1.f; }, "must be in (0, 1]");
				variant.zoneEffects.push_back(std::move(effect));
			}
			const Json actions = object.value("eventActions", Json::array());
			if (!actions.is_array()) throw std::runtime_error(context + ".eventActions must be an array");
			for (const Json& entry : actions)
			{
				ModuleEventAction action;
				action.trigger = ParseTriggerKind(entry.at("trigger").get<std::string>());
				const std::string kind = entry.at("kind").get<std::string>();
				if (kind == "ShieldOverflowToHealth") action.kind = ModuleEventActionKind::ShieldOverflowToHealth;
				else if (kind == "RestoreShield") action.kind = ModuleEventActionKind::RestoreShield;
				else if (kind == "RestoreHealth") action.kind = ModuleEventActionKind::RestoreHealth;
				else if (kind == "RestoreAfterburner") action.kind = ModuleEventActionKind::RestoreAfterburner;
				else if (kind == "ReduceAllCooldowns") action.kind = ModuleEventActionKind::ReduceAllCooldowns;
				else if (kind == "ReduceLongestCooldown") action.kind = ModuleEventActionKind::ReduceLongestCooldown;
				else if (kind == "RefundUsedCooldown") action.kind = ModuleEventActionKind::RefundUsedCooldown;
				else if (kind == "ApplyNearbyCryo") action.kind = ModuleEventActionKind::ApplyNearbyCryo;
				else if (kind == "AwardGold") action.kind = ModuleEventActionKind::AwardGold;
				else throw std::runtime_error(context + " unknown event action: " + kind);
				const bool refund = action.kind == ModuleEventActionKind::RefundUsedCooldown;
				const bool cryo = action.kind == ModuleEventActionKind::ApplyNearbyCryo;
				if (action.trigger != (refund ? ModuleTriggerKind::AbilityActivated : cryo ? ModuleTriggerKind::DamageTaken : ModuleTriggerKind::KillConfirmed))
					throw std::runtime_error(context + " incompatible event action trigger");
				action.amount = ParseLevelFloat(entry.at("amount"), context + ".eventActions.amount", minimumLevelCount, action.amountByLevel);
				const bool seconds = action.kind == ModuleEventActionKind::ReduceAllCooldowns;
				const auto validAmount = [seconds, cryo, gold = action.kind == ModuleEventActionKind::AwardGold](float value) { return value > 0.f && ((cryo || gold) ? value <= static_cast<float>(std::numeric_limits<int>::max() / 2) && std::floor(value) == value : seconds || value <= 1.f); };
				if (!validAmount(action.amount)) throw std::runtime_error(context + " invalid event action amount");
				ValidateTable(action.amountByLevel, context, validAmount, "invalid event action amount");
				action.everyCount = entry.contains("everyCount") ? ParsePositiveInt(entry.at("everyCount"), context) : 1;
				if (action.kind == ModuleEventActionKind::ShieldOverflowToHealth && action.everyCount != 1)
					throw std::runtime_error(context + " shield overflow conversion must apply to every shield grant");
				action.overflowHealthFraction = entry.contains("overflowHealthFraction") ? ParseFiniteFloat(entry.at("overflowHealthFraction"), context) : 0.f;
				if (action.overflowHealthFraction < 0.f || action.overflowHealthFraction > 1.f ||
					(entry.contains("overflowHealthFraction") && action.kind != ModuleEventActionKind::RestoreShield))
					throw std::runtime_error(context + " invalid shield overflow conversion");
				if (entry.contains("chancePerLuck"))
				{
					if (!refund) throw std::runtime_error(context + " chancePerLuck requires RefundUsedCooldown");
					action.chancePerLuck = ParseLevelFloat(entry.at("chancePerLuck"), context, minimumLevelCount, action.chancePerLuckByLevel);
					if (action.chancePerLuck <= 0.f) throw std::runtime_error(context + " chancePerLuck must be positive");
					ValidateTable(action.chancePerLuckByLevel, context, [](float value) { return value > 0.f; }, "chancePerLuck must be positive");
				}
				else if (refund) throw std::runtime_error(context + " refund requires chancePerLuck");
				if (cryo)
				{
					action.radius = ParseFiniteFloat(entry.at("radius"), context + ".radius");
					action.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), context + ".cooldownSeconds");
					action.targetCount = ParsePositiveLevelInt(entry.at("targetCount"), context + ".targetCount", minimumLevelCount, action.targetCountByLevel);
					if (action.radius <= 0.f || action.cooldownSeconds <= 0.f || action.everyCount != 1)
						throw std::runtime_error(context + " nearby Cryo requires positive radius/cooldown and everyCount=1");
				}
				else if (entry.contains("radius") || entry.contains("cooldownSeconds") || entry.contains("targetCount"))
					throw std::runtime_error(context + " nearby targeting fields require ApplyNearbyCryo");
				variant.eventActions.push_back(std::move(action));
			}
			const Json ownerRules = object.value("ownerAttributeRules", Json::array());
			if (!ownerRules.is_array()) throw std::runtime_error(context + ".ownerAttributeRules must be an array");
			for (const Json& entry : ownerRules)
			{
				ModuleOwnerAttributeRule rule;
				const std::string source = entry.at("source").get<std::string>();
				if (source == "HealthThreshold") rule.source = ModuleOwnerAttributeSource::HealthThreshold;
				else if (source == "MovementSpeed") rule.source = ModuleOwnerAttributeSource::MovementSpeed;
				else if (source == "HigherPower") rule.source = ModuleOwnerAttributeSource::HigherPower;
				else if (source == "Luck") rule.source = ModuleOwnerAttributeSource::Luck;
				else throw std::runtime_error(context + " unknown owner attribute source: " + source);
				rule.attributeId = sas::AttributeId{ entry.at("attributeId").get<std::string>() };
				std::string failure;
				if (!AttributeIdSchema::Validate(rule.attributeId, &failure)) throw std::runtime_error(context + failure);
				rule.target = ParseTarget(entry.value("target", std::string{ "Owner" }));
				rule.operation = AttributeJsonParser::ParseOperation(entry.value("operation", std::string{ "Add" }), context);
				const bool ownerAttribute = rule.target == ModuleAttributeTarget::Owner && rule.operation == sas::AttributeModifierOperation::Add &&
					(rule.attributeId == OwnerAttributeIds::AttackSpeed || rule.attributeId == OwnerAttributeIds::AttackPower || rule.attributeId == OwnerAttributeIds::EnergyPower ||
						((rule.source == ModuleOwnerAttributeSource::Luck || rule.source == ModuleOwnerAttributeSource::HealthThreshold) && rule.attributeId == OwnerAttributeIds::CriticalChance));
				const bool shipAttribute = rule.target == ModuleAttributeTarget::Ship && rule.operation == sas::AttributeModifierOperation::Multiply && rule.source == ModuleOwnerAttributeSource::HealthThreshold && rule.attributeId == ShipAttributeIds::ShieldRegen;
				if (!ownerAttribute && !shipAttribute)
					throw std::runtime_error(context + " unsupported conditional owner attribute");
				rule.healthAtMost = entry.contains("healthAtMost") ? ParseFiniteFloat(entry.at("healthAtMost"), context) : 1.f;
				if ((rule.source == ModuleOwnerAttributeSource::MovementSpeed || rule.source == ModuleOwnerAttributeSource::Luck) && entry.contains("healthAtMost"))
					throw std::runtime_error(context + " movement scaling does not accept a health threshold");
				if (rule.healthAtMost <= 0.f || rule.healthAtMost > 1.f) throw std::runtime_error(context + " invalid owner health threshold");
				rule.amount = ParseLevelFloat(entry.at("amount"), context, minimumLevelCount, rule.amountByLevel);
				if (rule.amount <= 0.f) throw std::runtime_error(context + " owner attribute amount must be positive");
				ValidateTable(rule.amountByLevel, context, [](float value) { return value > 0.f; }, "owner attribute amount must be positive");
				rule.fadeSeconds = entry.contains("fadeSeconds") ? ParseFiniteFloat(entry.at("fadeSeconds"), context) : 0.f;
				if (rule.fadeSeconds < 0.f || (entry.contains("fadeSeconds") && rule.source != ModuleOwnerAttributeSource::MovementSpeed))
					throw std::runtime_error(context + " invalid movement fade");
				variant.ownerAttributeRules.push_back(std::move(rule));
			}
			const Json fades = object.value("movementFadeRules", Json::array());
			if (!fades.is_array()) throw std::runtime_error(context + ".movementFadeRules must be an array");
			for (const Json& entry : fades)
			{
				if (!entry.is_object() || entry.size() != 1 || !entry.contains("fadeSeconds")) throw std::runtime_error(context + " invalid movement fade rule");
				const float seconds = ParseFiniteFloat(entry.at("fadeSeconds"), context);
				if (seconds <= 0.f) throw std::runtime_error(context + " movement fade must be positive");
				variant.movementFadeRules.push_back({ seconds });
			}
			const Json reflections = object.value("shieldReflections", Json::array());
			if (!reflections.is_array()) throw std::runtime_error(context + ".shieldReflections must be an array");
			for (const Json& entry : reflections)
			{
				ModuleRulePackage::ShieldReflection rule;
				rule.fraction = ParseLevelFloat(entry.at("fraction"), context + ".reflection.fraction", minimumLevelCount, rule.fractionByLevel);
				rule.durationSeconds = ParseFiniteFloat(entry.at("durationSeconds"), context + ".reflection.durationSeconds");
				const auto valid = [](float value) { return value > 0.f; };
				if (!valid(rule.fraction) || rule.durationSeconds <= 0.f) throw std::runtime_error(context + " invalid shield reflection");
				ValidateTable(rule.fractionByLevel, context, valid, "invalid shield reflection fraction");
				variant.shieldReflections.push_back(rule);
			}
			const Json statusRules = object.value("statusRules", Json::object());
			if (!statusRules.is_object()) throw std::runtime_error(context + ".statusRules must be an object");
			const std::set<std::string> statusKeys{ "electricOverrides", "cryoFreezes", "thermalCryoDurationBonuses", "freezeSpreads", "durationBonuses" };
			for (const auto& field : statusRules.items())
				if (!statusKeys.count(field.key()) || !field.value().is_array()) throw std::runtime_error(context + " invalid status rule list: " + field.key());
			const auto positive = [](float value) { return value > 0.f; };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			const auto validateStatusFields = [&context](const Json& entry, std::initializer_list<const char*> allowed)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " status rule must be an object");
				for (const auto& field : entry.items())
					if (std::none_of(allowed.begin(), allowed.end(), [&field](const char* name) { return field.key() == name; })) throw std::runtime_error(context + " unknown status rule field: " + field.key());
			};
			for (const Json& entry : statusRules.value("electricOverrides", Json::array()))
			{
				validateStatusFields(entry, { "damageTakenByStack" });
				ModuleElectricStatusRule rule;
				const auto& values = entry.at("damageTakenByStack");
				if (!values.is_array() || values.empty()) throw std::runtime_error(context + " electric stack table must not be empty");
				for (const auto& value : values)
				{
					ModuleLevelTable<float> table;
					const float base = ParseLevelFloat(value, context + ".electric.damageTakenByStack", minimumLevelCount, table);
					if (base < 0.f) throw std::runtime_error(context + " invalid electric bonus");
					ValidateTable(table, context, nonnegative, "invalid electric bonus");
					rule.damageTakenByStack.push_back(base);
					rule.damageTakenByStackByLevel.push_back(std::move(table));
				}
				variant.statusRules.electricOverrides.push_back(std::move(rule));
			}
			for (const Json& entry : statusRules.value("cryoFreezes", Json::array()))
			{
				ModuleCryoFreezeRule rule;
				validateStatusFields(entry, { "threshold", "durationSeconds", "cooldownSeconds", "normalTargetsOnly" });
				rule.threshold = ParsePositiveInt(entry.at("threshold"), context + ".freeze.threshold");
				rule.durationSeconds = ParseLevelFloat(entry.at("durationSeconds"), context, minimumLevelCount, rule.durationSecondsByLevel);
				rule.cooldownSeconds = ParseLevelFloat(entry.at("cooldownSeconds"), context, minimumLevelCount, rule.cooldownSecondsByLevel);
				rule.normalTargetsOnly = entry.value("normalTargetsOnly", true);
				if (!positive(rule.durationSeconds) || !positive(rule.cooldownSeconds)) throw std::runtime_error(context + " invalid freeze timing");
				ValidateTable(rule.durationSecondsByLevel, context, positive, "invalid freeze duration");
				ValidateTable(rule.cooldownSecondsByLevel, context, positive, "invalid freeze cooldown");
				variant.statusRules.cryoFreezes.push_back(std::move(rule));
			}
			for (const char* key : { "thermalCryoDurationBonuses", "durationBonuses" })
			{
				for (const Json& entry : statusRules.value(key, Json::array()))
				{
					validateStatusFields(entry, { "bonusSeconds" });
					ModuleStatusDurationRule rule;
					rule.bonusSeconds = ParseLevelFloat(entry.at("bonusSeconds"), context, minimumLevelCount, rule.bonusSecondsByLevel);
					if (!positive(rule.bonusSeconds)) throw std::runtime_error(context + " invalid status duration bonus");
					ValidateTable(rule.bonusSecondsByLevel, context, positive, "invalid status duration bonus");
					(std::string{ key } == "durationBonuses" ? variant.statusRules.durationBonuses : variant.statusRules.thermalCryoDurationBonuses).push_back(std::move(rule));
				}
			}
			for (const Json& entry : statusRules.value("freezeSpreads", Json::array()))
			{
				validateStatusFields(entry, { "radius", "stacks" });
				ModuleFreezeSpreadRule rule;
				rule.radius = ParseFiniteFloat(entry.at("radius"), context);
				rule.stacks = ParsePositiveInt(entry.at("stacks"), context);
				if (!positive(rule.radius)) throw std::runtime_error(context + " invalid freeze spread radius");
				variant.statusRules.freezeSpreads.push_back(rule);
			}
			const Json generated = object.value("generatedRules", Json::object());
			if (!generated.is_object()) throw std::runtime_error(context + ".generatedRules must be an object");
			const std::set<std::string> generatedKeys{ "ricochets", "guidedSalvos", "delayedExplosions", "trailMines", "killExplosions" };
			for (const auto& field : generated.items())
				if (!generatedKeys.count(field.key()) || !field.value().is_array() || field.value().size() > 1) throw std::runtime_error(context + " generated rule lists accept at most one rule per family");
			const auto fields = [&context](const Json& entry, std::initializer_list<const char*> allowed)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " generated rule must be an object");
				for (const auto& field : entry.items())
					if (std::none_of(allowed.begin(), allowed.end(), [&field](const char* name) { return field.key() == name; })) throw std::runtime_error(context + " unknown generated rule field: " + field.key());
			};
			const auto number = [&context](const Json& entry, const char* key)
			{
				if (!entry.contains(key)) return 0.f;
				const float value = ParseFiniteFloat(entry.at(key), context + ".generatedRules." + key);
				if (value <= 0.f) throw std::runtime_error(context + " generated value must be positive: " + key);
				return value;
			};
			const auto count = [&context](const Json& entry, const char* key)
			{
				const int value = entry.contains(key) ? ParsePositiveInt(entry.at(key), context + ".generatedRules." + key) : 0;
				if (value > 1024) throw std::runtime_error(context + " generated count exceeds technical ceiling: " + key);
				return value;
			};
			const auto fraction = [&context, minimumLevelCount](const Json& entry, const char* key, ModuleLevelTable<float>& table)
			{
				if (!entry.contains(key)) return 0.f;
				const float value = ParseLevelFloat(entry.at(key), context + ".generatedRules." + key, minimumLevelCount, table);
				const auto valid = [](float v) { return v > 0.f && v <= 1.f; };
				if (!valid(value)) throw std::runtime_error(context + " generated damage fraction must be in (0,1]");
				ValidateTable(table, context, valid, "generated fraction must be in (0,1]");
				return value;
			};
			for (const auto& entry : generated.value("ricochets", Json::array()))
			{
				fields(entry, { "damageFraction", "range", "secondaryFraction", "transferStatus" });
				ModuleRicochetRule rule;
				rule.damageFraction = fraction(entry, "damageFraction", rule.damageFractionByLevel);
				rule.range = number(entry, "range");
				rule.secondaryFraction = number(entry, "secondaryFraction");
				rule.transferStatus = entry.value("transferStatus", false);
				if ((rule.damageFraction > 0.f && rule.range <= 0.f) || rule.secondaryFraction > 1.f || (rule.damageFraction <= 0.f && rule.secondaryFraction <= 0.f && !rule.transferStatus)) throw std::runtime_error(context + " incomplete ricochet rule");
				variant.generatedRules.ricochets.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("guidedSalvos", Json::array()))
			{
				fields(entry, { "damageFraction", "projectileCount", "speed", "lifetime", "targetRange", "maxActive" });
				ModuleGuidedSalvoRule rule;
				rule.damageFraction = fraction(entry, "damageFraction", rule.damageFractionByLevel);
				rule.projectileCount = count(entry, "projectileCount"); rule.speed = number(entry, "speed"); rule.lifetime = number(entry, "lifetime"); rule.targetRange = number(entry, "targetRange"); rule.maxActive = count(entry, "maxActive");
				if (rule.projectileCount <= 0 || (rule.damageFraction > 0.f && (rule.speed <= 0.f || rule.lifetime <= 0.f || rule.targetRange <= 0.f || rule.maxActive <= 0))) throw std::runtime_error(context + " incomplete guided salvo rule");
				variant.generatedRules.guidedSalvos.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("delayedExplosions", Json::array()))
			{
				fields(entry, { "damageFraction", "delaySeconds", "radius", "maxActive" });
				ModuleDelayedExplosionRule rule;
				rule.damageFraction = fraction(entry, "damageFraction", rule.damageFractionByLevel);
				rule.delaySeconds = number(entry, "delaySeconds"); rule.radius = number(entry, "radius"); rule.maxActive = count(entry, "maxActive");
				if (rule.damageFraction <= 0.f || rule.delaySeconds <= 0.f || rule.radius <= 0.f || rule.maxActive <= 0) throw std::runtime_error(context + " incomplete delayed explosion rule");
				variant.generatedRules.delayedExplosions.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("trailMines", Json::array()))
			{
				fields(entry, { "damage", "spacing", "windowSeconds", "fuseSeconds", "radius", "maxActive", "additionalLimit", "cryoStacks" });
				ModuleTrailMineRule rule;
				if (entry.contains("damage"))
				{
					rule.damage = ParseLevelFloat(entry.at("damage"), context, minimumLevelCount, rule.damageByLevel);
					if (!positive(rule.damage)) throw std::runtime_error(context + " mine damage must be positive");
					ValidateTable(rule.damageByLevel, context, positive, "mine damage must be positive");
				}
				rule.spacing = number(entry, "spacing"); rule.windowSeconds = number(entry, "windowSeconds"); rule.fuseSeconds = number(entry, "fuseSeconds"); rule.radius = number(entry, "radius");
				if (entry.contains("maxActive")) rule.maxActive = ParsePositiveLevelInt(entry.at("maxActive"), context, minimumLevelCount, rule.maxActiveByLevel);
				rule.additionalLimit = count(entry, "additionalLimit"); rule.cryoStacks = count(entry, "cryoStacks");
				if ((rule.damage > 0.f && (rule.spacing <= 0.f || rule.windowSeconds <= 0.f || rule.fuseSeconds <= 0.f || rule.radius <= 0.f || rule.maxActive <= 0)) || (rule.damage <= 0.f && rule.additionalLimit <= 0 && rule.cryoStacks <= 0)) throw std::runtime_error(context + " incomplete trail mine rule");
				if (rule.maxActive > 1024) throw std::runtime_error(context + " mine limit exceeds technical ceiling");
				ValidateTable(rule.maxActiveByLevel, context, [](int v) { return v <= 1024; }, "mine limit exceeds technical ceiling");
				variant.generatedRules.trailMines.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("killExplosions", Json::array()))
			{
				fields(entry, { "maxHealthFraction", "radius", "maxActive", "damageType", "presentationId", "requiredTargetStatuses" });
				ModuleKillExplosionRule rule;
				rule.maxHealthFraction = fraction(entry, "maxHealthFraction", rule.maxHealthFractionByLevel);
				rule.radius = number(entry, "radius"); rule.maxActive = count(entry, "maxActive");
				rule.damageType = ParseTag(entry.at("damageType"), GameplayTagKind::DamageType, context + " kill explosion damageType");
				rule.presentationId = ParseContentId(entry.at("presentationId"), context + " kill explosion presentationId");
				const auto& statuses = entry.at("requiredTargetStatuses");
				if (!statuses.is_array() || statuses.empty()) throw std::runtime_error(context + " kill explosion requires target statuses");
				for (const auto& tag : statuses) rule.requiredTargetStatuses.push_back(ParseTag(tag, GameplayTagKind::Status, context));
				if (rule.maxHealthFraction <= 0.f || rule.radius <= 0.f || rule.maxActive <= 0) throw std::runtime_error(context + " incomplete kill explosion rule");
				variant.generatedRules.killExplosions.push_back(std::move(rule));
			}
			return variant;
		}

		void ValidateDescription(const ModuleDefinition& definition, const std::string& context)
		{
			for (const auto& variant : definition.sizeVariants) ValidateModuleDescriptionTemplate(definition.description, variant, context);
		}

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
			const Json& variants = object.at("sizeVariants");
			if (!variants.is_array() || variants.empty())
			{
				throw std::runtime_error(context + " must define at least one size variant");
			}
			std::size_t index = 0;
			for (const Json& variant : variants)
			{
				definition.sizeVariants.push_back(
					ParseVariant(variant, context + ".sizeVariants[" + std::to_string(index++) + "]", definition.levelCount, true)
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

	ModuleRulePackage ModuleLoader::ParseRulePackage(const nlohmann::json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus)
	{
		if (!object.is_object()) throw std::runtime_error(context + " must be an object");
		if (object.contains("shape")) throw std::runtime_error(context + " rule package must not define a shape");
		nlohmann::json shapedObject = object;
		nlohmann::json shape = nlohmann::json::array();
		shape.push_back(nlohmann::json::array({ 0, 0 }));
		shapedObject["shape"] = std::move(shape);
		ModuleSizeVariant parsed = ParseVariant(shapedObject, context, minimumLevelCount, defaultAffectedByZoneBonus);
		return static_cast<const ModuleRulePackage&>(parsed);
	}

	bool ModuleLoader::ValidateRulePackageLevelCount(const ModuleRulePackage& rules, int levelCount, const std::string& context, std::string* failureReason)
	{
		if (levelCount <= 0)
		{
			if (failureReason) *failureReason = context + " levelCount must be positive";
			return false;
		}
		const auto hasCoverage = [levelCount, &context, failureReason](const auto& table, const std::string& field)
		{
			if (table.values.empty() || table.values.size() >= static_cast<std::size_t>(levelCount)) return true;
			if (failureReason) *failureReason = context + " " + field + " level table is shorter than levelCount";
			return false;
		};
		for (std::size_t index = 0; index < rules.attributeModifiers.size(); ++index)
			if (!hasCoverage(rules.attributeModifiers[index].magnitudeByLevel, "attributeModifiers[" + std::to_string(index) + "].magnitude")) return false;
		for (std::size_t index = 0; index < rules.abilityModifiers.size(); ++index)
			if (!hasCoverage(rules.abilityModifiers[index].magnitudeByLevel, "abilityModifiers[" + std::to_string(index) + "].magnitude")) return false;
		for (std::size_t index = 0; index < rules.triggerBuffs.size(); ++index)
		{
			const ModuleTriggerBuff& buff = rules.triggerBuffs[index];
			const std::string prefix = "triggerBuffs[" + std::to_string(index) + "].";
			if (!hasCoverage(buff.durationSecondsByLevel, prefix + "durationSeconds") ||
				!hasCoverage(buff.movementSpeedMultiplierByLevel, prefix + "movementSpeedMultiplier")) return false;
			for (std::size_t modifierIndex = 0; modifierIndex < buff.modifiers.size(); ++modifierIndex)
				if (!hasCoverage(buff.modifiers[modifierIndex].magnitudeByLevel,
					prefix + "modifiers[" + std::to_string(modifierIndex) + "].magnitude")) return false;
		}
		for (std::size_t index = 0; index < rules.damageRules.size(); ++index)
		{
			const ModuleDamageRule& rule = rules.damageRules[index];
			const std::string prefix = "damageRules[" + std::to_string(index) + "].";
			if (!hasCoverage(rule.conditionValueByLevel, prefix + "conditionValue") ||
				!hasCoverage(rule.openingShotCountByLevel, prefix + "openingShotCount") ||
				!hasCoverage(rule.damageMultiplierByLevel, prefix + "damageMultiplier") ||
				!hasCoverage(rule.bonusDamageFractionByLevel, prefix + "bonusDamageFraction") ||
				!hasCoverage(rule.activeDurationSecondsByLevel, prefix + "activeDurationSeconds")) return false;
		}
		for (const ModuleEventAction& action : rules.eventActions)
			if (!hasCoverage(action.amountByLevel, "eventActions.amount") || !hasCoverage(action.chancePerLuckByLevel, "eventActions.chancePerLuck")) return false;
		for (const ModuleOwnerAttributeRule& rule : rules.ownerAttributeRules)
			if (!hasCoverage(rule.amountByLevel, "ownerAttributeRules.amount")) return false;
		for (const auto& rule : rules.statusRules.electricOverrides)
			for (const auto& table : rule.damageTakenByStackByLevel)
				if (!hasCoverage(table, "statusRules.electricOverrides.damageTakenByStack")) return false;
		for (const auto& rule : rules.statusRules.cryoFreezes)
			if (!hasCoverage(rule.durationSecondsByLevel, "statusRules.cryoFreezes.durationSeconds") ||
				!hasCoverage(rule.cooldownSecondsByLevel, "statusRules.cryoFreezes.cooldownSeconds")) return false;
		for (const auto& rule : rules.statusRules.durationBonuses)
			if (!hasCoverage(rule.bonusSecondsByLevel, "statusRules.durationBonuses.bonusSeconds")) return false;
		for (const auto& rule : rules.statusRules.thermalCryoDurationBonuses)
			if (!hasCoverage(rule.bonusSecondsByLevel, "statusRules.thermalCryoDurationBonuses.bonusSeconds")) return false;
		for (const auto& rule : rules.generatedRules.ricochets) if (!hasCoverage(rule.damageFractionByLevel, "generatedRules.ricochets.damageFraction")) return false;
		for (const auto& rule : rules.generatedRules.guidedSalvos) if (!hasCoverage(rule.damageFractionByLevel, "generatedRules.guidedSalvos.damageFraction")) return false;
		for (const auto& rule : rules.generatedRules.delayedExplosions) if (!hasCoverage(rule.damageFractionByLevel, "generatedRules.delayedExplosions.damageFraction")) return false;
		for (const auto& rule : rules.generatedRules.trailMines) if (!hasCoverage(rule.damageByLevel, "generatedRules.trailMines.damage") || !hasCoverage(rule.maxActiveByLevel, "generatedRules.trailMines.maxActive")) return false;
		for (const auto& rule : rules.generatedRules.killExplosions) if (!hasCoverage(rule.maxHealthFractionByLevel, "generatedRules.killExplosions.maxHealthFraction")) return false;
		for (std::size_t index = 0; index < rules.zoneEffects.size(); ++index)
			if (!hasCoverage(rules.zoneEffects[index].multiplierByLevel,
				"zoneEffects[" + std::to_string(index) + "].multiplier")) return false;
		return true;
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

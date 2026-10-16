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
			if (value == "PrimaryTargetWithoutStatus") return ModuleDamageCondition::PrimaryTargetWithoutStatus;
			if (value == "TargetThermalAndCryo") return ModuleDamageCondition::TargetThermalAndCryo;
			if (value == "SourceHasStatus") return ModuleDamageCondition::SourceHasStatus;
			if (value == "AdjacentAbilityDirectHit") return ModuleDamageCondition::AdjacentAbilityDirectHit;
			if (value == "PrimaryShotOnly") return ModuleDamageCondition::PrimaryShotOnly;
			if (value == "TargetHealthAtMost") return ModuleDamageCondition::TargetHealthAtMost;
			if (value == "FullMagazineShot") return ModuleDamageCondition::FullMagazineShot;
			if (value == "FirstTargetHit") return ModuleDamageCondition::FirstTargetHit;
			if (value == "Always") return ModuleDamageCondition::Always;
			if (value == "OwnerShieldFull") return ModuleDamageCondition::OwnerShieldFull;
			if (value == "OwnerShieldOverMax") return ModuleDamageCondition::OwnerShieldOverMax;
			if (value == "TargetIsBossClass") return ModuleDamageCondition::TargetIsBossClass;
			if (value == "TargetStatusKindsAtLeast") return ModuleDamageCondition::TargetStatusKindsAtLeast;
			if (value == "TargetMaxHealthRatioAtLeast") return ModuleDamageCondition::TargetMaxHealthRatioAtLeast;
			if (value == "SourceMaxHealthGreaterThanOwner") return ModuleDamageCondition::SourceMaxHealthGreaterThanOwner;
				if (value == "AbilityDirectHit") return ModuleDamageCondition::AbilityDirectHit;
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

		// "zoneRules": [{ "kind": "AmplifyZone" | "OutOfZoneMainBonus", "bonus": number or level table }]
		void ParseZoneRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& package)
		{
			const Json zoneRules = object.value("zoneRules", Json::array());
			if (!zoneRules.is_array()) throw std::runtime_error(context + ".zoneRules must be an array");
			std::size_t index = 0;
			for (const Json& entry : zoneRules)
			{
				const std::string entryContext = context + ".zoneRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
				{
					if (field.key() != "kind" && field.key() != "bonus") throw std::runtime_error(entryContext + " has unknown field: " + field.key());
				}
				const std::string kind = entry.at("kind").get<std::string>();
				ModuleZoneRule rule;
				if (kind == "AmplifyZone") rule.kind = ModuleZoneRuleKind::AmplifyZone;
				else if (kind == "OutOfZoneMainBonus") rule.kind = ModuleZoneRuleKind::OutOfZoneMainBonus;
				else throw std::runtime_error(entryContext + " unknown zone rule kind: " + kind);
				for (const ModuleZoneRule& existing : package.zoneRules)
				{
					if (existing.kind == rule.kind) throw std::runtime_error(entryContext + " duplicates zone rule kind: " + kind);
				}
				const bool amplifies = rule.kind == ModuleZoneRuleKind::AmplifyZone;
				const auto valid = [amplifies](float value)
				{
					return amplifies ? (value >= kReferenceZoneBonus && value <= 1.f) : (value > 0.f && value <= 1.f);
				};
				const char* message = amplifies ? "must be in [0.25, 1] for AmplifyZone" : "must be in (0, 1] for OutOfZoneMainBonus";
				rule.bonus = ParseLevelFloat(entry.at("bonus"), entryContext + ".bonus", minimumLevelCount, rule.bonusByLevel);
				if (!valid(rule.bonus)) throw std::runtime_error(entryContext + ".bonus " + message);
				ValidateTable(rule.bonusByLevel, entryContext + ".bonus", valid, message);
				package.zoneRules.push_back(std::move(rule));
			}
		}

		ModuleShieldRegenMode ParseShieldRegenMode(const std::string& value, const std::string& context)
		{
			if (value == "Normal") return ModuleShieldRegenMode::Normal;
			if (value == "SpeedScaled") return ModuleShieldRegenMode::SpeedScaled;
			if (value == "Continuous") return ModuleShieldRegenMode::Continuous;
			throw std::runtime_error(context + ".regenMode is not a known shield regen mode: " + value);
		}

		// shieldRules entries: regenMode (Normal by default); speedMultiplierAtMax (SpeedScaled only);
		// continuousRateFraction (Continuous only); breakRearmFraction and armorShare (any mode, 0 by default).
		void ParseShieldRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules)
		{
			const Json entries = object.value("shieldRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".shieldRules must be an array");
			const std::set<std::string> allowedKeys{ "regenMode", "speedMultiplierAtMax", "minimumSpeed", "continuousRateFraction", "breakRearmFraction", "armorShare", "frontRearSplit" };
			const auto positive = [](float value) { return value > 0.f; };
			const auto rateFraction = [](float value) { return value > 0.f && value <= 1.f; };
			const auto unitInterval = [](float value) { return value >= 0.f && value <= 1.f; };
			const auto parseField = [&context, minimumLevelCount](const Json& entry, const char* key, ModuleLevelTable<float>& table, bool (*valid)(float), const char* requirement)
			{
				const std::string fieldContext = context + ".shieldRules." + key;
				const float value = ParseLevelFloat(entry.at(key), fieldContext, minimumLevelCount, table);
				if (!valid(value)) throw std::runtime_error(fieldContext + " " + requirement);
				ValidateTable(table, fieldContext, valid, requirement);
				return value;
			};
			for (const Json& entry : entries)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " shieldRules entry must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(context + " unknown shieldRules field: " + field.key());
				ModuleShieldRule rule;
				rule.regenMode = ParseShieldRegenMode(entry.value("regenMode", std::string{ "Normal" }), context);
				if (rule.regenMode == ModuleShieldRegenMode::SpeedScaled)
				{
					if (!entry.contains("speedMultiplierAtMax")) throw std::runtime_error(context + ".shieldRules.speedMultiplierAtMax is required for SpeedScaled");
					rule.speedMultiplierAtMax = parseField(entry, "speedMultiplierAtMax", rule.speedMultiplierAtMaxByLevel, positive, "must be positive");
					if (entry.contains("minimumSpeed"))
					{
						rule.minimumSpeed = ParseFiniteFloat(entry.at("minimumSpeed"), context + ".shieldRules.minimumSpeed");
						if (rule.minimumSpeed < 0.f) throw std::runtime_error(context + ".shieldRules.minimumSpeed must be non-negative");
					}
				}
				else if (entry.contains("speedMultiplierAtMax") || entry.contains("minimumSpeed")) throw std::runtime_error(context + ".shieldRules.speedMultiplierAtMax is valid only for SpeedScaled");
				if (rule.regenMode == ModuleShieldRegenMode::Continuous)
				{
					if (!entry.contains("continuousRateFraction")) throw std::runtime_error(context + ".shieldRules.continuousRateFraction is required for Continuous");
					rule.continuousRateFraction = parseField(entry, "continuousRateFraction", rule.continuousRateFractionByLevel, rateFraction, "must be in (0, 1]");
				}
				else if (entry.contains("continuousRateFraction")) throw std::runtime_error(context + ".shieldRules.continuousRateFraction is valid only for Continuous");
				if (entry.contains("breakRearmFraction")) rule.breakRearmFraction = parseField(entry, "breakRearmFraction", rule.breakRearmFractionByLevel, unitInterval, "must be in [0, 1]");
				if (entry.contains("armorShare")) rule.armorShare = parseField(entry, "armorShare", rule.armorShareByLevel, unitInterval, "must be in [0, 1]");
				if (entry.contains("frontRearSplit")) { if (!entry.at("frontRearSplit").is_boolean()) throw std::runtime_error(context + " frontRearSplit must be boolean"); rule.frontRearSplit = entry.at("frontRearSplit").get<bool>(); }
				if (rule.regenMode == ModuleShieldRegenMode::Normal && rule.breakRearmFraction <= 0.f && rule.armorShare <= 0.f && !rule.frontRearSplit)
					throw std::runtime_error(context + ".shieldRules entry has no effect");
				rules.shieldRules.push_back(rule);
			}
		}

		ControlTargetClass ParseControlTargetClass(const Json& value, const std::string& context)
		{
			const std::string name = value.is_string() ? value.get<std::string>() : std::string{};
			if (name == "Normal") return ControlTargetClass::Normal;
			if (name == "Elite") return ControlTargetClass::Elite;
			if (name == "MiniBoss") return ControlTargetClass::MiniBoss;
			if (name == "Boss") return ControlTargetClass::Boss;
			throw std::runtime_error(context + ".stunImmuneClasses has an unknown class: " + name);
		}

		// ownerBurstRules entries: trigger (ShieldBroken, or KillConfirmed with chargeKills and chargeRadius), radius, damageType
		// (DamageType tag) and exactly one of damageFractionOfMaxHealth or damage (flat), each a scalar or a per-level array with
		// values > 0, are required. cooldownSeconds is required (> 0) for ShieldBroken and optional (>= 0) for charge rules.
		// chargeKills is an integer >= 1 and is rejected on ShieldBroken. pushImpulse, stunSeconds and echoDelaySeconds are >= 0 and
		// 0 turns the part off. echoFraction in [0, 1] is required with echoDelaySeconds and rejected without it. igniteStacks is an
		// integer >= 0 and needs the Thermal damageType. stunImmuneClasses (Normal, Elite, MiniBoss, Boss) defaults to Elite,
		// MiniBoss, Boss. Unknown fields are rejected.
		void ParseOwnerBurstRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules)
		{
			const Json entries = object.value("ownerBurstRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".ownerBurstRules must be an array");
			const std::set<std::string> allowedKeys{ "trigger", "radius", "damageFractionOfMaxHealth", "damage", "energyPowerCoefficient", "accumulatedArmorCoefficient", "requiredEventTags", "damageType", "igniteStacks", "chargeKills", "chargeRadius", "cooldownSeconds", "pushImpulse", "stunSeconds", "stunImmuneClasses", "echoDelaySeconds", "echoFraction" };
			const auto positive = [](float value) { return value > 0.f; };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			const auto unitInterval = [](float value) { return value >= 0.f && value <= 1.f; };
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				const Json& entry = entries[index];
				const std::string entryContext = context + ".ownerBurstRules[" + std::to_string(index) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				const auto requiredField = [&entry, &entryContext](const char* key) -> const Json&
				{
					if (!entry.contains(key)) throw std::runtime_error(entryContext + "." + key + " is required");
					return entry.at(key);
				};
				const auto scalar = [&entryContext](const char* key, const Json& value, bool (*valid)(float), const char* requirement)
				{
					const float number = ParseFiniteFloat(value, entryContext + "." + key);
					if (!valid(number)) throw std::runtime_error(entryContext + "." + key + " " + requirement);
					return number;
				};
				const Json& trigger = requiredField("trigger");
				if (!trigger.is_string()) throw std::runtime_error(entryContext + ".trigger must be ShieldBroken or KillConfirmed");
				const std::string triggerName = trigger.get<std::string>();
				if (triggerName != "ShieldBroken" && triggerName != "KillConfirmed" && triggerName != "AbilityActivated") throw std::runtime_error(entryContext + ".trigger must be ShieldBroken or KillConfirmed");
				ModuleOwnerBurstRule rule;
				rule.trigger = triggerName == "AbilityActivated" ? ModuleTriggerKind::AbilityActivated : triggerName == "KillConfirmed" ? ModuleTriggerKind::KillConfirmed : ModuleTriggerKind::ShieldBroken;
				rule.requiredEventTags = ParseRequiredEventTags(entry.value("requiredEventTags", Json::array()), rule.trigger, entryContext);
				if (entry.contains("accumulatedArmorCoefficient")) rule.accumulatedArmorCoefficient = scalar("accumulatedArmorCoefficient", entry.at("accumulatedArmorCoefficient"), positive, "must be positive");
				rule.radius = scalar("radius", requiredField("radius"), positive, "must be positive");
				const bool hasFraction = entry.contains("damageFractionOfMaxHealth");
				if ((hasFraction ? 1 : 0) + (entry.contains("damage") ? 1 : 0) + (entry.contains("accumulatedArmorCoefficient") ? 1 : 0) != 1) throw std::runtime_error(entryContext + " needs exactly one of damageFractionOfMaxHealth or damage");
				if (hasFraction)
				{
					const std::string fractionContext = entryContext + ".damageFractionOfMaxHealth";
					rule.damageFractionOfMaxHealth = ParseLevelFloat(entry.at("damageFractionOfMaxHealth"), fractionContext, minimumLevelCount, rule.damageFractionOfMaxHealthByLevel);
					if (!positive(rule.damageFractionOfMaxHealth)) throw std::runtime_error(fractionContext + " must be positive");
					ValidateTable(rule.damageFractionOfMaxHealthByLevel, fractionContext, positive, "must be positive");
				}
				else if (entry.contains("damage"))
				{
					const std::string damageContext = entryContext + ".damage";
					rule.damage = ParseLevelFloat(entry.at("damage"), damageContext, minimumLevelCount, rule.damageByLevel);
					if (!positive(rule.damage)) throw std::runtime_error(damageContext + " must be positive");
					ValidateTable(rule.damageByLevel, damageContext, positive, "must be positive");
				}
				if (entry.contains("energyPowerCoefficient"))
				{
					rule.energyPowerCoefficient = ParseFiniteFloat(entry.at("energyPowerCoefficient"), entryContext + ".energyPowerCoefficient");
					if (rule.energyPowerCoefficient < 0.f) throw std::runtime_error(entryContext + ".energyPowerCoefficient must be non-negative");
				}
				const Json& damageType = requiredField("damageType");
				rule.damageType = ParseTag(damageType, GameplayTagKind::DamageType, entryContext + " damageType");
				if (entry.contains("igniteStacks"))
				{
					const Json& stacks = entry.at("igniteStacks");
					if (!stacks.is_number_integer() || stacks.get<int>() < 0) throw std::runtime_error(entryContext + ".igniteStacks must be a non-negative integer");
					rule.igniteStacks = stacks.get<int>();
				}
				if (rule.igniteStacks > 0 && !(damageType.is_string() && damageType.get<std::string>() == "Damage.Type.Thermal"))
					throw std::runtime_error(entryContext + ".igniteStacks requires the Thermal damageType");
				if (entry.contains("chargeKills"))
				{
					const Json& kills = entry.at("chargeKills");
					if (!kills.is_number_integer() || kills.get<int>() < 1) throw std::runtime_error(entryContext + ".chargeKills must be an integer >= 1");
					rule.chargeKills = kills.get<int>();
				}
				if (entry.contains("chargeRadius"))
				{
					if (rule.chargeKills <= 0) throw std::runtime_error(entryContext + ".chargeRadius requires chargeKills");
					rule.chargeRadius = scalar("chargeRadius", entry.at("chargeRadius"), positive, "must be positive");
				}
				if (rule.trigger == ModuleTriggerKind::KillConfirmed && rule.chargeKills <= 0) throw std::runtime_error(entryContext + ".trigger KillConfirmed requires chargeKills");
				if (rule.trigger == ModuleTriggerKind::ShieldBroken && rule.chargeKills > 0) throw std::runtime_error(entryContext + ".chargeKills requires trigger KillConfirmed");
				if (rule.chargeKills > 0 && rule.chargeRadius <= 0.f) throw std::runtime_error(entryContext + ".chargeRadius is required with chargeKills");
				if (rule.chargeKills > 0)
				{
					if (entry.contains("cooldownSeconds")) rule.cooldownSeconds = scalar("cooldownSeconds", entry.at("cooldownSeconds"), nonnegative, "must not be negative");
				}
				else rule.cooldownSeconds = scalar("cooldownSeconds", requiredField("cooldownSeconds"), positive, "must be positive");
				if (entry.contains("pushImpulse")) rule.pushImpulse = scalar("pushImpulse", entry.at("pushImpulse"), nonnegative, "must not be negative");
				if (entry.contains("stunSeconds")) rule.stunSeconds = scalar("stunSeconds", entry.at("stunSeconds"), nonnegative, "must not be negative");
				if (entry.contains("echoDelaySeconds")) rule.echoDelaySeconds = scalar("echoDelaySeconds", entry.at("echoDelaySeconds"), nonnegative, "must not be negative");
				if (entry.contains("echoFraction"))
				{
					if (rule.echoDelaySeconds <= 0.f) throw std::runtime_error(entryContext + ".echoFraction requires echoDelaySeconds");
					rule.echoFraction = scalar("echoFraction", entry.at("echoFraction"), unitInterval, "must be in [0, 1]");
				}
				else if (rule.echoDelaySeconds > 0.f) throw std::runtime_error(entryContext + ".echoFraction is required with echoDelaySeconds");
				if (entry.contains("stunImmuneClasses"))
				{
					const Json& classes = entry.at("stunImmuneClasses");
					if (!classes.is_array()) throw std::runtime_error(entryContext + ".stunImmuneClasses must be an array");
					rule.stunImmuneClasses.clear();
					for (const Json& value : classes) rule.stunImmuneClasses.push_back(ParseControlTargetClass(value, entryContext));
				}
				rules.ownerBurstRules.push_back(std::move(rule));
			}
		}

		// ownerBurstModifiers entries patch the placement's main ownerBurst rules and never add one (side features use only this).
		// chargeRadius (> 0), stunSeconds (>= 0), echoDelaySeconds (> 0) with echoFraction in [0, 1] (required with it) and
		// echoPushFraction in [0, 1] (default 1), pullImpulse (> 0) with pullLeadSeconds (> 0) and optional pullImmuneClasses
		// (default Boss). Empty entries, unknown fields and echo or pull fields without their trigger field are rejected.
		void ParseOwnerBurstModifiers(const Json& object, const std::string& context, ModuleRulePackage& rules)
		{
			const Json entries = object.value("ownerBurstModifiers", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".ownerBurstModifiers must be an array");
			const std::set<std::string> allowedKeys{ "chargeRadius", "stunSeconds", "echoDelaySeconds", "echoFraction", "echoPushFraction", "pullImpulse", "pullLeadSeconds", "pullImmuneClasses" };
			const auto positive = [](float value) { return value > 0.f; };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			const auto unitInterval = [](float value) { return value >= 0.f && value <= 1.f; };
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				const Json& entry = entries[index];
				const std::string entryContext = context + ".ownerBurstModifiers[" + std::to_string(index) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				if (entry.empty()) throw std::runtime_error(entryContext + " must set at least one field");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				const auto scalar = [&entryContext](const char* key, const Json& value, bool (*valid)(float), const char* requirement)
				{
					const float number = ParseFiniteFloat(value, entryContext + "." + key);
					if (!valid(number)) throw std::runtime_error(entryContext + "." + key + " " + requirement);
					return number;
				};
				ModuleOwnerBurstModifier modifier;
				if (entry.contains("chargeRadius")) modifier.chargeRadius = scalar("chargeRadius", entry.at("chargeRadius"), positive, "must be positive");
				if (entry.contains("stunSeconds")) modifier.stunSeconds = scalar("stunSeconds", entry.at("stunSeconds"), nonnegative, "must not be negative");
				if (entry.contains("echoDelaySeconds"))
				{
					modifier.echoDelaySeconds = scalar("echoDelaySeconds", entry.at("echoDelaySeconds"), positive, "must be positive");
					if (!entry.contains("echoFraction")) throw std::runtime_error(entryContext + ".echoFraction is required with echoDelaySeconds");
					modifier.echoFraction = scalar("echoFraction", entry.at("echoFraction"), unitInterval, "must be in [0, 1]");
					if (entry.contains("echoPushFraction")) modifier.echoPushFraction = scalar("echoPushFraction", entry.at("echoPushFraction"), unitInterval, "must be in [0, 1]");
				}
				else if (entry.contains("echoFraction") || entry.contains("echoPushFraction")) throw std::runtime_error(entryContext + " echo fields require echoDelaySeconds");
				if (entry.contains("pullImpulse"))
				{
					modifier.pullImpulse = scalar("pullImpulse", entry.at("pullImpulse"), positive, "must be positive");
					if (!entry.contains("pullLeadSeconds")) throw std::runtime_error(entryContext + ".pullLeadSeconds is required with pullImpulse");
					modifier.pullLeadSeconds = scalar("pullLeadSeconds", entry.at("pullLeadSeconds"), positive, "must be positive");
					if (entry.contains("pullImmuneClasses"))
					{
						const Json& classes = entry.at("pullImmuneClasses");
						if (!classes.is_array()) throw std::runtime_error(entryContext + ".pullImmuneClasses must be an array");
						modifier.pullImmuneClasses.clear();
						for (const Json& value : classes) modifier.pullImmuneClasses.push_back(ParseControlTargetClass(value, entryContext));
					}
				}
				else if (entry.contains("pullLeadSeconds") || entry.contains("pullImmuneClasses")) throw std::runtime_error(entryContext + " pull fields require pullImpulse");
				rules.ownerBurstModifiers.push_back(std::move(modifier));
			}
		}

		// movementRules entries: thrustMultiplier (a scalar or a per-level array, each in (0, 1]) and
		// equalizeDirections (bool, false by default). Unknown fields and entries without an effect are rejected.
		void ParseMovementRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules)
		{
			const Json entries = object.value("movementRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".movementRules must be an array");
			const std::set<std::string> allowedKeys{ "thrustMultiplier", "equalizeDirections" };
			const auto valid = [](float value) { return value > 0.f && value <= 1.f; };
			for (const Json& entry : entries)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " movementRules entry must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(context + " unknown movementRules field: " + field.key());
				if (!entry.contains("thrustMultiplier")) throw std::runtime_error(context + ".movementRules.thrustMultiplier is required");
				const std::string fieldContext = context + ".movementRules.thrustMultiplier";
				ModuleMovementRule rule;
				rule.thrustMultiplier = ParseLevelFloat(entry.at("thrustMultiplier"), fieldContext, minimumLevelCount, rule.thrustMultiplierByLevel);
				if (!valid(rule.thrustMultiplier)) throw std::runtime_error(fieldContext + " must be in (0, 1]");
				ValidateTable(rule.thrustMultiplierByLevel, fieldContext, valid, "must be in (0, 1]");
				if (entry.contains("equalizeDirections"))
				{
					if (!entry.at("equalizeDirections").is_boolean()) throw std::runtime_error(context + ".movementRules.equalizeDirections must be a boolean");
					rule.equalizeDirections = entry.at("equalizeDirections").get<bool>();
				}
				bool hasEffect = rule.equalizeDirections || rule.thrustMultiplier < 1.f;
				for (float value : rule.thrustMultiplierByLevel.values) hasEffect = hasEffect || value < 1.f;
				if (!hasEffect) throw std::runtime_error(context + ".movementRules entry has no effect");
				rules.movementRules.push_back(rule);
			}
		}

		// sustainedSpeedRules entries: speedFraction in (0, 1] and holdSeconds >= 0 are required. primarySpeedMultiplier
		// and primaryDamageMultiplier (the latter a scalar or per-level array) are >= 1, primaryPierceBonus and
		// lingerSeconds are >= 0 and optional. An entry with no Primary effect is rejected, as is any unknown field.
		void ParseSustainedSpeedRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json entries = object.value("sustainedSpeedRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".sustainedSpeedRules must be an array");
			const std::set<std::string> allowedKeys{ "speedFraction", "holdSeconds", "primarySpeedMultiplier", "primaryPierceBonus", "primaryDamageMultiplier", "lingerSeconds" };
			const auto atLeastOne = [](float value) { return value >= 1.f; };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			for (const Json& entry : entries)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " sustainedSpeedRules entry must be an object");
				if (sideFeature)
				{
					// Side modifiers patch only this placement's sonic state.
					const std::set<std::string> sideKeys{ "lingerSeconds", "turnCapabilityMultiplier" };
					for (const auto& field : entry.items())
						if (!sideKeys.count(field.key())) throw std::runtime_error(context + ".sustainedSpeedRules side entry allows only lingerSeconds and turnCapabilityMultiplier");
					ModuleSustainedSpeedRule modifier;
					if (entry.contains("lingerSeconds"))
					{
						modifier.lingerSeconds = ParseFiniteFloat(entry.at("lingerSeconds"), context + ".sustainedSpeedRules.lingerSeconds");
						if (modifier.lingerSeconds <= 0.f) throw std::runtime_error(context + ".sustainedSpeedRules.lingerSeconds must be positive");
					}
					if (entry.contains("turnCapabilityMultiplier"))
					{
						modifier.turnCapabilityMultiplier = ParseFiniteFloat(entry.at("turnCapabilityMultiplier"), context + ".sustainedSpeedRules.turnCapabilityMultiplier");
						if (modifier.turnCapabilityMultiplier <= 1.f) throw std::runtime_error(context + ".sustainedSpeedRules.turnCapabilityMultiplier must exceed 1");
					}
					if (modifier.lingerSeconds <= 0.f && modifier.turnCapabilityMultiplier <= 1.f) throw std::runtime_error(context + ".sustainedSpeedRules side entry has no effect");
					rules.sustainedSpeedRules.push_back(modifier);
					continue;
				}
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(context + " unknown sustainedSpeedRules field: " + field.key());
				if (!entry.contains("speedFraction")) throw std::runtime_error(context + ".sustainedSpeedRules.speedFraction is required");
				if (!entry.contains("holdSeconds")) throw std::runtime_error(context + ".sustainedSpeedRules.holdSeconds is required");
				ModuleSustainedSpeedRule rule;
				rule.speedFraction = ParseFiniteFloat(entry.at("speedFraction"), context + ".sustainedSpeedRules.speedFraction");
				if (rule.speedFraction <= 0.f || rule.speedFraction > 1.f) throw std::runtime_error(context + ".sustainedSpeedRules.speedFraction must be in (0, 1]");
				rule.holdSeconds = ParseFiniteFloat(entry.at("holdSeconds"), context + ".sustainedSpeedRules.holdSeconds");
				if (rule.holdSeconds < 0.f) throw std::runtime_error(context + ".sustainedSpeedRules.holdSeconds must be non-negative");
				if (entry.contains("primarySpeedMultiplier"))
				{
					rule.primarySpeedMultiplier = ParseFiniteFloat(entry.at("primarySpeedMultiplier"), context + ".sustainedSpeedRules.primarySpeedMultiplier");
					if (!atLeastOne(rule.primarySpeedMultiplier)) throw std::runtime_error(context + ".sustainedSpeedRules.primarySpeedMultiplier must be at least 1");
				}
				if (entry.contains("primaryPierceBonus"))
				{
					rule.primaryPierceBonus = ParseFiniteFloat(entry.at("primaryPierceBonus"), context + ".sustainedSpeedRules.primaryPierceBonus");
					if (!nonnegative(rule.primaryPierceBonus)) throw std::runtime_error(context + ".sustainedSpeedRules.primaryPierceBonus must be non-negative");
				}
				if (entry.contains("primaryDamageMultiplier"))
				{
					const std::string fieldContext = context + ".sustainedSpeedRules.primaryDamageMultiplier";
					rule.primaryDamageMultiplier = ParseLevelFloat(entry.at("primaryDamageMultiplier"), fieldContext, minimumLevelCount, rule.primaryDamageMultiplierByLevel);
					if (!atLeastOne(rule.primaryDamageMultiplier)) throw std::runtime_error(fieldContext + " must be at least 1");
					ValidateTable(rule.primaryDamageMultiplierByLevel, fieldContext, atLeastOne, "must be at least 1");
				}
				if (entry.contains("lingerSeconds"))
				{
					rule.lingerSeconds = ParseFiniteFloat(entry.at("lingerSeconds"), context + ".sustainedSpeedRules.lingerSeconds");
					if (rule.lingerSeconds < 0.f) throw std::runtime_error(context + ".sustainedSpeedRules.lingerSeconds must be non-negative");
				}
				bool hasEffect = rule.primarySpeedMultiplier > 1.f || rule.primaryPierceBonus > 0.f || rule.primaryDamageMultiplier > 1.f;
				for (float value : rule.primaryDamageMultiplierByLevel.values) hasEffect = hasEffect || value > 1.f;
				if (!hasEffect) throw std::runtime_error(context + ".sustainedSpeedRules entry has no Primary effect");
				rules.sustainedSpeedRules.push_back(rule);
			}
		}

		// Thermal and Kinetic status stacks cap at 4 (damage_status_balance.json maxStacks); status thresholds may not exceed it.
		constexpr int kStatusRuleStackCap = 4;

		// Side-feature thermal modifier (Area Blast, Smoldering Remains). It has no damage of its own, so it carries only area and smolder
		// fields; the resolver merges them into the placement's burst rule. Smolder needs both smolderSeconds and smolderRadius.
		void ParseThermalBurstModifier(const Json& entry, const std::string& entryContext, ModuleRulePackage& rules)
		{
			ModuleThermalBurstRule modifier;
			modifier.areaRadius = entry.contains("areaRadius") ? ParseFiniteFloat(entry.at("areaRadius"), entryContext + ".areaRadius") : 0.f;
			modifier.areaDamageFraction = entry.contains("areaDamageFraction") ? ParseFiniteFloat(entry.at("areaDamageFraction"), entryContext + ".areaDamageFraction") : 0.f;
			modifier.smolderSeconds = entry.contains("smolderSeconds") ? ParseFiniteFloat(entry.at("smolderSeconds"), entryContext + ".smolderSeconds") : 0.f;
			modifier.smolderRadius = entry.contains("smolderRadius") ? ParseFiniteFloat(entry.at("smolderRadius"), entryContext + ".smolderRadius") : 0.f;
			if (modifier.areaRadius < 0.f) throw std::runtime_error(entryContext + ".areaRadius must be non-negative");
			if (modifier.areaDamageFraction < 0.f || modifier.areaDamageFraction > 1.f) throw std::runtime_error(entryContext + ".areaDamageFraction must be in [0, 1]");
			if ((modifier.areaRadius > 0.f) != (modifier.areaDamageFraction > 0.f)) throw std::runtime_error(entryContext + " area needs both areaRadius and areaDamageFraction");
			if (modifier.smolderSeconds < 0.f || modifier.smolderRadius < 0.f) throw std::runtime_error(entryContext + " smolder fields must be non-negative");
			if ((modifier.smolderSeconds > 0.f) != (modifier.smolderRadius > 0.f)) throw std::runtime_error(entryContext + " smolder needs both smolderSeconds and smolderRadius");
			if (entry.contains("smolderStacksPerSecond"))
			{
				if (modifier.smolderSeconds <= 0.f) throw std::runtime_error(entryContext + ".smolderStacksPerSecond needs smolderSeconds");
				modifier.smolderStacksPerSecond = ParsePositiveInt(entry.at("smolderStacksPerSecond"), entryContext + ".smolderStacksPerSecond");
				if (modifier.smolderStacksPerSecond > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".smolderStacksPerSecond must be 1 to 4");
			}
			else if (modifier.smolderSeconds > 0.f) modifier.smolderStacksPerSecond = 1;
			if (modifier.areaRadius <= 0.f && modifier.smolderSeconds <= 0.f) throw std::runtime_error(entryContext + " side modifier needs an area or a smolder field");
			rules.statusRules.thermalBursts.push_back(modifier);
		}

		// statusRules.thermalBursts entries: stackThreshold (required, 1..4), damageFraction (required, positive scalar or per-level),
		// optional areaRadius (non-negative) and areaDamageFraction ([0,1]); the area needs both or neither. Unknown fields are rejected.
		void ParseThermalBurstRules(const Json& statusRules, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json entries = statusRules.value("thermalBursts", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".statusRules.thermalBursts must be an array");
			const std::set<std::string> allowedKeys = sideFeature ? std::set<std::string>{ "areaRadius", "areaDamageFraction", "smolderSeconds", "smolderRadius", "smolderStacksPerSecond" }
								: std::set<std::string>{ "stackThreshold", "damageFraction", "areaRadius", "areaDamageFraction" };
			const auto positive = [](float value) { return value > 0.f; };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".statusRules.thermalBursts[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (sideFeature)
					{
						ParseThermalBurstModifier(entry, entryContext, rules);
						continue;
					}
				if (!entry.contains("stackThreshold") || !entry.contains("damageFraction")) throw std::runtime_error(entryContext + " requires stackThreshold and damageFraction");
				ModuleThermalBurstRule rule;
				rule.stackThreshold = ParsePositiveInt(entry.at("stackThreshold"), entryContext + ".stackThreshold");
				if (rule.stackThreshold > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".stackThreshold must be 1 to 4");
				rule.damageFraction = ParseLevelFloat(entry.at("damageFraction"), entryContext + ".damageFraction", minimumLevelCount, rule.damageFractionByLevel);
				if (!positive(rule.damageFraction)) throw std::runtime_error(entryContext + ".damageFraction must be positive");
				ValidateTable(rule.damageFractionByLevel, entryContext + ".damageFraction", positive, "must be positive");
				rule.areaRadius = entry.contains("areaRadius") ? ParseFiniteFloat(entry.at("areaRadius"), entryContext + ".areaRadius") : 0.f;
				rule.areaDamageFraction = entry.contains("areaDamageFraction") ? ParseFiniteFloat(entry.at("areaDamageFraction"), entryContext + ".areaDamageFraction") : 0.f;
				if (rule.areaRadius < 0.f) throw std::runtime_error(entryContext + ".areaRadius must be non-negative");
				if (rule.areaDamageFraction < 0.f || rule.areaDamageFraction > 1.f) throw std::runtime_error(entryContext + ".areaDamageFraction must be in [0, 1]");
				if ((rule.areaRadius > 0.f) != (rule.areaDamageFraction > 0.f)) throw std::runtime_error(entryContext + " area needs both areaRadius and areaDamageFraction");
				rules.statusRules.thermalBursts.push_back(std::move(rule));
			}
		}

		// Side-feature kinetic modifier (Hair Trigger, Armor Rend). It has no multiplier of its own; normalTargetThreshold and Armor Rend
		// merge into the placement's consume rule.
		void ParseKineticConsumeModifier(const Json& entry, const std::string& entryContext, ModuleRulePackage& rules)
		{
			ModuleKineticConsumeRule modifier;
			if (entry.contains("normalTargetThreshold"))
			{
				modifier.normalTargetThreshold = ParsePositiveInt(entry.at("normalTargetThreshold"), entryContext + ".normalTargetThreshold");
				if (modifier.normalTargetThreshold > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".normalTargetThreshold must be 1 to 4");
			}
			modifier.rendFraction = entry.contains("rendFraction") ? ParseFiniteFloat(entry.at("rendFraction"), entryContext + ".rendFraction") : 0.f;
			modifier.rendSeconds = entry.contains("rendSeconds") ? ParseFiniteFloat(entry.at("rendSeconds"), entryContext + ".rendSeconds") : 0.f;
			if (modifier.rendFraction < 0.f || modifier.rendFraction > 1.f) throw std::runtime_error(entryContext + ".rendFraction must be in [0, 1]");
			if (modifier.rendSeconds < 0.f) throw std::runtime_error(entryContext + ".rendSeconds must be non-negative");
			if ((modifier.rendFraction > 0.f) != (modifier.rendSeconds > 0.f)) throw std::runtime_error(entryContext + " Armor Rend needs both rendFraction and rendSeconds");
			if (modifier.normalTargetThreshold <= 0 && modifier.rendFraction <= 0.f) throw std::runtime_error(entryContext + " side modifier needs normalTargetThreshold or Armor Rend");
			rules.statusRules.kineticConsumes.push_back(modifier);
		}

		// statusRules.kineticConsumes entries: stackThreshold (required, 1..4), optional normalTargetThreshold (1..4), damageMultiplier
		// (required, positive scalar or per-level), optional rendFraction ([0,1]) and rendSeconds (non-negative); Armor Rend needs both or neither.
		void ParseKineticConsumeRules(const Json& statusRules, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json entries = statusRules.value("kineticConsumes", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".statusRules.kineticConsumes must be an array");
			const std::set<std::string> allowedKeys = sideFeature ? std::set<std::string>{ "normalTargetThreshold", "rendFraction", "rendSeconds" }
								: std::set<std::string>{ "stackThreshold", "normalTargetThreshold", "damageMultiplier", "rendFraction", "rendSeconds" };
			const auto positive = [](float value) { return value > 0.f; };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".statusRules.kineticConsumes[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (sideFeature)
					{
						ParseKineticConsumeModifier(entry, entryContext, rules);
						continue;
					}
				if (!entry.contains("stackThreshold") || !entry.contains("damageMultiplier")) throw std::runtime_error(entryContext + " requires stackThreshold and damageMultiplier");
				ModuleKineticConsumeRule rule;
				rule.stackThreshold = ParsePositiveInt(entry.at("stackThreshold"), entryContext + ".stackThreshold");
				if (rule.stackThreshold > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".stackThreshold must be 1 to 4");
				if (entry.contains("normalTargetThreshold"))
				{
					rule.normalTargetThreshold = ParsePositiveInt(entry.at("normalTargetThreshold"), entryContext + ".normalTargetThreshold");
					if (rule.normalTargetThreshold > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".normalTargetThreshold must be 1 to 4");
				}
				rule.damageMultiplier = ParseLevelFloat(entry.at("damageMultiplier"), entryContext + ".damageMultiplier", minimumLevelCount, rule.damageMultiplierByLevel);
				if (!positive(rule.damageMultiplier)) throw std::runtime_error(entryContext + ".damageMultiplier must be positive");
				ValidateTable(rule.damageMultiplierByLevel, entryContext + ".damageMultiplier", positive, "must be positive");
				rule.rendFraction = entry.contains("rendFraction") ? ParseFiniteFloat(entry.at("rendFraction"), entryContext + ".rendFraction") : 0.f;
				rule.rendSeconds = entry.contains("rendSeconds") ? ParseFiniteFloat(entry.at("rendSeconds"), entryContext + ".rendSeconds") : 0.f;
				if (rule.rendFraction < 0.f || rule.rendFraction > 1.f) throw std::runtime_error(entryContext + ".rendFraction must be in [0, 1]");
				if (rule.rendSeconds < 0.f) throw std::runtime_error(entryContext + ".rendSeconds must be non-negative");
				if ((rule.rendFraction > 0.f) != (rule.rendSeconds > 0.f)) throw std::runtime_error(entryContext + " Armor Rend needs both rendFraction and rendSeconds");
				rules.statusRules.kineticConsumes.push_back(std::move(rule));
			}
		}

		// runCounterRules entries: source "Distance" (unitsPerStack positive, damagePerStack and moveSpeedPerStack non-negative
		// scalars or per-level arrays, at least one positive) or "Kill" (damagePerStack per-level, positive; killWeights with normal,
		// elite, miniboss and boss; optional adjacentAbilityLimit 1-4). Unknown fields are rejected.
		void ParseRunCounterRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules)
		{
			const Json entries = object.value("runCounterRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".runCounterRules must be an array");
			const std::set<std::string> allowedKeys{ "source", "unitsPerStack", "damagePerStack", "moveSpeedPerStack", "killWeights", "adjacentAbilityLimit", "requiredFamily", "armorPerStack", "armorCap" };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			const auto positiveAnywhere = [](float scalar, const ModuleLevelTable<float>& table)
			{
				return scalar > 0.f || std::any_of(table.values.begin(), table.values.end(), [](float value) { return value > 0.f; });
			};
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".runCounterRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (!entry.contains("source")) throw std::runtime_error(entryContext + ".source is required");
				const std::string source = entry.at("source").get<std::string>();
				if (source != "Distance" && source != "Kill" && source != "AbilityUses") throw std::runtime_error(entryContext + ".source is not a known run counter source: " + source);
				const auto parsePerStack = [&entry, &entryContext, minimumLevelCount, nonnegative](const char* key, ModuleLevelTable<float>& table)
				{
					if (!entry.contains(key)) return 0.f;
					const std::string fieldContext = entryContext + "." + key;
					const float value = ParseLevelFloat(entry.at(key), fieldContext, minimumLevelCount, table);
					if (!nonnegative(value)) throw std::runtime_error(fieldContext + " must be non-negative");
					ValidateTable(table, fieldContext, nonnegative, "must be non-negative");
					return value;
				};
				if (source == "AbilityUses")
				{
					ModuleRunCounterRule rule; rule.source = ModuleRunCounterSource::AbilityUses;
					rule.requiredFamily = ParseTag(entry.at("requiredFamily"), GameplayTagKind::AbilityCategory, entryContext);
					rule.unitsPerStack = ParseFiniteFloat(entry.at("unitsPerStack"), entryContext);
					rule.armorPerStack = ParseFiniteFloat(entry.at("armorPerStack"), entryContext);
					rule.armorCap = ParseLevelFloat(entry.at("armorCap"), entryContext, minimumLevelCount, rule.armorCapByLevel);
					if (rule.requiredFamily == GameplayTags::Ability::Primary || rule.unitsPerStack <= 0.f || rule.armorPerStack <= 0.f || rule.armorCap <= 0.f) throw std::runtime_error(entryContext + " invalid ability-use Armor counter");
					ValidateTable(rule.armorCapByLevel, entryContext, [](float value) { return value > 0.f; }, "invalid Armor cap");
					if (entry.contains("damagePerStack") || entry.contains("moveSpeedPerStack") || entry.contains("killWeights")) throw std::runtime_error(entryContext + " mixed ability-use counter");
					rules.runCounterRules.push_back(rule); continue;
				}
				if (entry.contains("armorPerStack") || entry.contains("armorCap") || entry.contains("requiredFamily")) throw std::runtime_error(entryContext + " Armor fields require AbilityUses");
				if (source == "Kill")
				{
					if (entry.contains("unitsPerStack") || entry.contains("moveSpeedPerStack")) throw std::runtime_error(entryContext + " Kill takes only damagePerStack and killWeights");
					ModuleRunCounterRule kill;
					kill.source = ModuleRunCounterSource::Kill;
					kill.damagePerStack = parsePerStack("damagePerStack", kill.damagePerStackByLevel);
					if (!positiveAnywhere(kill.damagePerStack, kill.damagePerStackByLevel)) throw std::runtime_error(entryContext + ".damagePerStack must be positive");
					if (!entry.contains("killWeights") || !entry.at("killWeights").is_object()) throw std::runtime_error(entryContext + ".killWeights must be an object");
					const Json weights = entry.at("killWeights");
					for (const auto& field : weights.items())
						if (field.key() != "normal" && field.key() != "elite" && field.key() != "miniboss" && field.key() != "boss") throw std::runtime_error(entryContext + ".killWeights unknown field: " + field.key());
					for (const char* key : { "normal", "elite", "miniboss", "boss" })
						if (!weights.contains(key)) throw std::runtime_error(entryContext + ".killWeights." + key + " is required");
					const auto weight = [&weights, &entryContext, nonnegative](const char* key)
					{
						const float value = ParseFiniteFloat(weights.at(key), entryContext + ".killWeights." + key);
						if (!nonnegative(value)) throw std::runtime_error(entryContext + ".killWeights." + key + " must be non-negative");
						return value;
					};
					kill.killWeights.normal = weight("normal");
					kill.killWeights.elite = weight("elite");
					kill.killWeights.miniBoss = weight("miniboss");
					kill.killWeights.boss = weight("boss");
					if (!(kill.killWeights.normal > 0.f)) throw std::runtime_error(entryContext + ".killWeights.normal must be positive");
					if (entry.contains("adjacentAbilityLimit"))
					{
						if (!entry.at("adjacentAbilityLimit").is_number_integer()) throw std::runtime_error(entryContext + ".adjacentAbilityLimit must be an integer");
						kill.adjacentAbilityLimit = entry.at("adjacentAbilityLimit").get<int>();
						if (kill.adjacentAbilityLimit < 1 || kill.adjacentAbilityLimit > 4) throw std::runtime_error(entryContext + ".adjacentAbilityLimit must be in [1, 4]");
					}
					// A placement keeps one kill counter, so a package may carry at most one Kill rule.
					if (std::any_of(rules.runCounterRules.begin(), rules.runCounterRules.end(), [](const ModuleRunCounterRule& existing) { return existing.source == ModuleRunCounterSource::Kill; }))
						throw std::runtime_error(entryContext + " a package allows one Kill run counter rule");
					rules.runCounterRules.push_back(std::move(kill));
					continue;
				}
				if (entry.contains("killWeights") || entry.contains("adjacentAbilityLimit")) throw std::runtime_error(entryContext + " killWeights and adjacentAbilityLimit are for Kill only");
				if (!entry.contains("unitsPerStack")) throw std::runtime_error(entryContext + ".unitsPerStack is required");
				ModuleRunCounterRule rule;
				rule.source = ModuleRunCounterSource::Distance;
				rule.unitsPerStack = ParseFiniteFloat(entry.at("unitsPerStack"), entryContext + ".unitsPerStack");
				if (rule.unitsPerStack <= 0.f) throw std::runtime_error(entryContext + ".unitsPerStack must be positive");
				rule.damagePerStack = parsePerStack("damagePerStack", rule.damagePerStackByLevel);
				rule.moveSpeedPerStack = parsePerStack("moveSpeedPerStack", rule.moveSpeedPerStackByLevel);
				if (!positiveAnywhere(rule.damagePerStack, rule.damagePerStackByLevel) && !positiveAnywhere(rule.moveSpeedPerStack, rule.moveSpeedPerStackByLevel))
					throw std::runtime_error(entryContext + " has no effect");
				rules.runCounterRules.push_back(std::move(rule));
			}
		}

		// killStackCooldownRules entries (Bounty Mark): stacksPerStep (positive), reductionPerStep (non-empty fractions in (0,1)),
		// optional tailReduction (fraction in [0,1)), minimumSeconds (positive) and optional adjacentAbilityLimit 1-4. Unknown fields are rejected.
		void ParseKillStackCooldownRules(const Json& object, const std::string& context, ModuleRulePackage& rules)
		{
			const Json entries = object.value("killStackCooldownRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".killStackCooldownRules must be an array");
			const std::set<std::string> allowedKeys{ "stacksPerStep", "reductionPerStep", "tailReduction", "minimumSeconds", "adjacentAbilityLimit" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".killStackCooldownRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				ModuleKillStackCooldownRule rule;
				if (!entry.contains("stacksPerStep") || !entry.contains("reductionPerStep") || !entry.contains("minimumSeconds"))
					throw std::runtime_error(entryContext + " needs stacksPerStep, reductionPerStep and minimumSeconds");
				rule.stacksPerStep = ParseFiniteFloat(entry.at("stacksPerStep"), entryContext + ".stacksPerStep");
				if (!(rule.stacksPerStep > 0.f)) throw std::runtime_error(entryContext + ".stacksPerStep must be positive");
				const Json steps = entry.at("reductionPerStep");
				if (!steps.is_array() || steps.empty()) throw std::runtime_error(entryContext + ".reductionPerStep must be a non-empty array");
				for (const Json& value : steps)
				{
					const float reduction = ParseFiniteFloat(value, entryContext + ".reductionPerStep");
					if (!(reduction > 0.f && reduction < 1.f)) throw std::runtime_error(entryContext + ".reductionPerStep entries must be in (0, 1)");
					rule.reductionPerStep.push_back(reduction);
				}
				if (entry.contains("tailReduction"))
				{
					rule.tailReduction = ParseFiniteFloat(entry.at("tailReduction"), entryContext + ".tailReduction");
					if (!(rule.tailReduction >= 0.f && rule.tailReduction < 1.f)) throw std::runtime_error(entryContext + ".tailReduction must be in [0, 1)");
				}
				rule.minimumSeconds = ParseFiniteFloat(entry.at("minimumSeconds"), entryContext + ".minimumSeconds");
				if (!(rule.minimumSeconds > 0.f)) throw std::runtime_error(entryContext + ".minimumSeconds must be positive");
				if (entry.contains("adjacentAbilityLimit"))
				{
					if (!entry.at("adjacentAbilityLimit").is_number_integer()) throw std::runtime_error(entryContext + ".adjacentAbilityLimit must be an integer");
					rule.adjacentAbilityLimit = entry.at("adjacentAbilityLimit").get<int>();
					if (rule.adjacentAbilityLimit < 1 || rule.adjacentAbilityLimit > 4) throw std::runtime_error(entryContext + ".adjacentAbilityLimit must be in [1, 4]");
				}
				rules.killStackCooldownRules.push_back(std::move(rule));
			}
		}

		// killStackDamageRules entries (Trophy Wall): family (an AbilityCategory tag) and fraction in (0,1].
		void ParseKillStackDamageRules(const Json& object, const std::string& context, ModuleRulePackage& rules)
		{
			const Json entries = object.value("killStackDamageRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".killStackDamageRules must be an array");
			const std::set<std::string> allowedKeys{ "family", "fraction" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".killStackDamageRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (!entry.contains("family") || !entry.contains("fraction")) throw std::runtime_error(entryContext + " needs family and fraction");
				ModuleKillStackDamageRule rule;
				rule.family = ParseTag(entry.at("family"), GameplayTagKind::AbilityCategory, entryContext + " family");
				rule.fraction = ParseFiniteFloat(entry.at("fraction"), entryContext + ".fraction");
				if (!(rule.fraction > 0.f && rule.fraction <= 1.f)) throw std::runtime_error(entryContext + ".fraction must be in (0, 1]");
				rules.killStackDamageRules.push_back(std::move(rule));
			}
		}

		// chargeRules entries (Spare Capacitor). A main-package entry takes chargePerLevel only: a per-level array of positive
		// integers (charge capacity added to each adjacent Q/E/F/R ability). A side-feature entry takes overflowTransfer (bool)
		// and/or trickleSeconds (positive) with trickleCharges (positive integer), and never chargePerLevel. The adjacent
		// ability count is the cell count, so a variant with a charge rule holds at most 3 cells. Unknown fields are rejected.
		void ParseChargeRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleSizeVariant& variant, bool sideFeature)
		{
			const Json entries = object.value("chargeRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".chargeRules must be an array");
			const std::set<std::string> allowedKeys{ "chargePerLevel", "overflowTransfer", "trickleSeconds", "trickleCharges" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".chargeRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (variant.shape.cells.size() > 3) throw std::runtime_error(entryContext + " charge rules allow at most 3 cells (3 adjacent abilities)");
				ModuleChargeRule rule;
				const bool capacityRule = entry.contains("chargePerLevel");
				const bool sideRule = entry.contains("overflowTransfer") || entry.contains("trickleSeconds") || entry.contains("trickleCharges");
				if (capacityRule == sideRule) throw std::runtime_error(entryContext + " needs either chargePerLevel or overflowTransfer/trickleSeconds/trickleCharges");
				if (capacityRule)
				{
					if (sideFeature) throw std::runtime_error(entryContext + " chargePerLevel is main-package only");
					if (!entry.at("chargePerLevel").is_array()) throw std::runtime_error(entryContext + ".chargePerLevel must be a per-level array");
					ParsePositiveLevelInt(entry.at("chargePerLevel"), entryContext + ".chargePerLevel", minimumLevelCount, rule.chargePerLevel);
				}
				else
				{
					if (!sideFeature) throw std::runtime_error(entryContext + " overflowTransfer and trickle fields are side-feature only");
					if (entry.contains("overflowTransfer"))
					{
						if (!entry.at("overflowTransfer").is_boolean()) throw std::runtime_error(entryContext + ".overflowTransfer must be a boolean");
						rule.overflowTransfer = entry.at("overflowTransfer").get<bool>();
					}
					if (entry.contains("trickleSeconds") != entry.contains("trickleCharges")) throw std::runtime_error(entryContext + " trickleSeconds and trickleCharges go together");
					if (entry.contains("trickleSeconds"))
					{
						rule.trickleSeconds = ParseFiniteFloat(entry.at("trickleSeconds"), entryContext + ".trickleSeconds");
						if (rule.trickleSeconds <= 0.f) throw std::runtime_error(entryContext + ".trickleSeconds must be positive");
						rule.trickleCharges = ParsePositiveInt(entry.at("trickleCharges"), entryContext + ".trickleCharges");
					}
					if (!rule.overflowTransfer && rule.trickleSeconds <= 0.f) throw std::runtime_error(entryContext + " has no effect");
				}
				variant.chargeRules.push_back(std::move(rule));
			}
		}

		// targetSlowRules entries: minRatio (at least 1), slowFraction and bossSlowFraction (in (0,1)) and seconds (positive).
		// All four fields are required, not per-level, and unknown fields are rejected.
		void ParseTargetSlowRules(const Json& object, const std::string& context, ModuleRulePackage& rules)
		{
			const Json entries = object.value("targetSlowRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".targetSlowRules must be an array");
			const std::set<std::string> allowedKeys{ "minRatio", "slowFraction", "bossSlowFraction", "seconds" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".targetSlowRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				for (const char* key : { "minRatio", "slowFraction", "bossSlowFraction", "seconds" })
					if (!entry.contains(key)) throw std::runtime_error(entryContext + "." + key + " is required");
				ModuleTargetSlowRule rule;
				rule.minRatio = ParseFiniteFloat(entry.at("minRatio"), entryContext + ".minRatio");
				rule.slowFraction = ParseFiniteFloat(entry.at("slowFraction"), entryContext + ".slowFraction");
				rule.bossSlowFraction = ParseFiniteFloat(entry.at("bossSlowFraction"), entryContext + ".bossSlowFraction");
				rule.seconds = ParseFiniteFloat(entry.at("seconds"), entryContext + ".seconds");
				if (rule.minRatio < 1.f) throw std::runtime_error(entryContext + ".minRatio must be at least 1");
				if (rule.slowFraction <= 0.f || rule.slowFraction >= 1.f) throw std::runtime_error(entryContext + ".slowFraction must be between 0 and 1");
				if (rule.bossSlowFraction <= 0.f || rule.bossSlowFraction >= 1.f) throw std::runtime_error(entryContext + ".bossSlowFraction must be between 0 and 1");
				if (rule.seconds <= 0.f) throw std::runtime_error(entryContext + ".seconds must be positive");
				rules.targetSlowRules.push_back(rule);
			}
		}

		// fragmentation (F1 Parçalama) is a side-feature switch: it may only be true, and only in a side package. A main package never splits.
		void ParseFragmentation(const Json& object, const std::string& context, ModuleRulePackage& rules, bool sideFeature)
		{
			if (!object.contains("fragmentation")) return;
			if (!sideFeature) throw std::runtime_error(context + ".fragmentation is side-feature only");
			if (!object.at("fragmentation").is_boolean() || !object.at("fragmentation").get<bool>())
				throw std::runtime_error(context + ".fragmentation must be true when present");
			rules.fragmentation = true;
		}

		ModuleSizeVariant ParseVariant(const Json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus, bool sideFeature)
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
				if (entry.contains("perEmptyCellMultiplier"))
				{
					if (rule.condition != ModuleDamageCondition::Always || rule.incoming)
						throw std::runtime_error(entryContext + " perEmptyCellMultiplier requires an outgoing Always rule");
					if (entry.contains("damageMultiplier"))
						throw std::runtime_error(entryContext + " perEmptyCellMultiplier cannot be combined with damageMultiplier");
					rule.perEmptyCellMultiplier = ParseLevelFloat(entry.at("perEmptyCellMultiplier"), entryContext + ".perEmptyCellMultiplier", minimumLevelCount,
						rule.perEmptyCellMultiplierByLevel);
					if (rule.perEmptyCellMultiplier <= 0.f) throw std::runtime_error(entryContext + " perEmptyCellMultiplier must be positive");
					ValidateTable(rule.perEmptyCellMultiplierByLevel, entryContext + ".perEmptyCellMultiplier", [](float value) { return value > 0.f; }, "must be positive");
				}
				if (entry.contains("requiredEmptyAbilitySlots"))
				{
					if (rule.incoming) throw std::runtime_error(entryContext + " requiredEmptyAbilitySlots requires an outgoing rule");
					rule.requiredEmptyAbilitySlots = ParsePositiveInt(entry.at("requiredEmptyAbilitySlots"), entryContext + ".requiredEmptyAbilitySlots");
					if (rule.requiredEmptyAbilitySlots > 4) throw std::runtime_error(entryContext + " requiredEmptyAbilitySlots must be between 1 and 4");
				}
				if (entry.contains("ratioSlope"))
				{
					if (rule.condition != ModuleDamageCondition::TargetMaxHealthRatioAtLeast || rule.incoming)
						throw std::runtime_error(entryContext + " ratioSlope requires an outgoing TargetMaxHealthRatioAtLeast rule");
					rule.ratioSlope = ParseFiniteFloat(entry.at("ratioSlope"), entryContext + ".ratioSlope");
					if (rule.ratioSlope < 0.f) throw std::runtime_error(entryContext + " ratioSlope must not be negative");
				}
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
				else if (rule.condition == ModuleDamageCondition::TargetStatusKindsAtLeast)
				{
					const auto isStatusKindCount = [](float value) { return value >= 2.f && std::floor(value) == value; };
					if (!entry.contains("conditionValue") || !isStatusKindCount(rule.conditionValue))
						throw std::runtime_error(entryContext + " TargetStatusKindsAtLeast requires an integer conditionValue of at least 2");
					ValidateTable(rule.conditionValueByLevel, entryContext + ".conditionValue", isStatusKindCount, "must be an integer of at least 2");
				}
				else if (rule.condition == ModuleDamageCondition::TargetMaxHealthRatioAtLeast)
				{
					const auto atLeastOne = [](float value) { return value >= 1.f; };
					if (!entry.contains("conditionValue") || !atLeastOne(rule.conditionValue))
						throw std::runtime_error(entryContext + " TargetMaxHealthRatioAtLeast requires a conditionValue of at least 1");
					ValidateTable(rule.conditionValueByLevel, entryContext + ".conditionValue", atLeastOne, "must be at least 1");
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
				if (!changesDamageMultiplier && !hasBonusDamage && rule.maxHealthDamageFraction <= 0.f && rule.perEmptyCellMultiplier <= 0.f && rule.ratioSlope <= 0.f)
					throw std::runtime_error(entryContext + " must change outgoing damage");
				if (rule.incoming && ((rule.condition != ModuleDamageCondition::OwnerHealthAtMost && rule.condition != ModuleDamageCondition::SourceHasStatus && rule.condition != ModuleDamageCondition::Always && rule.condition != ModuleDamageCondition::SourceMaxHealthGreaterThanOwner && rule.condition != ModuleDamageCondition::OwnerShieldOverMax) || hasBonusDamage))
					throw std::runtime_error(entryContext + " incoming rules require an owner-health/source-status/always/source-max-health condition and a multiplier only");
				if (!rule.incoming && rule.condition == ModuleDamageCondition::SourceMaxHealthGreaterThanOwner)
					throw std::runtime_error(entryContext + " SourceMaxHealthGreaterThanOwner is an incoming condition");
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
			ParseZoneRules(object, context, minimumLevelCount, variant);
			const Json actions = object.value("eventActions", Json::array());
			if (!actions.is_array()) throw std::runtime_error(context + ".eventActions must be an array");
			for (const Json& entry : actions)
			{
				ModuleEventAction action;
				action.trigger = ParseTriggerKind(entry.at("trigger").get<std::string>());
				const std::string kind = entry.at("kind").get<std::string>();
				if (kind == "ShieldOverflowToHealth") action.kind = ModuleEventActionKind::ShieldOverflowToHealth;
				else if (kind == "RestoreShield") action.kind = ModuleEventActionKind::RestoreShield;
				else if (kind == "GrantTemporaryShield") action.kind = ModuleEventActionKind::GrantTemporaryShield;
				else if (kind == "RestoreHealth") action.kind = ModuleEventActionKind::RestoreHealth;
				else if (kind == "RestoreHealthOverTime") action.kind = ModuleEventActionKind::RestoreHealthOverTime;
				else if (kind == "RestoreAfterburner") action.kind = ModuleEventActionKind::RestoreAfterburner;
				else if (kind == "ReduceAllCooldowns") action.kind = ModuleEventActionKind::ReduceAllCooldowns;
				else if (kind == "ReduceFamilyCooldowns") action.kind = ModuleEventActionKind::ReduceFamilyCooldowns;
				else if (kind == "ReduceLongestCooldown") action.kind = ModuleEventActionKind::ReduceLongestCooldown;
				else if (kind == "RefundUsedCooldown") action.kind = ModuleEventActionKind::RefundUsedCooldown;
				else if (kind == "ApplyNearbyCryo") action.kind = ModuleEventActionKind::ApplyNearbyCryo;
				else if (kind == "AwardGold") action.kind = ModuleEventActionKind::AwardGold;
				else throw std::runtime_error(context + " unknown event action: " + kind);
				if (action.kind == ModuleEventActionKind::GrantTemporaryShield || action.kind == ModuleEventActionKind::RestoreHealthOverTime)
				{
					action.durationSeconds = ParseFiniteFloat(entry.at("durationSeconds"), context);
					if (action.durationSeconds <= 0.f || action.trigger != ModuleTriggerKind::AbilityActivated) throw std::runtime_error(context + " invalid temporary shield timing");
				}
				const bool refund = action.kind == ModuleEventActionKind::RefundUsedCooldown;
				const bool cryo = action.kind == ModuleEventActionKind::ApplyNearbyCryo;
				const bool abilityRestore = action.trigger == ModuleTriggerKind::AbilityActivated && (action.kind == ModuleEventActionKind::RestoreShield || action.kind == ModuleEventActionKind::RestoreHealth || action.kind == ModuleEventActionKind::GrantTemporaryShield || action.kind == ModuleEventActionKind::RestoreHealthOverTime);
				if (!abilityRestore && action.trigger != (refund ? ModuleTriggerKind::AbilityActivated : cryo ? ModuleTriggerKind::DamageTaken : ModuleTriggerKind::KillConfirmed))
					throw std::runtime_error(context + " incompatible event action trigger");
				action.requiredEventTags = ParseRequiredEventTags(entry.value("requiredEventTags", Json::array()), action.trigger, context + ".eventActions.requiredEventTags");
				action.amount = ParseLevelFloat(entry.at("amount"), context + ".eventActions.amount", minimumLevelCount, action.amountByLevel);
				const bool seconds = action.kind == ModuleEventActionKind::ReduceAllCooldowns;
				const auto validAmount = [seconds, cryo, gold = action.kind == ModuleEventActionKind::AwardGold](float value) { return value > 0.f && ((cryo || gold) ? value <= static_cast<float>(std::numeric_limits<int>::max() / 2) && std::floor(value) == value : seconds || value <= 1.f); };
				if (!validAmount(action.amount)) throw std::runtime_error(context + " invalid event action amount");
				ValidateTable(action.amountByLevel, context, validAmount, "invalid event action amount");
				if (entry.contains("abilityFamily"))
				{
					if (action.kind != ModuleEventActionKind::ReduceFamilyCooldowns && action.kind != ModuleEventActionKind::ReduceLongestCooldown)
						throw std::runtime_error(context + " abilityFamily requires a family cooldown action");
					action.abilityFamily = ParseTag(entry.at("abilityFamily"), GameplayTagKind::AbilityCategory, context + ".abilityFamily");
					if (action.abilityFamily == GameplayTags::Ability::Primary) throw std::runtime_error(context + " cooldown refunds exclude Primary");
				}
				if (action.kind == ModuleEventActionKind::ReduceFamilyCooldowns && !action.abilityFamily.IsValid())
					throw std::runtime_error(context + " family cooldown refund requires abilityFamily");
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
				else if (action.kind == ModuleEventActionKind::RestoreHealthOverTime)
				{
					action.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), context);
					if (action.cooldownSeconds < action.durationSeconds) throw std::runtime_error(context + " healing cooldown must cover duration");
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
				else if (source == "Armor") rule.source = ModuleOwnerAttributeSource::Armor;
				else if (source == "ArmorHigherPower") rule.source = ModuleOwnerAttributeSource::ArmorHigherPower;
				else if (source == "ArmorLowerPower") rule.source = ModuleOwnerAttributeSource::ArmorLowerPower;
				else if (source == "Gold") rule.source = ModuleOwnerAttributeSource::Gold;
				else throw std::runtime_error(context + " unknown owner attribute source: " + source);
				rule.attributeId = sas::AttributeId{ entry.at("attributeId").get<std::string>() };
				std::string failure;
				if (!AttributeIdSchema::Validate(rule.attributeId, &failure)) throw std::runtime_error(context + failure);
				rule.target = ParseTarget(entry.value("target", std::string{ "Owner" }));
				rule.operation = AttributeJsonParser::ParseOperation(entry.value("operation", std::string{ "Add" }), context);
				const bool ownerAttribute = rule.target == ModuleAttributeTarget::Owner && rule.operation == sas::AttributeModifierOperation::Add &&
					(rule.attributeId == OwnerAttributeIds::AttackSpeed || rule.attributeId == OwnerAttributeIds::AttackPower || rule.attributeId == OwnerAttributeIds::EnergyPower ||
						((rule.source == ModuleOwnerAttributeSource::Luck || rule.source == ModuleOwnerAttributeSource::HealthThreshold) && rule.attributeId == OwnerAttributeIds::CriticalChance) || (rule.source == ModuleOwnerAttributeSource::Gold && rule.attributeId == OwnerAttributeIds::Armor));
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
				if (entry.contains("maximumAmount"))
				{
					rule.maximumAmount = ParseLevelFloat(entry.at("maximumAmount"), context, minimumLevelCount, rule.maximumAmountByLevel);
					if (rule.source != ModuleOwnerAttributeSource::Gold || rule.maximumAmount <= 0.f) throw std::runtime_error(context + " maximumAmount requires positive Gold-based output");
					ValidateTable(rule.maximumAmountByLevel, context, [](float value) { return value > 0.f; }, "invalid Gold Armor cap");
				}
				if (rule.source == ModuleOwnerAttributeSource::Gold && (!entry.contains("maximumAmount") || rule.attributeId != OwnerAttributeIds::Armor || entry.contains("healthAtMost"))) throw std::runtime_error(context + " Gold requires capped Armor output without health threshold");
				rule.fadeSeconds = entry.contains("fadeSeconds") ? ParseFiniteFloat(entry.at("fadeSeconds"), context) : 0.f;
				if (rule.fadeSeconds < 0.f || (entry.contains("fadeSeconds") && rule.source != ModuleOwnerAttributeSource::MovementSpeed))
					throw std::runtime_error(context + " invalid movement fade");
				rule.minimumSpeed = entry.contains("minimumSpeed") ? ParseFiniteFloat(entry.at("minimumSpeed"), context) : 0.f;
				rule.speedStep = entry.contains("speedStep") ? ParseFiniteFloat(entry.at("speedStep"), context) : 0.f;
				if (rule.minimumSpeed < 0.f || rule.speedStep < 0.f || ((entry.contains("minimumSpeed") || entry.contains("speedStep")) && rule.source != ModuleOwnerAttributeSource::MovementSpeed))
					throw std::runtime_error(context + " invalid movement speed steps");
				if (entry.contains("speedStep") && rule.speedStep <= 0.f) throw std::runtime_error(context + " movement speed step must be positive");
				variant.ownerAttributeRules.push_back(std::move(rule));
			}
			const Json movement = object.value("conditionalMovementRules", Json::array());
			if (!movement.is_array()) throw std::runtime_error(context + " conditionalMovementRules must be an array");
			for (const Json& entry : movement)
			{
				if (!entry.is_object() || entry.size() != 2 || !entry.contains("condition") || !entry.contains("multiplier")) throw std::runtime_error(context + " invalid conditional movement rule");
				ModuleConditionalMovementRule rule;
				if (entry.at("condition") != "OwnerShieldOverMax") throw std::runtime_error(context + " unsupported conditional movement condition");
				rule.multiplier = ParseLevelFloat(entry.at("multiplier"), context, minimumLevelCount, rule.multiplierByLevel);
				if (rule.multiplier <= 1.f) throw std::runtime_error(context + " movement multiplier must exceed one");
				ValidateTable(rule.multiplierByLevel, context, [](float value) { return value > 1.f; }, "invalid conditional movement multiplier");
				variant.conditionalMovementRules.push_back(rule);
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
					rule.damageType = ParseTag(entry.at("damageType"), GameplayTagKind::DamageType, context + " shield reflection damageType");
				const auto valid = [](float value) { return value > 0.f; };
				if (!valid(rule.fraction) || rule.durationSeconds <= 0.f) throw std::runtime_error(context + " invalid shield reflection");
				ValidateTable(rule.fractionByLevel, context, valid, "invalid shield reflection fraction");
				variant.shieldReflections.push_back(rule);
			}
			ParseShieldRules(object, context, minimumLevelCount, variant);
			ParseMovementRules(object, context, minimumLevelCount, variant);
			ParseRunCounterRules(object, context, minimumLevelCount, variant);
			ParseSustainedSpeedRules(object, context, minimumLevelCount, variant, sideFeature);
			ParseOwnerBurstRules(object, context, minimumLevelCount, variant);
			ParseOwnerBurstModifiers(object, context, variant);
			ParseTargetSlowRules(object, context, variant);
			ParseKillStackCooldownRules(object, context, variant);
			ParseKillStackDamageRules(object, context, variant);
			ParseChargeRules(object, context, minimumLevelCount, variant, sideFeature);
			ParseFragmentation(object, context, variant, sideFeature);
			const Json statusRules = object.value("statusRules", Json::object());
			if (!statusRules.is_object()) throw std::runtime_error(context + ".statusRules must be an object");
			const std::set<std::string> statusKeys{ "electricOverrides", "electricSpreads", "primaryStatusPrimers", "cryoFreezes", "thermalCryoDurationBonuses", "freezeSpreads", "durationBonuses", "thermalBursts", "kineticConsumes" };
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
			for (const Json& entry : statusRules.value("primaryStatusPrimers", Json::array()))
			{
				validateStatusFields(entry, { "cooldownSeconds" });
				ModulePrimaryStatusPrimerRule rule; rule.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), context);
				if (rule.cooldownSeconds <= 0.f) throw std::runtime_error(context + " invalid status primer cooldown");
				variant.statusRules.primaryStatusPrimers.push_back(rule);
			}
			for (const Json& entry : statusRules.value("electricSpreads", Json::array()))
			{
				validateStatusFields(entry, { "threshold", "radius", "stacks", "targetCount", "cooldownSeconds" });
				ModuleElectricSpreadRule rule;
				rule.threshold = ParsePositiveInt(entry.at("threshold"), context);
				rule.radius = ParseFiniteFloat(entry.at("radius"), context);
				rule.stacks = ParsePositiveInt(entry.at("stacks"), context);
				rule.targetCount = ParsePositiveInt(entry.at("targetCount"), context);
				rule.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), context);
				if (rule.threshold > 5 || !positive(rule.radius) || !positive(rule.cooldownSeconds)) throw std::runtime_error(context + " invalid electric spread");
				variant.statusRules.electricSpreads.push_back(rule);
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
					validateStatusFields(entry, { "bonusSeconds", "durationMultiplier" });
					ModuleStatusDurationRule rule;
					if (entry.contains("bonusSeconds")) rule.bonusSeconds = ParseLevelFloat(entry.at("bonusSeconds"), context, minimumLevelCount, rule.bonusSecondsByLevel);
					if (entry.contains("durationMultiplier"))
					{
						if (std::string{ key } != "durationBonuses") throw std::runtime_error(context + " duration multiplier is general-only");
						rule.durationMultiplier = ParseLevelFloat(entry.at("durationMultiplier"), context, minimumLevelCount, rule.durationMultiplierByLevel);
						ValidateTable(rule.durationMultiplierByLevel, context, [](float value) { return value >= 1.f; }, "invalid status duration multiplier");
					}
					if (rule.bonusSeconds < 0.f || rule.durationMultiplier < 1.f || (rule.bonusSeconds == 0.f && rule.durationMultiplier == 1.f)) throw std::runtime_error(context + " invalid status duration bonus");
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
			ParseThermalBurstRules(statusRules, context, minimumLevelCount, variant, sideFeature);
			ParseKineticConsumeRules(statusRules, context, minimumLevelCount, variant, sideFeature);
			const Json generated = object.value("generatedRules", Json::object());
			if (!generated.is_object()) throw std::runtime_error(context + ".generatedRules must be an object");
			const std::set<std::string> generatedKeys{ "ricochets", "guidedSalvos", "delayedExplosions", "trailMines", "killExplosions", "speedLances", "annularBursts" };
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
				fields(entry, { "damageFraction", "projectileCount", "speed", "lifetime", "targetRange", "maxActive", "damageType" });
				ModuleGuidedSalvoRule rule;
				rule.damageFraction = fraction(entry, "damageFraction", rule.damageFractionByLevel);
				rule.projectileCount = count(entry, "projectileCount"); rule.speed = number(entry, "speed"); rule.lifetime = number(entry, "lifetime"); rule.targetRange = number(entry, "targetRange"); rule.maxActive = count(entry, "maxActive");
				if (rule.damageFraction > 0.f) rule.damageType = ParseTag(entry.at("damageType"), GameplayTagKind::DamageType, context + " guided salvo damageType");
				else if (entry.contains("damageType")) throw std::runtime_error(context + " guided salvo damageType requires damageFraction");
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
				fields(entry, { "damage", "attackPowerCoefficient", "spacing", "windowSeconds", "fuseSeconds", "radius", "maxActive", "additionalLimit", "cryoStacks", "damageType" });
				ModuleTrailMineRule rule;
				if (entry.contains("damage"))
				{
					rule.damage = ParseLevelFloat(entry.at("damage"), context, minimumLevelCount, rule.damageByLevel);
					if (!positive(rule.damage)) throw std::runtime_error(context + " mine damage must be positive");
					ValidateTable(rule.damageByLevel, context, positive, "mine damage must be positive");
				}
				if (entry.contains("attackPowerCoefficient"))
				{
					rule.attackPowerCoefficient = ParseLevelFloat(entry.at("attackPowerCoefficient"), context, minimumLevelCount, rule.attackPowerCoefficientByLevel);
					const auto nonNegative = [](float value) { return std::isfinite(value) && value >= 0.f; };
					if (rule.damage <= 0.f || !nonNegative(rule.attackPowerCoefficient)) throw std::runtime_error(context + " mine AP coefficient requires damage and must be non-negative");
					ValidateTable(rule.attackPowerCoefficientByLevel, context, nonNegative, "mine AP coefficient must be non-negative");
				}
				rule.spacing = number(entry, "spacing"); rule.windowSeconds = number(entry, "windowSeconds"); rule.fuseSeconds = number(entry, "fuseSeconds"); rule.radius = number(entry, "radius");
				if (entry.contains("maxActive")) rule.maxActive = ParsePositiveLevelInt(entry.at("maxActive"), context, minimumLevelCount, rule.maxActiveByLevel);
				rule.additionalLimit = count(entry, "additionalLimit"); rule.cryoStacks = count(entry, "cryoStacks");
				if (rule.damage > 0.f) rule.damageType = ParseTag(entry.at("damageType"), GameplayTagKind::DamageType, context + " trail mine damageType");
				else if (entry.contains("damageType")) throw std::runtime_error(context + " trail mine damageType requires damage");
				if ((rule.damage > 0.f && (rule.spacing <= 0.f || rule.windowSeconds <= 0.f || rule.fuseSeconds <= 0.f || rule.radius <= 0.f || rule.maxActive <= 0)) || (rule.damage <= 0.f && rule.additionalLimit <= 0 && rule.cryoStacks <= 0)) throw std::runtime_error(context + " incomplete trail mine rule");
				if (rule.maxActive > 1024) throw std::runtime_error(context + " mine limit exceeds technical ceiling");
				ValidateTable(rule.maxActiveByLevel, context, [](int v) { return v <= 1024; }, "mine limit exceeds technical ceiling");
				variant.generatedRules.trailMines.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("annularBursts", Json::array()))
			{
				fields(entry, { "innerRadius", "outerRadius", "energyPowerCoefficient" });
				ModuleAnnularBurstRule rule{ number(entry, "innerRadius"), number(entry, "outerRadius"), number(entry, "energyPowerCoefficient") };
				if (sideFeature || rule.innerRadius <= 0.f || rule.outerRadius <= rule.innerRadius || rule.energyPowerCoefficient <= 0.f) throw std::runtime_error(context + " annular burst needs an ordered positive ring and Energy Power coefficient; main sources only");
				variant.generatedRules.annularBursts.push_back(rule);
			}
			for (const auto& entry : generated.value("speedLances", Json::array()))
			{
				fields(entry, { "speedDamageCoefficient", "sampleSeconds", "projectileSpeed", "range", "echoDelaySeconds", "echoFraction", "firstTargetElectricStacks" });
				ModuleSpeedLanceRule rule;
				rule.speedDamageCoefficient = number(entry, "speedDamageCoefficient"); rule.sampleSeconds = number(entry, "sampleSeconds"); rule.projectileSpeed = number(entry, "projectileSpeed"); rule.range = number(entry, "range");
				rule.echoDelaySeconds = number(entry, "echoDelaySeconds"); rule.echoFraction = number(entry, "echoFraction"); rule.firstTargetElectricStacks = count(entry, "firstTargetElectricStacks");
				if ((rule.speedDamageCoefficient > 0.f && (rule.sampleSeconds <= 0.f || rule.projectileSpeed <= 0.f || rule.range <= 0.f)) || ((rule.echoDelaySeconds > 0.f) != (rule.echoFraction > 0.f)) || rule.echoFraction > 1.f || (rule.speedDamageCoefficient == 0.f && rule.echoFraction == 0.f && rule.firstTargetElectricStacks == 0)) throw std::runtime_error(context + " incomplete Speed Lance rule");
				variant.generatedRules.speedLances.push_back(rule);
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
			if (!variants.is_array() || variants.empty())
			{
				throw std::runtime_error(context + " must define at least one size variant");
			}
			std::size_t index = 0;
			for (const Json& variant : variants)
			{
				definition.sizeVariants.push_back(
					ParseVariant(variant, context + ".sizeVariants[" + std::to_string(index++) + "]", definition.levelCount, true, false)
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
		nlohmann::json shapedObject = object;
		nlohmann::json shape = nlohmann::json::array();
		shape.push_back(nlohmann::json::array({ 0, 0 }));
		shapedObject["shape"] = std::move(shape);
		ModuleSizeVariant parsed = ParseVariant(shapedObject, context, minimumLevelCount, defaultAffectedByZoneBonus, sideFeature);
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
			result.zoneTable = ParseZoneTable(root);
			std::set<std::string> moduleIds;
			for (const Json& module : root.at("modules"))
			{
				if (!selectedId.empty() && module.value("id", std::string{}) != selectedId) continue;
				ModuleDefinition definition = ParseModule(module, result.zoneTable);
				if (!moduleIds.insert(definition.moduleId.ToString()).second)
				{
					throw std::runtime_error("Duplicate module ID: " + definition.moduleId.ToString());
				}
				result.definitions.push_back(std::move(definition));
			}
			for (const Json& module : root.at("modules")) moduleIds.insert(module.at("id").get<std::string>());
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

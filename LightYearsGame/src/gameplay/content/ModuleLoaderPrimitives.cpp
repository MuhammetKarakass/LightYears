#include "ModuleLoaderParsing.h"

#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/content/AttributeJsonParser.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/module/ModuleDescriptionValues.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace vega::content
{
	namespace module_loader_detail
	{
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

		void ValidateLevelGrowthFields(const Json& object, const std::string& context)
		{
			if (object.is_array())
			{
				for (const auto& entry : object) ValidateLevelGrowthFields(entry, context);
				return;
			}
			if (!object.is_object()) return;
			for (const auto& entry : object.items())
			{
				const auto& key = entry.key();
				if (key.size() > 6 && key.compare(key.size() - 6, 6, "Growth") == 0)
				{
					const std::string field = key.substr(0, key.size() - 6);
					if (!object.contains(field) || !object.at(field).is_array()) throw std::runtime_error(context + " " + key + " requires a sibling level table");
				}
				else ValidateLevelGrowthFields(entry.value(), context + "." + key);
			}
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
				ParseLevelFloat(object, "magnitude", context + ".magnitude", minimumLevelCount, magnitudeByLevel),
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

		void ValidateDescription(const ModuleDefinition& definition, const std::string& context)
		{
			for (const auto& variant : definition.sizeVariants) ValidateModuleDescriptionTemplate(definition.description, variant, context);
		}
	}
}

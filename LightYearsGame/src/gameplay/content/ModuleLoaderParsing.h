#pragma once

#include "framework/JsonDocumentLoader.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace vega::content
{
	// Parsing helpers shared by the split ModuleLoader translation units. Templates stay here because
	// every caller instantiates them; other definitions live in ModuleLoaderPrimitives.cpp and
	// ModuleLoaderRules.cpp.
	namespace module_loader_detail
	{
		using Json = JsonDocumentLoader::Json;
		AbilityModuleRules ParseAbilityModuleRules(const Json& rules, const std::string& id);

		float ParseFiniteFloat(const Json& value, const std::string& context);

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

		float ParseLevelFloat(const Json& value, const std::string& context, int minimumLevelCount, ModuleLevelTable<float>& table);

		int ParsePositiveLevelInt(const Json& value, const std::string& context, int minimumLevelCount, ModuleLevelTable<int>& table);

		int ParsePositiveInt(const Json& value, const std::string& context);

		template<typename T>
		void ParseLevelGrowth(const Json& object, const std::string& field, const std::string& context, ModuleLevelTable<T>& table)
		{
			const std::string key = field + "Growth";
			if (!object.contains(key)) return;
			const Json& value = object.at(key);
			if (table.values.empty()) throw std::runtime_error(context + " growth requires a level table");
			std::string type;
			if (value.is_string()) type = value.get<std::string>();
			else if (value.is_object() && value.contains("type") && value.at("type").is_string()) type = value.at("type").get<std::string>();
			else throw std::runtime_error(context + " growth must be a type string or object with type");
			if (type == "linear") table.growth = ModuleLevelGrowth::Linear;
			else if (type == "compound") table.growth = ModuleLevelGrowth::Compound;
			else if (type == "step") table.growth = ModuleLevelGrowth::Step;
			else throw std::runtime_error(context + " unknown growth type: " + type);
			if (value.is_object())
			{
				for (auto it = value.begin(); it != value.end(); ++it)
				{
					if (it.key() == "type") continue;
					if (it.key() == "ratio" && type == "compound") table.ratio = ParseFiniteFloat(it.value(), context + " growth ratio");
					else if (it.key() == "every" && type == "step") table.every = ParsePositiveInt(it.value(), context + " growth every");
					else if (it.key() == "amount" && type != "linear")
					{
						table.amount = ParseFiniteFloat(it.value(), context + " growth amount");
						if constexpr (std::is_integral_v<T>) if (!it.value().is_number_integer()) throw std::runtime_error(context + " integer table needs integer growth amount");
					}
					else throw std::runtime_error(context + " unexpected growth field: " + it.key());
				}
			}
			if (type == "compound")
			{
				if (value.is_object() && value.contains("ratio") && table.ratio <= 0.0) throw std::runtime_error(context + " growth ratio must be positive");
				if (table.ratio == 0.0 && (table.values.size() < 2 || table.values.back() <= 0 || table.values[table.values.size() - 2] <= 0))
					throw std::runtime_error(context + " compound growth needs a positive ratio or two positive final values");
			}
			if (type == "step" && (!value.is_object() || !value.contains("every") || !value.contains("amount")))
				throw std::runtime_error(context + " step growth requires every and amount");
		}

		inline float ParseLevelFloat(const Json& object, const std::string& field, const std::string& context, int minimumLevelCount, ModuleLevelTable<float>& table)
		{
			const float result = ParseLevelFloat(object.at(field), context, minimumLevelCount, table);
			ParseLevelGrowth(object, field, context, table);
			return result;
		}

		inline int ParsePositiveLevelInt(const Json& object, const std::string& field, const std::string& context, int minimumLevelCount, ModuleLevelTable<int>& table)
		{
			const int result = ParsePositiveLevelInt(object.at(field), context, minimumLevelCount, table);
			ParseLevelGrowth(object, field, context, table);
			return result;
		}

		template<typename T, typename Predicate>
		void ValidateTable(const ModuleLevelTable<T>& table, const std::string& context, Predicate valid, const char* message)
		{
			for (const T& value : table.values)
				if (!valid(value)) throw std::runtime_error(context + " " + message);
		}

		void ValidateLevelGrowthFields(const Json& object, const std::string& context);

		GameplayTag ParseTag(const Json& value, GameplayTagKind kind, const std::string& context);

		ModuleAttributeTarget ParseTarget(const std::string& value);

		ModuleLegendaryRule ParseLegendaryRule(const Json& object, const GameplayTag& family, const std::string& context, int levelCount);

		ModuleRarity ParseRarity(const std::string& value);

		sas::ContentId ParseContentId(const Json& value, const std::string& context);

		ModuleTriggerKind ParseTriggerKind(const std::string& value);

		List<GameplayTag> ParseRequiredEventTags(const Json& tags, ModuleTriggerKind trigger, const std::string& context);

		ModuleDamageCondition ParseDamageCondition(const std::string& value);

		ModuleCooldownScope ParseCooldownScope(const std::string& value);

		DamageCriticalPolicy ParseCriticalPolicy(const std::string& value);

		sas::AttributeModifier ParseModifier(const Json& object, const std::string& context, int minimumLevelCount, ModuleLevelTable<float>& magnitudeByLevel);

		ModuleAttributeModifier ParseAttributeModifier(const Json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus);

		ModuleShape ParseShape(const Json& value, const std::string& context);

		void ValidateDescription(const ModuleDefinition& definition, const std::string& context);

		void RejectZoneRules(const Json& object, const std::string& context);

		void ParseShieldRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules);

		void ParseShieldBatteryRules(const Json& object, const std::string& context, int minimumLevelCount, bool sideFeature, ModuleRulePackage& rules);

		void ParseShieldRuptureRules(const Json& object, const std::string& context, int minimumLevelCount, bool sideFeature, ModuleRulePackage& rules);

		void ParseOwnerBurstRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules);

		void ParseOwnerBurstModifiers(const Json& object, const std::string& context, ModuleRulePackage& rules);

		void ParseMovementRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules);

		void ParseSustainedSpeedRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);

		void ParseThermalBurstRules(const Json& statusRules, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);

		void ParseKineticConsumeRules(const Json& statusRules, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);

		void ParseRunCounterRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules);

		void ParseKillStackCooldownRules(const Json& object, const std::string& context, ModuleRulePackage& rules);

		void ParseKillStackDamageRules(const Json& object, const std::string& context, ModuleRulePackage& rules);
		void ParseFamilyStatRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);

		void ParseChargeRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleSizeVariant& variant, bool sideFeature);

		void ParseEpicUtilityMovementRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);

		void ParseTargetSlowRules(const Json& object, const std::string& context, ModuleRulePackage& rules);

		void ParseFragmentation(const Json& object, const std::string& context, ModuleRulePackage& rules, bool sideFeature);

		void ParseEpicDefenseRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);
		void ParseSideRules(const Json& object, const std::string& context, ModuleRulePackage& rules, bool sideFeature);
		void ParsePrimaryMountRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);
		void ParseEpicOffenseRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleSizeVariant& variant, bool sideFeature);

		ModuleSizeVariant ParseVariant(const Json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus, bool sideFeature);
	}
}

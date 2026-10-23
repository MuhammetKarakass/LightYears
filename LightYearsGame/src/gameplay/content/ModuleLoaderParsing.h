#pragma once

#include "framework/JsonDocumentLoader.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <cstddef>
#include <stdexcept>
#include <string>

namespace vega::content
{
	// Parsing helpers shared by the split ModuleLoader translation units. Templates stay here because
	// every caller instantiates them; other definitions live in ModuleLoaderPrimitives.cpp and
	// ModuleLoaderRules.cpp.
	namespace module_loader_detail
	{
		using Json = JsonDocumentLoader::Json;

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

		template<typename T, typename Predicate>
		void ValidateTable(const ModuleLevelTable<T>& table, const std::string& context, Predicate valid, const char* message)
		{
			for (const T& value : table.values)
				if (!valid(value)) throw std::runtime_error(context + " " + message);
		}

		GameplayTag ParseTag(const Json& value, GameplayTagKind kind, const std::string& context);

		ModuleAttributeTarget ParseTarget(const std::string& value);

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

		void ParseZoneRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& package);

		void ParseShieldRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules);

		void ParseOwnerBurstRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules);

		void ParseOwnerBurstModifiers(const Json& object, const std::string& context, ModuleRulePackage& rules);

		void ParseMovementRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules);

		void ParseSustainedSpeedRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);

		void ParseThermalBurstRules(const Json& statusRules, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);

		void ParseKineticConsumeRules(const Json& statusRules, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature);

		void ParseRunCounterRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules);

		void ParseKillStackCooldownRules(const Json& object, const std::string& context, ModuleRulePackage& rules);

		void ParseKillStackDamageRules(const Json& object, const std::string& context, ModuleRulePackage& rules);

		void ParseChargeRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleSizeVariant& variant, bool sideFeature);

		void ParseTargetSlowRules(const Json& object, const std::string& context, ModuleRulePackage& rules);

		void ParseFragmentation(const Json& object, const std::string& context, ModuleRulePackage& rules, bool sideFeature);

		ModuleSizeVariant ParseVariant(const Json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus, bool sideFeature);
	}
}

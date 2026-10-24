#include "ModuleLoaderParsing.h"

#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/content/AttributeJsonParser.h"

#include <set>
#include <stdexcept>

namespace vega::content
{
	namespace module_loader_detail
	{
		void ParseFamilyStatRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json entries = object.value("familyStatRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".familyStatRules must be an array");
			if (sideFeature && !entries.empty()) throw std::runtime_error(context + " familyStatRules is only allowed on the main package");
			const std::set<std::string> allowedKeys{ "family", "target", "attributeId", "operation", "perModule", "perModuleGrowth", "reduce", "doubleOnAbilityUse", "durationSeconds", "durationSecondsGrowth" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".familyStatRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());

				EpicFamilyStatRule rule;
				rule.family = ParseTag(entry.at("family"), GameplayTagKind::AbilityCategory, entryContext + " family");
				const std::string target = entry.value("target", std::string{ "Owner" });
				if (target == "Owner") rule.target = EpicFamilyStatTarget::Owner;
				else if (target == "PrimaryWeapon") rule.target = EpicFamilyStatTarget::PrimaryWeapon;
				else if (target == "Ability") rule.target = EpicFamilyStatTarget::Ability;
				else throw std::runtime_error(entryContext + " unknown target: " + target);

				rule.attributeId = sas::AttributeId{ entry.at("attributeId").get<std::string>() };
				std::string failure;
				if (!AttributeIdSchema::Validate(rule.attributeId, &failure)) throw std::runtime_error(entryContext + failure);
				rule.operation = AttributeJsonParser::ParseOperation(entry.value("operation", std::string{ "Add" }), entryContext);

				const Json& perModule = entry.at("perModule");
				if (!perModule.is_array()) throw std::runtime_error(entryContext + " perModule must be a level table");
				ModuleLevelTable<float> table;
				ParseLevelFloat(entry, "perModule", entryContext + " perModule", minimumLevelCount, table);
				if (table.values.size() < static_cast<std::size_t>(minimumLevelCount)) throw std::runtime_error(entryContext + " perModule must cover every level");
				for (const float value : table.values)
					if (!(value > 0.f)) throw std::runtime_error(entryContext + " perModule must be positive");
				rule.perModuleByLevel = std::move(table);

				rule.reduce = entry.value("reduce", false);
				if (rule.reduce && rule.operation != sas::AttributeModifierOperation::Multiply) throw std::runtime_error(entryContext + " reduce requires Multiply");

				rule.doubleOnAbilityUse = entry.value("doubleOnAbilityUse", false);
				if (rule.doubleOnAbilityUse)
				{
					if (rule.target != EpicFamilyStatTarget::Owner || rule.operation != sas::AttributeModifierOperation::Add)
						throw std::runtime_error(entryContext + " doubleOnAbilityUse requires an Owner Add rule");
					if (!entry.contains("durationSeconds")) throw std::runtime_error(entryContext + " doubleOnAbilityUse requires durationSeconds");
					rule.durationSeconds = ParseFiniteFloat(entry.at("durationSeconds"), entryContext + " durationSeconds");
					if (!(rule.durationSeconds > 0.f)) throw std::runtime_error(entryContext + " durationSeconds must be positive");
				}
				else if (entry.contains("durationSeconds")) throw std::runtime_error(entryContext + " durationSeconds requires doubleOnAbilityUse");

				rules.familyStatRules.push_back(std::move(rule));
			}
		}
	}
}

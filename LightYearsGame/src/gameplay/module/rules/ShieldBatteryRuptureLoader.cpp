#include "../../content/ModuleLoaderParsing.h"

#include <set>
#include <stdexcept>
#include <string>

namespace vega::content
{
	namespace module_loader_detail
	{
		// shieldBatteryRules entries. Main packages take reserveFraction (scalar or per-level array, each in (0, 1]), the
		// fraction of Max Shield the reserve may hold. Side features take damagedFillFraction (scalar in (0, 1]). Unknown fields
		// are rejected, and each package kind rejects the other's field.
		void ParseShieldBatteryRules(const Json& object, const std::string& context, int minimumLevelCount, bool sideFeature, ModuleRulePackage& rules)
		{
			const Json entries = object.value("shieldBatteryRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".shieldBatteryRules must be an array");
			const auto unitRange = [](float value) { return value > 0.f && value <= 1.f; };
			const std::set<std::string> allowedKeys = sideFeature ? std::set<std::string>{ "damagedFillFraction" } : std::set<std::string>{ "reserveFraction" };
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				const Json& entry = entries[index];
				const std::string entryContext = context + ".shieldBatteryRules[" + std::to_string(index) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				ModuleShieldBatteryRule rule;
				if (sideFeature)
				{
					if (!entry.contains("damagedFillFraction")) throw std::runtime_error(entryContext + ".damagedFillFraction is required");
					rule.damagedFillFraction = ParseFiniteFloat(entry.at("damagedFillFraction"), entryContext + ".damagedFillFraction");
					if (!unitRange(rule.damagedFillFraction)) throw std::runtime_error(entryContext + ".damagedFillFraction must be in (0, 1]");
				}
				else
				{
					if (!entry.contains("reserveFraction")) throw std::runtime_error(entryContext + ".reserveFraction is required");
					const std::string fieldContext = entryContext + ".reserveFraction";
					rule.reserveFraction = ParseLevelFloat(entry.at("reserveFraction"), fieldContext, minimumLevelCount, rule.reserveFractionByLevel);
					if (!unitRange(rule.reserveFraction)) throw std::runtime_error(fieldContext + " must be in (0, 1]");
					ValidateTable(rule.reserveFractionByLevel, fieldContext, unitRange, "must be in (0, 1]");
				}
				rules.shieldBatteryRules.push_back(rule);
			}
		}

		// shieldRuptureRules entries, main packages only: damageFraction (scalar or per-level array, > 0) and radius and
		// historySeconds (> 0), all required. Unknown fields are rejected.
		void ParseShieldRuptureRules(const Json& object, const std::string& context, int minimumLevelCount, bool sideFeature, ModuleRulePackage& rules)
		{
			const Json entries = object.value("shieldRuptureRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".shieldRuptureRules must be an array");
			if (sideFeature && !entries.empty()) throw std::runtime_error(context + ".shieldRuptureRules is valid only on the main package");
			const auto positive = [](float value) { return value > 0.f; };
			const std::set<std::string> allowedKeys{ "damageFraction", "radius", "historySeconds" };
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				const Json& entry = entries[index];
				const std::string entryContext = context + ".shieldRuptureRules[" + std::to_string(index) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				for (const char* key : { "damageFraction", "radius", "historySeconds" })
					if (!entry.contains(key)) throw std::runtime_error(entryContext + "." + key + " is required");
				ModuleShieldRuptureRule rule;
				const std::string fractionContext = entryContext + ".damageFraction";
				rule.damageFraction = ParseLevelFloat(entry.at("damageFraction"), fractionContext, minimumLevelCount, rule.damageFractionByLevel);
				if (!positive(rule.damageFraction)) throw std::runtime_error(fractionContext + " must be positive");
				ValidateTable(rule.damageFractionByLevel, fractionContext, positive, "must be positive");
				rule.radius = ParseFiniteFloat(entry.at("radius"), entryContext + ".radius");
				if (!positive(rule.radius)) throw std::runtime_error(entryContext + ".radius must be positive");
				rule.historySeconds = ParseFiniteFloat(entry.at("historySeconds"), entryContext + ".historySeconds");
				if (!positive(rule.historySeconds)) throw std::runtime_error(entryContext + ".historySeconds must be positive");
				rules.shieldRuptureRules.push_back(rule);
			}
		}
	}
}

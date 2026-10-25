#include "ModuleLoaderParsing.h"

#include <set>
#include <stdexcept>
#include <utility>

namespace vega::content
{
	namespace module_loader_detail
	{
		namespace
		{
			// Unknown fields are rejected so a misspelled key cannot silently drop part of a rule.
			void RejectUnknownFields(const Json& entry, const std::string& entryContext, const std::set<std::string>& allowedKeys)
			{
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
			}

			Json EpicEntries(const Json& object, const std::string& context, const char* key)
			{
				const Json entries = object.value(key, Json::array());
				if (!entries.is_array()) throw std::runtime_error(context + "." + key + " must be an array");
				return entries;
			}

			float PositiveFloat(const Json& object, const char* key, float fallback, const std::string& entryContext)
			{
				if (!object.contains(key)) return fallback;
				const float value = ParseFiniteFloat(object.at(key), entryContext + "." + key);
				if (value <= 0.f) throw std::runtime_error(entryContext + "." + key + " must be positive");
				return value;
			}

			int BoundedInt(const Json& object, const char* key, int fallback, int maximum, const std::string& entryContext)
			{
				if (!object.contains(key)) return fallback;
				const int value = ParsePositiveInt(object.at(key), entryContext + "." + key);
				if (value > maximum) throw std::runtime_error(entryContext + "." + key + " must be at most " + std::to_string(maximum));
				return value;
			}
		}

		// Epic Offense rule arrays. Prism Core and Blood Contract are main-package rules; Storm Capacitor, Alternating Current, Clot
		// Armor and Debt Collection are side-feature rules. Level tables follow the same rules as every other package entry.
		void ParseEpicOffenseRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleSizeVariant& variant, bool sideFeature)
		{
			ModuleEpicOffenseRules& rules = variant.epicOffense;
			std::size_t index = 0;
			for (const Json& entry : EpicEntries(object, context, "monochromeCoreRules"))
			{
				const std::string entryContext = context + ".monochromeCoreRules[" + std::to_string(index++) + "]";
				RejectUnknownFields(entry, entryContext, { "pairMultiplier", "pairMultiplierGrowth", "additionalAbilityBonus" });
				if (sideFeature) throw std::runtime_error(entryContext + " is main-package only");
				ModuleMonochromeCoreRule rule;
				rule.pairMultiplier = ParseLevelFloat(entry, "pairMultiplier", entryContext, minimumLevelCount, rule.pairMultiplierByLevel);
				if (rule.pairMultiplier <= 1.f) throw std::runtime_error(entryContext + " pairMultiplier must exceed one");
				ValidateTable(rule.pairMultiplierByLevel, entryContext, [](float value) { return value > 1.f; }, "pairMultiplier must exceed one");
				rule.additionalAbilityBonus = ParseFiniteFloat(entry.at("additionalAbilityBonus"), entryContext);
				if (rule.additionalAbilityBonus <= 0.f) throw std::runtime_error(entryContext + " additionalAbilityBonus must be positive");
				rules.monochromeCores.push_back(std::move(rule));
			}
			index = 0;
			for (const Json& entry : EpicEntries(object, context, "prismCoreRules"))
			{
				const std::string entryContext = context + ".prismCoreRules[" + std::to_string(index++) + "]";
				RejectUnknownFields(entry, entryContext, { "extraStacks", "statusDurationMultiplier", "statusDurationMultiplierGrowth" });
				if (sideFeature) throw std::runtime_error(entryContext + " is main-package only");
				if (!entry.contains("statusDurationMultiplier")) throw std::runtime_error(entryContext + ".statusDurationMultiplier is required");
				ModulePrismCoreRule rule;
				rule.extraStacks = BoundedInt(entry, "extraStacks", 1, 4, entryContext);
				rule.statusDurationMultiplier = ParseLevelFloat(entry, "statusDurationMultiplier", entryContext + ".statusDurationMultiplier", minimumLevelCount, rule.statusDurationMultiplierByLevel);
				ValidateTable(rule.statusDurationMultiplierByLevel, entryContext + ".statusDurationMultiplier", [](float value) { return value > 0.f; }, "must be positive");
				if (rule.statusDurationMultiplier <= 0.f) throw std::runtime_error(entryContext + ".statusDurationMultiplier must be positive");
				rules.prismCores.push_back(std::move(rule));
			}

			index = 0;
			for (const Json& entry : EpicEntries(object, context, "stormCapacitorRules"))
			{
				const std::string entryContext = context + ".stormCapacitorRules[" + std::to_string(index++) + "]";
				RejectUnknownFields(entry, entryContext, { "chargesRequired", "targetCount", "targetCountGrowth", "electricStacks", "radius", "primaryDamageFraction" });
				if (!sideFeature) throw std::runtime_error(entryContext + " is side-feature only");
				ModuleStormCapacitorRule rule;
				rule.chargesRequired = BoundedInt(entry, "chargesRequired", rule.chargesRequired, 10, entryContext);
				rule.targetCount = BoundedInt(entry, "targetCount", rule.targetCount, 10, entryContext);
				rule.electricStacks = BoundedInt(entry, "electricStacks", rule.electricStacks, 4, entryContext);
				rule.radius = PositiveFloat(entry, "radius", rule.radius, entryContext);
				rule.primaryDamageFraction = PositiveFloat(entry, "primaryDamageFraction", rule.primaryDamageFraction, entryContext);
				if (rule.primaryDamageFraction > 1.f) throw std::runtime_error(entryContext + " primaryDamageFraction must be in (0,1]");
				rules.stormCapacitors.push_back(rule);
			}

			index = 0;
			for (const Json& entry : EpicEntries(object, context, "alternatingCurrentRules"))
			{
				const std::string entryContext = context + ".alternatingCurrentRules[" + std::to_string(index++) + "]";
				RejectUnknownFields(entry, entryContext, { "damageMultiplier", "damageMultiplierGrowth" });
				if (!sideFeature) throw std::runtime_error(entryContext + " is side-feature only");
				ModuleAlternatingCurrentRule rule;
				rule.damageMultiplier = PositiveFloat(entry, "damageMultiplier", rule.damageMultiplier, entryContext);
				rules.alternatingCurrents.push_back(rule);
			}

			index = 0;
			for (const Json& entry : EpicEntries(object, context, "bloodContractRules"))
			{
				const std::string entryContext = context + ".bloodContractRules[" + std::to_string(index++) + "]";
				RejectUnknownFields(entry, entryContext, { "costFraction", "healFraction", "healWindowSeconds", "fullHealthMultiplier", "fullHealthMultiplierGrowth", "emptyHealthMultiplier", "emptyHealthMultiplierGrowth" });
				if (sideFeature) throw std::runtime_error(entryContext + " is main-package only");
				if (!entry.contains("fullHealthMultiplier") || !entry.contains("emptyHealthMultiplier")) throw std::runtime_error(entryContext + " needs fullHealthMultiplier and emptyHealthMultiplier");
				ModuleBloodContractRule rule;
				rule.costFraction = PositiveFloat(entry, "costFraction", rule.costFraction, entryContext);
				if (rule.costFraction >= 1.f) throw std::runtime_error(entryContext + ".costFraction must be below 1");
				if (entry.contains("healFraction"))
				{
					rule.healFraction = ParseFiniteFloat(entry.at("healFraction"), entryContext + ".healFraction");
					if (rule.healFraction < 0.f || rule.healFraction >= 1.f) throw std::runtime_error(entryContext + ".healFraction must be in [0, 1)");
				}
				rule.healWindowSeconds = PositiveFloat(entry, "healWindowSeconds", rule.healWindowSeconds, entryContext);
				rule.fullHealthMultiplier = ParseLevelFloat(entry, "fullHealthMultiplier", entryContext + ".fullHealthMultiplier", minimumLevelCount, rule.fullHealthMultiplierByLevel);
				rule.emptyHealthMultiplier = ParseLevelFloat(entry, "emptyHealthMultiplier", entryContext + ".emptyHealthMultiplier", minimumLevelCount, rule.emptyHealthMultiplierByLevel);
				ValidateTable(rule.fullHealthMultiplierByLevel, entryContext + ".fullHealthMultiplier", [](float value) { return value > 0.f; }, "must be positive");
				ValidateTable(rule.emptyHealthMultiplierByLevel, entryContext + ".emptyHealthMultiplier", [](float value) { return value > 0.f; }, "must be positive");
				if (rule.fullHealthMultiplier <= 0.f || rule.emptyHealthMultiplier <= 0.f) throw std::runtime_error(entryContext + " multipliers must be positive");
				rules.bloodContracts.push_back(std::move(rule));
			}

			index = 0;
			for (const Json& entry : EpicEntries(object, context, "clotArmorRules"))
			{
				const std::string entryContext = context + ".clotArmorRules[" + std::to_string(index++) + "]";
				RejectUnknownFields(entry, entryContext, { "armorPerCost", "seconds" });
				if (!sideFeature) throw std::runtime_error(entryContext + " is side-feature only");
				ModuleClotArmorRule rule;
				rule.armorPerCost = PositiveFloat(entry, "armorPerCost", rule.armorPerCost, entryContext);
				rule.seconds = PositiveFloat(entry, "seconds", rule.seconds, entryContext);
				rules.clotArmors.push_back(rule);
			}

			index = 0;
			for (const Json& entry : EpicEntries(object, context, "debtCollectionRules"))
			{
				const std::string entryContext = context + ".debtCollectionRules[" + std::to_string(index++) + "]";
				RejectUnknownFields(entry, entryContext, { "healWindowSeconds" });
				if (!sideFeature) throw std::runtime_error(entryContext + " is side-feature only");
				ModuleDebtCollectionRule rule;
				rule.healWindowSeconds = PositiveFloat(entry, "healWindowSeconds", rule.healWindowSeconds, entryContext);
				rules.debtCollections.push_back(rule);
			}
		}
	}
}

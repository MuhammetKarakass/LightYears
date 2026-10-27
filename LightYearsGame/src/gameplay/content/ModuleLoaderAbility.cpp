#include "ModuleLoaderParsing.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include <set>

namespace vega::content::module_loader_detail
{
	namespace
	{
		void Fields(const Json& object, const std::set<std::string>& allowed, const std::string& context)
		{
			if (!object.is_object()) throw std::runtime_error(context + " must be an object");
			for (const auto& field : object.items()) if (!allowed.count(field.key())) throw std::runtime_error(context + " unknown field " + field.key());
		}
		float Positive(const Json& object, const char* field, const std::string& context)
		{
			const float value = ParseFiniteFloat(object.at(field), context + "." + field);
			if (value <= 0.f) throw std::runtime_error(context + "." + field + " must be positive");
			return value;
		}
		AbilityActivationModifiers Result(const Json& object, const std::string& context)
		{
			Fields(object, { "mainEffectMultiplier", "cooldownMultiplier" }, context);
			return { Positive(object, "mainEffectMultiplier", context), Positive(object, "cooldownMultiplier", context) };
		}
	}
	AbilityModuleRules ParseAbilityModuleRules(const Json& rules, const std::string& id)
	{
		const std::string context = "Module " + id + " abilityModule";
		std::set<std::string> allowed{ "mainEffectMultiplier", "cooldownMultiplier" };
		if (id == "Module.AbilityModule.TimingProtocol") allowed.insert("timing");
		else if (id == "Module.AbilityModule.AutocastProtocol") allowed.insert("autocast");
		else if (id == "Module.AbilityModule.ElementCore") allowed.insert("elementCore");
		else if (id == "Module.AbilityModule.FortuneProtocol") allowed.insert("fortune");
		else if (id != "Module.AbilityModule.UltimateProtocol") throw std::runtime_error(context + " unknown ability module");
		Fields(rules, allowed, context);
		AbilityModuleRules values;
		values.mainEffectMultiplier = Positive(rules, "mainEffectMultiplier", context);
		values.cooldownMultiplier = Positive(rules, "cooldownMultiplier", context);
		if (allowed.count("timing"))
		{
			const auto& object = rules.at("timing");
			Fields(object, { "durationSeconds", "windowCenterFraction", "bigWindowSeconds", "smallWindowSeconds", "small", "big", "miss", "feedbackSeconds" }, context);
			TimingProtocolRules spec;
			spec.durationSeconds = Positive(object, "durationSeconds", context);
			spec.windowCenterFraction = Positive(object, "windowCenterFraction", context);
			spec.bigWindowSeconds = Positive(object, "bigWindowSeconds", context);
			spec.smallWindowSeconds = Positive(object, "smallWindowSeconds", context);
			spec.feedbackSeconds = Positive(object, "feedbackSeconds", context);
			spec.small = Result(object.at("small"), context + ".small"); spec.big = Result(object.at("big"), context + ".big"); spec.miss = Result(object.at("miss"), context + ".miss");
			const float halfWindow = spec.bigWindowSeconds / (2.f * spec.durationSeconds);
			if (spec.smallWindowSeconds > spec.bigWindowSeconds || spec.windowCenterFraction - halfWindow < 0.f || spec.windowCenterFraction + halfWindow > 1.f) throw std::runtime_error(context + " timing windows must be nested inside the bar");
			values.timing = spec;
		}
		if (allowed.count("autocast"))
		{
			const auto& object = rules.at("autocast"); Fields(object, { "recentDamageSeconds", "defaultRange" }, context);
			AutocastProtocolRules spec; spec.recentDamageSeconds = Positive(object, "recentDamageSeconds", context); spec.defaultRange = Positive(object, "defaultRange", context); values.autocast = spec;
		}
		if (allowed.count("elementCore"))
		{
			const auto& object = rules.at("elementCore"); Fields(object, { "elements" }, context);
			const auto& elements = object.at("elements");
			if (!elements.is_array() || elements.size() != 5) throw std::runtime_error(context + " requires the five damaging elements");
			ElementCoreRules spec;
			for (const auto& element : elements)
			{
				const auto tag = ParseTag(element, GameplayTagKind::DamageType, context);
				const List<GameplayTag> allowedElements{ DamageTypeSchema::Thermal, DamageTypeSchema::Cryo, DamageTypeSchema::Electric, DamageTypeSchema::Kinetic, DamageTypeSchema::Energy };
				if (std::find(allowedElements.begin(), allowedElements.end(), tag) == allowedElements.end() || std::find(spec.elements.begin(), spec.elements.end(), tag) != spec.elements.end()) throw std::runtime_error(context + " invalid or repeated element");
				spec.elements.push_back(tag);
			}
			values.elementCore = spec;
		}
		if (allowed.count("fortune"))
		{
			const auto& object = rules.at("fortune"); Fields(object, { "minimum", "maximum", "doubleRollChancePerLuck", "feedbackSeconds" }, context);
			FortuneProtocolRules spec; spec.minimum = Positive(object, "minimum", context); spec.maximum = Positive(object, "maximum", context); spec.doubleRollChancePerLuck = ParseFiniteFloat(object.at("doubleRollChancePerLuck"), context); spec.feedbackSeconds = Positive(object, "feedbackSeconds", context);
			if (spec.maximum < spec.minimum || spec.doubleRollChancePerLuck < 0.f || spec.doubleRollChancePerLuck > 1.f) throw std::runtime_error(context + " invalid random power interval or probability");
			values.fortune = spec;
		}
		return values;
	}
}

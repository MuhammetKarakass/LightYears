#include "ModuleLoaderParsing.h"

namespace vega::content::module_loader_detail
{
	void ParseSideRules(const Json& object, const std::string& context, ModuleRulePackage& rules, bool sideFeature)
	{
		const auto requireSide = [&]() { if (!sideFeature) throw std::runtime_error(context + " rule is side-feature only"); };
		const auto fields = [&](const Json& entry, std::initializer_list<const char*> allowed)
		{
			if (!entry.is_object()) throw std::runtime_error(context + " side rule must be an object");
			for (const auto& field : entry.items())
				if (std::none_of(allowed.begin(), allowed.end(), [&](const char* key) { return field.key() == key; })) throw std::runtime_error(context + " unknown side rule field: " + field.key());
		};
		if (object.contains("minimumZoneLevelBonus"))
		{
			requireSide();
			rules.minimumZoneLevelBonus = ParsePositiveInt(object.at("minimumZoneLevelBonus"), context);
			if (rules.minimumZoneLevelBonus > 2) throw std::runtime_error(context + " minimumZoneLevelBonus must be 1 or 2");
		}
		if (object.contains("cooldownRecoveryRules"))
		{
			requireSide();
			const Json& entries = object.at("cooldownRecoveryRules");
			if (!entries.is_array() || entries.empty()) throw std::runtime_error(context + " cooldownRecoveryRules must be nonempty array");
			for (const Json& entry : entries)
			{
				fields(entry, { "bonus", "damageFreeSeconds", "perEmptyAbilitySlot" });
				ModuleCooldownRecoveryRule rule;
				rule.bonus = ParseFiniteFloat(entry.at("bonus"), context);
				rule.damageFreeSeconds = entry.contains("damageFreeSeconds") ? ParseFiniteFloat(entry.at("damageFreeSeconds"), context) : 0.f;
				rule.perEmptyAbilitySlot = entry.value("perEmptyAbilitySlot", false);
				if (rule.bonus <= 0.f || rule.damageFreeSeconds < 0.f || (rule.damageFreeSeconds > 0.f && rule.perEmptyAbilitySlot)) throw std::runtime_error(context + " invalid cooldown recovery rule");
				rules.cooldownRecoveryRules.push_back(rule);
			}
		}
		if (object.contains("deferredDamage"))
		{
			requireSide();
			const Json& entry = object.at("deferredDamage");
			fields(entry, { "fraction", "seconds" });
			DeferredDamageSpec rule{ ParseFiniteFloat(entry.at("fraction"), context), ParseFiniteFloat(entry.at("seconds"), context) };
			if (rule.fraction <= 0.f || rule.fraction >= 1.f || rule.seconds <= 0.f) throw std::runtime_error(context + " invalid deferred damage");
			rules.deferredDamage = rule;
		}
		if (object.contains("momentumExtension"))
		{
			requireSide();
			const Json& entry = object.at("momentumExtension");
			fields(entry, { "seconds", "maximumSeconds" });
			ModuleMomentumExtension rule{ ParseFiniteFloat(entry.at("seconds"), context), ParseFiniteFloat(entry.at("maximumSeconds"), context) };
			if (rule.seconds <= 0.f || rule.maximumSeconds < rule.seconds) throw std::runtime_error(context + " invalid momentum extension");
			rules.momentumExtension = rule;
		}
		if (object.contains("primaryShareOfAbilityBonus"))
		{
			requireSide();
			rules.primaryShareOfAbilityBonus = ParseFiniteFloat(object.at("primaryShareOfAbilityBonus"), context);
			if (rules.primaryShareOfAbilityBonus <= 0.f || rules.primaryShareOfAbilityBonus > 1.f) throw std::runtime_error(context + " invalid Primary bonus share");
		}
		if (object.contains("primarySplitFire"))
		{
			requireSide();
			if (!object.at("primarySplitFire").is_boolean() || !object.at("primarySplitFire").get<bool>()) throw std::runtime_error(context + " primarySplitFire must be true");
			rules.primarySplitFire = true;
		}
		if (object.contains("distinctFamilyMainEffectBonus"))
		{
			requireSide();
			rules.distinctFamilyMainEffectBonus = ParseFiniteFloat(object.at("distinctFamilyMainEffectBonus"), context);
			if (rules.distinctFamilyMainEffectBonus <= 0.f || rules.distinctFamilyMainEffectBonus > 1.f) throw std::runtime_error(context + " invalid distinct family main effect bonus");
		}
	}
}

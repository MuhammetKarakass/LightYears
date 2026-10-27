#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "gameplay/module/ModuleSideFeature.h"
#include "gameplay/module/ModuleDescriptionValues.h"
#include <algorithm>
#include <cstdio>

namespace vega
{
	namespace
	{
		std::string FormatRules(std::string text, const ModuleRulePackage& rules, int level, const AbilityModuleRules* abilityModule = nullptr, const UniqueModuleRule* unique = nullptr)
		{
			const ModuleDescriptionValueVisitor replace = [&text](const std::string& key, float value)
			{
				char buffer[32]; std::snprintf(buffer, sizeof(buffer), "%.6g", value);
				std::string replacement{ buffer }; std::replace(replacement.begin(), replacement.end(), '.', ',');
				const std::string token = "{" + key + "}"; std::size_t at = 0;
				while ((at = text.find(token, at)) != std::string::npos) { text.replace(at, token.size(), replacement); at += replacement.size(); }
			};
			VisitModuleDescriptionValues(rules, level, replace);
			if (abilityModule) VisitAbilityModuleDescriptionValues(*abilityModule, replace);
			if (unique) VisitUniqueModuleDescriptionValues(*unique, replace);
			return text;
		}
	}
	std::string FormatModuleDescription(const ModuleDefinition& definition, const ModuleSizeVariant& variant, int level) { return FormatRules(definition.description, variant, level, definition.abilityModule ? &*definition.abilityModule : nullptr, definition.unique ? &*definition.unique : nullptr); }
	std::string FormatModuleSideFeatureDescription(const ModuleSideFeatureDefinition& feature, int level) { return FormatRules(feature.description, feature.rules, level); }
}

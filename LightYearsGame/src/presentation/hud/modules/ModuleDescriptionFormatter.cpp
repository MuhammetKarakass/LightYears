#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "gameplay/module/ModuleSideFeature.h"
#include "gameplay/module/ModuleDescriptionValues.h"
#include <algorithm>
#include <cstdio>

namespace ly
{
	namespace
	{
		std::string FormatRules(std::string text, const ModuleRulePackage& rules, int level)
		{
			VisitModuleDescriptionValues(rules, level, [&text](const std::string& key, float value)
			{
				char buffer[32]; std::snprintf(buffer, sizeof(buffer), "%.6g", value);
				std::string replacement{ buffer }; std::replace(replacement.begin(), replacement.end(), '.', ',');
				const std::string token = "{" + key + "}"; std::size_t at = 0;
				while ((at = text.find(token, at)) != std::string::npos) { text.replace(at, token.size(), replacement); at += replacement.size(); }
			});
			return text;
		}
	}
	std::string FormatModuleDescription(const ModuleDefinition& definition, const ModuleSizeVariant& variant, int level) { return FormatRules(definition.description, variant, level); }
	std::string FormatModuleSideFeatureDescription(const ModuleSideFeatureDefinition& feature, int level) { return FormatRules(feature.description, feature.rules, level); }
}

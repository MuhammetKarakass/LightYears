#pragma once
#include "gameplay/module/ModuleDefinition.h"
#include <functional>

namespace vega
{
	using ModuleDescriptionValueVisitor = std::function<void(const std::string&, float)>;
	void VisitModuleDescriptionValues(const ModuleRulePackage& rules, int level, const ModuleDescriptionValueVisitor& visitor);
	void VisitUniqueModuleDescriptionValues(const UniqueModuleRule& rules, const ModuleDescriptionValueVisitor& visitor);
	void VisitAbilityModuleDescriptionValues(const AbilityModuleRules& rules, const ModuleDescriptionValueVisitor& visitor);
	void ValidateModuleDescriptionTemplate(const std::string& text, const ModuleRulePackage& rules, const std::string& context, const AbilityModuleRules* abilityModule = nullptr, const UniqueModuleRule* unique = nullptr);
}

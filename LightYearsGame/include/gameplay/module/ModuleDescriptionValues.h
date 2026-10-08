#pragma once
#include "gameplay/module/ModuleDefinition.h"
#include <functional>

namespace ly
{
	using ModuleDescriptionValueVisitor = std::function<void(const std::string&, float)>;
	void VisitModuleDescriptionValues(const ModuleRulePackage& rules, int level, const ModuleDescriptionValueVisitor& visitor);
	void ValidateModuleDescriptionTemplate(const std::string& text, const ModuleRulePackage& rules, const std::string& context);
}

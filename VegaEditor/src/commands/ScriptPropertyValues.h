#pragma once
#include "editor/EditorCommands.h"
namespace vega
{
	bool ParsePropertyValue(const ly::reflect::PropertyInfo&, const std::vector<std::string>&, bool, ly::reflect::PropertyValue&, float&);
	bool PropertyEquals(const ly::reflect::PropertyValue&, const ly::reflect::PropertyValue&, float);
} // namespace vega

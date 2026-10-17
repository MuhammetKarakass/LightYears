#pragma once
#include "editor/EditorCommands.h"
namespace vega
{
	bool ParsePropertyValue(const vega::reflect::PropertyInfo&, const std::vector<std::string>&, bool, vega::reflect::PropertyValue&, float&);
	bool PropertyEquals(const vega::reflect::PropertyValue&, const vega::reflect::PropertyValue&, float);
} // namespace vega

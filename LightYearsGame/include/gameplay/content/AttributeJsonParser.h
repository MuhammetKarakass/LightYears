#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/JsonDocumentLoader.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace ly::content::AttributeJsonParser
{
	using Json = JsonDocumentLoader::Json;

	inline sas::AttributeModifierOperation ParseOperation(
		const std::string& value,
		const std::string& errorContext = ""
	)
	{
		if (value == "Add") return sas::AttributeModifierOperation::Add;
		if (value == "Multiply") return sas::AttributeModifierOperation::Multiply;
		if (value == "Override") return sas::AttributeModifierOperation::Override;
		const std::string prefix = errorContext.empty() ? "" : errorContext + ": ";
		throw std::runtime_error(prefix + "Unknown attribute modifier operation: " + value);
	}

	inline sas::AttributeScalingRule ParseScalingRule(
		const Json& object,
		const std::string& contextPath = "scalingRule"
	)
	{
		const std::string prefix = contextPath.empty() ? "" : contextPath + ".";
		if (!object.is_object())
		{
			throw std::runtime_error((contextPath.empty() ? "Scaling rule" : contextPath) + " must be an object");
		}
		if (!object.contains("targetAttributeId") || !object["targetAttributeId"].is_string() ||
			object["targetAttributeId"].get<std::string>().empty())
		{
			throw std::runtime_error(prefix + "targetAttributeId is required and must be a non-empty string");
		}
		if (!object.contains("sourceAttributeId") || !object["sourceAttributeId"].is_string() ||
			object["sourceAttributeId"].get<std::string>().empty())
		{
			throw std::runtime_error(prefix + "sourceAttributeId is required and must be a non-empty string");
		}
		if (!object.contains("operation") || !object["operation"].is_string())
		{
			throw std::runtime_error(prefix + "operation is required and must be a string");
		}
		if (!object.contains("coefficient"))
		{
			throw std::runtime_error(prefix + "coefficient is required");
		}
		if (!object["coefficient"].is_number())
		{
			throw std::runtime_error(prefix + "coefficient must be a number");
		}
		const float coeff = object["coefficient"].get<float>();
		if (!std::isfinite(coeff))
		{
			throw std::runtime_error(prefix + "coefficient must be finite");
		}
		return sas::AttributeScalingRule{
			sas::AttributeId{ object["targetAttributeId"].get<std::string>() },
			sas::AttributeId{ object["sourceAttributeId"].get<std::string>() },
			ParseOperation(object["operation"].get<std::string>(), contextPath),
			coeff
		};
	}

	inline List<sas::AttributeScalingRule> ParseScalingRules(
		const Json& values,
		const std::string& contextPath = "scalingRules"
	)
	{
		if (!values.is_array())
		{
			throw std::runtime_error(contextPath + " must be an array");
		}
		List<sas::AttributeScalingRule> rules;
		rules.reserve(values.size());
		for (std::size_t i = 0; i < values.size(); ++i)
		{
			rules.push_back(ParseScalingRule(values[i], contextPath + "[" + std::to_string(i) + "]"));
		}
		return rules;
	}
}

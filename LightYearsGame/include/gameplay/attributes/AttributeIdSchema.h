#pragma once

#include "attributes/AttributeId.h"

#include <cctype>
#include <string>
#include <string_view>

namespace ly
{
	// This is a structural boundary, not a registry of shipped attributes.
	// Feature contracts and catalogs remain the owners of which IDs exist.
	// Ability definitions may carry Common.* values shared by a consumer and
	// Ability.* values owned by one family; GameAbilityDefinitionValidator
	// enforces that scoped rule at the ability boundary.
	class AttributeIdSchema final
	{
	public:
		static bool Validate(
			const sas::AttributeId& id,
			std::string* failureReason = nullptr
		)
		{
			if (!id.IsValid())
			{
				return Fail(failureReason, "Attribute ID must not be empty.");
			}

			const std::string_view name = id.GetName();
			if (name.rfind("Attribute.", 0) == 0)
			{
				return Fail(failureReason, "Attribute ID must not use the legacy Attribute. prefix.");
			}

			if (name.front() == '.' || name.back() == '.' || name.find("..") != std::string_view::npos)
			{
				return Fail(failureReason, "Attribute ID must use non-empty dot-separated segments.");
			}

			for (const unsigned char character : name)
			{
				if (std::isalnum(character) == 0 && character != '_' && character != '.')
				{
					return Fail(failureReason, "Attribute ID contains an invalid character.");
				}
			}

			return true;
		}

		static bool IsInNamespace(
			const sas::AttributeId& id,
			std::string_view namespaceName
		)
		{
			if (!id.IsValid() || namespaceName.empty())
			{
				return false;
			}

			if (namespaceName.back() == '.')
			{
				namespaceName.remove_suffix(1);
			}

			const std::string_view name = id.GetName();
			return name.size() > namespaceName.size() &&
				name.compare(0, namespaceName.size(), namespaceName) == 0 &&
				name[namespaceName.size()] == '.';
		}

	private:
		static bool Fail(std::string* failureReason, const char* message)
		{
			if (failureReason)
			{
				*failureReason = message;
			}
			return false;
		}
	};
}

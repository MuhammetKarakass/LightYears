#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace sas
{
	class AttributeId
	{
	public:
		AttributeId() = default;
		explicit AttributeId(std::string_view name)
			: mName{ name }
		{
		}

		bool IsValid() const noexcept
		{
			return !mName.empty();
		}

		std::string_view GetName() const noexcept
		{
			return mName;
		}

		friend bool operator==(const AttributeId& left, const AttributeId& right) noexcept
		{
			return left.mName == right.mName;
		}

		friend bool operator!=(const AttributeId& left, const AttributeId& right) noexcept
		{
			return !(left == right);
		}

		friend bool operator<(const AttributeId& left, const AttributeId& right) noexcept
		{
			return left.mName < right.mName;
		}

	private:
		std::string mName;
	};

	struct AttributeIdHash
	{
		std::size_t operator()(const AttributeId& id) const noexcept
		{
			return std::hash<std::string_view>{}(id.GetName());
		}
	};
}

#pragma once

#include <string>
#include <utility>

namespace sas
{
	// A content identity is distinct from a GameplayTag. Grammar and domain
	// validation remain owned by the game content schema at the load boundary.
	class ContentId
	{
	public:
		ContentId() = default;
		ContentId(const char* value)
			: mValue{ value ? value : "" }
		{
		}
		ContentId(std::string value)
			: mValue{ std::move(value) }
		{
		}

		bool IsValid() const { return !mValue.empty(); }
		const std::string& ToString() const { return mValue; }

		friend bool operator==(const ContentId& left, const ContentId& right) noexcept
		{
			return left.mValue == right.mValue;
		}

		friend bool operator!=(const ContentId& left, const ContentId& right) noexcept
		{
			return !(left == right);
		}

	private:
		std::string mValue;
	};
}

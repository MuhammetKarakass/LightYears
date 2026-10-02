#pragma once

#include <string>
#include <vector>

namespace ly::visual
{
	// What a hot reload did. Failed files keep their previous working version and appear in errors.
	struct ReloadReport
	{
		std::vector<std::string> reloaded;
		std::vector<std::string> errors;

		bool Empty() const
		{
			return reloaded.empty() && errors.empty();
		}

		void Append(const ReloadReport& other)
		{
			reloaded.insert(reloaded.end(), other.reloaded.begin(), other.reloaded.end());
			errors.insert(errors.end(), other.errors.begin(), other.errors.end());
		}
	};
}

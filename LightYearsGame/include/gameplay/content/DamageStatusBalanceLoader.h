#pragma once

#include "gameConfigs/combat/DamageTypeConfig.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class DamageStatusBalanceLoader
	{
	public:
		struct Result
		{
			DamageStatusBalance balance;
			std::string error;

			bool Succeeded() const noexcept
			{
				return error.empty();
			}
		};

		static Result LoadFromFile(const std::filesystem::path& filePath);
	};
}

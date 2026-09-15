#pragma once

#include "gameplay/enemy/EnemyCombatProfile.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class EnemyCombatProfileLoader
	{
	public:
		struct Result
		{
			List<EnemyCombatProfile> profiles;
			std::string error;

			bool Succeeded() const noexcept { return error.empty(); }
		};

		static Result LoadFromFile(const std::filesystem::path& filePath);
	};
}

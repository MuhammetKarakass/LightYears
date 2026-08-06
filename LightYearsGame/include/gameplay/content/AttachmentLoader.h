#pragma once

#include "gameplay/attachment/AttachmentDefinition.h"

#include <filesystem>
#include <string>

namespace ly::content
{
	class AttachmentLoader
	{
	public:
		struct Result
		{
			List<AttachmentDefinition> definitions;
			std::string error;

			bool Succeeded() const noexcept
			{
				return error.empty();
			}
		};

		static Result LoadFromFile(
			const std::filesystem::path& filePath
		);
	};
}

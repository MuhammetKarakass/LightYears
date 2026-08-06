#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace ly
{
	class JsonDocumentLoader
	{
	public:
		using Json = nlohmann::json;

		struct Result
		{
			std::optional<Json> document;
			std::string error;

			bool Succeeded() const noexcept
			{
				return document.has_value();
			}
		};

		static Result LoadFromFile(
			const std::filesystem::path& filePath
		);

		static Result Parse(
			std::string_view source,
			std::string_view sourceName = "<memory>"
		);
	};
}
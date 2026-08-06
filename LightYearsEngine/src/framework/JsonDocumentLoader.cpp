#include "framework/JsonDocumentLoader.h"

#include <fstream>
#include <sstream>

namespace ly
{
	JsonDocumentLoader::Result JsonDocumentLoader::LoadFromFile(
		const std::filesystem::path& filePath)
	{
		std::ifstream input{
			filePath,
			std::ios::in | std::ios::binary
		};

		if (!input.is_open())
		{
			return Result{
				std::nullopt,
				"Failed to open JSON file: " + filePath.string()
			};
		}

		std::ostringstream source;
		source << input.rdbuf();

		if (input.fail() && !input.eof())
		{
			return Result{
				std::nullopt,
				"Failed to read JSON file: " + filePath.string()
			};
		}

		return Parse(source.str(), filePath.string());
	}

	JsonDocumentLoader::Result JsonDocumentLoader::Parse(
		std::string_view source,
		std::string_view sourceName)
	{
		try
		{
			return Result{
				nlohmann::json::parse(source),
				{}
			};
		}
		catch (const nlohmann::json::parse_error& exception)
		{
			return Result{
				std::nullopt,
				"Failed to parse JSON '" +
				std::string{ sourceName } +
				"': " +
				exception.what()
			};
		}
	}
}
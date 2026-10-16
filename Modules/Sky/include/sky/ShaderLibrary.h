#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include <SFML/Graphics.hpp>

#include "sky/ColorRamp.h"
#include "sky/ReloadReport.h"

namespace vega::sky
{
	// Fragment shaders loaded from <dir>/<name>.frag. Reload keeps the previous shader when a file fails to compile.
	class ShaderLibrary
	{
	public:
		bool LoadDirectory(const std::filesystem::path& dir, std::string& error);
		void Reload(bool force, ReloadReport& report);

		sf::Shader* Find(const std::string& name);

	private:
		struct Entry
		{
			std::filesystem::path file;
			std::unique_ptr<sf::Shader> shader;
		};

		std::filesystem::path mDirectory;
		std::map<std::string, Entry> mShaders;
		std::map<std::filesystem::path, std::filesystem::file_time_type> mSeenStamps;
	};

	// Uploads count evenly spaced samples of the ramp as a vec3 uniform array (0..1 sRGB).
	void SetRampUniform(sf::Shader& shader, const std::string& name, const ColorRamp& ramp, int count);

	// Draws a rect whose texCoords equal its positions, so shaders see the layer pixel coordinates.
	void DrawShaderQuad(sf::RenderTarget& target, sf::Shader& shader, const sf::FloatRect& rect, const sf::BlendMode& blend = sf::BlendAlpha);
}

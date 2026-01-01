#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"

namespace ly
{
	class ShaderManager
	{
	public:
		static ShaderManager& GetShaderManager();

		bool LoadShader(const std::string& name, const std::string& fragmentPath);
		bool LoadShader(const std::string& name, const std::string& fragmentPath, const std::string& vertexPath);

		sf::Shader* GetShader(const std::string& name);

		void LoadDefaultShaders();

		void CleanUp();

	private:

		ShaderManager();

		static unique_ptr<ShaderManager> mShaderManager;
		Dictionary<std::string, unique_ptr<sf::Shader>> mShader;
	};
}
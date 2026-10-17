#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"

namespace vega
{
	class EngineContext;
	class ShaderManager
	{
	public:
		static ShaderManager& GetShaderManager();
		static void ShutdownShaderManager();

		bool LoadShader(const std::string& name, const std::string& fragmentPath);
		bool LoadShader(const std::string& name, const std::string& fragmentPath, const std::string& vertexPath);

		sf::Shader* GetShader(const std::string& name);

		void LoadDefaultShaders();
		void LoadDefaultShaders(EngineContext& context);

		void CleanUp();

	private:

		ShaderManager();

		static unique_ptr<ShaderManager> mShaderManager;
		Dictionary<std::string, unique_ptr<sf::Shader>> mShader;
	};
}

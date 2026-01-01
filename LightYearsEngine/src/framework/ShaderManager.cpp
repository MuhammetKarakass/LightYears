#include "framework/ShaderManager.h"

namespace ly
{
	unique_ptr<ShaderManager> ShaderManager::mShaderManager = nullptr;

	ShaderManager& ShaderManager::GetShaderManager()
	{
		if (mShaderManager == nullptr)
		{
			mShaderManager = unique_ptr<ShaderManager>(new ShaderManager());
		}
		return *mShaderManager;
	}

	bool ShaderManager::LoadShader(const std::string& name, const std::string& fragmentPath)
	{
		if(!sf::Shader::isAvailable())
			return false;

		auto shader = std::make_unique<sf::Shader>();

		if (!shader->loadFromFile(fragmentPath, sf::Shader::Type::Fragment))
		{
			LOG("ERROR: Failed to load fragment shader: %s", fragmentPath.c_str());
			return false;
		}

		mShader[name] = std::move(shader);
		return true;
	}

	bool ShaderManager::LoadShader(const std::string& name, const std::string& fragmentPath, const std::string& vertexPath)
	{
		if(!sf::Shader::isAvailable())
			return false;

		auto shader = std::make_unique<sf::Shader>();
		if (!shader->loadFromFile(vertexPath, sf::Shader::Type::Vertex) ||
			!shader->loadFromFile(fragmentPath, sf::Shader::Type::Fragment))
		{
			LOG("ERROR: Failed to load vertex/fragment shader: %s , %s", vertexPath.c_str(), fragmentPath.c_str());
			return false;
		}
		mShader[name] = std::move(shader);
		return true;
	}

	sf::Shader* ShaderManager::GetShader(const std::string& name)
	{
		for (auto& it = mShader.begin(); it != mShader.end(); ++it)
		{
			if (it->first == name)
			{
				return it->second.get();
			}
		}
		return nullptr;
	}

	void ShaderManager::LoadDefaultShaders()
	{
		if(!sf::Shader::isAvailable())
			return;

		LoadShader("simple_color", "SpaceShooterRedux/Shaders/simple_color.frag");
	}

	void ShaderManager::CleanUp()
	{
		mShader.clear();
	}

	ShaderManager::ShaderManager():
		mShader{}
	{
	}
}
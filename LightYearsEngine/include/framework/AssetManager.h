#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

namespace ly
{
	class AssetManager
	{

	public:
		static AssetManager& GetAssetManager();
		shared_ptr<sf::Texture> LoadTexture(const std::string& texturePath);
		shared_ptr<sf::Font> LoadFont(const std::string& fontPath);
		shared_ptr<sf::SoundBuffer> LoadSoundBuffer(const std::string& soundPath);
		shared_ptr<sf::Shader> LoadShader(const std::string& fragmentPath, const std::string& vertexPath = "");
		shared_ptr<sf::Texture> GetDefaultTexture() ;

		void CleanCycle();
		void SetAssetRootDirectory(const std::string& rootDirectory) { mAssetRootDirectory = rootDirectory; }
		std::string GetAssetRootDirectory() const { return mAssetRootDirectory; }
	protected:
		AssetManager();

	private:
		static unique_ptr<AssetManager> assetManager;
		Dictionary<std::string,shared_ptr<sf::Texture>> mLoadedTextureMap;
		Dictionary<std::string, shared_ptr<sf::Font>> mLoadedFontMap;
		Dictionary<std::string, shared_ptr<sf::SoundBuffer>> mLoadedSoundBufferMap;
		Dictionary<std::string, shared_ptr<sf::Shader>> mLoadedShaderMap;
		std::string mAssetRootDirectory;

		shared_ptr<sf::Texture> mDefaultTexture;
	};
}
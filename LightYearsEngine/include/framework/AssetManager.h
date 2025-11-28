#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>

namespace ly
{
	class AssetManager
	{

	public:
		static AssetManager& GetAssetManager();
		shared_ptr<sf::Texture> LoadTexture(const std::string& texturePath);
		shared_ptr<sf::Font> LoadFont(const std::string& fontPath);
		void CleanCycle();
		void SetAssetRootDirectory(const std::string& rootDirectory) { mAssetRootDirectory = rootDirectory; }

	protected:
		AssetManager();

	private:
		static unique_ptr<AssetManager> assetManager;
		Dictionary<std::string,shared_ptr<sf::Texture>> mLoadedTextureMap;
		Dictionary<std::string, shared_ptr<sf::Font>> mLoadedFontMap;
		std::string mAssetRootDirectory;
	};
}
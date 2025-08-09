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
		void CleanCycle();

	protected:
		AssetManager();

	private:
		static unique_ptr<AssetManager> assetManager;
		Dictionary<std::string,shared_ptr<sf::Texture>> mLoadedTextureMap;
	};
}
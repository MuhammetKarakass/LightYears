#include "framework/AssetManager.h"

namespace ly
{
	unique_ptr<AssetManager> AssetManager::assetManager = nullptr;

	AssetManager::AssetManager()
	{
	}

	AssetManager& AssetManager::GetAssetManager()
	{
		if (!assetManager)
		{
			assetManager = unique_ptr<AssetManager>{new AssetManager};
		}
		return *assetManager;
	}
	shared_ptr<sf::Texture> AssetManager::LoadTexture(const std::string& texturePath)
	{
		auto found = mLoadedTextureMap.find(texturePath);
		if (found!=mLoadedTextureMap.end())
		{
			return found->second;
		}

		shared_ptr<sf::Texture> newTexture{ new sf::Texture };
		if (newTexture->loadFromFile(texturePath))
		{
			mLoadedTextureMap.insert({ texturePath, newTexture });
			return newTexture;
		}

		return shared_ptr<sf::Texture> {nullptr};
	}
	void AssetManager::CleanCycle()
	{
		for(auto iter = mLoadedTextureMap.begin(); iter != mLoadedTextureMap.end();)
		{
            if (iter->second.unique())//only asset manager holds the reference
			{
				LOG("Removed unused texture: %s", iter->first.c_str());
				iter = mLoadedTextureMap.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
}
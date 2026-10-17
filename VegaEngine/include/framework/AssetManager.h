#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

namespace vega
{
	class EngineContext;
	class AssetManager
	{

	public:
		// Deprecated: forwards to EngineContext::Current().Assets().
		static AssetManager& GetAssetManager();
		static void ShutdownAssetManager();
		shared_ptr<sf::Texture> LoadTexture(const std::string& texturePath);
		shared_ptr<sf::Font> LoadFont(const std::string& fontPath);
		shared_ptr<sf::SoundBuffer> LoadSoundBuffer(const std::string& soundPath);
		shared_ptr<sf::Shader> LoadShader(const std::string& fragmentPath, const std::string& vertexPath = "");
		shared_ptr<sf::Texture> GetDefaultTexture() ;

		void CleanCycle();
		void Shutdown();
		void SetAssetRootDirectory(const std::string& rootDirectory);
		std::string GetAssetRootDirectory() const { return mAssetRootDirectory; }
	protected:
		explicit AssetManager(EngineContext& context);

	private:
		friend class EngineContext;
		EngineContext& mContext;
		Dictionary<std::string,shared_ptr<sf::Texture>> mLoadedTextureMap;
		Dictionary<std::string, shared_ptr<sf::Font>> mLoadedFontMap;
		Dictionary<std::string, shared_ptr<sf::SoundBuffer>> mLoadedSoundBufferMap;
		Dictionary<std::string, shared_ptr<sf::Shader>> mLoadedShaderMap;
		// Consecutive clean cycles each cached asset has gone without an outside owner, per asset kind.
		Dictionary<std::string, int> mIdleTextureCycles;
		Dictionary<std::string, int> mIdleFontCycles;
		Dictionary<std::string, int> mIdleSoundBufferCycles;
		Dictionary<std::string, int> mIdleShaderCycles;
		std::string mAssetRootDirectory;

		shared_ptr<sf::Texture> mDefaultTexture;
	};
}

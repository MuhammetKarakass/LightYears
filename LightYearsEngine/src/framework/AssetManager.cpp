#include "framework/AssetManager.h"
#include "framework/debug/Log.h"

#include <chrono>

namespace ly
{
	namespace
	{
		// Hitch diagnostics: every cache miss is a synchronous disk read (and, for
		// shaders, a driver compile) on the game thread. Logging them shows which
		// assets are reloaded after CleanCycle evicts them.
		using AssetClock = std::chrono::steady_clock;

		void LogAssetMiss(const char* kind, const std::string& path, AssetClock::time_point start, bool success)
		{
			const double ms = std::chrono::duration<double, std::milli>(AssetClock::now() - start).count();
			LY_CORE_INFO("AssetMiss %s %s %.2fms%s", kind, path.c_str(), ms, success ? "" : " FAILED");
		}
	}

	// Singleton deseninin statik (static) örneðini (instance) baþlangýçta null olarak ayarla.
	// Bu, GetAssetManager ilk çaðrýldýðýnda oluþturulacak.
	unique_ptr<AssetManager> AssetManager::assetManager = nullptr;

	// AssetManager'ýn kurucu fonksiyonu (constructor).
	// Baþlangýçta asset'lerin bulunduðu kök dizin boþtur.
	AssetManager::AssetManager():
	mAssetRootDirectory{}
	{
	}

	// Singleton örneðine eriþim saðlayan statik fonksiyon.
	AssetManager& AssetManager::GetAssetManager()
	{
		// Eðer assetManager daha önce oluþturulmamýþsa...
		if (!assetManager)
		{
			// ...yeni bir AssetManager nesnesi oluþtur ve unique_ptr içine al.
			// Bu "lazy initialization" (tembel baþlangýç) olarak bilinir.
			assetManager = unique_ptr<AssetManager>{new AssetManager};
		}
		// Oluþturulmuþ olan tekil (singleton) örneði referans olarak döndür.
		return *assetManager;
	}

	void AssetManager::ShutdownAssetManager()
	{
		if (assetManager)
		{
			assetManager->Shutdown();
			assetManager.reset();
		}
	}
	// Verilen yoldan bir texture y?kleyen veya ?nbellekten (cache) getiren fonksiyon.
	shared_ptr<sf::Texture> AssetManager::LoadTexture(const std::string& texturePath)
	{
		// 1. Texture'?n daha ?nce y?klenip y?klenmedi?ini kontrol et.
		// mLoadedTextureMap, y?klenmi? texture'lar? ve yollar?n? tutan bir haritad?r.
		auto found = mLoadedTextureMap.find(texturePath);
		
		// 2. E?er haritada bulunduysa...
		if (found!=mLoadedTextureMap.end())
		{
			// ...diskten tekrar y?klemek yerine haritadaki kopyay? (shared_ptr) d?nd?r.
			return found->second;
		}

		// 3. E?er haritada bulunamad?ysa, yeni bir texture olu?tur.
		const auto missStart = AssetClock::now();
		shared_ptr<sf::Texture> newTexture{ new sf::Texture };
		
		// 4. Texture'? diskten y?klemeyi dene. K?k dizin + verilen yolu birle?tir.
		std::string fullPath = mAssetRootDirectory + texturePath;
		const bool textureLoaded = newTexture->loadFromFile(fullPath);
		LogAssetMiss("texture", texturePath, missStart, textureLoaded);
		if (textureLoaded)
		{
			// 5. Y?kleme ba?ar?l?ysa, yeni texture'? haritaya ekle.
			mLoadedTextureMap.insert({ texturePath, newTexture });
			// Ve yeni olu?turulan shared_ptr'? d?nd?r.
			return newTexture;
		}

		// 6. Y?kleme ba?ar?s?z olursa, hata mesaj? g?ster ve bo? bir shared_ptr d?nd?r.
		LOG("Failed to load image\n    Provided path: %s\n    Absolute path: %s\nReason: No such file or directory", 
			texturePath.c_str(), fullPath.c_str());
		return shared_ptr<sf::Texture> {nullptr};
	}

	shared_ptr<sf::Font> AssetManager::LoadFont(const std::string& fontPath)
	{

		auto found = mLoadedFontMap.find(fontPath);

		if (found != mLoadedFontMap.end())
		{
			return found->second;
		}

		const auto missStart = AssetClock::now();
		shared_ptr<sf::Font> newFont{ new sf::Font };

		std::string fullPath = mAssetRootDirectory + fontPath;
		const bool fontLoaded = newFont->openFromFile(fullPath);
		LogAssetMiss("font", fontPath, missStart, fontLoaded);
		if (fontLoaded)
		{
			mLoadedFontMap.insert({ fontPath, newFont });
			return newFont;
		}
		LOG("Failed to load font (failed to open file): No such file or directory\n  Provided path: %s\n    Absolute path: %s", 
			fontPath.c_str(), fullPath.c_str());
		return shared_ptr<sf::Font>{nullptr};
	}

	shared_ptr<sf::SoundBuffer> AssetManager::LoadSoundBuffer(const std::string& soundPath)
	{
		auto found = mLoadedSoundBufferMap.find(soundPath);
		if (found != mLoadedSoundBufferMap.end())
		{
			return found->second;
		}

		const auto missStart = AssetClock::now();
		shared_ptr<sf::SoundBuffer> newSoundBuffer{ new sf::SoundBuffer };

		const bool soundLoaded = newSoundBuffer->loadFromFile(mAssetRootDirectory + soundPath);
		LogAssetMiss("sound", soundPath, missStart, soundLoaded);
		if (soundLoaded)
		{
			mLoadedSoundBufferMap.insert({ soundPath, newSoundBuffer });
			return newSoundBuffer;
		}
		return shared_ptr<sf::SoundBuffer>{nullptr};
	}

	shared_ptr<sf::Shader> AssetManager::LoadShader(const std::string& fragmentPath, const std::string& vertexPath)
	{
		if(!sf::Shader::isAvailable())
			return shared_ptr<sf::Shader>{ nullptr };

		std::string shaderKey = fragmentPath + "|" + vertexPath;
		auto found = mLoadedShaderMap.find(shaderKey);
		if (found != mLoadedShaderMap.end())
		{
			return found->second;
		}

		const auto missStart = AssetClock::now();
		shared_ptr<sf::Shader> newShader{ new sf::Shader };
		bool success = false;

		if (vertexPath.empty())
		{
			success = newShader->loadFromFile(mAssetRootDirectory + fragmentPath, sf::Shader::Type::Fragment);
		}
		else
		{
			success = newShader->loadFromFile(mAssetRootDirectory + vertexPath, mAssetRootDirectory + fragmentPath);
		}

		LogAssetMiss("shader", shaderKey, missStart, success);
		if (success)
		{
			mLoadedShaderMap.insert({ shaderKey, newShader });
			return newShader;
		}
		return shared_ptr<sf::Shader>{ nullptr};
	}

	shared_ptr<sf::Texture> AssetManager::GetDefaultTexture()
	{

		if (!mDefaultTexture)
		{
			mDefaultTexture = std::make_shared<sf::Texture>();
			if (!mDefaultTexture->resize(sf::Vector2u(1, 1)))
			{
				 mDefaultTexture.reset();
				return nullptr;
			}
			std::uint8_t pixelData[4] = { 255, 255, 255, 255 };
			mDefaultTexture->update(pixelData);

		}
		return mDefaultTexture;
	}

	void AssetManager::CleanCycle()
	{
		// Yüklenmiþ tüm texture'lar üzerinde döngüye gir.
		for(auto iter = mLoadedTextureMap.begin(); iter != mLoadedTextureMap.end();)
		{
			// iter->second: Texture'ý tutan shared_ptr.
			// .unique(): Bu shared_ptr'ýn referans sayýsýnýn 1 olup olmadýðýný kontrol eder.
            // Eðer referans sayýsý 1 ise, bu demektir ki texture'ý sadece AssetManager tutuyor,
			// oyundaki baþka hiçbir nesne (gemi, mermi vb.) kullanmýyor.
            if (iter->second.unique())
			{
				// Kullanýlmayan texture'ý konsola yazdýr.
				// Haritadan bu texture'ý sil. erase(), silinen elemanýn bir sonrakini döndürür.
				iter = mLoadedTextureMap.erase(iter);
			}
			else
			{
				// Eðer texture hala kullanýlýyorsa, bir sonraki elemana geç.
				++iter;
			}
		}

		for(auto iter =mLoadedFontMap.begin(); iter != mLoadedFontMap.end();)
		{
			if (iter->second.unique())
			{
				iter = mLoadedFontMap.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		for(auto iter = mLoadedSoundBufferMap.begin(); iter != mLoadedSoundBufferMap.end();)
		{
			if (iter->second.unique())
			{
				iter = mLoadedSoundBufferMap.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		for(auto iter = mLoadedShaderMap.begin(); iter != mLoadedShaderMap.end();)
		{
			if (iter->second.unique())
			{
				iter = mLoadedShaderMap.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
	void AssetManager::Shutdown()
	{
		mLoadedShaderMap.clear();
		mLoadedTextureMap.clear();
		mLoadedFontMap.clear();
		mLoadedSoundBufferMap.clear();
		mDefaultTexture.reset();
	}
}
#include "framework/AssetManager.h"

namespace ly
{
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

	// Verilen yoldan bir texture yükleyen veya önbellekten (cache) getiren fonksiyon.
	shared_ptr<sf::Texture> AssetManager::LoadTexture(const std::string& texturePath)
	{
		// 1. Texture'ýn daha önce yüklenip yüklenmediðini kontrol et.
		// mLoadedTextureMap, yüklenmiþ texture'larý ve yollarýný tutan bir haritadýr.
		auto found = mLoadedTextureMap.find(texturePath);
		
		// 2. Eðer haritada bulunduysa...
		if (found!=mLoadedTextureMap.end())
		{
			// ...diskten tekrar yüklemek yerine haritadaki kopyayý (shared_ptr) döndür.
			return found->second;
		}

		// 3. Eðer haritada bulunamadýysa, yeni bir texture oluþtur.
		shared_ptr<sf::Texture> newTexture{ new sf::Texture };
		
		// 4. Texture'ý diskten yüklemeyi dene. Kök dizin + verilen yolu birleþtir.
		if (newTexture->loadFromFile(mAssetRootDirectory+texturePath))
		{
			// 5. Yükleme baþarýlýysa, yeni texture'ý haritaya ekle.
			mLoadedTextureMap.insert({ texturePath, newTexture });
			// Ve yeni oluþturulan shared_ptr'ý döndür.
			return newTexture;
		}

		// 6. Yükleme baþarýsýz olursa, boþ bir shared_ptr (nullptr) döndür.
		return shared_ptr<sf::Texture> {nullptr};
	}

	// Artýk kullanýlmayan texture'larý bellekten temizleyen fonksiyon.
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
				LOG("Removed unused texture: %s", iter->first.c_str());
				// Haritadan bu texture'ý sil. erase(), silinen elemanýn bir sonrakini döndürür.
				iter = mLoadedTextureMap.erase(iter);
			}
			else
			{
				// Eðer texture hala kullanýlýyorsa, bir sonraki elemana geç.
				++iter;
			}
		}
	}
}
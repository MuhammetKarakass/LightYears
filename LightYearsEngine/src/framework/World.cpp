#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "gameplay/GameStage.h"

namespace ly{

	// Çaðrýldýðý Yer: Application::LoadWorld() içinde make_shared<WorldType>(this) ile.
	// Açýklama: World nesnesi oluþturulduðunda çalýþýr. Üye deðiþkenleri baþlangýç deðerlerine ayarlar.
	World::World(Application* owningApp):
		mOwningApp{ owningApp },
		mBeganPlay{ false },   
		mPendingActors{},      
		mActors{},
		mCurrentStage{mGameStages.end()},
		mGameStages{}
	{

	}

	// Çaðrýldýðý Yer: Application::LoadWorld() - yeni dünya yüklendikten hemen sonra.
	// Açýklama: BeginPlay'in sadece bir kez çaðrýlmasýný garanti eden bir sarmalayýcý (wrapper).
	// Ayrýca oyun aþamalarýný baþlatýr.
	void World::BeginPlayInternal()
	{
		if (!mBeganPlay)
		{
			mBeganPlay = true;
			BeginPlay();  
			InitGameStages();
			BeginStages();
		}
	}

	// Çaðrýldýðý Yer: Application::TickInternal() - her frame.
	// Açýklama: Dünyanýn ana güncelleme döngüsü. Yeni oluþturulan (bekleyen) Actor'larý oyuna dahil eder,
	// mevcut tüm Actor'larý ve aktif oyun aþamasýný günceller.
	void World::TickInternal(float deltaTime)
	{

        // Bekleyen actor'larý aktif listeye ekle ve baþlat
        for (std::shared_ptr<Actor> actor : mPendingActors) 
		{
              mActors.push_back(actor);           // Aktif listeye ekle
              actor->BeginPlayInternal();         // Actor'ý baþlat
        }

		mPendingActors.clear();  // Bekleyen listeyi temizle

		// Tüm aktif actor'larý güncelle
		for (auto iter = mActors.begin(); iter != mActors.end();)
		{
			iter->get()->TickInternal(deltaTime);  // Her actor'ý güncelle
			iter++;
		}

		if(mCurrentStage!=mGameStages.end())
		{
			mCurrentStage->get()->TickStage(deltaTime);
		}

		if (mBeganPlay)
		{
			Tick(deltaTime);  // Virtual - türetilen sýnýflar override eder
		}
	}

	// Çaðrýldýðý Yer: Application::TickInternal() - periyodik olarak (örn: her 2 saniyede bir).
	// Açýklama: "Çöp toplama" iþlevi görür. Yok edilmek üzere iþaretlenmiþ Actor'larý ve
	// bitmiþ oyun aþamalarýný bellekten temizler.
	void World::CleanCycle()
	{
		for (auto iter = mActors.begin(); iter != mActors.end();)
		{
			if (iter->get()->GetIsPendingDestroy())
			{
				iter = mActors.erase(iter);  // Listeden kaldýr ve (shared_ptr sayesinde) belleði serbest býrak
			}
			else
			{
				iter++;
			}
		}
	}

	// Çaðrýldýðý Yer: Application::Render() - her frame.
	// Açýklama: Sahnedeki tüm aktif Actor'larý ekrana çizer.
	void World::Render(sf::RenderWindow& window)
	{
		for (const std::shared_ptr<Actor>& actor : mActors)
		{
			actor->Render(window);
		}
	}

	// Çaðrýldýðý Yer: Actor sýnýflarý tarafýndan (örn: Actor::IsActorOutOfWindow).
	// Açýklama: Sahip olan Application üzerinden pencere boyutunu alýr.
	sf::Vector2u World::GetWindowSize()
	{
		return mOwningApp->GetWindowSize();
	}


	// Çaðrýldýðý Yer: C++ tarafýndan, World'ü tutan son shared_ptr yok olduðunda.
	// Açýklama: World nesnesi yok edilirken çalýþýr. mActors listesindeki shared_ptr'ler
	// otomatik olarak temizlenir, bu da içerdikleri Actor'larýn yok olmasýný tetikler.
	World::~World()
	{
		// Shared_ptr'ler otomatik temizlenecek
	}

	// Çaðrýldýðý Yer: Genellikle türetilmiþ World sýnýflarýnýn (örn: LevelOne) InitGameStages fonksiyonunda.
	// Açýklama: Seviyeye yeni bir oyun aþamasý ekler.
	void World::AddGameStage(shared_ptr<GameStage> newStage)
	{
		mGameStages.push_back(newStage);
	}

	// Çaðrýldýðý Yer: World::BeginPlayInternal() içinde.
	// Açýklama: Türetilmiþ sýnýflarýn (örn: LevelOne) seviye baþlangýcýnda kendi özel mantýklarýný
	// çalýþtýrmasý için override edebileceði sanal (virtual) bir fonksiyondur.
	void World::BeginPlay()
	{
		
	}

	// Çaðrýldýðý Yer: World::TickInternal() içinde.
	// Açýklama: Türetilmiþ sýnýflarýn her frame'de kendi özel dünya mantýklarýný
	// çalýþtýrmasý için override edebileceði sanal (virtual) bir fonksiyondur.
	void World::Tick(float deltaTime)
	{

	}

	// Çaðrýldýðý Yer: World::BeginPlayInternal() içinde.
	// Açýklama: Türetilmiþ sýnýflarýn seviyeye ait oyun aþamalarýný (GameStage)
	// eklemesi için override edebileceði sanal (virtual) bir fonksiyondur.
	void World::InitGameStages()
	{
	}

	// Çaðrýldýðý Yer: World::NextGameStage() - tüm aþamalar bittiðinde.
	// Açýklama: Türetilmiþ sýnýflarýn, seviyedeki tüm aþamalar tamamlandýðýnda ne olacaðýný
	// (örn: "Level Bitti" ekraný göstermek) tanýmlamasý için override edebileceði sanal bir fonksiyondur.
	void World::AllGameStagesFinished()
	{
	}

	// Çaðrýldýðý Yer: Baþlangýçta BeginPlayInternal() tarafýndan, sonra her aþama bittiðinde kendisini tekrar tetikler.
	// Açýklama: Bir sonraki oyun aþamasýna geçer. Mevcut aþamanýn "bitti" event'ine baðlanarak
	// zincirleme bir þekilde aþamalarýn sýrayla ilerlemesini saðlar.
	void World::NextGameStage()
	{
		mCurrentStage = mGameStages.erase(mCurrentStage);
		if(mCurrentStage!=mGameStages.end())
		{
			mCurrentStage->get()->BeginStage();
			mCurrentStage->get()->onStageFinished.BindAction(GetWeakPtr(), &World::NextGameStage);
		}
		else
		{
			AllGameStagesFinished();
		}
	}
	void World::BeginStages()
	{
		mCurrentStage = mGameStages.begin();
		mCurrentStage->get()->BeginStage();
		mCurrentStage->get()->onStageFinished.BindAction(GetWeakPtr(), &World::NextGameStage);
	}
}
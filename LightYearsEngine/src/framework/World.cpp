#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "gameplay/GameStage.h"
#include "widget/HUD.h"

namespace ly{

	// Çağrıldığı Yer: Application::LoadWorld() içinde make_shared<WorldType>(this) ile.
	// Açıklama: World nesnesi oluşturulduğunda çalışır. Üye değişkenleri başlangıç değerlerine ayarlar.
	World::World(Application* owningApp):
		mOwningApp{ owningApp },
		mBeganPlay{ false },   
		mPendingActors{},      
		mActors{},
		mCurrentStage{mGameStages.end()},
		mGameStages{},
		mIsPaused{ false }
	{

	}

	// Çağrıldığı Yer: Application::LoadWorld() - yeni dünya yüklendikten hemen sonra.
	// Açıklama: BeginPlay'in sadece bir kez çağrılmasını garanti eden bir sarmalayıcı (wrapper).
	// Ayrıca oyun aşamalarını başlatır.
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

	// Çağrıldığı Yer: Application::TickInternal() - her frame.
	// Açıklama: Dünyanın ana güncelleme döngüsü. Yeni oluşturulan (bekleyen) Actor'ları oyuna dahil eder,
	// mevcut tüm Actor'ları ve aktif oyun aşamasını günceller.
	void World::TickInternal(float deltaTime)
	{
		if (!mIsPaused) {

			for (std::shared_ptr<Actor> actor : mPendingActors)
			{
				mActors.push_back(actor);
				actor->BeginPlayInternal();

				OnActorSpawned(actor.get());
			}

			mPendingActors.clear();

			for (auto iter = mActors.begin(); iter != mActors.end();)
			{
				iter->get()->TickInternal(deltaTime);
				iter++;
			}

			if (mCurrentStage != mGameStages.end())
			{
				mCurrentStage->get()->TickStage(deltaTime);
			}

			if (mBeganPlay)
			{
				Tick(deltaTime);  // Virtual - türetilen sınıflar override eder
			}
		}

		else
		{
			for(auto iter = mActors.begin(); iter != mActors.end(); iter++)
			{
				if(iter->get()->GetTickWhenPaused())
				{
					iter->get()->TickInternal(deltaTime);
				}
			}
		}

		if(mHUD)
		{
			if(!mHUD->HasInit())
				mHUD->NativeInit(mOwningApp->GetRenderWindow());
			mHUD->Tick(deltaTime);
		}
		if(mOverlayHUD)
		{
			if(!mOverlayHUD->HasInit())
				mOverlayHUD->NativeInit(mOwningApp->GetRenderWindow());
			mOverlayHUD->Tick(deltaTime);
		}
	}

	// Çağrıldığı Yer: Application::TickInternal() - periyodik olarak (örn: her 2 saniyede bir).
	// Açıklama: "Çöp toplama" işlevi görür. Yok edilmek üzere işaretlenmiş Actor'ları ve
	// bitmiş oyun aşamalarını bellekten temizler.
	void World::CleanCycle()
	{
		for (auto iter = mActors.begin(); iter != mActors.end();)
		{
			if (iter->get()->GetIsPendingDestroy())
			{
				iter = mActors.erase(iter);  // Listeden kaldır ve (shared_ptr sayesinde) belleği serbest bırak
			}
			else
			{
				iter++;
			}
		}
	}

	void World::SetPaused(bool paused)
	{
		mIsPaused = paused;
	}

	void World::RemoveOverlayHUD()
	{
		mOverlayHUD.reset();
	}

	// Çağrıldığı Yer: Application::Render() - her frame.
	// Açıklama: Sahnedeki tüm aktif Actor'ları ekrana çizer.
	void World::Render(sf::RenderWindow& window)
	{
		for (const std::shared_ptr<Actor>& actor : mActors)
		{
			actor->Render(window);
		}
		RenderHUD(window);
	}

	// Çağrıldığı Yer: Actor sınıfları tarafından (örn: Actor::IsActorOutOfWindow).
	// Açıklama: Sahip olan Application üzerinden pencere boyutunu alır.
	sf::Vector2u World::GetWindowSize()
	{
		return mOwningApp->GetWindowSize();
	}


	bool World::DispatchEvent(const sf::Event& event)
	{
		if (mOverlayHUD)
		{
			return mOverlayHUD->HandleEvent(event);
		}

		if (mHUD)
		{
			return mHUD->HandleEvent(event);
		}
		return false;
	}

	World::~World()
	{
		// Shared_ptr'ler otomatik temizlenecek
	}


	void World::AddGameStage(shared_ptr<GameStage> newStage)
	{
		mGameStages.push_back(newStage);
	}


	void World::BeginPlay()
	{
		
	}

	// Çağrıldığı Yer: World::TickInternal() içinde.
	// Açıklama: Türetilmiş sınıfların her frame'de kendi özel dünya mantıklarını
	// çalıştırması için override edebileceği sanal (virtual) bir fonksiyondur.
	void World::Tick(float deltaTime)
	{

	}

	void World::OnActorSpawned(Actor* actor)
	{
	}

	// Çağrıldığı Yer: World::BeginPlayInternal() içinde.
	// Açıklama: Türetilmiş sınıfların seviyeye ait oyun aşamalarını (GameStage)
	// eklemesi için override edebileceği sanal (virtual) bir fonksiyondur.
	void World::InitGameStages()
	{
	}

	// Çağrıldığı Yer: World::NextGameStage() - tüm aşamalar bittiğinde.
	// Açıklama: Türetilmiş sınıfların, seviyedeki tüm aşamalar tamamlandığında ne olacağını
	// (örn: "Level Bitti" ekranı göstermek) tanımlaması için override edebileceği sanal bir fonksiyondur.
	void World::AllGameStagesFinished()
	{
	}

	// Çağrıldığı Yer: Başlangıçta BeginPlayInternal() tarafından, sonra her aşama bittiğinde kendisini tekrar tetikler.
	// Açıklama: Bir sonraki oyun aşamasına geçer. Mevcut aşamanın "bitti" event'ine bağlanarak
	// zincirleme bir şekilde aşamaların sırayla ilerlemesini sağlar.
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
		if(mCurrentStage != mGameStages.end())
		{
			mCurrentStage->get()->BeginStage();
			mCurrentStage->get()->onStageFinished.BindAction(GetWeakPtr(), &World::NextGameStage);
		}
	}
	void World::RenderHUD(sf::RenderWindow& window)
	{
		if(mHUD)
		{
			if (mHUD->HasInit())
			{
				mHUD->Draw(window);
			}
		}
		if(mOverlayHUD)
		{
			if (mOverlayHUD->HasInit())
			{
				mOverlayHUD->Draw(window);
			}
		}
	}
}


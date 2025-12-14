#include "enemy/LevelOneBossStage.h"
#include "enemy/LevelOneBoss.h"
#include "framework/World.h"
#include "widget/GameHUD.h"

namespace ly
{
	LevelOneBossStage::LevelOneBossStage(World* world) :
		GameStage{ world }
	{
	}
	void LevelOneBossStage::BeginStage()
	{
		GameStage::BeginStage();
		onNotification.Broadcast("BOSS Approching", 0.5f, 2.5f, 0.5f, sf::Vector2f{ GetWorld()->GetWindowSize().x/2.f,GetWorld()->GetWindowSize().x / 2.f }, 30.f, sf::Color::Red);
		mBoss = GetWorld()->SpawnActor<LevelOneBoss>();
		mBoss.lock()->SetInvulnerability(true);
		mBoss.lock()->SetActorLocation({0.f, -200.f});
		mBoss.lock()->SetVisibility(false);
		mBoss.lock()->SetEnablePhysics(false);
		mBoss.lock()->SetCollisionLayer(CollisionLayer::None);
		mBoss.lock()->SetCollisionMask(CollisionLayer::None); 
		TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), [this]()
			{
				onBossSpawned.Broadcast(mBoss.lock().get());
			}, .05f);

		TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), [this]()
			{
				auto windowSize = GetWorld()->GetWindowSize();
				mBoss.lock()->SetCollisionLayer(CollisionLayer::Enemy);
				mBoss.lock()->SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
				mBoss.lock()->SetVisibility(true);
				mBoss.lock()->SetActorLocation({ windowSize.x / 2.f, -200.f });
				mBoss.lock()->SetVelocity({ 0.f, 100.f });
				mBoss.lock()->onActorDestroyed.BindAction(GetWeakPtr(), &LevelOneBossStage::BossDestroyed);
			}, 5.f);
	}
	void LevelOneBossStage::TickStage(float deltaTime)
	{
		if (mBoss.lock() && mBoss.lock()->GetActorLocation().y >= 225.f && !mBossArrivedLocation)
		{
			mBossArrivedLocation = true;

			onBossHealthBarCreated.Broadcast(std::string{"BABA YAGA"},mBoss.lock()->GetHealthComponent().GetHealth(), mBoss.lock()->GetHealthComponent().GetMaxHealth());
			mBoss.lock()->SetVelocity({ 0.f, 0.f });
			TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), &LevelOneBossStage::ArrivedLocation, 2.f, false);
		}
	}
	void LevelOneBossStage::StageFinished()
	{
		LOG("LevelOneBossStage::StageFinished - Boss defeated!");
	}
	void LevelOneBossStage::BossDestroyed(Actor* bossActor)
	{
		StageFinished();
	}

	void LevelOneBossStage::ArrivedLocation()
	{
		mBoss.lock()->SetInvulnerability(false);
		onArrivedLocation.Broadcast();
	}
}
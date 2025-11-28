#include "player/PlayerSpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "weapon/BulletShooter.h"
#include "weapon/ThreeWayShooter.h"
#include "weapon/FrontalWiper.h"
#include "weapon/Shooter.h"

namespace ly
{
	// PlayerSpaceShip constructor - Oyuncu kontrollü uzay gemisi
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld,const std::string& texturePath)
		:SpaceShip(owningWorld, texturePath),  
		mSpeed(300.f),                         
		mMoveInput{ 0.f, 0.f },               
		mShooter{ new BulletShooter{this,"SpaceShooterRedux/PNG/Lasers/laserBlue01.png",0.2f, sf::Vector2f{0.f,50.f}} },
		mInvulnerabilityTime{ 2.5f },
		mInvulnerable{true},
		mInvulnerabilityBlinkInterval{ 0.4f },
		mInvulnerabilityBlinkTimer{ 0.f },
		mInvulnerabilityDir{ 1.f }
	{
		SetActorRotation(0.f);
		SetExplosionType(ExplosionType::Medium);
	}

	void PlayerSpaceShip::SetShooter(unique_ptr<Shooter>&& shooter)
	{
		if (mShooter && typeid(*mShooter) == typeid(*shooter))
		{
			mShooter->IncrementLevel();
			return;
		}
		mShooter = std::move(shooter);
	}

	void PlayerSpaceShip::SetupCollisionLayers()
	{
		// Bu gemi Player katmanýnda
		SetCollisionLayer(CollisionLayer::Player);
		// Düþmanlar, düþman mermileri ve powerup'larla çarpýþabilir
		SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::EnemyBullet | CollisionLayer::Powerup);
	
	}

	void PlayerSpaceShip::BeginPlay()
	{
		SpaceShip::BeginPlay();  
		TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&PlayerSpaceShip::StopInvulnerability,
			mInvulnerabilityTime,
			false
		);
	}

	void PlayerSpaceShip::ApplyDamage(float amt)
	{
		if (mInvulnerable)  
		{
			return;  
		}
		SpaceShip::ApplyDamage(amt);  
	}

	void PlayerSpaceShip::Tick(float deltaTime)
	{
		SpaceShip::Tick(deltaTime);  
		SetInput();                  
		ConsumeInput();    
		if(mInvulnerable)            
		{
			UpdateInvulnerability(deltaTime); 
		}

	}
	
	void PlayerSpaceShip::SetInput()
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)|| sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
		{
			mMoveInput.y = -1.f;  
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
		{
			mMoveInput.y = 1.f;   
		}

		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
		{
			mMoveInput.x = -1.f;  
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
		{
			mMoveInput.x = 1.f;   
		}
		
		ClampInputOnEdge();  // Ekran kenarýnda sýnýrla
		NormalizeInput();    // Input vektörünü normalize et
	}
	
	// Input vektörünü gerçek harekete çevir
	void PlayerSpaceShip::ConsumeInput()
	{
		SetVelocity(mMoveInput * mSpeed); 
		mMoveInput.x = 0.f;               
		mMoveInput.y = 0.f;
	}
	
	// Input vektörünü normalize et (diagonal movement fix)
	void PlayerSpaceShip::NormalizeInput()
	{
		NormalizeVector(mMoveInput); 
	}

	// Ekran kenarýnda hareket sýnýrlamasý ve ateþ etme
	void PlayerSpaceShip::ClampInputOnEdge()
	{
		//Sol kenar kontrolü
		if(GetActorLocation().x-40.f<=0.f && mMoveInput.x==-1.f)
		{
			mMoveInput.x = 0.f; 
		}
		// Sað kenar kontrolü
		else if (GetActorLocation().x + 40.f >= GetWindowSize().x && mMoveInput.x == 1.f)
		{
			mMoveInput.x = 0.f;  
		}

		// Üst kenar kontrolü
		if (GetActorLocation().y - 40.f <= 0.f && mMoveInput.y == -1.f)
		{
			mMoveInput.y = 0.f;  
		}
		// Alt kenar kontrolü
		else if (GetActorLocation().y + 40.f >= GetWindowSize().y && mMoveInput.y == 1.f)
		{
			mMoveInput.y = 0.f; 
		}

		// Ateþ etme (Space tuþu)
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
		{
			Shoot();  
		}
	}

	void PlayerSpaceShip::Shoot()
	{
		if(mShooter)  
		{
			mShooter->Shoot();  
		}
	}

	void PlayerSpaceShip::StopInvulnerability()
	{
		GetSprite().setColor({255,255,255,255});
		mInvulnerable = false;
	}

	void PlayerSpaceShip::UpdateInvulnerability(float deltaTime)
	{
		mInvulnerabilityBlinkTimer += deltaTime * mInvulnerabilityDir;
		if (mInvulnerabilityBlinkTimer < 0 || mInvulnerabilityBlinkTimer > mInvulnerabilityBlinkInterval)
		{
			mInvulnerabilityDir *= -1;
		}

		GetSprite().setColor(LerpColor({ 255,255, 255, 96 }, { 255, 255, 255, 160 }, mInvulnerabilityBlinkTimer / mInvulnerabilityBlinkInterval));
	}

	void PlayerSpaceShip::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);

		if (otherActor && GetCanCollide() && !mInvulnerable)
		{
			otherActor->ApplyDamage(25.f);
		}
	}
}
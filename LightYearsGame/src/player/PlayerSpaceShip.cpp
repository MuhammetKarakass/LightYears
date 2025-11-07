#include "player/PlayerSpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "weapon/BulletShooter.h"

namespace ly
{
	// PlayerSpaceShip constructor - Oyuncu kontrollü uzay gemisi
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld,const std::string& texturePath)
		:SpaceShip(owningWorld, texturePath),  
		mSpeed(300.f),                         
		mMoveInput{ 0.f, 0.f },               
		mShooter{new BulletShooter(this,"SpaceShooterRedux/PNG/Lasers/laserBlue01.png", 0.15f)}
	{
		SetActorRotation(0.f);
		SetExplosionType(ExplosionType::Medium);
	}

	void PlayerSpaceShip::SetupCollisionLayers()
	{
		// Bu gemi Player katmanýnda
		SetCollisionLayer(CollisionLayer::Player);
		// Düþmanlar, düþman mermileri ve powerup'larla çarpýþabilir
		SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::EnemyBullet | CollisionLayer::Powerup);
	
	}

	void PlayerSpaceShip::Tick(float deltaTime)
	{
		SpaceShip::Tick(deltaTime);  
		SetInput();                  
		ConsumeInput();    

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

	void PlayerSpaceShip::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);
		
		if (otherActor && GetCanCollide())
		{
			otherActor->ApplyDamage(25.f); // Düþmana hasar ver
		}
	}
}
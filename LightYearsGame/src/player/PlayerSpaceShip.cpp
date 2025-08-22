#include "player/PlayerSpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "weapon/BulletShooter.h"

namespace ly
{
	// PlayerSpaceShip constructor - Oyuncu kontroll� uzay gemisi
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld,const std::string& texturePath)
		:SpaceShip(owningWorld, texturePath),  
		mSpeed(200.f),                         
		mMoveInput{ 0.f, 0.f },               
		mShooter{new BulletShooter(this,"SpaceShooterRedux/PNG/Lasers/laserBlue01.png", 0.2f)}
	{
		SetActorRotation(0.f);
	}

	void PlayerSpaceShip::SetupCollisionLayers()
	{
		// Bu gemi Player katman�nda
		SetCollisionLayer(CollisionLayer::Player);
		// D��manlar, d��man mermileri ve powerup'larla �arp��abilir
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
		
		ClampInputOnEdge();  // Ekran kenar�nda s�n�rla
		NormalizeInput();    // Input vekt�r�n� normalize et
	}
	
	// Input vekt�r�n� ger�ek harekete �evir
	void PlayerSpaceShip::ConsumeInput()
	{
		SetVelocity(mMoveInput * mSpeed); 
		mMoveInput.x = 0.f;               
		mMoveInput.y = 0.f;
	}
	
	// Input vekt�r�n� normalize et (diagonal movement fix)
	void PlayerSpaceShip::NormalizeInput()
	{
		NormalizeVector(mMoveInput); 
	}

	// Ekran kenar�nda hareket s�n�rlamas� ve ate� etme
	void PlayerSpaceShip::ClampInputOnEdge()
	{
		if(GetActorLocation().x-40.f<=0.f && mMoveInput.x==-1.f)
		{
			mMoveInput.x = 0.f; 
		}
		// Sa� kenar kontrol�
		else if (GetActorLocation().x + 40.f >= GetWindowSize().x && mMoveInput.x == 1.f)
		{
			mMoveInput.x = 0.f;  
		}

		// �st kenar kontrol�
		if (GetActorLocation().y - 40.f <= 0.f && mMoveInput.y == -1.f)
		{
			mMoveInput.y = 0.f;  
		}
		// Alt kenar kontrol�
		else if (GetActorLocation().y + 40.f >= GetWindowSize().y && mMoveInput.y == 1.f)
		{
			mMoveInput.y = 0.f; 
		}

		// Ate� etme (Space tu�u)
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
			otherActor->ApplyDamage(25.f); // D��mana hasar ver
		}
	}
}
#include "spaceShip/SpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "VFX/Explosion.h"  // ? Eksik include eklendi

namespace ly
{
	// SpaceShip constructor - Uzay gemisi nesnesini olu�turur
	SpaceShip::SpaceShip(World* owningWorld, std::string texturePath):
		Actor(owningWorld, texturePath),
		mVelocity(0.0f, 0.0f),          
		mHealthComponent{100.f,100.f},  
		mBlinkColor{ 255,0,0,255 },     
		mBlinkTime{0.f},                
		mBlinkDuration{.25f},
		mExplosionType{ExplosionType::Medium}  // ? Default explosion type
	{
		SetCollisionLayer(CollisionLayer::None);
		// T�retilen s�n�flar kendi collision ayarlar�n� yapacak:
		// Player: SetCollisionLayer(Player); SetCollisionMask(Enemy | EnemyBullet | Powerup);
		// Enemy:  SetCollisionLayer(Enemy);  SetCollisionMask(Player | PlayerBullet);
	}

	void SpaceShip::BeginPlay()
	{
		Actor::BeginPlay();        
		SetEnablePhysics(true);    
		
		SetupCollisionLayers();
		
		// Health component event'lerine abone ol (delegate pattern)
		mHealthComponent.onHealthChanged.BindAction(GetWeakPtr(), &SpaceShip::OnHealthChanged);
		mHealthComponent.onTakenDamage.BindAction(GetWeakPtr(), &SpaceShip::OnTakenDamage);
		mHealthComponent.onHealthEmpty.BindAction(GetWeakPtr(), &SpaceShip::Blow);
	}

	void SpaceShip::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);                           
		AddActorLocationOffset(mVelocity * deltaTime);    
		UpdateBlink(deltaTime);      


	}

	void SpaceShip::SetVelocity(const sf::Vector2f& velocity)
	{
		mVelocity = velocity; 
	}

	// Ate� etme i�lemi (t�retilen s�n�flar override eder)
	void SpaceShip::Shoot()
	{
		// Base class'ta bo� - PlayerSpaceShip, EnemySpaceShip override eder
	}
	
	void SpaceShip::Blink()
	{
		mBlinkTime = mBlinkDuration; 
	}

	void SpaceShip::UpdateBlink(float deltaTime)
	{
		if (mBlinkTime > 0) 
		{
			mBlinkTime -= deltaTime;  // Kalan s�reyi azalt

			// S�re bitti�inde s�f�rla
			mBlinkTime = mBlinkTime > 0 ? mBlinkTime : 0.f;
			
			// Beyazdan k�rm�z�ya ge�i� efekti (mBlinkTime azald�k�a k�rm�z�la��r)
			GetSprite().setColor(LerpColor(sf::Color::White, mBlinkColor, mBlinkTime));
		}
	}

	// Health de�i�ti�inde �a�r�lan callback (delegate)
	void SpaceShip::OnHealthChanged(float amt, float health, float maxHealth)
	{
		LOG("amt:%f health percentage %f/%f", amt, health, maxHealth);
	}
	
	// Hasar al�nd���nda �a�r�lan callback (delegate)
	void SpaceShip::OnTakenDamage(float amt, float health, float maxHealth)
	{
		Blink();
	}
	
	// Can bitti�inde �a�r�lan callback (delegate)
	void SpaceShip::Blow()
	{
		Explosion::SpawnExplosion(GetWorld(), GetActorLocation(), Explosion::GetPreset(GetExplosionType()));
		Destroy();
	}
	
	// Bu gemiye hasar verme i�lemi
	void SpaceShip::ApplyDamage(float amt)
	{
		LOG("%f", amt);                          
		mHealthComponent.ChangeHealth(-amt);     
	}
}
#include "spaceShip/SpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "VFX/Explosion.h" 

namespace ly
{
	// SpaceShip constructor - Uzay gemisi nesnesini oluþturur
	SpaceShip::SpaceShip(World* owningWorld, const ShipDefinition& shipDef):
		Actor(owningWorld, shipDef.texturePath),
		mHealthComponent{shipDef.health, shipDef.health},
		mExplosionType{ (ExplosionType)shipDef.explosionType },
		mBlinkColor{255, 0, 0, 255},
		mBlinkTime{0.f},
		mBlinkDuration{.25f},
		mInvulnerability{ false }
	{
		SetCollisionLayer(CollisionLayer::None);
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



	// Ateþ etme iþlemi (türetilen sýnýflar override eder)
	void SpaceShip::Shoot()
	{
	}

	void SpaceShip::SetupCollisionLayers()
	{
	}

	void SpaceShip::Blink()
	{
		mBlinkTime = mBlinkDuration; 
	}

	void SpaceShip::UpdateBlink(float deltaTime)
	{
		if (mBlinkTime > 0) 
		{
			mBlinkTime -= deltaTime;  // Kalan süreyi azalt

			// Süre bittiðinde sýfýrla
			mBlinkTime = mBlinkTime > 0 ? mBlinkTime : 0.f;
			
			// Beyazdan kýrmýzýya geçiþ efekti (mBlinkTime azaldýkça kýrmýzýlaþýr)
			GetSprite().value().setColor(LerpColor(sf::Color::White, mBlinkColor, mBlinkTime));
		}
	}

	// Health deðiþtiðinde çaðrýlan callback (delegate)
	void SpaceShip::OnHealthChanged(float amt, float health, float maxHealth)
	{
	}
	
	// Hasar alýndýðýnda çaðrýlan callback (delegate)
	void SpaceShip::OnTakenDamage(float amt, float health, float maxHealth)
	{
		Blink();
	}
	
	// Can bittiðinde çaðrýlan callback (delegate)
	void SpaceShip::Blow()
	{
		Explosion::SpawnExplosion(GetWorld(), GetActorLocation(), Explosion::GetPreset(GetExplosionType()));
		Blew();
		Destroy();
	}

	void SpaceShip::Blew()
	{
	}
	
	// Bu gemiye hasar verme iþlemi
	void SpaceShip::ApplyDamage(float amt)
	{                        
		mHealthComponent.ChangeHealth(-amt);     
	}
}
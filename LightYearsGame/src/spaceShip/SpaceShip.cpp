#include "spaceShip/SpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "VFX/Explosion.h" 

namespace ly
{
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
			mBlinkTime -= deltaTime;

			mBlinkTime = mBlinkTime > 0 ? mBlinkTime : 0.f;
			
			GetSprite().value().setColor(LerpColor(sf::Color::White, mBlinkColor, mBlinkTime));
		}
	}

	void SpaceShip::OnHealthChanged(float amt, float health, float maxHealth)
	{
	}
	
	void SpaceShip::OnTakenDamage(float amt, float health, float maxHealth)
	{
		Blink();
	}
	
	void SpaceShip::Blow()
	{
		Explosion::SpawnExplosion(GetWorld(), GetActorLocation(), Explosion::GetPreset(GetExplosionType()));
		Blew();
		Destroy();
	}

	void SpaceShip::Blew()
	{
	}
	
	void SpaceShip::ApplyDamage(float amt)
	{                        
		mHealthComponent.ChangeHealth(-amt);     
	}
}
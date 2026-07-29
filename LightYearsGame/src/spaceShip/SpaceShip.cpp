#include "spaceShip/SpaceShip.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "VFX/Explosion.h" 
#include <algorithm>
#include <cmath>

namespace ly
{
	SpaceShip::SpaceShip(World* owningWorld, const ShipDefinition& shipDef):
		Actor(owningWorld, shipDef.texturePath),
		mHealthComponent{shipDef.health, shipDef.health},
		mShieldComponent{},
		mEnergyComponent{},
		mCombatRuntime{ *this },
		mShipRuntime{ mCombatRuntime.GetAttributes() },
		mMovementComponent{ *this, shipDef },
		mBlinkColor{255, 0, 0, 255},
		mBlinkTime{0.f},
		mBlinkDuration{.25f},
		mInvulnerability{ false },
		mExplosionType{ (ExplosionType)shipDef.explosionType }
	{
		SetCollisionLayer(CollisionLayer::None);
		mCombatRuntime.InitializeOwnerAttributes(shipDef.health);
		mShipRuntime.InitializeFromShipDefinition(shipDef);
		std::string primaryWeaponFailureReason;
		const AbilityHandle primaryWeaponHandle = mCombatRuntime.GetAbilities().GrantAbility(
			AbilityData::MakePrimaryFireAbilityDefinition(shipDef.primaryWeaponDefinition),
			&primaryWeaponFailureReason
		);
		if (!primaryWeaponHandle.IsValid())
		{
			LY_GAME_ERROR("Invalid primary weapon '%s': %s", shipDef.primaryWeaponDefinition.weaponId.c_str(), primaryWeaponFailureReason.c_str());
		}
	}

	void SpaceShip::BeginPlay()
	{
		Actor::BeginPlay();        
		SetEnablePhysics(true);    
		
		SetupCollisionLayers();
		
		mHealthComponent.onHealthChanged.BindAction(GetWeakPtr(), &SpaceShip::OnHealthChanged);
		mHealthComponent.onTakenDamage.BindAction(GetWeakPtr(), &SpaceShip::OnTakenDamage);
		mHealthComponent.onHealthEmpty.BindAction(GetWeakPtr(), &SpaceShip::Blow);
		mCombatRuntime.GetAttributes().onAttributeChanged.BindAction(GetWeakPtr(), &SpaceShip::OnRuntimeAttributeChanged);
		mShipRuntime.GetAttributes().onAttributeChanged.BindAction(GetWeakPtr(), &SpaceShip::OnShipAttributeChanged);
		RefreshMovementAttributesFromRuntime();
	}

	void SpaceShip::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);
		mMovementComponent.Tick(deltaTime, GetMovementSpeedCapMultiplier());
		UpdateBlink(deltaTime);      
		mCombatRuntime.Tick(deltaTime);
		UpdateRegeneration(deltaTime);
	}

	void SpaceShip::RefreshMovementAttributesFromRuntime()
	{
		const AttributeSystem& attributes = mCombatRuntime.GetAttributes();
		const AttributeSystem& shipAttributes = mShipRuntime.GetAttributes();
		if (attributes.HasAttribute(OwnerAttributeIds::MaxHealth))
		{
			const float previousMaxHealth = mHealthComponent.GetMaxHealth();
			const float resolvedMaxHealth = std::max(1.f, attributes.GetCurrentValue(OwnerAttributeIds::MaxHealth));
			mHealthComponent.SetMaxHealth(resolvedMaxHealth);
			if (resolvedMaxHealth > previousMaxHealth)
			{
				mHealthComponent.Regenerate(resolvedMaxHealth - previousMaxHealth);
			}
		}
		if (shipAttributes.HasAttribute(ShipAttributeIds::MaxShield))
		{
			mShieldComponent.SetMaxShield(shipAttributes.GetCurrentValue(ShipAttributeIds::MaxShield));
		}
		if (shipAttributes.HasAttribute(ShipAttributeIds::ShieldRechargeDelay))
		{
			mShieldComponent.SetRechargeDelay(shipAttributes.GetCurrentValue(ShipAttributeIds::ShieldRechargeDelay));
		}
		mEnergyComponent.SetMaxEnergy(mShipRuntime.GetAfterburnerCapacity());
		mEnergyComponent.SetRechargeDelay(mShipRuntime.GetAfterburnerRechargeDelay());

		mMovementComponent.RefreshAttributes();
	}

	void SpaceShip::OnRuntimeAttributeChanged(GameplayTag attributeId, float previousValue, float currentValue)
	{
		(void)previousValue;
		(void)currentValue;
		if (attributeId == OwnerAttributeIds::MaxHealth ||
			attributeId == OwnerAttributeIds::EnergyMax ||
			attributeId == OwnerAttributeIds::MoveSpeedHorizontal ||
			attributeId == OwnerAttributeIds::MoveSpeedVertical)
		{
			RefreshMovementAttributesFromRuntime();
		}
	}

	void SpaceShip::OnShipAttributeChanged(GameplayTag attributeId, float previousValue, float currentValue)
	{
		(void)previousValue;
		(void)currentValue;
		if (attributeId == ShipAttributeIds::MaxShield ||
			attributeId == ShipAttributeIds::ShieldRechargeDelay ||
			attributeId == ShipAttributeIds::AfterburnerCapacity ||
			attributeId == ShipAttributeIds::AfterburnerRechargeDelay)
		{
			RefreshMovementAttributesFromRuntime();
		}
	}

	void SpaceShip::UpdateRegeneration(float deltaTime)
	{
		const AttributeSystem& attributes = mCombatRuntime.GetAttributes();
		const AttributeSystem& shipAttributes = mShipRuntime.GetAttributes();
		float healthRegenerationTime = std::max(0.f, deltaTime);
		if (mHealthRegenDelayRemaining > 0.f)
		{
			const float consumedDelay = std::min(mHealthRegenDelayRemaining, healthRegenerationTime);
			mHealthRegenDelayRemaining -= consumedDelay;
			healthRegenerationTime -= consumedDelay;
		}
		if (healthRegenerationTime > 0.f)
		{
			mHealthComponent.Regenerate(
				std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::HealthRegen)) * healthRegenerationTime
			);
		}
		const bool allowRecharge = !IsAfterburnerRechargeBlocked();
		mShieldComponent.Tick(
			deltaTime,
			std::max(0.f, shipAttributes.GetCurrentValue(ShipAttributeIds::ShieldRegen)),
			allowRecharge
		);
		mEnergyComponent.Tick(deltaTime, mShipRuntime.GetAfterburnerRegenPerSecond(), allowRecharge);
	}

	sf::Vector2f SpaceShip::ResolveLegacyMovementSpeed(const sf::Vector2f& baseSpeed) const
	{
		return mMovementComponent.ResolveLegacySpeed(baseSpeed);
	}

	void SpaceShip::AddShipRelativeThrust(const sf::Vector2f& localThrustInput, float deltaTime)
	{
		mMovementComponent.AddShipRelativeThrust(localThrustInput, deltaTime);
	}

	void SpaceShip::RotateTowardWorldLocation(const sf::Vector2f& worldLocation, float deltaTime)
	{
		mMovementComponent.RotateTowardWorldLocation(
			worldLocation,
			deltaTime,
			GetMovementTurnCapabilityMultiplier()
		);
	}

	sf::Vector2f SpaceShip::ResolveDashDirection() const
	{
		return mMovementComponent.ResolveDashDirection();
	}

	bool SpaceShip::StartDash(const DashRequest& request)
	{
		return mMovementComponent.StartDash(request);
	}

	void SpaceShip::EndDash()
	{
		mMovementComponent.EndDash();
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
		mHealthRegenDelayRemaining = 4.f;
		Blink();
	}
	
	void SpaceShip::Blow()
	{
		Explosion::SpawnExplosion(GetWorld(), GetActorLocation(), Explosion::GetPreset(GetExplosionType()));
		Blew();
		mShipRuntime.Clear();
		mCombatRuntime.Clear();
		Destroy();
	}

	void SpaceShip::Blew()
	{
	}
	
	void SpaceShip::ApplyDamage(float amt)
	{
		DamageContext context;
		context.target = this;
		context.originalDamage = amt;
		context.remainingDamage = amt;
		ReceiveDamage(context);
	}

	void SpaceShip::ReceiveDamage(DamageContext context)
	{
		if (IsInvulnerable() || context.remainingDamage <= 0.f)
		{
			return;
		}

		context.target = this;

		mCombatRuntime.ProcessIncomingDamage(context);
		if (context.remainingDamage > 0.f)
		{
			const float shieldAbsorbedDamage = mShieldComponent.AbsorbDamage(
				context.remainingDamage,
				context.payload.shieldDamageMultiplier,
				context.payload.shieldRegenerationDelay
			);
			context.remainingDamage = std::max(0.f, context.remainingDamage - shieldAbsorbedDamage);
			context.absorbedDamage += shieldAbsorbedDamage;

			const float healthBeforeDamage = mHealthComponent.GetHealth();
			mHealthComponent.ChangeHealth(-context.remainingDamage);
			const float healthDamage = std::max(0.f, healthBeforeDamage - mHealthComponent.GetHealth());
			context.appliedDamage = shieldAbsorbedDamage + healthDamage;
		}

		mCombatRuntime.NotifyDamageResolved(context);
	}
}

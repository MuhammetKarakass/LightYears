#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/WeaponContentCatalog.h"
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
		mShipRuntime{ mCombatRuntime.GetAbilitySystemComponent().GetAttributes() },
		mMovementComponent{ *this, shipDef },
		mControlTargetClass{ shipDef.controlTargetClass },
		mBlinkColor{255, 0, 0, 255},
		mBlinkTime{0.f},
		mBlinkDuration{.25f},
		mInvulnerability{ false },
		mExplosionType{ (ExplosionType)shipDef.explosionType }
	{
		SetCollisionLayer(CollisionLayer::None);
		mCombatRuntime.InitializeOwnerAttributes(shipDef.health);
		mShipRuntime.InitializeFromShipDefinition(shipDef);
		if (!shipDef.primaryWeaponId.empty())
		{
			const PrimaryWeaponDefinition* runtimeWeapon =
				content::WeaponContentCatalog::FindById(shipDef.primaryWeaponId);
			if (!runtimeWeapon)
			{
				LY_GAME_ERROR("Primary weapon '%s' was not found in JSON content", shipDef.primaryWeaponId.c_str());
				return;
			}
			std::string primaryWeaponFailureReason;
			const sas::AbilityHandle primaryWeaponHandle =
				mCombatRuntime.GetAbilitySystemComponent().GrantAbility(
					AbilityData::MakePrimaryFireAbilityDefinition(*runtimeWeapon),
					&primaryWeaponFailureReason
				);
			if (!primaryWeaponHandle.IsValid())
			{
				LY_GAME_ERROR("Invalid primary weapon '%s': %s", runtimeWeapon->weaponId.c_str(), primaryWeaponFailureReason.c_str());
			}
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
		mCombatRuntime.GetAbilitySystemComponent().GetAttributes().onAttributeChanged.BindAction(GetWeakPtr(), &SpaceShip::OnRuntimeAttributeChanged);
		mShipRuntime.GetAttributes().onAttributeChanged.BindAction(GetWeakPtr(), &SpaceShip::OnShipAttributeChanged);
		RefreshMovementAttributesFromRuntime();
		mTemporalStateHistory.CaptureInitial(*this);
	}

	void SpaceShip::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);
		if (mPortalTransit)
		{
			mCombatRuntime.Tick(deltaTime);
			return;
		}
		sf::Vector2f currentMovementDirection = GetVelocity();
		const float currentMovementLength = GetVectorLength(currentMovementDirection);
		if (currentMovementLength > 0.001f)
		{
			currentMovementDirection /= currentMovementLength;
		}
		else
		{
			currentMovementDirection = { 0.f, 0.f };
		}
		mMovementComponent.Tick(
			deltaTime,
			GetMovementSpeedCapMultiplier() *
			GetMovementSpeedMultiplier() *
			GetConditionalMovementSpeedMultiplier(currentMovementDirection)
		);
		UpdateBlink(deltaTime);      
		mCombatRuntime.Tick(deltaTime);
		UpdateRegeneration(deltaTime);
		mTemporalStateHistory.AdvanceAndCapture(*this, deltaTime);
	}

	bool SpaceShip::CanEnterPortalTransfer() const
	{
		return !GetIsPendingDestroy() &&
			GetHealthComponent().GetHealth() > 0.f;
	}

	float SpaceShip::GetPortalTransferRadius() const
	{
		const sf::FloatRect bounds = GetActorGlobalBounds();
		return std::max(1.f, std::min(bounds.size.x, bounds.size.y) * 0.5f);
	}

	void SpaceShip::BeginPortalTransit()
	{
		if (mPortalTransit)
		{
			return;
		}

		mPortalTransit = true;
		mPortalPhysicsWasEnabled = IsPhysicsEnabled();
		mPortalCollisionLayer = GetCollisionLayer();
		mPortalCollisionMask = GetCollisionMask();
		mPortalVelocity = GetVelocity();
		SetRenderEnabled(false);
		SetVelocity({});
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetEnablePhysics(false);
	}

	void SpaceShip::CompletePortalTransit(const sf::Vector2f& exitLocation)
	{
		if (!mPortalTransit)
		{
			return;
		}

		SetActorLocation(exitLocation);
		SetCollisionLayer(mPortalCollisionLayer);
		SetCollisionMask(mPortalCollisionMask);
		if (mPortalPhysicsWasEnabled)
		{
			SetEnablePhysics(true);
		}
		SetVelocity(mPortalVelocity);
		SetRenderEnabled(true);
		mPortalTransit = false;
	}

	void SpaceShip::RefreshMovementAttributesFromRuntime()
	{
		const sas::AttributeSystem& attributes =
			mCombatRuntime.GetAbilitySystemComponent().GetAttributes();
		const sas::AttributeSystem& shipAttributes = mShipRuntime.GetAttributes();
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

	void SpaceShip::OnRuntimeAttributeChanged(sas::AttributeId attributeId, float previousValue, float currentValue)
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

	void SpaceShip::OnShipAttributeChanged(sas::AttributeId attributeId, float previousValue, float currentValue)
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
		const sas::AttributeSystem& attributes =
			mCombatRuntime.GetAbilitySystemComponent().GetAttributes();
		const sas::AttributeSystem& shipAttributes = mShipRuntime.GetAttributes();
		// Overcap decay is resource-owned and intentionally runs before normal
		// regeneration, so regular regen resumes as soon as the excess is gone.
		mHealthComponent.TickTemporaryOverhealths(deltaTime);
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
		// Temporary overshield decay is independent from normal recharge. It is
		// ticked first so the regular shield rules can resume immediately after
		// the excess reaches the ship's normal maximum.
		mShieldComponent.TickTemporaryOvershields(deltaTime);
		mShieldComponent.Tick(
			deltaTime,
			std::max(0.f, shipAttributes.GetCurrentValue(ShipAttributeIds::ShieldRegen)) *
				GetShieldRegenMultiplier(),
			allowRecharge
		);
		mEnergyComponent.Tick(
			deltaTime,
			mShipRuntime.GetAfterburnerRegenPerSecond() * GetAfterburnerRegenMultiplier(),
			allowRecharge
		);
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
			GetMovementTurnCapabilityMultiplier() *
			mRuntimeModifiers.GetTurnCapabilityMultiplier()
		);
	}

	sf::Vector2f SpaceShip::ResolveDashDirection() const
	{
		return mMovementComponent.ResolveMovementBurstDirection();
	}

	bool SpaceShip::StartDash(const DashRequest& request)
	{
		return mMovementComponent.StartMovementBurst(request);
	}

	void SpaceShip::EndDash()
	{
		mMovementComponent.EndMovementBurst();
	}

	ControlResponse SpaceShip::ResolveControlResponse(
		const GameplayTag& controlTag
	) const
	{
		(void)controlTag;
		switch (mControlTargetClass)
		{
		case ControlTargetClass::Normal:
			return { ControlResponseMode::Full, 1.f, 0.f, true };
		case ControlTargetClass::Elite:
			return { ControlResponseMode::Reduced, 0.6f, 0.f, true };
		case ControlTargetClass::MiniBoss:
			return { ControlResponseMode::Reduced, 0.3f, 0.f, true };
		case ControlTargetClass::Boss:
			return { ControlResponseMode::InterruptOnly, 0.f, 0.15f, true };
		}
		return {};
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
		if (IsInvulnerable() ||
			GetCombatRuntime().BlocksIncomingDamage() ||
			context.remainingDamage <= 0.f)
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
			context.targetWasKilled = healthBeforeDamage > 0.f &&
				mHealthComponent.GetHealth() <= 0.f;
		}

		mCombatRuntime.NotifyDamageResolved(context);
		if (context.appliedDamage > 0.f)
		{
			onDamageTaken.Broadcast(
				this,
				context.appliedDamage,
				mHealthComponent.GetHealth(),
				mHealthComponent.GetMaxHealth()
			);
		}
	}
}

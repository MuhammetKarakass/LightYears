#include "attributes/AttributeMath.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "spaceShip/SpaceShip.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include <framework/SpriteComponent.h>
#include <framework/reflection/Reflection.h>
#include <framework/debug/Profiler.h>
#include "VFX/Explosion.h" 
#include <algorithm>

namespace vega
{
	VEGA_REFLECT(SpaceShip, Actor)
	{
		VEGA_REQUIRES(HealthComponent);
		VEGA_REQUIRES(ShieldComponent);
		VEGA_REQUIRES(EnergyComponent);
		VEGA_REQUIRES(MovementComponent);
	}

	SpaceShip::SpaceShip(World* owningWorld, const ShipDefinition& shipDef):
		Actor(owningWorld, shipDef.texturePath),
		mHealthComponent{ AddComponent<HealthComponent>(shipDef.health, shipDef.health) },
		mShieldComponent{ AddComponent<ShieldComponent>() },
		mEnergyComponent{ AddComponent<EnergyComponent>() },
		mCombatRuntime{ *this },
		mShipRuntime{ mCombatRuntime.GetAbilitySystemComponent().GetAttributes() },
		mMovementComponent{ AddComponent<MovementComponent>(shipDef) },
		mControlTargetClass{ shipDef.controlTargetClass },
		mBlinkColor{255, 0, 0, 255},
		mBlinkTime{0.f},
		mBlinkDuration{.25f},
		mInvulnerability{ false },
		mExplosionType{ (ExplosionType)shipDef.explosionType }
	{
		SetCollisionLayer(vega::CollisionLayer::None);
		mCombatRuntime.InitializeOwnerAttributes(shipDef.health);
		mShipRuntime.InitializeFromShipDefinition(shipDef);
		if (!shipDef.primaryWeaponId.empty())
		{
			const PrimaryWeaponDefinition* runtimeWeapon =
				content::WeaponContentCatalog::FindById(shipDef.primaryWeaponId);
			if (!runtimeWeapon)
			{
				VEGA_GAME_ERROR("Primary weapon '%s' was not found in JSON content", shipDef.primaryWeaponId.c_str());
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
				VEGA_GAME_ERROR("Invalid primary weapon '%s': %s", runtimeWeapon->weaponId.c_str(), primaryWeaponFailureReason.c_str());
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
		mDeferredDamage.Tick(deltaTime);
		if (GetIsPendingDestroy()) return;
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
		{
			VEGA_PROFILE_SCOPE("Ship.CombatRuntimeTick");
			mCombatRuntime.Tick(deltaTime);
		}
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
		SetCollisionLayer(vega::CollisionLayer::None);
		SetCollisionMask(vega::CollisionLayer::None);
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

	void SpaceShip::SuspendBody()
	{
		if (mBodySuspended) return;
		mBodySuspended = true;
		mSuspendedPhysicsWasEnabled = IsPhysicsEnabled();
		mSuspendedCollisionLayer = GetCollisionLayer();
		mSuspendedCollisionMask = GetCollisionMask();
		SetRenderEnabled(false);
		SetVelocity({});
		SetCollisionLayer(vega::CollisionLayer::None);
		SetCollisionMask(vega::CollisionLayer::None);
		SetEnablePhysics(false);
		mCombatRuntime.SetDamageProtection("SpaceShip.SuspendedBody", true, true);
	}

	void SpaceShip::ResumeBody(const sf::Vector2f& location)
	{
		if (!mBodySuspended) return;
		mBodySuspended = false;
		mCombatRuntime.RemoveDamageProtection("SpaceShip.SuspendedBody");
		SetActorLocation(location);
		SetCollisionLayer(mSuspendedCollisionLayer);
		SetCollisionMask(mSuspendedCollisionMask);
		if (mSuspendedPhysicsWasEnabled) SetEnablePhysics(true);
		SetVelocity({});
		SetRenderEnabled(true);
	}

	void SpaceShip::RefreshMovementAttributesFromRuntime()
	{
		GetAbilitySystemComponent().RefreshModulePoolConversion();
		const sas::AttributeSystem& attributes =
			mCombatRuntime.GetAbilitySystemComponent().GetAttributes();
		const sas::AttributeSystem& shipAttributes = mShipRuntime.GetAttributes();
		if (attributes.HasAttribute(OwnerAttributeIds::MaxHealth))
		{
			const float previousMaxHealth = mHealthComponent.GetMaxHealth();
			const float resolvedMaxHealth = std::max(1.f, attributes.GetCurrentValue(OwnerAttributeIds::MaxHealth));
			const bool moduleChange = GetAbilitySystemComponent().IsApplyingModuleAttributes();
			mHealthComponent.SetMaxHealth(resolvedMaxHealth, false, moduleChange);
			if (!moduleChange && resolvedMaxHealth > previousMaxHealth)
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
		if (GetAbilitySystemComponent().IsApplyingModuleAttributes()) return;
		(void)previousValue;
		(void)currentValue;
		if (attributeId == OwnerAttributeIds::MaxHealth ||
			attributeId == OwnerAttributeIds::EnergyPower ||
			attributeId == OwnerAttributeIds::MoveSpeedHorizontal ||
			attributeId == OwnerAttributeIds::MoveSpeedVertical)
		{
			RefreshMovementAttributesFromRuntime();
		}
	}

	void SpaceShip::OnShipAttributeChanged(sas::AttributeId attributeId, float previousValue, float currentValue)
	{
		if (GetAbilitySystemComponent().IsApplyingModuleAttributes()) return;
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
		const float normalShieldRegen =
			std::max(0.f, shipAttributes.GetCurrentValue(ShipAttributeIds::ShieldRegen)) *
				GetShieldRegenMultiplier();
		float shieldRegen = normalShieldRegen;
		bool allowShieldRecharge = allowRecharge;
		// Module regen shaping. SpeedScaled replaces passive regen with a speed-proportional rate;
		// Continuous keeps the normal rate while damaged. Both skip the recharge delay.
		switch (mShieldComponent.GetModuleBehavior().regenMode)
		{
		case ShieldModuleBehavior::RegenMode::Normal:
			break;
		case ShieldModuleBehavior::RegenMode::SpeedScaled:
		{
			const float speedCap = mMovementComponent.ResolveCurrentSpeedCap(
				GetMovementSpeedCapMultiplier() * GetMovementSpeedMultiplier());
			const float speed = GetVectorLength(GetVelocity());
			const float speedRatio = speed >= mShieldComponent.GetModuleBehavior().minimumSpeed && speedCap > 0.f
				? std::clamp(speed / speedCap, 0.f, 1.f)
				: 0.f;
			shieldRegen = normalShieldRegen * mShieldComponent.GetModuleBehavior().speedMultiplierAtMax * speedRatio;
			allowShieldRecharge = true;
			mShieldComponent.ClearRechargeDelay();
			break;
		}
		case ShieldModuleBehavior::RegenMode::Continuous:
			shieldRegen = normalShieldRegen * mShieldComponent.GetModuleBehavior().continuousRateFraction;
			mShieldComponent.ClearRechargeDelay();
			break;
		}
		mShieldComponent.TickBatteryReserve(deltaTime, shieldRegen, allowShieldRecharge);
		mShieldComponent.Tick(deltaTime, shieldRegen, allowShieldRecharge);
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
			
			// Headless or sprite-less ships still take damage; they just have
			// nothing to tint.
			if (SpriteComponent* spriteComponent = FindComponent<SpriteComponent>(); spriteComponent && spriteComponent->HasSprite())
			{
				spriteComponent->GetSprite()->setColor(LerpColor(sf::Color::White, mBlinkColor, mBlinkTime));
			}
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
		VEGA_PROFILE_SCOPE("Ship.Blow");
		Explosion::SpawnExplosion(GetWorld(), GetActorLocation(), Explosion::GetPreset(GetExplosionType()));
		{
			VEGA_PROFILE_SCOPE("Ship.Blow.Blew");
			Blew();
		}
		{
			VEGA_PROFILE_SCOPE("Ship.Blow.ShipRuntimeClear");
			mShipRuntime.Clear();
		}
		// Pending means the ability system kept retained cleanup (for example EndFire debt) and its
		// clear stays requested. The ship is destroyed regardless: a destroyed actor is no longer
		// ticked, so nothing retries the clear and the retained ability state is released by the
		// combat runtime's destructor with the actor. Callback exceptions still propagate from Clear().
		{
			VEGA_PROFILE_SCOPE("Ship.Blow.CombatRuntimeClear");
			static_cast<void>(mCombatRuntime.Clear());
		}
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

	namespace
	{
		// Scales the shield's damage multiplier by the Armor reduction that the shield takes with armorShare.
		// Mirrors CombatRuntime::ApplyHullDamageMitigation (shred, then penetration) without applying it, so the
		// hull step still applies once to the damage that passes the shield. Factor 1 means no change.
		float ResolveShieldArmorShareFactor(const DamageContext& context, float armorShare, const sas::AttributeSystem& attributes)
		{
			const float armor = std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::Armor));
			const float shreddedArmor = armor * (1.f - std::clamp(context.targetArmorShred, 0.f, 1.f));
			const float armorDamageMultiplier = sas::AttributeMath::GetArmorDamageMultiplier(shreddedArmor);
			const float hullDamageMultiplier = armorDamageMultiplier +
				context.payload.armorPenetration * (1.f - armorDamageMultiplier);
			return 1.f - std::clamp(armorShare, 0.f, 1.f) * (1.f - std::clamp(hullDamageMultiplier, 0.f, 1.f));
		}
	}

	LethalDamageResolution SpaceShip::ResolveLethalDamage(const DamageContext& context)
	{
		const LethalDamageResolution prevention = mCombatRuntime.GetOwnerDeathPreventions().Resolve(*this, context);
		if (prevention != LethalDamageResolution::None) return prevention;
		auto* source = context.source ? dynamic_cast<Combatant*>(context.source) : nullptr;
		return source ? source->GetCombatRuntime().GetLethalHitInterceptors().Resolve(*this, context) : LethalDamageResolution::None;
	}

	void SpaceShip::ReceiveDamage(DamageContext context)
	{
		if ((!context.deferredDamage && (IsInvulnerable() || GetCombatRuntime().BlocksIncomingDamage())) ||
			context.remainingDamage <= 0.f)
		{
			return;
		}

		context.target = this;

		context.shieldDamage = 0.f;
		if (!context.deferredDamage) mCombatRuntime.ProcessIncomingDamage(context);
		mDeferredDamage.Defer(context);
		if (context.remainingDamage > 0.f)
		{
			float shieldDamageMultiplier = context.payload.shieldDamageMultiplier;
			if (mShieldComponent.GetModuleBehavior().armorShare > 0.f && mShieldComponent.GetShield() > 0.f)
			{
				shieldDamageMultiplier *= ResolveShieldArmorShareFactor(context,
					mShieldComponent.GetModuleBehavior().armorShare,
					mCombatRuntime.GetAbilitySystemComponent().GetAttributes());
			}
			ShieldImpactSide impactSide = ShieldImpactSide::Shared;
			if (mShieldComponent.GetModuleBehavior().frontRearSplit)
			{
				const Actor* originActor = context.deliveryActor ? context.deliveryActor : context.source;
				if (originActor)
				{
					auto approach = originActor->GetActorLocation() - GetActorLocation();
					if (context.deliveryType == DamageDeliveryType::Projectile && context.deliveryActor) approach = -context.deliveryActor->GetVelocity();
					if (approach.x == 0.f && approach.y == 0.f) approach = originActor->GetActorLocation() - GetActorLocation();
					const auto forward = GetActorForwardDirection();
					if (approach.x != 0.f || approach.y != 0.f) impactSide = forward.x * approach.x + forward.y * approach.y >= 0.f ? ShieldImpactSide::Front : ShieldImpactSide::Rear;
				}
			}
			context.incomingShieldDamage = context.remainingDamage * std::max(0.f, context.payload.shieldDamageMultiplier);
			const float shieldAbsorbedDamage = mShieldComponent.AbsorbDamage(
				context.remainingDamage,
				shieldDamageMultiplier,
				context.payload.shieldRegenerationDelay,
				context.payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed,
				impactSide
			);
			context.remainingDamage = std::max(0.f, context.remainingDamage - shieldAbsorbedDamage);
			context.absorbedDamage += shieldAbsorbedDamage;
			context.shieldDamage = shieldAbsorbedDamage * std::max(0.f, shieldDamageMultiplier);
			mCombatRuntime.ApplyHullDamageMitigation(context);

			const float healthBeforeDamage = mHealthComponent.GetHealth();
			const LethalDamageResolution lethal = healthBeforeDamage > 0.f && healthBeforeDamage - context.remainingDamage <= 0.f ? ResolveLethalDamage(context) : LethalDamageResolution::None;
			if (lethal != LethalDamageResolution::None) context.remainingDamage = 0.f;
			const float healthAfterLethalResolution = mHealthComponent.GetHealth();
			mHealthComponent.ChangeHealth(-context.remainingDamage);
			// A converted kill may have changed health itself (the target lives on); the hit still dealt what it would have.
			const float healthDamage = lethal == LethalDamageResolution::ConvertedKill ? healthBeforeDamage : std::max(0.f, healthAfterLethalResolution - mHealthComponent.GetHealth());
			context.appliedDamage = shieldAbsorbedDamage + healthDamage;
			context.targetWasKilled = lethal == LethalDamageResolution::ConvertedKill || (healthBeforeDamage > 0.f &&
				mHealthComponent.GetHealth() <= 0.f);
		}

		mCombatRuntime.NotifyDamageResolved(context);
	}
}

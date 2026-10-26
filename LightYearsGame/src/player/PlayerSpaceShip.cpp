#include "gameConfigs/CollisionLayers.h"
#include "gameConfigs/GameplayTags.h"
#include "player/PlayerSpaceShip.h"

#include "gameplay/ability/loadout/DefaultAbilityLoadout.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/input/AbilityInputSchema.h"
#include "enemy/EnemyActor.h"
#include <framework/LightComponent.h>
#include <framework/SpriteComponent.h>
#include <framework/MathUtility.h>
#include <framework/World.h>
#include <framework/reflection/Reflection.h>
#include <stdexcept>

namespace vega
{
	VEGA_REFLECT(PlayerSpaceShip, SpaceShip)
	{
		VEGA_SPAWNABLE([](World& world)
		{
			const auto* ship = content::ShipContentCatalog::FindById(DefaultPlayerShipId);
			if (!ship) throw std::runtime_error("Default player ship is unavailable");
			return world.SpawnActor<PlayerSpaceShip>(*ship).lock();
		});
		VEGA_REQUIRES(PlayerMovementComponent);
		VEGA_CATEGORY("Player");
		VEGA_PROPERTY("Ship", GetShip, SetShip).Tooltip("Construction only: changing Ship is refused; place a prefab with the desired catalog Ship instead.").Construction();
		VEGA_PROPERTY("Speed", GetSpeed, SetSpeed).Range(0.f, 100000.f);
		VEGA_PROPERTY("Use Screen Clamp", GetUseScreenClamp, SetUseScreenClamp);
		VEGA_READONLY("In Combat", IsInCombat);
		VEGA_READONLY("Combat Window Remaining", GetCombatWindowRemaining);
	}

	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef)
		: SpaceShip(owningWorld, shipDef)
		, mShip(shipDef.catalogId)
		, mAbilityLoadout(GetAbilitySystemComponent())
		, mPlayerMovement(AddComponent<PlayerMovementComponent>())
		, mModuleTriggerRuntime(GetAbilitySystemComponent(), GetShipRuntime(), GetRuntimeModifiers(), GetCombatRuntime(), GetShieldComponent())
		, mModuleApplier(GetShipRuntime(), GetAbilitySystemComponent(), mModuleTriggerRuntime, GetCombatRuntime())
		, mInvulnerabilityTime(2.f)
		, mInvulnerabilityBlinkInterval(0.4f)
		, mInvulnerabilityBlinkTimer(0.f)
		, mInvulnerabilityDir(1.f)
		, mCollisionDamage(shipDef.collisionDamage)
	{
		SetInvulnerability(true);
		for (const DefaultAbilityLoadoutEntry& entry : GetDefaultAbilityLoadout())
		{
			std::string failureReason;
			if (!mAbilityLoadout.EquipAbility(entry.abilityId, entry.slot, &failureReason))
			{
				VEGA_GAME_ERROR(
					"Invalid default player loadout ability '%s': %s",
					entry.abilityId.c_str(),
					failureReason.c_str()
				);
			}
		}
		SetActorRotation(0.f);
		// Player hulls have a left and a right engine; content validation
		// requires both for Ship.Player.* entries.
		if (shipDef.engineMounts.size() >= 2)
		{
			mAttachedLightTags.push_back(GetOrAddComponent<LightComponent>().AddLight(GameTags::Ship::Engine_Left, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
			mAttachedLightTags.push_back(GetOrAddComponent<LightComponent>().AddLight(GameTags::Ship::Engine_Right, shipDef.engineMounts[1].pointLightDef, shipDef.engineMounts[1].offset));
		}
	}

	void PlayerSpaceShip::SetShip(const std::string& ship)
	{
		if (ship != mShip) throw std::runtime_error("Ship is construction-only; place a prefab with the desired catalog Ship instead of changing an existing PlayerSpaceShip.");
	}

	void PlayerSpaceShip::SetupCollisionLayers()
	{
		SetCollisionLayer(GameCollision::Player);
		SetCollisionMask(
			GameCollision::Enemy |
			GameCollision::EnemyBullet |
			GameCollision::Powerup |
			GameCollision::RelayProjectile
		);
	}

	void PlayerSpaceShip::BeginPlay()
	{
		SpaceShip::BeginPlay();
		// Read-only observation of the combat runtime's resolved-damage boundary: damage taken
		// fires onDamageResolved here and damage dealt fires onDamageDealt on the source ship.
		GetCombatRuntime().onDamageResolved.BindAction(GetWeakPtr(), &PlayerSpaceShip::OnCombatDamageObserved);
		GetCombatRuntime().onDamageDealt.BindAction(GetWeakPtr(), &PlayerSpaceShip::OnCombatDamageObserved);
		TimerManager& timers = GetWorld()->GetTimerManager();
		timers.SetTimer(
			GetWeakPtr(),
			&PlayerSpaceShip::StopInvulnerability,
			mInvulnerabilityTime,
			false
		);
	}

	void PlayerSpaceShip::ApplyDamage(float amt)
	{
		DamageContext context;
		context.target = this;
		context.originalDamage = amt;
		context.remainingDamage = amt;
		ReceiveDamage(context);
	}

	void PlayerSpaceShip::ReceiveDamage(DamageContext context)
	{
		if (!context.deferredDamage && (IsInvulnerable() || GetCombatRuntime().BlocksIncomingDamage()))
		{
			return;
		}

		const float currentHealth = GetHealthComponent().GetHealth();
		VEGA_GAME_DEBUG("PlayerSpaceShip::ReceiveDamage - Current Health: %.1f, Damage: %.1f", currentHealth, context.remainingDamage);
		if (currentHealth - context.remainingDamage <= 0.f)
		{
			VEGA_GAME_INFO("Player ship dying");
		}
		SpaceShip::ReceiveDamage(context);
	}

	void PlayerSpaceShip::Tick(float deltaTime)
	{
		mPlayerMovement.Tick(deltaTime);
		SpaceShip::Tick(deltaTime);
		mCombatWindowRemaining = std::max(0.f, mCombatWindowRemaining - deltaTime);
		if (deltaTime > 0.f && !IsInPortalTransit())
		{
			const float distance = GetVectorLength(GetVelocity()) * deltaTime;
			if (distance > 0.f) onRunDistanceTraveled.Broadcast(distance);
		}
		mModuleApplier.Tick(deltaTime);
		mModuleTriggerRuntime.Tick(deltaTime);

		if (IsInvulnerable())
		{
			UpdateInvulnerability(deltaTime);
		}
	}

	LethalDamageResolution PlayerSpaceShip::ResolveLethalDamage(const DamageContext& context)
	{
		return mModuleApplier.TryPreventOwnerDeath(*this, context) ? LethalDamageResolution::Prevented : LethalDamageResolution::None;
	}

	bool PlayerSpaceShip::BeginPossession(EnemyActor& host)
	{
		if (IsPossessing() || host.GetIsPendingDestroy()) return false;
		for (const AbilityInputBinding& binding : AbilityInputSchema::GetBindings()) GetAbilitySystemComponent().SetLoadoutKeyInput(binding.slot, false, false);
		SuspendBody();
		SetActorLocation(host.GetActorLocation());
		// Control passes at once: the host's own behavior never acts again, even before this ship's next input tick.
		EnemyBehaviorIntent idle;
		idle.aimTargetLocation = host.GetActorLocation() + host.GetActorForwardDirection();
		host.SetExternalIntent(idle);
		mPlayerMovement.SetPossessedHost(std::static_pointer_cast<EnemyActor>(host.GetWeakActor().lock()));
		return true;
	}

	void PlayerSpaceShip::EndPossession(const sf::Vector2f& location)
	{
		if (const auto host = mPlayerMovement.GetPossessedHost()) host->ClearExternalControl();
		mPlayerMovement.ClearPossessedHost();
		ResumeBody(location);
	}

	void PlayerSpaceShip::OnCombatDamageObserved(const DamageContext& context)
	{
		if (context.appliedDamage <= 0.f) return;
		mCombatWindowRemaining = content::ModuleContentCatalog::GetZoneTable().combatWindowSeconds;
	}

	void PlayerSpaceShip::Shoot()
	{
	}

	bool PlayerSpaceShip::AssignSecondPrimaryWeapon(const std::string& weaponId)
	{
		auto* runtime = GetAbilitySystemComponent().GetSecondHardpointRuntime();
		return runtime && runtime->AssignSecondWeapon(weaponId);
	}

	bool PlayerSpaceShip::HandleWeaponInput(const sf::Event& event)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit()) return false;
		const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>();
		if (!wheel || wheel->wheel != sf::Mouse::Wheel::Vertical || wheel->delta == 0.f) return false;
		auto* runtime = GetAbilitySystemComponent().GetSecondHardpointRuntime();
		return runtime && runtime->SwitchWeapon();
	}

	void PlayerSpaceShip::StopInvulnerability()
	{
		if (SpriteComponent* spriteComponent = FindComponent<SpriteComponent>(); spriteComponent && spriteComponent->HasSprite())
		{
			spriteComponent->GetSprite()->setColor({ 255, 255, 255, 255 });
		}
		SetInvulnerability(false);
		if (LightComponent* lights = FindComponent<LightComponent>())
		{
			lights->SetAllLightsIntensity(GameTags::Ship::Engine_Left, 1.5f);
			lights->SetAllLightsIntensity(GameTags::Ship::Engine_Right, 1.5f);
		}
	}

	void PlayerSpaceShip::UpdateInvulnerability(float deltaTime)
	{
		mInvulnerabilityBlinkTimer += deltaTime * mInvulnerabilityDir;
		if (mInvulnerabilityBlinkTimer < 0 || mInvulnerabilityBlinkTimer > mInvulnerabilityBlinkInterval)
		{
			mInvulnerabilityDir *= -1;
		}

		const float blinkAlpha = mInvulnerabilityBlinkTimer / mInvulnerabilityBlinkInterval;
		if (SpriteComponent* spriteComponent = FindComponent<SpriteComponent>(); spriteComponent && spriteComponent->HasSprite())
		{
			spriteComponent->GetSprite()->setColor(LerpColor({ 255, 255, 255, 96 }, { 255, 255, 255, 160 }, blinkAlpha));
		}
		const float lightIntensity = 0.3f + (blinkAlpha * 5.f);
		if (LightComponent* lights = FindComponent<LightComponent>())
		{
			lights->SetAllLightsIntensity(GameTags::Ship::Engine_Left, lightIntensity);
			lights->SetAllLightsIntensity(GameTags::Ship::Engine_Right, lightIntensity);
		}
	}

	void PlayerSpaceShip::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);
		if (otherActor && !IsInPortalTransit() && GetCanCollide() && !IsInvulnerable() &&
			CanApplyContactDamage(*this, *otherActor))
		{
			ApplyCombatDamage(*otherActor, mCollisionDamage, this);
		}
	}

	float PlayerSpaceShip::GetMovementSpeedCapMultiplier() const
	{
		return mPlayerMovement.GetMovementSpeedCapMultiplier();
	}

	float PlayerSpaceShip::GetMovementTurnCapabilityMultiplier() const
	{
		return mPlayerMovement.GetMovementTurnCapabilityMultiplier();
	}

	bool PlayerSpaceShip::IsAfterburnerRechargeBlocked() const
	{
		return mPlayerMovement.IsAfterburnerRechargeBlocked();
	}
}

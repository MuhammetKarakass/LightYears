#include "player/PlayerSpaceShip.h"

#include "gameplay/ability/loadout/DefaultAbilityLoadout.h"
#include "gameplay/combat/Combatant.h"
#include <framework/MathUtility.h>

namespace ly
{
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef)
		: SpaceShip(owningWorld, shipDef)
		, mAbilityLoadout(GetAbilitySystemComponent())
		, mPlayerMovement(*this)
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
				LY_GAME_ERROR(
					"Invalid default player loadout ability '%s': %s",
					entry.abilityId.c_str(),
					failureReason.c_str()
				);
			}
		}
		SetActorRotation(0.f);
		mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Left, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
		mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Right, shipDef.engineMounts[1].pointLightDef, shipDef.engineMounts[1].offset));
	}

	void PlayerSpaceShip::SetupCollisionLayers()
	{
		SetCollisionLayer(CollisionLayer::Player);
		SetCollisionMask(
			CollisionLayer::Enemy |
			CollisionLayer::EnemyBullet |
			CollisionLayer::Powerup |
			CollisionLayer::RelayProjectile
		);
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
		DamageContext context;
		context.target = this;
		context.originalDamage = amt;
		context.remainingDamage = amt;
		ReceiveDamage(context);
	}

	void PlayerSpaceShip::ReceiveDamage(DamageContext context)
	{
		if (IsInvulnerable() || GetCombatRuntime().BlocksIncomingDamage())
		{
			return;
		}

		const float currentHealth = GetHealthComponent().GetHealth();
		LY_GAME_DEBUG("PlayerSpaceShip::ReceiveDamage - Current Health: %.1f, Damage: %.1f", currentHealth, context.remainingDamage);
		if (currentHealth - context.remainingDamage <= 0.f)
		{
			LY_GAME_INFO("Player ship dying");
		}
		SpaceShip::ReceiveDamage(context);
	}

	void PlayerSpaceShip::Tick(float deltaTime)
	{
		mPlayerMovement.Tick(deltaTime);
		SpaceShip::Tick(deltaTime);

		if (IsInvulnerable())
		{
			UpdateInvulnerability(deltaTime);
		}
	}

	void PlayerSpaceShip::Shoot()
	{
	}

	void PlayerSpaceShip::StopInvulnerability()
	{
		GetSprite().value().setColor({ 255, 255, 255, 255 });
		SetInvulnerability(false);
		SetAllLightsIntensity(GameTags::Ship::Engine_Left, 1.5f);
		SetAllLightsIntensity(GameTags::Ship::Engine_Right, 1.5f);
	}

	void PlayerSpaceShip::UpdateInvulnerability(float deltaTime)
	{
		mInvulnerabilityBlinkTimer += deltaTime * mInvulnerabilityDir;
		if (mInvulnerabilityBlinkTimer < 0 || mInvulnerabilityBlinkTimer > mInvulnerabilityBlinkInterval)
		{
			mInvulnerabilityDir *= -1;
		}

		const float blinkAlpha = mInvulnerabilityBlinkTimer / mInvulnerabilityBlinkInterval;
		GetSprite().value().setColor(LerpColor({ 255, 255, 255, 96 }, { 255, 255, 255, 160 }, blinkAlpha));
		const float lightIntensity = 0.3f + (blinkAlpha * 5.f);
		SetAllLightsIntensity(GameTags::Ship::Engine_Left, lightIntensity);
		SetAllLightsIntensity(GameTags::Ship::Engine_Right, lightIntensity);
	}

	void PlayerSpaceShip::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);
		if (otherActor && GetCanCollide() && !IsInvulnerable())
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

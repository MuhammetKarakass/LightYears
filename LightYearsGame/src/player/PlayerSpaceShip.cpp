#include "player/PlayerSpaceShip.h"

#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/combat/Combatant.h"
#include <framework/MathUtility.h>

namespace ly
{
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef)
		: SpaceShip(owningWorld, shipDef)
		, mPlayerMovement(*this)
		, mInvulnerabilityTime(2.f)
		, mInvulnerabilityBlinkInterval(0.4f)
		, mInvulnerabilityBlinkTimer(0.f)
		, mInvulnerabilityDir(1.f)
		, mCollisionDamage(shipDef.collisionDamage)
	{
		SetInvulnerability(true);
		auto grantPlayerAbility = [this](const AbilityDefinition& definition)
		{
			std::string failureReason;
			const AbilityHandle handle = GetCombatRuntime().GetAbilities().GrantAbility(
				definition,
				&failureReason
			);
			if (!handle.IsValid())
			{
				LY_GAME_ERROR(
					"Invalid player ability '%s': %s",
					definition.abilityId.c_str(),
					failureReason.c_str()
				);
			}
		};
		grantPlayerAbility(AbilityData::Definitions::GravityAnomaly_Basic);
		grantPlayerAbility(AbilityData::Definitions::SunBeam_Strike_Basic);
		grantPlayerAbility(AbilityData::Definitions::Dash_Basic);
		grantPlayerAbility(AbilityData::Definitions::Rocket_Basic);

		SetActorRotation(0.f);
		mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Left, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
		mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Right, shipDef.engineMounts[1].pointLightDef, shipDef.engineMounts[1].offset));
	}

	void PlayerSpaceShip::SetupCollisionLayers()
	{
		SetCollisionLayer(CollisionLayer::Player);
		SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::EnemyBullet | CollisionLayer::Powerup);
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
		if (IsInvulnerable())
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

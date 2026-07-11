#include "player/PlayerSpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "framework/AssetManager.h"
#include "gameConfigs/AbilityConfig.h"
#include "gameplay/combat/Combatant.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef)
		:SpaceShip(owningWorld, shipDef),
		mSpeed(shipDef.speed.x),
		mMoveInput{ 0.f, 0.f },
		mSmoothedMoveInput{ 0.f, 0.f },
		mInvulnerabilityTime{ 2.f },
		mInvulnerabilityBlinkInterval{ 0.4f },
		mInvulnerabilityBlinkTimer{ 0.f },
		mInvulnerabilityDir{ 1.f },
		mCollisionDamage{ shipDef.collisionDamage }
	{
		SetInvulnerability(true);
		GetCombatRuntime().GetAbilities().GrantAbility(AbilityData::Definitions::Shield_Basic);
		GetCombatRuntime().GetAbilities().GrantAbility(AbilityData::Definitions::SunBeam_Strike_Basic);

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
		LOG("PlayerSpaceShip::ReceiveDamage - Current Health: %.1f, Damage: %.1f", currentHealth, context.remainingDamage);

		if (currentHealth - context.remainingDamage <= 0.f)
		{
			LOG("===== PLAYER SHIP DYING =====");
		}
		SpaceShip::ReceiveDamage(context);
	}

	void PlayerSpaceShip::Tick(float deltaTime)
	{
		mShaderTime += deltaTime;

		SetInput();
		ConsumeInput(deltaTime);
		SpaceShip::Tick(deltaTime);

		if (IsInvulnerable())
		{
			UpdateInvulnerability(deltaTime);
		}

	}

	void PlayerSpaceShip::SetInput()
	{
		mMoveInput = sf::Vector2f{ 0.f, 0.f };

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
		{
			mMoveInput.y = -1.f;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
		{
			mMoveInput.y = 1.f;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
		{
			mMoveInput.x = -1.f;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
		{
			mMoveInput.x = 1.f;
		}

		if (mUseScreenClamp)
		{
			ClampInputOnEdge();
		}

		const bool wantsToFire = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
		const bool wantsShield = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q);
		const bool wantsSunBeam = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E);

		GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::PrimaryFire, wantsToFire);
		GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::Ability1, wantsShield);
		GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::Ability2, wantsSunBeam);

		if (GetMovementMode() == ShipMovementMode::LegacyVelocity)
		{
			NormalizeInput();
		}
	}

	void PlayerSpaceShip::ConsumeInput(float deltaTime)
	{
		if (GetMovementMode() == ShipMovementMode::ThrustDrift)
		{
			const ShipMovementAttributes& movementAttributes = GetMovementAttributes();
			const float inputResponsiveness = std::max(0.f, movementAttributes.inputResponsiveness.currentValue);
			const float inputResponseAlpha = inputResponsiveness > 0.f
				? 1.f - std::exp(-inputResponsiveness * deltaTime)
				: 1.f;

			mSmoothedMoveInput = LerpVector(mSmoothedMoveInput, mMoveInput, inputResponseAlpha);

			const float forwardInput = std::clamp(-mSmoothedMoveInput.y, -1.f, 1.f);
			const float strafeInput = std::clamp(mSmoothedMoveInput.x, -1.f, 1.f);

			const float forwardThrust = forwardInput >= 0.f
				? movementAttributes.forwardThrust.currentValue
				: movementAttributes.reverseThrust.currentValue;
			const float strafeThrust = movementAttributes.strafeThrust.currentValue;

			const sf::Vector2f strafeDirection = GetAdaptiveScreenStrafeDirection();
			sf::Vector2f worldAcceleration =
				GetActorForwardDirection() * forwardInput * forwardThrust +
				strafeDirection * strafeInput * strafeThrust;

			const float dominantAxisMagnitude = std::max(
				std::abs(forwardInput) * forwardThrust,
				std::abs(strafeInput) * strafeThrust
			);
			const float accelerationLength = GetVectorLength(worldAcceleration);

			if (accelerationLength > dominantAxisMagnitude && dominantAxisMagnitude > 0.f)
			{
				worldAcceleration *= dominantAxisMagnitude / accelerationLength;
			}

			mVelocity += worldAcceleration * deltaTime;
			RotateTowardMouseCursor(deltaTime);
		}
		else
		{
			mSmoothedMoveInput = sf::Vector2f{ 0.f, 0.f };
			SetVelocity(mMoveInput * mSpeed);
		}

		mMoveInput.x = 0.f;
		mMoveInput.y = 0.f;
	}

	sf::Vector2f PlayerSpaceShip::GetAdaptiveScreenStrafeDirection() const
	{
		const sf::Vector2f screenRight{ 1.f, 0.f };
		const sf::Vector2f horizontalFacingStrafe{ 0.f, -1.f };
		const sf::Vector2f shipForward = GetActorForwardDirection();

		const float horizontalFacingAmount = std::abs(shipForward.x);
		const float blendStart = 0.25f;
		const float blendEnd = 0.85f;
		float blendAlpha = std::clamp((horizontalFacingAmount - blendStart) / (blendEnd - blendStart), 0.f, 1.f);
		blendAlpha = blendAlpha * blendAlpha * (3.f - 2.f * blendAlpha);

		sf::Vector2f strafeDirection = screenRight * (1.f - blendAlpha) + horizontalFacingStrafe * blendAlpha;
		NormalizeVector(strafeDirection);
		return strafeDirection;
	}

	void PlayerSpaceShip::NormalizeInput()
	{
		NormalizeVector(mMoveInput);
	}

	void PlayerSpaceShip::ClampInputOnEdge()
	{
		if (GetActorLocation().x - 40.f <= 0.f && mMoveInput.x == -1.f)
		{
			mMoveInput.x = 0.f;
		}
		else if (GetActorLocation().x + 40.f >= GetWindowSize().x && mMoveInput.x == 1.f)
		{
			mMoveInput.x = 0.f;
		}

		if (GetActorLocation().y - 40.f <= 0.f && mMoveInput.y == -1.f)
		{
			mMoveInput.y = 0.f;
		}
		else if (GetActorLocation().y + 40.f >= GetWindowSize().y && mMoveInput.y == 1.f)
		{
			mMoveInput.y = 0.f;
		}

		
	}

	void PlayerSpaceShip::RotateTowardMouseCursor(float deltaTime)
	{
		if (World* world = GetWorld())
		{
			RotateTowardWorldLocation(world->GetMouseWorldPosition(), deltaTime);
		}
	}

	void PlayerSpaceShip::Shoot()
	{
	}

	void PlayerSpaceShip::StopInvulnerability()
	{
		GetSprite().value().setColor({255,255,255,255});
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

		float blinkAlpha = mInvulnerabilityBlinkTimer / mInvulnerabilityBlinkInterval;
		GetSprite().value().setColor(LerpColor({ 255,255, 255, 96 }, { 255, 255, 255, 160 }, blinkAlpha));
		float lightIntensity = 0.3f + (blinkAlpha * 5.f);
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

}



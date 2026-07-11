#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"
#include "gameplay/GameplayAttribute.h"
#include "engineConfigs/EngineStructs.h"
#include "player/Reward.h"
#include "gameConfigs/WeaponStructs.h"

struct EngineMount
{
	sf::Vector2f offset;
	PointLightDefinition pointLightDef;
};

struct ShipMovementAttributes
{
	ly::GameplayAttribute forwardThrust;
	ly::GameplayAttribute reverseThrust;
	ly::GameplayAttribute strafeThrust;
	ly::GameplayAttribute angularTurnSpeed;
	ly::GameplayAttribute angularTurnResponsiveness;
	ly::GameplayAttribute linearDamping;
	ly::GameplayAttribute maxSpeed;
	ly::GameplayAttribute inputResponsiveness;
	ly::GameplayAttribute mouseAimDeadZone;

	ShipMovementAttributes(
		float inForwardThrust = 350.f,
		float inReverseThrust = 120.f,
		float inStrafeThrust = 150.f,
		float inAngularTurnSpeed = 100.f,
		float inAngularTurnResponsiveness = 2.f,
		float inLinearDamping = 0.55f,
		float inMaxSpeed = 520.f,
		float inInputResponsiveness = 14.f,
		float inMouseAimDeadZone = 28.f
	)
		: forwardThrust{ inForwardThrust },
		reverseThrust{ inReverseThrust },
		strafeThrust{ inStrafeThrust },
		angularTurnSpeed{ inAngularTurnSpeed },
		angularTurnResponsiveness{ inAngularTurnResponsiveness },
		linearDamping{ inLinearDamping },
		maxSpeed{ inMaxSpeed },
		inputResponsiveness{ inInputResponsiveness },
		mouseAimDeadZone{ inMouseAimDeadZone }
	{
	}
};

struct ShipDefinition
{
	std::string texturePath;
	float health;
	sf::Vector2f speed;

	float collisionDamage;
	unsigned int scoreAmt;
	int explosionType;

	ly::List<EngineMount> engineMounts;

	ly::List<ly::WeightedReward> rewards;
	PrimaryWeaponDefinition primaryWeaponDefinition;
	ShipMovementAttributes movementAttributes;

	ShipDefinition
	(
		const std::string& inTexturePath,
		float inHealth,
		const sf::Vector2f& inSpeed,
		float inCollisionDamage,
		float inScoreAmt,
		int inExplosionType,
		const ly::List<EngineMount>& inEngineMounts,
		const ly::List<ly::WeightedReward>& inRewards,
		const PrimaryWeaponDefinition& inPrimaryWeaponDefinition = PrimaryWeaponDefinition{},
		const ShipMovementAttributes& inMovementAttributes = ShipMovementAttributes{}
	)
		: texturePath(inTexturePath)
		, health(inHealth)
		, speed(inSpeed)
		, collisionDamage(inCollisionDamage)
		, scoreAmt(inScoreAmt)
		, explosionType(inExplosionType)
		, engineMounts(inEngineMounts)
		, rewards(inRewards)
		, primaryWeaponDefinition(inPrimaryWeaponDefinition)
		, movementAttributes(inMovementAttributes)
	{
	}
};



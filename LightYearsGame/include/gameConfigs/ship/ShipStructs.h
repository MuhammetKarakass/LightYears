#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include "framework/Core.h"
#include "attributes/GameplayAttribute.h"
#include "engineConfigs/EngineStructs.h"
#include "player/Reward.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/control/ControlResponse.h"

struct EngineMount
{
	sf::Vector2f offset;
	PointLightDefinition pointLightDef;
};

struct ShipMovementAttributes
{
	sas::GameplayAttribute forwardThrust;
	sas::GameplayAttribute reverseThrust;
	sas::GameplayAttribute strafeThrust;
	sas::GameplayAttribute angularTurnSpeed;
	sas::GameplayAttribute angularTurnResponsiveness;
	sas::GameplayAttribute linearDamping;
	sas::GameplayAttribute maxSpeed;
	sas::GameplayAttribute inputResponsiveness;
	sas::GameplayAttribute mouseAimDeadZone;
	float horizontalRatingToThrust = 60.f;
	float verticalRatingToThrust = 60.f;
	float ratingToMaxSpeed = 20.f;
	// Ratings retain their early-game impact but have diminishing marginal value at high stacks.
	float movementRatingDiminishingScale = 20.f;

	ShipMovementAttributes(
		float inForwardThrust = 350.f,
		float inReverseThrust = 120.f,
		float inStrafeThrust = 150.f,
		float inAngularTurnSpeed = 100.f,
		float inAngularTurnResponsiveness = 2.f,
		float inLinearDamping = 0.55f,
		float inMaxSpeed = 520.f,
		float inInputResponsiveness = 14.f,
		float inMouseAimDeadZone = 28.f,
		float inHorizontalRatingToThrust = 60.f,
		float inVerticalRatingToThrust = 60.f,
		float inRatingToMaxSpeed = 20.f,
		float inMovementRatingDiminishingScale = 20.f
	)
		: forwardThrust{ inForwardThrust },
		reverseThrust{ inReverseThrust },
		strafeThrust{ inStrafeThrust },
		angularTurnSpeed{ inAngularTurnSpeed },
		angularTurnResponsiveness{ inAngularTurnResponsiveness },
		linearDamping{ inLinearDamping },
		maxSpeed{ inMaxSpeed },
		inputResponsiveness{ inInputResponsiveness },
		mouseAimDeadZone{ inMouseAimDeadZone },
		horizontalRatingToThrust{ inHorizontalRatingToThrust },
		verticalRatingToThrust{ inVerticalRatingToThrust },
		ratingToMaxSpeed{ inRatingToMaxSpeed },
		movementRatingDiminishingScale{ inMovementRatingDiminishingScale }
	{
	}

	float GetDiminishingRatingContribution(float rating, float contributionPerRating) const
	{
		const float nonNegativeRating = std::max(0.f, rating);
		const float scale = std::max(0.001f, movementRatingDiminishingScale);
		return contributionPerRating * scale * (1.f - std::exp(-nonNegativeRating / scale));
	}
};

struct AttributeGrowthEntry
{
	sas::AttributeId attributeId;
	// Raw unit of the target owner attribute. Percentage-like attributes use
	// their established rating units (PercentageRatingScale = 100), not a
	// 0..1 fraction or a display-percent literal.
	float perLevel = 0.f;
};

struct ShipProgressionDefinition
{
	float baseXP = 100.f;
	float xpExponent = 1.25f;
	// Missing entries have zero natural growth. Values are flat bonuses per
	// completed level; this is not the retired base-growth multiplier model.
	ly::List<AttributeGrowthEntry> naturalGrowth;
};

struct ShipEnergyAttributes
{
	// EnergyPower is an owner attribute, but its level-one value belongs to the
	// ship profile. It is reactor output, never a consumable resource.
	float baseEnergyPower = 0.f;
	float baseMaxShield = 0.f;
	float shieldFullRechargeDuration = 5.5f;
	float baseShieldRechargeDelay = 3.f;

	float baseAfterburnerCapacity = 0.f;
	float afterburnerFullRechargeDuration = 8.f;
	float baseAfterburnerRechargeDelay = 1.5f;
	float baseAfterburnerSpeedMultiplier = 1.f;
	float baseAfterburnerAccelerationMultiplier = 1.f;
	float baseAfterburnerEnergyDrainPerSecond = 15.f;

	// The two closed configuration values split the linear reactor budget.
	// Content validation requires their sum to equal 1.0.
	float shieldAffinity = 0.5f;
	float afterburnerAffinity = 0.5f;

	float baseAfterburnerRampUpDuration = 0.22f;
	float baseAfterburnerRampDownDuration = 0.20f;
	float baseAfterburnerManeuverabilityMultiplier = 1.f;
};

struct OwnerAttributeBaseEntry
{
	sas::AttributeId attributeId;
	float baseValue = 0.f;
};

struct ShipDefinition
{
	std::string texturePath;
	float health;
	sf::Vector2f speed;

	float collisionDamage;
	unsigned int scoreAmt;
	// Kept separate from score so progression balance never depends on UI scoring.
	float shipXPReward;
	int explosionType;

	ly::List<EngineMount> engineMounts;

	ly::List<ly::WeightedReward> rewards;
	std::string primaryWeaponId;
	ShipMovementAttributes movementAttributes;
	ShipEnergyAttributes energyAttributes;
	ShipProgressionDefinition progressionDefinition;
	ly::ControlTargetClass controlTargetClass = ly::ControlTargetClass::Normal;
	ly::List<OwnerAttributeBaseEntry> baseOwnerAttributes;

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
		const std::string& inPrimaryWeaponId = {},
		const ShipMovementAttributes& inMovementAttributes = ShipMovementAttributes{},
		const ShipEnergyAttributes& inEnergyAttributes = ShipEnergyAttributes{},
		const ShipProgressionDefinition& inProgressionDefinition = ShipProgressionDefinition{},
		float inShipXPReward = -1.f,
		ly::ControlTargetClass inControlTargetClass = ly::ControlTargetClass::Normal,
		const ly::List<OwnerAttributeBaseEntry>& inBaseOwnerAttributes = {}
	)
		: texturePath(inTexturePath)
		, health(inHealth)
		, speed(inSpeed)
		, collisionDamage(inCollisionDamage)
		, scoreAmt(inScoreAmt)
		// Existing content keeps its current kill reward until it is explicitly rebalanced.
		, shipXPReward(inShipXPReward >= 0.f ? inShipXPReward : inScoreAmt)
		, explosionType(inExplosionType)
		, engineMounts(inEngineMounts)
		, rewards(inRewards)
		, primaryWeaponId(inPrimaryWeaponId)
		, movementAttributes(inMovementAttributes)
		, energyAttributes(inEnergyAttributes)
		, progressionDefinition(inProgressionDefinition)
		, controlTargetClass(inControlTargetClass)
		, baseOwnerAttributes(inBaseOwnerAttributes)
	{
	}
};



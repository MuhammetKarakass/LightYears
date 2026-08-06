#include "gameplay/content/ShipContentCatalog.h"

#include "gameConfigs/ship/ShipConfig.h"
#include "gameplay/content/ShipLoader.h"
#include "gameplay/content/WeaponContentCatalog.h"

#include <cmath>

namespace ly::content
{
	namespace
	{
		List<ShipLoader::LoadedDefinition>& GetDefinitions()
		{
			static List<ShipLoader::LoadedDefinition> definitions;
			return definitions;
		}

		bool& GetLoadedState()
		{
			static bool loaded = false;
			return loaded;
		}

		bool Fail(std::string* failureReason, const std::string& message)
		{
			if (failureReason)
			{
				*failureReason = message;
			}
			return false;
		}

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}

		bool IsValidShipDefinition(const ShipDefinition& definition)
		{
			const ShipMovementAttributes& movement = definition.movementAttributes;
			const ShipEnergyAttributes& energy = definition.energyAttributes;
			const ShipProgressionDefinition& progression = definition.progressionDefinition;
			return std::isfinite(definition.health) && definition.health > 0.f &&
				std::isfinite(definition.speed.x) && std::isfinite(definition.speed.y) &&
				IsFiniteNonNegative(definition.collisionDamage) &&
				IsFiniteNonNegative(definition.shipXPReward) &&
				IsFiniteNonNegative(movement.forwardThrust.baseValue) &&
				IsFiniteNonNegative(movement.reverseThrust.baseValue) &&
				IsFiniteNonNegative(movement.strafeThrust.baseValue) &&
				IsFiniteNonNegative(movement.angularTurnSpeed.baseValue) &&
				IsFiniteNonNegative(movement.angularTurnResponsiveness.baseValue) &&
				IsFiniteNonNegative(movement.linearDamping.baseValue) &&
				IsFiniteNonNegative(movement.maxSpeed.baseValue) &&
				IsFiniteNonNegative(movement.inputResponsiveness.baseValue) &&
				IsFiniteNonNegative(movement.mouseAimDeadZone.baseValue) &&
				IsFiniteNonNegative(movement.horizontalRatingToThrust) &&
				IsFiniteNonNegative(movement.verticalRatingToThrust) &&
				IsFiniteNonNegative(movement.ratingToMaxSpeed) &&
				IsFiniteNonNegative(movement.movementRatingDiminishingScale) &&
				IsFiniteNonNegative(energy.baseMaxShield) &&
				IsFiniteNonNegative(energy.shieldFullRechargeDuration) &&
				IsFiniteNonNegative(energy.baseShieldRechargeDelay) &&
				IsFiniteNonNegative(energy.baseAfterburnerCapacity) &&
				IsFiniteNonNegative(energy.afterburnerFullRechargeDuration) &&
				IsFiniteNonNegative(energy.baseAfterburnerRechargeDelay) &&
				IsFiniteNonNegative(energy.baseAfterburnerSpeedMultiplier) &&
				IsFiniteNonNegative(energy.baseAfterburnerAccelerationMultiplier) &&
				IsFiniteNonNegative(energy.baseAfterburnerEnergyDrainPerSecond) &&
				IsFiniteNonNegative(energy.maxShieldPerMaxEnergy) &&
				IsFiniteNonNegative(energy.afterburnerCapacityPerMaxEnergy) &&
				IsFiniteNonNegative(energy.baseAfterburnerRampUpDuration) &&
				IsFiniteNonNegative(energy.baseAfterburnerRampDownDuration) &&
				IsFiniteNonNegative(energy.baseAfterburnerManeuverabilityMultiplier) &&
				IsFiniteNonNegative(progression.baseXP) &&
				std::isfinite(progression.xpExponent) && progression.xpExponent > 0.f;
		}
	}

	bool ShipContentCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		std::string* failureReason)
	{
		const ShipLoader::Result loaded = ShipLoader::LoadFromFile(
			filePath,
			ShipData::Ship_Player_Fighter
		);
		if (!loaded.Succeeded())
		{
			return Fail(failureReason, loaded.error);
		}
		if (loaded.definitions.empty())
		{
			return Fail(failureReason, "Ship catalog is empty");
		}

		for (const ShipLoader::LoadedDefinition& definition : loaded.definitions)
		{
			if (definition.id.empty())
			{
				return Fail(failureReason, "Ship content ID cannot be empty");
			}
			if (!definition.definition.primaryWeaponId.empty() &&
				!WeaponContentCatalog::FindById(definition.definition.primaryWeaponId))
			{
				return Fail(
					failureReason,
					"Ship '" + definition.id + "' references missing weapon '" +
					definition.definition.primaryWeaponId + "'"
				);
			}
			if (!IsValidShipDefinition(definition.definition))
			{
				return Fail(
					failureReason,
					"Invalid numeric values in ship '" + definition.id + "'"
				);
			}
		}

		GetDefinitions() = loaded.definitions;
		GetLoadedState() = true;
		return true;
	}

	const ShipDefinition* ShipContentCatalog::FindById(const std::string& shipId)
	{
		for (const ShipLoader::LoadedDefinition& definition : GetDefinitions())
		{
			if (definition.id == shipId)
			{
				return &definition.definition;
			}
		}
		return nullptr;
	}

	const ShipDefinition& ShipContentCatalog::GetPlayerFighterDefinition()
	{
		const ShipDefinition* loaded = FindById("Player.Fighter");
		return loaded ? *loaded : ShipData::Ship_Player_Fighter;
	}

	bool ShipContentCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}

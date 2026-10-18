#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/RetainedContent.h"

#include "gameConfigs/ship/ShipConfig.h"
#include "gameplay/content/ShipLoader.h"
#include "gameplay/content/ShipPrefabSpawner.h"
#include <framework/AssetManager.h>
#include <framework/EngineContext.h>
#include "gameplay/progression/ShipProgression.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <cmath>

namespace vega::content
{
	namespace
	{
		constexpr char kPlayerShipIdPrefix[] = "Ship.Player.";

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

		bool IsValidProgression(const ShipProgressionDefinition& progression)
		{
			if (!IsFiniteNonNegative(progression.baseXP) || progression.baseXP <= 0.f ||
				!std::isfinite(progression.xpExponent) || progression.xpExponent < 0.f ||
				!IsFiniteNonNegative(progression.xpPerLevel))
			{
				return false;
			}
			for (std::size_t index = 0; index < progression.naturalGrowth.size(); ++index)
			{
				const AttributeGrowthEntry& growth = progression.naturalGrowth[index];
				if (!IsNaturalGrowthAttribute(growth.attributeId) ||
					!IsFiniteNonNegative(growth.perLevel))
				{
					return false;
				}
				for (std::size_t previous = 0; previous < index; ++previous)
				{
					if (progression.naturalGrowth[previous].attributeId == growth.attributeId)
					{
						return false;
					}
				}
			}
			return true;
		}

		bool IsValidBaseOwnerAttributes(const List<OwnerAttributeBaseEntry>& baseOwnerAttributes)
		{
			for (std::size_t index = 0; index < baseOwnerAttributes.size(); ++index)
			{
				const OwnerAttributeBaseEntry& entry = baseOwnerAttributes[index];
				if (!IsAllowedBaseOwnerCombatAttribute(entry.attributeId) ||
					!IsFiniteNonNegative(entry.baseValue))
				{
					return false;
				}
				for (std::size_t previous = 0; previous < index; ++previous)
				{
					if (baseOwnerAttributes[previous].attributeId == entry.attributeId)
					{
						return false;
					}
				}
			}
			return true;
		}
	}

	bool ShipContentCatalog::IsValidShipDefinition(const ShipDefinition& definition)
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
			IsFiniteNonNegative(energy.baseEnergyPower) &&
			IsFiniteNonNegative(energy.baseMaxShield) &&
			IsFiniteNonNegative(energy.maxShieldPerLevel) &&
			IsFiniteNonNegative(energy.shieldFullRechargeDuration) &&
			IsFiniteNonNegative(energy.baseShieldRechargeDelay) &&
			IsFiniteNonNegative(energy.baseAfterburnerCapacity) &&
			IsFiniteNonNegative(energy.afterburnerCapacityPerLevel) &&
			IsFiniteNonNegative(energy.afterburnerFullRechargeDuration) &&
			IsFiniteNonNegative(energy.baseAfterburnerRechargeDelay) &&
			IsFiniteNonNegative(energy.baseAfterburnerSpeedMultiplier) &&
			IsFiniteNonNegative(energy.baseAfterburnerAccelerationMultiplier) &&
			IsFiniteNonNegative(energy.baseAfterburnerEnergyDrainPerSecond) &&
			IsFiniteNonNegative(energy.baseAfterburnerRampUpDuration) &&
			IsFiniteNonNegative(energy.baseAfterburnerRampDownDuration) &&
			IsFiniteNonNegative(energy.baseAfterburnerManeuverabilityMultiplier) &&
			IsValidProgression(progression) &&
			IsValidBaseOwnerAttributes(definition.baseOwnerAttributes);
	}

	bool ShipContentCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		std::string* failureReason, bool preserveExisting)
	{

		if (!preserveExisting)
		{
			GetLoadedState() = false;
			RetainContent(GetDefinitions());
		}

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
			const auto configuredRoot = EngineContext::Current().Assets().GetAssetRootDirectory();
			const auto assetRoot = configuredRoot.empty() ? filePath.parent_path().parent_path().parent_path() : std::filesystem::path{ configuredRoot };
			if (definition.definition.prefabPath && !ValidateShipPrefab(EngineContext::Current(), assetRoot, *definition.definition.prefabPath, definition.id, failureReason)) return false;
			if (!definition.definition.primaryWeaponId.empty() &&
				!WeaponContentCatalog::FindById(definition.definition.primaryWeaponId))
			{
				return Fail(
					failureReason,
					"Ship '" + definition.id + "' references missing weapon '" +
					definition.definition.primaryWeaponId + "'"
				);
			}
			if (definition.id.rfind(kPlayerShipIdPrefix, 0) == 0 &&
				definition.definition.engineMounts.size() != 2)
			{
				return Fail(
					failureReason,
					"Player ship '" + definition.id + "' must have exactly two engine mounts"
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

		RetainContent(GetDefinitions());
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

	List<std::string> ShipContentCatalog::GetPlayerShipIds()
	{
		List<std::string> shipIds;
		for (const ShipLoader::LoadedDefinition& definition : GetDefinitions())
		{
			if (definition.id.rfind(kPlayerShipIdPrefix, 0) == 0)
			{
				shipIds.push_back(definition.id);
			}
		}
		return shipIds;
	}

	const ShipDefinition& ShipContentCatalog::GetPlayerShipDefinition(const std::string& shipId)
	{
		if (shipId.rfind(kPlayerShipIdPrefix, 0) == 0)
		{
			if (const ShipDefinition* definition = FindById(shipId))
			{
				return *definition;
			}
		}

		VEGA_GAME_ERROR("Cannot resolve player ship '%s'; using Fighter.", shipId.c_str());
		return GetPlayerFighterDefinition();
	}

	const ShipDefinition& ShipContentCatalog::GetPlayerFighterDefinition()
	{
		const ShipDefinition* loaded = FindById(DefaultPlayerShipId);
		return loaded ? *loaded : ShipData::Ship_Player_Fighter;
	}

	bool ShipContentCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}

#include "gameplay/content/ShipLoader.h"

#include "gameplay/content/ContentIdSchema.h"

#include "framework/JsonDocumentLoader.h"

#include <set>
#include <stdexcept>
#include <utility>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		sf::Vector2f ParseVector2(const Json& value)
		{
			if (!value.is_array() || value.size() != 2)
			{
				throw std::runtime_error("Expected a two-element ship vector");
			}
			return { value.at(0).get<float>(), value.at(1).get<float>() };
		}

		void ParseMovement(
			ShipMovementAttributes& movement,
			const Json& object
		)
		{
			movement.forwardThrust = object.at("forwardThrust").get<float>();
			movement.reverseThrust = object.at("reverseThrust").get<float>();
			movement.strafeThrust = object.at("strafeThrust").get<float>();
			movement.angularTurnSpeed = object.at("angularTurnSpeed").get<float>();
			movement.angularTurnResponsiveness = object.at("angularTurnResponsiveness").get<float>();
			movement.linearDamping = object.at("linearDamping").get<float>();
			movement.maxSpeed = object.at("maxSpeed").get<float>();
			movement.inputResponsiveness = object.at("inputResponsiveness").get<float>();
			movement.mouseAimDeadZone = object.at("mouseAimDeadZone").get<float>();
			movement.horizontalRatingToThrust = object.value("horizontalRatingToThrust", 60.f);
			movement.verticalRatingToThrust = object.value("verticalRatingToThrust", 60.f);
			movement.ratingToMaxSpeed = object.value("ratingToMaxSpeed", 20.f);
			movement.movementRatingDiminishingScale = object.value(
				"movementRatingDiminishingScale",
				20.f
			);
		}

		void ParseEnergy(
			ShipEnergyAttributes& energy,
			const Json& object
		)
		{
			energy.baseMaxShield = object.at("baseMaxShield").get<float>();
			energy.shieldFullRechargeDuration = object.at("shieldFullRechargeDuration").get<float>();
			energy.baseShieldRechargeDelay = object.at("baseShieldRechargeDelay").get<float>();
			energy.baseAfterburnerCapacity = object.at("baseAfterburnerCapacity").get<float>();
			energy.afterburnerFullRechargeDuration = object.at("afterburnerFullRechargeDuration").get<float>();
			energy.baseAfterburnerRechargeDelay = object.at("baseAfterburnerRechargeDelay").get<float>();
			energy.baseAfterburnerSpeedMultiplier = object.at("baseAfterburnerSpeedMultiplier").get<float>();
			energy.baseAfterburnerAccelerationMultiplier = object.at("baseAfterburnerAccelerationMultiplier").get<float>();
			energy.baseAfterburnerEnergyDrainPerSecond = object.at("baseAfterburnerEnergyDrainPerSecond").get<float>();
			energy.maxShieldPerMaxEnergy = object.at("maxShieldPerMaxEnergy").get<float>();
			energy.afterburnerCapacityPerMaxEnergy = object.at("afterburnerCapacityPerMaxEnergy").get<float>();
			energy.baseAfterburnerRampUpDuration = object.at("baseAfterburnerRampUpDuration").get<float>();
			energy.baseAfterburnerRampDownDuration = object.at("baseAfterburnerRampDownDuration").get<float>();
			energy.baseAfterburnerManeuverabilityMultiplier = object.at(
				"baseAfterburnerManeuverabilityMultiplier"
			).get<float>();
		}

		void ParseProgression(
			ShipProgressionDefinition& progression,
			const Json& object
		)
		{
			progression.baseXP = object.at("baseXP").get<float>();
			progression.xpExponent = object.at("xpExponent").get<float>();
			progression.growthOverrides.clear();
			for (const Json& growth : object.value("growthOverrides", Json::array()))
			{
				progression.growthOverrides.push_back({
					sas::AttributeId{ growth.at("attributeId").get<std::string>() },
					growth.at("multiplier").get<float>()
				});
			}
		}

		ShipLoader::LoadedDefinition ParseShip(
			const Json& object,
			const ShipDefinition& presentationBase
		)
		{
			ShipLoader::LoadedDefinition loaded{
				object.at("id").get<std::string>(),
				presentationBase
			};
			std::string shipIdFailure;
			if (!ContentIdSchema::ValidateShipId(loaded.id, &shipIdFailure))
			{
				throw std::runtime_error(shipIdFailure);
			}
			loaded.definition.health = object.at("health").get<float>();
			loaded.definition.speed = ParseVector2(object.at("speed"));
			loaded.definition.collisionDamage = object.at("collisionDamage").get<float>();
			loaded.definition.scoreAmt = object.at("score").get<unsigned int>();
			loaded.definition.shipXPReward = object.value(
				"shipXPReward",
				static_cast<float>(loaded.definition.scoreAmt)
			);
			loaded.definition.primaryWeaponId = object.value(
				"primaryWeaponId",
				std::string{}
			);
			if (!loaded.definition.primaryWeaponId.empty())
			{
				std::string weaponIdFailure;
				if (!ContentIdSchema::ValidateWeaponId(
					loaded.definition.primaryWeaponId,
					&weaponIdFailure
				))
				{
					throw std::runtime_error(weaponIdFailure);
				}
			}
			ParseMovement(loaded.definition.movementAttributes, object.at("movement"));
			ParseEnergy(loaded.definition.energyAttributes, object.at("energy"));
			ParseProgression(loaded.definition.progressionDefinition, object.at("progression"));
			return loaded;
		}
	}

	ShipLoader::Result ShipLoader::LoadFromFile(
		const std::filesystem::path& filePath,
		const ShipDefinition& presentationBase
	)
	{
		const JsonDocumentLoader::Result documentResult =
			JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded())
		{
			return Result{ {}, documentResult.error };
		}

		try
		{
			const Json& root = *documentResult.document;
			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, "Unsupported ship schema version" };
			}

			Result result;
			std::set<std::string> shipIds;
			for (const Json& ship : root.at("ships"))
			{
				LoadedDefinition definition = ParseShip(ship, presentationBase);
				if (definition.id.empty())
				{
					throw std::runtime_error("Ship ID cannot be empty");
				}
				if (!shipIds.insert(definition.id).second)
				{
					throw std::runtime_error(
						"Duplicate ship ID: " + definition.id
					);
				}
				result.definitions.push_back(std::move(definition));
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{
				{},
				"Failed to load ships from '" + filePath.string() + "': " + exception.what()
			};
		}
	}
}

#include "gameplay/content/ShipLoader.h"

#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include "framework/JsonDocumentLoader.h"

#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace vega::content
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

		vega::ControlTargetClass ParseControlTargetClass(const Json& value)
		{
			if (!value.is_string()) throw std::runtime_error("Ship controlTargetClass must be a string");
			const std::string name = value.get<std::string>();
			if (name == "Normal") return vega::ControlTargetClass::Normal;
			if (name == "Elite") return vega::ControlTargetClass::Elite;
			if (name == "MiniBoss") return vega::ControlTargetClass::MiniBoss;
			if (name == "Boss") return vega::ControlTargetClass::Boss;
			throw std::runtime_error("Unknown ship controlTargetClass: " + name);
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
			energy.baseEnergyPower = object.at("baseEnergyPower").get<float>();
			energy.baseMaxShield = object.at("baseMaxShield").get<float>();
			energy.maxShieldPerLevel = object.value("maxShieldPerLevel", 0.f);
			energy.shieldFullRechargeDuration = object.at("shieldFullRechargeDuration").get<float>();
			energy.baseShieldRechargeDelay = object.at("baseShieldRechargeDelay").get<float>();
			energy.baseAfterburnerCapacity = object.at("baseAfterburnerCapacity").get<float>();
			energy.afterburnerCapacityPerLevel = object.value("afterburnerCapacityPerLevel", 0.f);
			energy.afterburnerFullRechargeDuration = object.at("afterburnerFullRechargeDuration").get<float>();
			energy.baseAfterburnerRechargeDelay = object.at("baseAfterburnerRechargeDelay").get<float>();
			energy.baseAfterburnerSpeedMultiplier = object.at("baseAfterburnerSpeedMultiplier").get<float>();
			energy.baseAfterburnerAccelerationMultiplier = object.at("baseAfterburnerAccelerationMultiplier").get<float>();
			energy.baseAfterburnerEnergyDrainPerSecond = object.at("baseAfterburnerEnergyDrainPerSecond").get<float>();
			energy.baseAfterburnerRampUpDuration = object.at("baseAfterburnerRampUpDuration").get<float>();
			energy.baseAfterburnerRampDownDuration = object.at("baseAfterburnerRampDownDuration").get<float>();
			energy.baseAfterburnerManeuverabilityMultiplier = object.at(
				"baseAfterburnerManeuverabilityMultiplier"
			).get<float>();
		}

		vega::List<vega::WeightedReward> ParseRewards(const Json& value)
		{
			if (!value.is_array()) throw std::runtime_error("Ship rewards must be an array");
			vega::List<vega::WeightedReward> rewards;
			float totalWeight = 0.f;
			for (const Json& entry : value)
			{
				const std::string type = entry.at("type").get<std::string>();
				vega::RewardFactoryFunc factory;
				if (type == "Health") factory = &vega::CreateRewardHealth;
				else if (type == "Life") factory = &vega::CreateRewardLife;
				else if (type == "Shield") factory = &vega::CreateRewardShield;
				else throw std::runtime_error("Unknown ship reward type: " + type);
				const float weight = entry.at("weight").get<float>();
				if (!std::isfinite(weight) || weight < 0.f || weight > 1.f)
					throw std::runtime_error("Ship reward weight must be finite and within [0, 1]");
				totalWeight += weight;
				rewards.push_back({ std::move(factory), weight });
			}
			if (!std::isfinite(totalWeight) || totalWeight > 1.f)
				throw std::runtime_error("Ship reward weights must total no more than 1");
			return rewards;
		}

		void ParseProgression(
			ShipProgressionDefinition& progression,
			const Json& object
		)
		{
			progression.baseXP = object.at("baseXP").get<float>();
			progression.xpExponent = object.at("xpExponent").get<float>();
			progression.xpPerLevel = object.value("xpPerLevel", 0.f);
			progression.naturalGrowth.clear();
			for (const Json& growth : object.value("naturalGrowth", Json::array()))
			{
				progression.naturalGrowth.push_back({
					sas::AttributeId{ growth.at("attributeId").get<std::string>() },
					growth.at("perLevel").get<float>()
				});
			}
		}

		ShipSchematic ParseSchematic(const Json& object)
		{
			if (!object.is_object())
			{
				throw std::runtime_error("Ship schematic must be an object");
			}
			std::map<char, GameplayTag> legend;
			for (auto it = object.at("legend").begin(); it != object.at("legend").end(); ++it)
			{
				if (it.key().size() != 1 || it.key()[0] == '.')
				{
					throw std::runtime_error("Schematic legend keys must be a single character other than '.'");
				}
				const GameplayTag zone{ it.value().get<std::string>() };
				std::string zoneFailure;
				if (!GameplayTagSchema::Validate(zone, GameplayTagKind::ModuleZone, &zoneFailure))
				{
					throw std::runtime_error("Schematic legend '" + it.key() + "': " + zoneFailure);
				}
				legend[it.key()[0]] = zone;
			}
			// Optional: characters that mean "this zone, but locked".
			std::map<char, GameplayTag> lockedLegend;
			if (object.contains("lockedLegend"))
			{
				for (auto it = object.at("lockedLegend").begin(); it != object.at("lockedLegend").end(); ++it)
				{
					if (it.key().size() != 1 || it.key()[0] == '.' || legend.count(it.key()[0]))
					{
						throw std::runtime_error("Schematic lockedLegend keys must be a single character that is not '.' or in the legend");
					}
					const GameplayTag zone{ it.value().get<std::string>() };
					std::string zoneFailure;
					if (!GameplayTagSchema::Validate(zone, GameplayTagKind::ModuleZone, &zoneFailure))
					{
						throw std::runtime_error("Schematic lockedLegend '" + it.key() + "': " + zoneFailure);
					}
					lockedLegend[it.key()[0]] = zone;
				}
			}

			const Json& rows = object.at("rows");
			if (!rows.is_array() || rows.empty())
			{
				throw std::runtime_error("Schematic rows must be a non-empty array");
			}
			ShipSchematic schematic;
			schematic.height = static_cast<int>(rows.size());
			schematic.width = static_cast<int>(rows.front().get<std::string>().size());
			if (schematic.width == 0)
			{
				throw std::runtime_error("Schematic rows must not be empty");
			}
			for (const Json& rowValue : rows)
			{
				const std::string row = rowValue.get<std::string>();
				if (static_cast<int>(row.size()) != schematic.width)
				{
					throw std::runtime_error("Schematic rows must all have the same width");
				}
				for (const char cell : row)
				{
					if (cell == '.')
					{
						schematic.cellZones.push_back(GameplayTag{});
						schematic.cellLocked.push_back(0);
						continue;
					}
					const auto found = legend.find(cell);
					if (found == legend.end())
					{
						const auto lockedFound = lockedLegend.find(cell);
						if (lockedFound == lockedLegend.end())
						{
							throw std::runtime_error(std::string("Schematic uses a character missing from its legend: ") + cell);
						}
						schematic.cellZones.push_back(lockedFound->second);
						schematic.cellLocked.push_back(1);
						continue;
					}
					schematic.cellZones.push_back(found->second);
					schematic.cellLocked.push_back(0);
				}
			}

			for (const Json& entry : object.value("defaultModules", Json::array()))
			{
				DefaultModulePlacement placement;
				const std::string moduleId = entry.at("moduleId").get<std::string>();
				std::string moduleIdFailure;
				if (!ContentIdSchema::ValidateModuleId(moduleId, &moduleIdFailure))
				{
					throw std::runtime_error(moduleIdFailure);
				}
				placement.moduleId = sas::ContentId{ moduleId };
				placement.variantIndex = entry.at("variant").get<int>();
				const Json& origin = entry.at("origin");
				if (!origin.is_array() || origin.size() != 2)
				{
					throw std::runtime_error("Default module origin must be [x, y]");
				}
				placement.origin = sf::Vector2i{ origin.at(0).get<int>(), origin.at(1).get<int>() };
				placement.rotationQuarterTurns = entry.at("rotation").get<int>();
				if (placement.rotationQuarterTurns < 0 || placement.rotationQuarterTurns > 3)
				{
					throw std::runtime_error("Default module rotation must be 0..3 quarter turns");
				}
				schematic.defaultModules.push_back(std::move(placement));
			}
			return schematic;
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
			loaded.definition.texturePath = object.value("texturePath", loaded.definition.texturePath);
			if (object.contains("engineMounts"))
			{
				// Offsets come from JSON so each hull places its exhaust lights;
				// the light style stays the presentation base's engine light.
				const Json& mounts = object.at("engineMounts");
				if (!mounts.is_array())
				{
					throw std::runtime_error("Ship engineMounts must be an array");
				}
				const PointLightDefinition engineLight = presentationBase.engineMounts.empty()
					? PointLightDefinition{}
					: presentationBase.engineMounts.front().pointLightDef;
				loaded.definition.engineMounts.clear();
				for (const Json& mount : mounts)
				{
					const sf::Vector2f offset = ParseVector2(mount.at("offset"));
					if (!std::isfinite(offset.x) || !std::isfinite(offset.y))
					{
						throw std::runtime_error("Ship engine mount offset must be finite");
					}
					loaded.definition.engineMounts.push_back(EngineMount{ offset, engineLight });
				}
			}
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
			loaded.definition.controlTargetClass = vega::ControlTargetClass::Normal;
			if (object.contains("controlTargetClass"))
			{
				loaded.definition.controlTargetClass = ParseControlTargetClass(object.at("controlTargetClass"));
			}
			loaded.definition.rewards = ParseRewards(object.value("rewards", Json::array()));
			loaded.definition.baseOwnerAttributes.clear();
			for (const Json& baseAttr : object.value("baseOwnerAttributes", Json::array()))
			{
				loaded.definition.baseOwnerAttributes.push_back({
					sas::AttributeId{ baseAttr.at("attributeId").get<std::string>() },
					baseAttr.at("baseValue").get<float>()
				});
			}
			loaded.definition.schematic.reset();
			if (object.contains("schematic"))
			{
				loaded.definition.schematic = ParseSchematic(object.at("schematic"));
			}
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

		return LoadFromJson(*documentResult.document, presentationBase);
	}

	ShipLoader::Result ShipLoader::LoadFromJson(const nlohmann::json& root, const ShipDefinition& presentationBase, const std::string& selectedId)
	{
		try
		{

			if (root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, "Unsupported ship schema version" };
			}

			Result result;
			std::set<std::string> shipIds;
			for (const Json& ship : root.at("ships"))
			{
				if (!selectedId.empty() && ship.value("id", std::string{}) != selectedId) continue;
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
				"Failed to load ships from '" + std::string{ "content document" } + "': " + exception.what()
			};
		}
	}
}

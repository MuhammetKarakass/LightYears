#include "gameplay/content/EnemyCombatProfileLoader.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/enemy/EnemyBehaviorProfile.h"
#include "gameplay/attributes/AttributeIds.h"
#include "framework/JsonDocumentLoader.h"

#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		sas::AbilitySlot ParseLoadoutSlot(const Json& slotJson)
		{
			if (!slotJson.is_string())
			{
				throw std::runtime_error("Enemy loadout slot must be a string.");
			}
			const std::string slotStr = slotJson.get<std::string>();
			if (const std::optional<sas::AbilitySlot> slot = ParseEnemySlot(slotStr)) return *slot;
			throw std::runtime_error("Invalid enemy loadout slot: '" + slotStr + "'. Expected None, PrimaryFire, or Ability1-Ability4.");
		}

		EnemyPowerScalingPolicy ParseScalingPolicy(const Json& policyJson)
		{
			if (!policyJson.is_string())
			{
				throw std::runtime_error("Enemy powerScalingPolicy must be a string.");
			}
			const std::string policyStr = policyJson.get<std::string>();
			if (policyStr == "Disabled") return EnemyPowerScalingPolicy::Disabled;
			if (policyStr == "Allowed") return EnemyPowerScalingPolicy::Allowed;
			throw std::runtime_error("Unknown powerScalingPolicy: '" + policyStr + "'. Expected 'Disabled' or 'Allowed'.");
		}

		EnemyVariationRange ParseVariationRange(const Json& object, const char* field, const std::string& profileId)
		{
			if (!object.contains(field) || !object.at(field).is_object()) throw std::runtime_error("progression.variation." + std::string{ field } + " must be an object in profile '" + profileId + "'.");
			const Json& range = object.at(field);
			static const std::set<std::string> allowedKeys = { "minimumMultiplier", "maximumMultiplier" };
			for (auto it = range.begin(); it != range.end(); ++it) if (allowedKeys.find(it.key()) == allowedKeys.end()) throw std::runtime_error("Unknown variation key '" + it.key() + "' in profile '" + profileId + "'.");
			if (!range.contains("minimumMultiplier") || !range.contains("maximumMultiplier") || !range.at("minimumMultiplier").is_number() || !range.at("maximumMultiplier").is_number()) throw std::runtime_error("Variation range requires numeric minimumMultiplier and maximumMultiplier in profile '" + profileId + "'.");
			const EnemyVariationRange result{ range.at("minimumMultiplier").get<float>(), range.at("maximumMultiplier").get<float>() };
			if (!std::isfinite(result.minimumMultiplier) || !std::isfinite(result.maximumMultiplier) || result.minimumMultiplier < 0.f || result.maximumMultiplier < 0.f || result.minimumMultiplier > result.maximumMultiplier) throw std::runtime_error("Invalid variation range for '" + std::string{ field } + "' in profile '" + profileId + "'.");
			return result;
		}

		void ParseProgression(const Json& object, EnemyCombatProfile& profile)
		{
			if (!object.is_object()) throw std::runtime_error("progression must be an object in profile '" + profile.profileId + "'.");
			static const std::set<std::string> allowedKeys = { "naturalGrowth", "maxShieldPerLevel", "outgoingDamagePerLevel", "variation" };
			for (auto it = object.begin(); it != object.end(); ++it) if (allowedKeys.find(it.key()) == allowedKeys.end()) throw std::runtime_error("Unknown progression key '" + it.key() + "' in profile '" + profile.profileId + "'.");
			if (!object.contains("naturalGrowth") || !object.at("naturalGrowth").is_array()) throw std::runtime_error("progression.naturalGrowth must be an array in profile '" + profile.profileId + "'.");
			for (const Json& growthJson : object.at("naturalGrowth"))
			{
				static const std::set<std::string> allowedGrowthKeys = { "attributeId", "perLevel" };
				if (growthJson.is_object()) for (auto it = growthJson.begin(); it != growthJson.end(); ++it) if (allowedGrowthKeys.find(it.key()) == allowedGrowthKeys.end()) throw std::runtime_error("Unknown naturalGrowth key '" + it.key() + "' in profile '" + profile.profileId + "'.");
				if (!growthJson.is_object() || !growthJson.contains("attributeId") || !growthJson.contains("perLevel") || !growthJson.at("attributeId").is_string() || !growthJson.at("perLevel").is_number()) throw std::runtime_error("Each progression naturalGrowth entry requires attributeId and perLevel in profile '" + profile.profileId + "'.");
				const sas::AttributeId attributeId{ growthJson.at("attributeId").get<std::string>() };
				const float perLevel = growthJson.at("perLevel").get<float>();
				if (!attributeId.IsValid() || !std::isfinite(perLevel) || perLevel < 0.f) throw std::runtime_error("Invalid progression naturalGrowth in profile '" + profile.profileId + "'.");
				for (const AttributeGrowthEntry& existing : profile.progression.naturalGrowth) if (existing.attributeId == attributeId) throw std::runtime_error("Duplicate progression growth attribute in profile '" + profile.profileId + "'.");
				profile.progression.naturalGrowth.push_back({ attributeId, perLevel });
			}
			for (const char* field : { "maxShieldPerLevel", "outgoingDamagePerLevel" }) if (!object.contains(field) || !object.at(field).is_number()) throw std::runtime_error("progression." + std::string{ field } + " must be numeric in profile '" + profile.profileId + "'.");
			profile.progression.maxShieldPerLevel = object.at("maxShieldPerLevel").get<float>();
			profile.progression.outgoingDamagePerLevel = object.at("outgoingDamagePerLevel").get<float>();
			if (!std::isfinite(profile.progression.maxShieldPerLevel) || !std::isfinite(profile.progression.outgoingDamagePerLevel) || profile.progression.maxShieldPerLevel < 0.f || profile.progression.outgoingDamagePerLevel < 0.f) throw std::runtime_error("Invalid progression growth value in profile '" + profile.profileId + "'.");
			if (!object.contains("variation") || !object.at("variation").is_object()) throw std::runtime_error("progression.variation must be an object in profile '" + profile.profileId + "'.");
			const Json& variation = object.at("variation");
			static const std::set<std::string> allowedVariationKeys = { "maxHealth", "armor", "maxShield", "outgoingDamage" };
			for (auto it = variation.begin(); it != variation.end(); ++it) if (allowedVariationKeys.find(it.key()) == allowedVariationKeys.end()) throw std::runtime_error("Unknown variation channel '" + it.key() + "' in profile '" + profile.profileId + "'.");
			profile.progression.maxHealthVariation = ParseVariationRange(variation, "maxHealth", profile.profileId);
			profile.progression.armorVariation = ParseVariationRange(variation, "armor", profile.profileId);
			profile.progression.maxShieldVariation = ParseVariationRange(variation, "maxShield", profile.profileId);
			profile.progression.outgoingDamageVariation = ParseVariationRange(variation, "outgoingDamage", profile.profileId);
		}

		EnemyCombatProfile ParseProfile(const Json& object)
		{
			if (!object.is_object())
			{
				throw std::runtime_error("Each enemy profile entry must be a JSON object.");
			}

			static const std::set<std::string> allowedProfileKeys = {
				"id", "weapons", "powerScalingPolicy", "abilities", "allowContactDamageOnly", "progression"
			};
			for (auto it = object.begin(); it != object.end(); ++it)
			{
				if (allowedProfileKeys.find(it.key()) == allowedProfileKeys.end())
				{
					const std::string profileId = (object.contains("id") && object.at("id").is_string())
						? object.at("id").get<std::string>()
						: "unknown";
					throw std::runtime_error("Unknown key '" + it.key() + "' in profile '" + profileId + "'.");
				}
			}

			if (!object.contains("id") || !object.at("id").is_string())
			{
				throw std::runtime_error("Enemy profile requires a string 'id' field.");
			}

			EnemyCombatProfile profile;
			profile.profileId = object.at("id").get<std::string>();
			std::string schemaError;
			if (!ContentIdSchema::ValidateEnemyCombatProfileId(profile.profileId, &schemaError))
			{
				throw std::runtime_error("Invalid enemy combat profile ID format in profile: " + schemaError);
			}

			if (object.contains("weapons"))
			{
				const Json& weaponsJson = object.at("weapons");
				if (!weaponsJson.is_array())
				{
					throw std::runtime_error("Field 'weapons' must be an array in profile '" + profile.profileId + "'.");
				}
				static const std::set<std::string> allowedBindingKeys = { "weaponId", "slot", "level" };
				for (const Json& weaponObj : weaponsJson)
				{
					if (!weaponObj.is_object()) throw std::runtime_error("Each weapon binding in profile '" + profile.profileId + "' must be a JSON object.");
					for (auto it = weaponObj.begin(); it != weaponObj.end(); ++it)
						if (allowedBindingKeys.find(it.key()) == allowedBindingKeys.end()) throw std::runtime_error("Unknown key '" + it.key() + "' in weapon binding for profile '" + profile.profileId + "'.");
					if (!weaponObj.contains("weaponId") || !weaponObj.at("weaponId").is_string()) throw std::runtime_error("Weapon binding in profile '" + profile.profileId + "' requires a string 'weaponId'.");
					EnemyWeaponBinding binding;
					binding.weaponId = weaponObj.at("weaponId").get<std::string>();
					if (!ContentIdSchema::ValidateWeaponId(binding.weaponId, &schemaError)) throw std::runtime_error("Invalid weapon ID format '" + binding.weaponId + "' in profile '" + profile.profileId + "': " + schemaError);
					if (!weaponObj.contains("slot")) throw std::runtime_error("Weapon binding in profile '" + profile.profileId + "' requires a 'slot' field.");
					binding.slot = ParseLoadoutSlot(weaponObj.at("slot"));
					if (weaponObj.contains("level"))
					{
						const Json& levelJson = weaponObj.at("level");
						if (!levelJson.is_number_integer()) throw std::runtime_error("Weapon level must be an integer in profile '" + profile.profileId + "'.");
						binding.level = levelJson.get<int>();
						if (binding.level < 1) throw std::runtime_error("Weapon level must be at least 1 in profile '" + profile.profileId + "'.");
					}
					profile.weapons.push_back(std::move(binding));
				}
			}

			if (object.contains("powerScalingPolicy"))
			{
				profile.powerScalingPolicy = ParseScalingPolicy(object.at("powerScalingPolicy"));
			}
			if (object.contains("progression")) ParseProgression(object.at("progression"), profile);

			if (object.contains("allowContactDamageOnly"))
			{
				if (!object.at("allowContactDamageOnly").is_boolean())
				{
					throw std::runtime_error("allowContactDamageOnly must be a boolean in profile '" + profile.profileId + "'.");
				}
				profile.allowContactDamageOnly = object.at("allowContactDamageOnly").get<bool>();
			}

			if (object.contains("abilities"))
			{
				const Json& abilitiesJson = object.at("abilities");
				if (!abilitiesJson.is_array())
				{
					throw std::runtime_error("Field 'abilities' must be an array in profile '" + profile.profileId + "'.");
				}

				static const std::set<std::string> allowedBindingKeys = {
					"abilityId", "slot", "level"
				};

				for (const Json& abilityObj : abilitiesJson)
				{
					if (!abilityObj.is_object())
					{
						throw std::runtime_error("Each ability binding in profile '" + profile.profileId + "' must be a JSON object.");
					}

					for (auto it = abilityObj.begin(); it != abilityObj.end(); ++it)
					{
						if (allowedBindingKeys.find(it.key()) == allowedBindingKeys.end())
						{
							throw std::runtime_error("Unknown key '" + it.key() + "' in ability binding for profile '" + profile.profileId + "'.");
						}
					}

					if (!abilityObj.contains("abilityId") || !abilityObj.at("abilityId").is_string())
					{
						throw std::runtime_error("Ability binding in profile '" + profile.profileId + "' requires a string 'abilityId'.");
					}

					EnemyAbilityBinding binding;
					binding.abilityId = abilityObj.at("abilityId").get<std::string>();
					if (!ContentIdSchema::ValidateAbilityId(binding.abilityId, &schemaError))
					{
						throw std::runtime_error("Invalid ability ID format '" + binding.abilityId + "' in profile '" + profile.profileId + "': " + schemaError);
					}

					if (!abilityObj.contains("slot"))
					{
						throw std::runtime_error("Ability binding in profile '" + profile.profileId + "' requires a 'slot' field.");
					}
					binding.slot = ParseLoadoutSlot(abilityObj.at("slot"));

					if (abilityObj.contains("level"))
					{
						const Json& levelJson = abilityObj.at("level");
						if (!levelJson.is_number_integer())
						{
							throw std::runtime_error("Ability level must be an integer in profile '" + profile.profileId + "'.");
						}
						binding.level = levelJson.get<int>();
						if (binding.level < 1)
						{
							throw std::runtime_error("Ability level must be at least 1 in profile '" + profile.profileId + "'.");
						}
					}
					else
					{
						binding.level = 1;
					}

					profile.abilities.push_back(std::move(binding));
				}
			}
			return profile;
		}
	}

	EnemyCombatProfileLoader::Result EnemyCombatProfileLoader::LoadFromFile(const std::filesystem::path& filePath)
	{
		const JsonDocumentLoader::Result documentResult = JsonDocumentLoader::LoadFromFile(filePath);
		if (!documentResult.Succeeded())
		{
			return Result{ {}, documentResult.error };
		}

		try
		{
			const Json& root = *documentResult.document;
			if (!root.is_object())
			{
				return Result{ {}, "Enemy combat profile JSON root must be an object" };
			}

			static const std::set<std::string> allowedRootKeys = {
				"schemaVersion", "profiles"
			};
			for (auto it = root.begin(); it != root.end(); ++it)
			{
				if (allowedRootKeys.find(it.key()) == allowedRootKeys.end())
				{
					return Result{ {}, "Failed to load enemy combat profiles from '" + filePath.string() + "': Unknown key '" + it.key() + "' in document root." };
				}
			}

			if (!root.contains("schemaVersion") || !root.at("schemaVersion").is_number_integer() || root.at("schemaVersion").get<int>() != 1)
			{
				return Result{ {}, "Unsupported or missing enemy combat profile schema version" };
			}
			if (!root.contains("profiles") || !root.at("profiles").is_array())
			{
				return Result{ {}, "Field 'profiles' must be an array" };
			}

			Result result;
			std::set<std::string> profileIds;
			for (const Json& profileJson : root.at("profiles"))
			{
				EnemyCombatProfile profile = ParseProfile(profileJson);
				if (profile.profileId.empty())
				{
					throw std::runtime_error("Enemy profile ID cannot be empty");
				}
				if (!profileIds.insert(profile.profileId).second)
				{
					throw std::runtime_error("Duplicate enemy combat profile ID: " + profile.profileId);
				}
				result.profiles.push_back(std::move(profile));
			}
			return result;
		}
		catch (const std::exception& exception)
		{
			return Result{ {}, "Failed to load enemy combat profiles from '" + filePath.string() + "': " + exception.what() };
		}
	}
}

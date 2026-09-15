#include "gameplay/content/EnemyContentCatalog.h"

#include "framework/JsonDocumentLoader.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "abilities/AbilityGrantRules.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/enemy/EnemyBehaviorDecision.h"
#include "gameplay/enemy/EnemyBehaviorProfileValidator.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <utility>

namespace ly::content
{
	namespace
	{
		using Json = JsonDocumentLoader::Json;

		List<EnemyDefinition>& DefinitionsStorage()
		{
			static List<EnemyDefinition> definitions;
			return definitions;
		}

		List<EnemyBehaviorProfile>& BehaviorStorage()
		{
			static List<EnemyBehaviorProfile> profiles;
			return profiles;
		}

		bool& LoadedState()
		{
			static bool loaded = false;
			return loaded;
		}

		bool Fail(std::string* failureReason, const std::string& message)
		{
			if (failureReason) *failureReason = message;
			return false;
		}

		EnemyMovementMode ParseMovementMode(const Json& value)
		{
			const std::string mode = value.get<std::string>();
			if (mode == "Approach") return EnemyMovementMode::Approach;
			if (mode == "HoldRange") return EnemyMovementMode::HoldRange;
			if (mode == "Strafe") return EnemyMovementMode::Strafe;
			throw std::runtime_error("Unknown enemy movementMode: " + mode);
		}

		EnemySlotInputMode ParseSlotInputMode(const Json& value)
		{
			if (!value.is_string()) throw std::runtime_error("Enemy slot inputMode must be a string.");
			const std::string mode = value.get<std::string>();
			if (mode == "Hold") return EnemySlotInputMode::Hold;
			if (mode == "Pulse") return EnemySlotInputMode::Pulse;
			throw std::runtime_error("Unknown enemy slot inputMode: " + mode + ". Expected Hold or Pulse.");
		}

		EnemyBehaviorProfile ParseBehaviorProfile(const Json& object)
		{
			if (!object.is_object()) throw std::runtime_error("Each enemy behavior profile entry must be a JSON object.");
			static const std::set<std::string> allowedKeys = {
				"id", "targetSearchRange", "targetRefreshInterval", "slotRules",
				"desiredDistance", "minimumDistance", "maximumDistance", "movementMode",
				"strafeDirectionChangeInterval", "aimTurnSpeed"
			};
			for (auto it = object.begin(); it != object.end(); ++it)
				if (allowedKeys.find(it.key()) == allowedKeys.end()) throw std::runtime_error("Unknown key '" + it.key() + "' in enemy behavior profile.");
			EnemyBehaviorProfile profile;
			profile.profileId = object.at("id").get<std::string>();
			profile.targetSearchRange = object.at("targetSearchRange").get<float>();
			profile.targetRefreshInterval = object.at("targetRefreshInterval").get<float>();
			profile.desiredDistance = object.at("desiredDistance").get<float>();
			profile.minimumDistance = object.at("minimumDistance").get<float>();
			profile.maximumDistance = object.at("maximumDistance").get<float>();
			profile.movementMode = ParseMovementMode(object.at("movementMode"));
			profile.strafeDirectionChangeInterval = object.value("strafeDirectionChangeInterval", 2.f);
			profile.aimTurnSpeed = object.value("aimTurnSpeed", 1.f);
			if (object.contains("slotRules"))
			{
				const Json& rulesJson = object.at("slotRules");
				if (!rulesJson.is_array()) throw std::runtime_error("Field 'slotRules' must be an array in enemy behavior profile '" + profile.profileId + "'.");
				static const std::set<std::string> allowedRuleKeys = {
					"slot", "inputMode", "requiresTarget", "minimumRange", "maximumRange",
					"aimConeThreshold", "pulseRetryInterval", "priority"
				};
				for (const Json& ruleJson : rulesJson)
				{
					if (!ruleJson.is_object()) throw std::runtime_error("Each slot rule in enemy behavior profile '" + profile.profileId + "' must be an object.");
					for (auto it = ruleJson.begin(); it != ruleJson.end(); ++it)
						if (allowedRuleKeys.find(it.key()) == allowedRuleKeys.end()) throw std::runtime_error("Unknown key '" + it.key() + "' in slot rule for enemy behavior profile '" + profile.profileId + "'.");
					if (!ruleJson.contains("slot")) throw std::runtime_error("Enemy slot rule in profile '" + profile.profileId + "' requires a 'slot'.");
					const Json& slotJson = ruleJson.at("slot");
					if (!slotJson.is_string()) throw std::runtime_error("Enemy slot rule slot must be a string in profile '" + profile.profileId + "'.");
					const std::optional<sas::AbilitySlot> slot = ParseEnemySlot(slotJson.get<std::string>());
					if (!slot) throw std::runtime_error("Invalid enemy slot rule slot in profile '" + profile.profileId + "'.");
					EnemySlotDecisionRule rule;
					rule.slot = *slot;
					rule.inputMode = ruleJson.contains("inputMode") ? ParseSlotInputMode(ruleJson.at("inputMode")) : EnemySlotInputMode::Hold;
					rule.requiresTarget = ruleJson.value("requiresTarget", true);
					rule.minimumRange = ruleJson.value("minimumRange", 0.f);
					rule.maximumRange = ruleJson.value("maximumRange", 0.f);
					rule.aimConeThreshold = ruleJson.value("aimConeThreshold", -1.f);
					if (ruleJson.contains("pulseRetryInterval") && rule.inputMode == EnemySlotInputMode::Hold)
						throw std::runtime_error("Hold slot rule in profile '" + profile.profileId + "' must not define pulseRetryInterval.");
					rule.pulseRetryInterval = ruleJson.value("pulseRetryInterval", 0.25f);
					rule.priority = ruleJson.value("priority", 0);
					profile.slotRules.push_back(rule);
				}
			}
			return profile;
		}

		bool ValidateCombinedBehaviorProfile(const EnemyDefinition& enemy, const EnemyCombatProfile& combatProfile,
			const EnemyBehaviorProfile& behaviorProfile, std::string* failureReason)
		{
			struct AttackBand { float minimum; float maximum; };
			List<AttackBand> attackBands;
			const auto findRule = [&](sas::AbilitySlot slot) -> const EnemySlotDecisionRule*
			{
				const EnemySlotDecisionRule* result = nullptr;
				for (const EnemySlotDecisionRule& rule : behaviorProfile.slotRules)
					if (rule.slot == slot)
					{
						if (result) return nullptr;
						result = &rule;
					}
				return result;
			};
			const auto activeBindingCount = [&](sas::AbilitySlot slot)
			{
				int count = 0;
				for (const EnemyWeaponBinding& binding : combatProfile.weapons)
					if (binding.slot == slot) ++count;
				for (const EnemyAbilityBinding& binding : combatProfile.abilities)
					if (binding.slot == slot) ++count;
				return count;
			};
			const auto addWeaponAttackBand = [&](const EnemySlotDecisionRule& rule)
			{
				if (rule.requiresTarget) attackBands.push_back({ rule.minimumRange, rule.maximumRange });
			};

			for (const EnemySlotDecisionRule& rule : behaviorProfile.slotRules)
			{
				if (rule.slot == sas::AbilitySlot::None)
					return Fail(failureReason, "Enemy '" + enemy.id + "' behavior rule cannot use slot None.");
				if (activeBindingCount(rule.slot) != 1)
					return Fail(failureReason, "Enemy '" + enemy.id + "' behavior rule slot '" + std::string{ EnemySlotToString(rule.slot) } + "' must have exactly one active combat binding.");

				for (const EnemyWeaponBinding& binding : combatProfile.weapons)
				{
					if (binding.slot != rule.slot) continue;
					const PrimaryWeaponDefinition* weapon = WeaponContentCatalog::FindById(binding.weaponId);
					if (!weapon) return Fail(failureReason, "Enemy '" + enemy.id + "' references missing weapon '" + binding.weaponId + "'.");
					const sas::AbilityActivationPolicy policy = AbilityData::MakePrimaryFireAbilityDefinition(*weapon).activationPolicy;
					if (!IsEnemyInputModeCompatible(policy, rule.inputMode))
						return Fail(failureReason, "Enemy '" + enemy.id + "' behavior rule for weapon '" + binding.weaponId + "' does not match its activation policy.");
					std::string rangeFailure;
					const std::optional<float> range = WeaponContentCatalog::ResolveAuthoredAttributeAtLevel(
						binding.weaponId, binding.level, CommonAttributeIds::Range, &rangeFailure);
					if (!range) return Fail(failureReason, "Enemy '" + enemy.id + "' weapon '" + binding.weaponId + "' range validation failed: " + rangeFailure);
					if (rule.maximumRange > *range + 0.001f)
						return Fail(failureReason, "Enemy '" + enemy.id + "' behavior rule for weapon '" + binding.weaponId + "' exceeds authored weapon range.");
					addWeaponAttackBand(rule);
				}

				for (const EnemyAbilityBinding& binding : combatProfile.abilities)
				{
					if (binding.slot != rule.slot) continue;
					const GameAbilityDefinition* ability = AbilityData::FindShippedAbilityDefinition(binding.abilityId);
					if (!ability) return Fail(failureReason, "Enemy '" + enemy.id + "' references missing ability '" + binding.abilityId + "'.");
					if (sas::IsPassiveAbility(*ability))
						return Fail(failureReason, "Enemy '" + enemy.id + "' passive ability '" + binding.abilityId + "' cannot have a behavior rule.");
					if (!IsEnemyInputModeCompatible(ability->activationPolicy, rule.inputMode))
						return Fail(failureReason, "Enemy '" + enemy.id + "' behavior rule for ability '" + binding.abilityId + "' does not match its activation policy.");
				}
			}

			int activeBindingCountTotal = 0;
			const auto requireRuleForBinding = [&](sas::AbilitySlot slot, const std::string& id)
			{
				if (slot == sas::AbilitySlot::None) return true;
				const EnemySlotDecisionRule* rule = findRule(slot);
				if (!rule)
					return Fail(failureReason, "Enemy '" + enemy.id + "' active binding '" + id + "' on slot '" + std::string{ EnemySlotToString(slot) } + "' has no unique behavior rule.");
				++activeBindingCountTotal;
				return true;
			};
			for (const EnemyWeaponBinding& binding : combatProfile.weapons)
			{
				if (!WeaponContentCatalog::FindById(binding.weaponId))
					return Fail(failureReason, "Enemy '" + enemy.id + "' references missing weapon '" + binding.weaponId + "'.");
				if (!requireRuleForBinding(binding.slot, binding.weaponId)) return false;
			}
			for (const EnemyAbilityBinding& binding : combatProfile.abilities)
			{
				const GameAbilityDefinition* ability = AbilityData::FindShippedAbilityDefinition(binding.abilityId);
				if (!ability) return Fail(failureReason, "Enemy '" + enemy.id + "' references missing ability '" + binding.abilityId + "'.");
				if (sas::IsPassiveAbility(*ability))
				{
					if (binding.slot != sas::AbilitySlot::None)
						return Fail(failureReason, "Enemy '" + enemy.id + "' passive ability '" + binding.abilityId + "' must use slot None.");
					continue;
				}
				if (binding.slot == sas::AbilitySlot::None || !requireRuleForBinding(binding.slot, binding.abilityId))
					return Fail(failureReason, "Enemy '" + enemy.id + "' active ability '" + binding.abilityId + "' must use a valid slot with one behavior rule.");
			}

			if (activeBindingCountTotal == 0)
				return combatProfile.allowContactDamageOnly ? true : Fail(failureReason, "Enemy '" + enemy.id + "' has no active combat binding and is not contact-only.");
			if (combatProfile.weapons.empty() || attackBands.empty()) return true;

			float movementMinimum = behaviorProfile.minimumDistance;
			float movementMaximum = behaviorProfile.maximumDistance;
			if (behaviorProfile.movementMode == EnemyMovementMode::Approach) movementMaximum = behaviorProfile.desiredDistance;
			std::sort(attackBands.begin(), attackBands.end(), [](const AttackBand& left, const AttackBand& right) { return left.minimum < right.minimum; });
			float coveredUntil = movementMinimum;
			for (const AttackBand& band : attackBands)
			{
				if (band.maximum <= band.minimum || band.maximum < coveredUntil - 0.001f) continue;
				if (band.minimum > coveredUntil + 0.001f)
					return Fail(failureReason, "Enemy '" + enemy.id + "' attack slot rules leave a gap in the movement combat band.");
				coveredUntil = std::max(coveredUntil, band.maximum);
				if (coveredUntil >= movementMaximum - 0.001f) return true;
			}
			if (coveredUntil < movementMaximum - 0.001f)
				return Fail(failureReason, "Enemy '" + enemy.id + "' attack slot rules do not cover the movement combat band.");
			return true;
		}
	}

	namespace
	{

		EnemyDefinition ParseDefinition(const Json& object)
		{
			if (!object.is_object()) throw std::runtime_error("Each enemy definition entry must be a JSON object.");
			static const std::set<std::string> allowedKeys = { "id", "shipId", "combatProfileId", "behaviorProfileId" };
			for (auto it = object.begin(); it != object.end(); ++it)
				if (allowedKeys.find(it.key()) == allowedKeys.end()) throw std::runtime_error("Unknown key '" + it.key() + "' in enemy definition.");
			if (!object.contains("id") || !object.contains("shipId") || !object.contains("combatProfileId") || !object.contains("behaviorProfileId"))
				throw std::runtime_error("Enemy definition requires id, shipId, combatProfileId, and behaviorProfileId.");
			return {
				object.at("id").get<std::string>(),
				object.at("shipId").get<std::string>(),
				object.at("combatProfileId").get<std::string>(),
				object.at("behaviorProfileId").get<std::string>()
			};
		}

		bool LoadDocument(const std::filesystem::path& path, Json& document, std::string* failureReason)
		{
			const JsonDocumentLoader::Result result = JsonDocumentLoader::LoadFromFile(path);
			if (!result.Succeeded()) return Fail(failureReason, result.error);
			document = *result.document;
			return true;
		}
	}

	bool EnemyContentCatalog::LoadFromFiles(
		const std::filesystem::path& enemyDefinitionsPath,
		const std::filesystem::path& behaviorProfilesPath,
		std::string* failureReason)
	{
		try
		{
			Json definitionDocument;
			Json behaviorDocument;
			if (!LoadDocument(enemyDefinitionsPath, definitionDocument, failureReason) ||
				!LoadDocument(behaviorProfilesPath, behaviorDocument, failureReason)) return false;
			if (!definitionDocument.is_object() || !behaviorDocument.is_object() ||
				definitionDocument.at("schemaVersion").get<int>() != 1 ||
				behaviorDocument.at("schemaVersion").get<int>() != 1)
				return Fail(failureReason, "Unsupported enemy content schema version.");
			static const std::set<std::string> definitionRootKeys = { "schemaVersion", "enemies" };
			static const std::set<std::string> behaviorRootKeys = { "schemaVersion", "profiles" };
			for (auto it = definitionDocument.begin(); it != definitionDocument.end(); ++it)
				if (definitionRootKeys.find(it.key()) == definitionRootKeys.end()) return Fail(failureReason, "Unknown key '" + it.key() + "' in enemy definitions document.");
			for (auto it = behaviorDocument.begin(); it != behaviorDocument.end(); ++it)
				if (behaviorRootKeys.find(it.key()) == behaviorRootKeys.end()) return Fail(failureReason, "Unknown key '" + it.key() + "' in enemy behavior document.");
			if (!definitionDocument.contains("enemies") || !definitionDocument.at("enemies").is_array() || definitionDocument.at("enemies").empty())
				return Fail(failureReason, "Enemy definitions 'enemies' must be a nonempty array.");
			if (!behaviorDocument.contains("profiles") || !behaviorDocument.at("profiles").is_array() || behaviorDocument.at("profiles").empty())
				return Fail(failureReason, "Enemy behavior 'profiles' must be a nonempty array.");

			List<EnemyDefinition> definitions;
			List<EnemyBehaviorProfile> behaviors;
			std::set<std::string> definitionIds;
			for (const Json& entry : definitionDocument.at("enemies"))
			{
				EnemyDefinition definition = ParseDefinition(entry);
				std::string idFailure;
				if (!ContentIdSchema::ValidateEnemyId(definition.id, &idFailure) ||
					!ContentIdSchema::ValidateShipId(definition.shipId, &idFailure) ||
					!ContentIdSchema::ValidateEnemyCombatProfileId(definition.combatProfileId, &idFailure) ||
					!ContentIdSchema::ValidateEnemyBehaviorProfileId(definition.behaviorProfileId, &idFailure))
					return Fail(failureReason, "Invalid enemy definition '" + definition.id + "': " + idFailure);
				if (!definitionIds.insert(definition.id).second)
					return Fail(failureReason, "Duplicate enemy definition ID '" + definition.id + "'.");
				definitions.push_back(std::move(definition));
			}

			std::set<std::string> behaviorIds;
			for (const Json& entry : behaviorDocument.at("profiles"))
			{
				EnemyBehaviorProfile profile = ParseBehaviorProfile(entry);
				if (!behaviorIds.insert(profile.profileId).second)
					return Fail(failureReason, "Duplicate enemy behavior profile ID '" + profile.profileId + "'.");
				if (!EnemyBehaviorProfileValidator::Validate(profile, failureReason)) return false;
				behaviors.push_back(std::move(profile));
			}

			for (const EnemyDefinition& definition : definitions)
			{
				const ShipDefinition* ship = ShipContentCatalog::FindById(definition.shipId);
				if (!ship) return Fail(failureReason, "Enemy '" + definition.id + "' references missing ship '" + definition.shipId + "'.");
				if (!ship->primaryWeaponId.empty())
					return Fail(failureReason, "Enemy chassis '" + definition.shipId + "' must not own primaryWeaponId.");
				if (!EnemyCombatProfileCatalog::FindById(definition.combatProfileId))
					return Fail(failureReason, "Enemy '" + definition.id + "' references missing combat profile '" + definition.combatProfileId + "'.");
				const EnemyBehaviorProfile* behavior = nullptr;
				for (const EnemyBehaviorProfile& profile : behaviors)
					if (profile.profileId == definition.behaviorProfileId) behavior = &profile;
				if (!behavior)
					return Fail(failureReason, "Enemy '" + definition.id + "' references missing behavior profile '" + definition.behaviorProfileId + "'.");
				const EnemyCombatProfile* combat = EnemyCombatProfileCatalog::FindById(definition.combatProfileId);
				if (!ValidateCombinedBehaviorProfile(definition, *combat, *behavior, failureReason)) return false;
			}

			DefinitionsStorage() = std::move(definitions);
			BehaviorStorage() = std::move(behaviors);
			LoadedState() = true;
			return true;
		}
		catch (const std::exception& exception)
		{
			return Fail(failureReason, "Failed to load enemy content: " + std::string{ exception.what() });
		}
	}

	const EnemyDefinition* EnemyContentCatalog::FindById(const std::string& enemyId)
	{
		for (const EnemyDefinition& definition : DefinitionsStorage())
			if (definition.id == enemyId) return &definition;
		return nullptr;
	}

	const EnemyBehaviorProfile* EnemyContentCatalog::FindBehaviorById(const std::string& profileId)
	{
		for (const EnemyBehaviorProfile& profile : BehaviorStorage())
			if (profile.profileId == profileId) return &profile;
		return nullptr;
	}

	const List<EnemyDefinition>& EnemyContentCatalog::GetDefinitions()
	{
		return DefinitionsStorage();
	}

	const List<EnemyBehaviorProfile>& EnemyContentCatalog::GetBehaviorProfiles()
	{
		return BehaviorStorage();
	}

	bool EnemyContentCatalog::IsLoaded() noexcept
	{
		return LoadedState();
	}

	void EnemyContentCatalog::Clear()
	{
		DefinitionsStorage().clear();
		BehaviorStorage().clear();
		LoadedState() = false;
	}
}

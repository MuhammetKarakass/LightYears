#include "ModuleLoaderParsing.h"

#include "gameplay/tags/GameplayTagSchema.h"

#include <algorithm>
#include <set>
#include <utility>

namespace vega::content
{
	namespace module_loader_detail
	{
		void RejectZoneRules(const Json& object, const std::string& context)
		{
			if (object.contains("zoneRules")) throw std::runtime_error(context + ".zoneRules is no longer supported");
		}

		ModuleShieldRegenMode ParseShieldRegenMode(const std::string& value, const std::string& context)
		{
			if (value == "Normal") return ModuleShieldRegenMode::Normal;
			if (value == "SpeedScaled") return ModuleShieldRegenMode::SpeedScaled;
			if (value == "Continuous") return ModuleShieldRegenMode::Continuous;
			throw std::runtime_error(context + ".regenMode is not a known shield regen mode: " + value);
		}

		// shieldRules entries: regenMode (Normal by default); speedMultiplierAtMax (SpeedScaled only);
		// continuousRateFraction (Continuous only); breakRearmFraction and armorShare (any mode, 0 by default).
		void ParseShieldRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules)
		{
			const Json entries = object.value("shieldRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".shieldRules must be an array");
			const std::set<std::string> allowedKeys{ "regenMode", "speedMultiplierAtMax", "minimumSpeed", "continuousRateFraction", "breakRearmFraction", "armorShare", "frontRearSplit" };
			const auto positive = [](float value) { return value > 0.f; };
			const auto rateFraction = [](float value) { return value > 0.f && value <= 1.f; };
			const auto unitInterval = [](float value) { return value >= 0.f && value <= 1.f; };
			const auto parseField = [&context, minimumLevelCount](const Json& entry, const char* key, ModuleLevelTable<float>& table, bool (*valid)(float), const char* requirement)
			{
				const std::string fieldContext = context + ".shieldRules." + key;
				const float value = ParseLevelFloat(entry.at(key), fieldContext, minimumLevelCount, table);
				if (!valid(value)) throw std::runtime_error(fieldContext + " " + requirement);
				ValidateTable(table, fieldContext, valid, requirement);
				return value;
			};
			for (const Json& entry : entries)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " shieldRules entry must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(context + " unknown shieldRules field: " + field.key());
				ModuleShieldRule rule;
				rule.regenMode = ParseShieldRegenMode(entry.value("regenMode", std::string{ "Normal" }), context);
				if (rule.regenMode == ModuleShieldRegenMode::SpeedScaled)
				{
					if (!entry.contains("speedMultiplierAtMax")) throw std::runtime_error(context + ".shieldRules.speedMultiplierAtMax is required for SpeedScaled");
					rule.speedMultiplierAtMax = parseField(entry, "speedMultiplierAtMax", rule.speedMultiplierAtMaxByLevel, positive, "must be positive");
					if (entry.contains("minimumSpeed"))
					{
						rule.minimumSpeed = ParseFiniteFloat(entry.at("minimumSpeed"), context + ".shieldRules.minimumSpeed");
						if (rule.minimumSpeed < 0.f) throw std::runtime_error(context + ".shieldRules.minimumSpeed must be non-negative");
					}
				}
				else if (entry.contains("speedMultiplierAtMax") || entry.contains("minimumSpeed")) throw std::runtime_error(context + ".shieldRules.speedMultiplierAtMax is valid only for SpeedScaled");
				if (rule.regenMode == ModuleShieldRegenMode::Continuous)
				{
					if (!entry.contains("continuousRateFraction")) throw std::runtime_error(context + ".shieldRules.continuousRateFraction is required for Continuous");
					rule.continuousRateFraction = parseField(entry, "continuousRateFraction", rule.continuousRateFractionByLevel, rateFraction, "must be in (0, 1]");
				}
				else if (entry.contains("continuousRateFraction")) throw std::runtime_error(context + ".shieldRules.continuousRateFraction is valid only for Continuous");
				if (entry.contains("breakRearmFraction")) rule.breakRearmFraction = parseField(entry, "breakRearmFraction", rule.breakRearmFractionByLevel, unitInterval, "must be in [0, 1]");
				if (entry.contains("armorShare")) rule.armorShare = parseField(entry, "armorShare", rule.armorShareByLevel, unitInterval, "must be in [0, 1]");
				if (entry.contains("frontRearSplit")) { if (!entry.at("frontRearSplit").is_boolean()) throw std::runtime_error(context + " frontRearSplit must be boolean"); rule.frontRearSplit = entry.at("frontRearSplit").get<bool>(); }
				if (rule.regenMode == ModuleShieldRegenMode::Normal && rule.breakRearmFraction <= 0.f && rule.armorShare <= 0.f && !rule.frontRearSplit)
					throw std::runtime_error(context + ".shieldRules entry has no effect");
				rules.shieldRules.push_back(rule);
			}
		}

		ControlTargetClass ParseControlTargetClass(const Json& value, const std::string& context)
		{
			const std::string name = value.is_string() ? value.get<std::string>() : std::string{};
			if (name == "Normal") return ControlTargetClass::Normal;
			if (name == "Elite") return ControlTargetClass::Elite;
			if (name == "MiniBoss") return ControlTargetClass::MiniBoss;
			if (name == "Boss") return ControlTargetClass::Boss;
			throw std::runtime_error(context + ".stunImmuneClasses has an unknown class: " + name);
		}

		// ownerBurstRules entries: trigger (ShieldBroken, or KillConfirmed with chargeKills and chargeRadius), radius, damageType
		// (DamageType tag) and exactly one of damageFractionOfMaxHealth or damage (flat), each a scalar or a per-level array with
		// values > 0, are required. cooldownSeconds is required (> 0) for ShieldBroken and optional (>= 0) for charge rules.
		// chargeKills is an integer >= 1 and is rejected on ShieldBroken. pushImpulse, stunSeconds and echoDelaySeconds are >= 0 and
		// 0 turns the part off. echoFraction in [0, 1] is required with echoDelaySeconds and rejected without it. igniteStacks is an
		// integer >= 0 and needs the Thermal damageType. stunImmuneClasses (Normal, Elite, MiniBoss, Boss) defaults to Elite,
		// MiniBoss, Boss. Unknown fields are rejected.
		void ParseOwnerBurstRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules)
		{
			const Json entries = object.value("ownerBurstRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".ownerBurstRules must be an array");
			const std::set<std::string> allowedKeys{ "trigger", "radius", "damageFractionOfMaxHealth", "damage", "energyPowerCoefficient", "accumulatedArmorCoefficient", "requiredEventTags", "damageType", "igniteStacks", "chargeKills", "chargeRadius", "cooldownSeconds", "pushImpulse", "stunSeconds", "stunImmuneClasses", "echoDelaySeconds", "echoFraction" };
			const auto positive = [](float value) { return value > 0.f; };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			const auto unitInterval = [](float value) { return value >= 0.f && value <= 1.f; };
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				const Json& entry = entries[index];
				const std::string entryContext = context + ".ownerBurstRules[" + std::to_string(index) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				const auto requiredField = [&entry, &entryContext](const char* key) -> const Json&
				{
					if (!entry.contains(key)) throw std::runtime_error(entryContext + "." + key + " is required");
					return entry.at(key);
				};
				const auto scalar = [&entryContext](const char* key, const Json& value, bool (*valid)(float), const char* requirement)
				{
					const float number = ParseFiniteFloat(value, entryContext + "." + key);
					if (!valid(number)) throw std::runtime_error(entryContext + "." + key + " " + requirement);
					return number;
				};
				const Json& trigger = requiredField("trigger");
				if (!trigger.is_string()) throw std::runtime_error(entryContext + ".trigger must be ShieldBroken or KillConfirmed");
				const std::string triggerName = trigger.get<std::string>();
				if (triggerName != "ShieldBroken" && triggerName != "KillConfirmed" && triggerName != "AbilityActivated") throw std::runtime_error(entryContext + ".trigger must be ShieldBroken or KillConfirmed");
				ModuleOwnerBurstRule rule;
				rule.trigger = triggerName == "AbilityActivated" ? ModuleTriggerKind::AbilityActivated : triggerName == "KillConfirmed" ? ModuleTriggerKind::KillConfirmed : ModuleTriggerKind::ShieldBroken;
				rule.requiredEventTags = ParseRequiredEventTags(entry.value("requiredEventTags", Json::array()), rule.trigger, entryContext);
				if (entry.contains("accumulatedArmorCoefficient")) rule.accumulatedArmorCoefficient = scalar("accumulatedArmorCoefficient", entry.at("accumulatedArmorCoefficient"), positive, "must be positive");
				rule.radius = scalar("radius", requiredField("radius"), positive, "must be positive");
				const bool hasFraction = entry.contains("damageFractionOfMaxHealth");
				if ((hasFraction ? 1 : 0) + (entry.contains("damage") ? 1 : 0) + (entry.contains("accumulatedArmorCoefficient") ? 1 : 0) != 1) throw std::runtime_error(entryContext + " needs exactly one of damageFractionOfMaxHealth or damage");
				if (hasFraction)
				{
					const std::string fractionContext = entryContext + ".damageFractionOfMaxHealth";
					rule.damageFractionOfMaxHealth = ParseLevelFloat(entry.at("damageFractionOfMaxHealth"), fractionContext, minimumLevelCount, rule.damageFractionOfMaxHealthByLevel);
					if (!positive(rule.damageFractionOfMaxHealth)) throw std::runtime_error(fractionContext + " must be positive");
					ValidateTable(rule.damageFractionOfMaxHealthByLevel, fractionContext, positive, "must be positive");
				}
				else if (entry.contains("damage"))
				{
					const std::string damageContext = entryContext + ".damage";
					rule.damage = ParseLevelFloat(entry.at("damage"), damageContext, minimumLevelCount, rule.damageByLevel);
					if (!positive(rule.damage)) throw std::runtime_error(damageContext + " must be positive");
					ValidateTable(rule.damageByLevel, damageContext, positive, "must be positive");
				}
				if (entry.contains("energyPowerCoefficient"))
				{
					rule.energyPowerCoefficient = ParseFiniteFloat(entry.at("energyPowerCoefficient"), entryContext + ".energyPowerCoefficient");
					if (rule.energyPowerCoefficient < 0.f) throw std::runtime_error(entryContext + ".energyPowerCoefficient must be non-negative");
				}
				const Json& damageType = requiredField("damageType");
				rule.damageType = ParseTag(damageType, GameplayTagKind::DamageType, entryContext + " damageType");
				if (entry.contains("igniteStacks"))
				{
					const Json& stacks = entry.at("igniteStacks");
					if (!stacks.is_number_integer() || stacks.get<int>() < 0) throw std::runtime_error(entryContext + ".igniteStacks must be a non-negative integer");
					rule.igniteStacks = stacks.get<int>();
				}
				if (rule.igniteStacks > 0 && !(damageType.is_string() && damageType.get<std::string>() == "Damage.Type.Thermal"))
					throw std::runtime_error(entryContext + ".igniteStacks requires the Thermal damageType");
				if (entry.contains("chargeKills"))
				{
					const Json& kills = entry.at("chargeKills");
					if (!kills.is_number_integer() || kills.get<int>() < 1) throw std::runtime_error(entryContext + ".chargeKills must be an integer >= 1");
					rule.chargeKills = kills.get<int>();
				}
				if (entry.contains("chargeRadius"))
				{
					if (rule.chargeKills <= 0) throw std::runtime_error(entryContext + ".chargeRadius requires chargeKills");
					rule.chargeRadius = scalar("chargeRadius", entry.at("chargeRadius"), positive, "must be positive");
				}
				if (rule.trigger == ModuleTriggerKind::KillConfirmed && rule.chargeKills <= 0) throw std::runtime_error(entryContext + ".trigger KillConfirmed requires chargeKills");
				if (rule.trigger == ModuleTriggerKind::ShieldBroken && rule.chargeKills > 0) throw std::runtime_error(entryContext + ".chargeKills requires trigger KillConfirmed");
				if (rule.chargeKills > 0 && rule.chargeRadius <= 0.f) throw std::runtime_error(entryContext + ".chargeRadius is required with chargeKills");
				if (rule.chargeKills > 0)
				{
					if (entry.contains("cooldownSeconds")) rule.cooldownSeconds = scalar("cooldownSeconds", entry.at("cooldownSeconds"), nonnegative, "must not be negative");
				}
				else rule.cooldownSeconds = scalar("cooldownSeconds", requiredField("cooldownSeconds"), positive, "must be positive");
				if (entry.contains("pushImpulse")) rule.pushImpulse = scalar("pushImpulse", entry.at("pushImpulse"), nonnegative, "must not be negative");
				if (entry.contains("stunSeconds")) rule.stunSeconds = scalar("stunSeconds", entry.at("stunSeconds"), nonnegative, "must not be negative");
				if (entry.contains("echoDelaySeconds")) rule.echoDelaySeconds = scalar("echoDelaySeconds", entry.at("echoDelaySeconds"), nonnegative, "must not be negative");
				if (entry.contains("echoFraction"))
				{
					if (rule.echoDelaySeconds <= 0.f) throw std::runtime_error(entryContext + ".echoFraction requires echoDelaySeconds");
					rule.echoFraction = scalar("echoFraction", entry.at("echoFraction"), unitInterval, "must be in [0, 1]");
				}
				else if (rule.echoDelaySeconds > 0.f) throw std::runtime_error(entryContext + ".echoFraction is required with echoDelaySeconds");
				if (entry.contains("stunImmuneClasses"))
				{
					const Json& classes = entry.at("stunImmuneClasses");
					if (!classes.is_array()) throw std::runtime_error(entryContext + ".stunImmuneClasses must be an array");
					rule.stunImmuneClasses.clear();
					for (const Json& value : classes) rule.stunImmuneClasses.push_back(ParseControlTargetClass(value, entryContext));
				}
				rules.ownerBurstRules.push_back(std::move(rule));
			}
		}

		// ownerBurstModifiers entries patch the placement's main ownerBurst rules and never add one (side features use only this).
		// chargeRadius (> 0), stunSeconds (>= 0), echoDelaySeconds (> 0) with echoFraction in [0, 1] (required with it) and
		// echoPushFraction in [0, 1] (default 1), pullImpulse (> 0) with pullLeadSeconds (> 0) and optional pullImmuneClasses
		// (default Boss). Empty entries, unknown fields and echo or pull fields without their trigger field are rejected.
		void ParseOwnerBurstModifiers(const Json& object, const std::string& context, ModuleRulePackage& rules)
		{
			const Json entries = object.value("ownerBurstModifiers", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".ownerBurstModifiers must be an array");
			const std::set<std::string> allowedKeys{ "chargeRadius", "stunSeconds", "echoDelaySeconds", "echoFraction", "echoPushFraction", "pullImpulse", "pullLeadSeconds", "pullImmuneClasses" };
			const auto positive = [](float value) { return value > 0.f; };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			const auto unitInterval = [](float value) { return value >= 0.f && value <= 1.f; };
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				const Json& entry = entries[index];
				const std::string entryContext = context + ".ownerBurstModifiers[" + std::to_string(index) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				if (entry.empty()) throw std::runtime_error(entryContext + " must set at least one field");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				const auto scalar = [&entryContext](const char* key, const Json& value, bool (*valid)(float), const char* requirement)
				{
					const float number = ParseFiniteFloat(value, entryContext + "." + key);
					if (!valid(number)) throw std::runtime_error(entryContext + "." + key + " " + requirement);
					return number;
				};
				ModuleOwnerBurstModifier modifier;
				if (entry.contains("chargeRadius")) modifier.chargeRadius = scalar("chargeRadius", entry.at("chargeRadius"), positive, "must be positive");
				if (entry.contains("stunSeconds")) modifier.stunSeconds = scalar("stunSeconds", entry.at("stunSeconds"), nonnegative, "must not be negative");
				if (entry.contains("echoDelaySeconds"))
				{
					modifier.echoDelaySeconds = scalar("echoDelaySeconds", entry.at("echoDelaySeconds"), positive, "must be positive");
					if (!entry.contains("echoFraction")) throw std::runtime_error(entryContext + ".echoFraction is required with echoDelaySeconds");
					modifier.echoFraction = scalar("echoFraction", entry.at("echoFraction"), unitInterval, "must be in [0, 1]");
					if (entry.contains("echoPushFraction")) modifier.echoPushFraction = scalar("echoPushFraction", entry.at("echoPushFraction"), unitInterval, "must be in [0, 1]");
				}
				else if (entry.contains("echoFraction") || entry.contains("echoPushFraction")) throw std::runtime_error(entryContext + " echo fields require echoDelaySeconds");
				if (entry.contains("pullImpulse"))
				{
					modifier.pullImpulse = scalar("pullImpulse", entry.at("pullImpulse"), positive, "must be positive");
					if (!entry.contains("pullLeadSeconds")) throw std::runtime_error(entryContext + ".pullLeadSeconds is required with pullImpulse");
					modifier.pullLeadSeconds = scalar("pullLeadSeconds", entry.at("pullLeadSeconds"), positive, "must be positive");
					if (entry.contains("pullImmuneClasses"))
					{
						const Json& classes = entry.at("pullImmuneClasses");
						if (!classes.is_array()) throw std::runtime_error(entryContext + ".pullImmuneClasses must be an array");
						modifier.pullImmuneClasses.clear();
						for (const Json& value : classes) modifier.pullImmuneClasses.push_back(ParseControlTargetClass(value, entryContext));
					}
				}
				else if (entry.contains("pullLeadSeconds") || entry.contains("pullImmuneClasses")) throw std::runtime_error(entryContext + " pull fields require pullImpulse");
				rules.ownerBurstModifiers.push_back(std::move(modifier));
			}
		}

		// movementRules entries: thrustMultiplier (a scalar or a per-level array, each in (0, 1]) and
		// equalizeDirections (bool, false by default). Unknown fields and entries without an effect are rejected.
		void ParseMovementRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules)
		{
			const Json entries = object.value("movementRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".movementRules must be an array");
			const std::set<std::string> allowedKeys{ "thrustMultiplier", "equalizeDirections" };
			const auto valid = [](float value) { return value > 0.f && value <= 1.f; };
			for (const Json& entry : entries)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " movementRules entry must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(context + " unknown movementRules field: " + field.key());
				if (!entry.contains("thrustMultiplier")) throw std::runtime_error(context + ".movementRules.thrustMultiplier is required");
				const std::string fieldContext = context + ".movementRules.thrustMultiplier";
				ModuleMovementRule rule;
				rule.thrustMultiplier = ParseLevelFloat(entry.at("thrustMultiplier"), fieldContext, minimumLevelCount, rule.thrustMultiplierByLevel);
				if (!valid(rule.thrustMultiplier)) throw std::runtime_error(fieldContext + " must be in (0, 1]");
				ValidateTable(rule.thrustMultiplierByLevel, fieldContext, valid, "must be in (0, 1]");
				if (entry.contains("equalizeDirections"))
				{
					if (!entry.at("equalizeDirections").is_boolean()) throw std::runtime_error(context + ".movementRules.equalizeDirections must be a boolean");
					rule.equalizeDirections = entry.at("equalizeDirections").get<bool>();
				}
				bool hasEffect = rule.equalizeDirections || rule.thrustMultiplier < 1.f;
				for (float value : rule.thrustMultiplierByLevel.values) hasEffect = hasEffect || value < 1.f;
				if (!hasEffect) throw std::runtime_error(context + ".movementRules entry has no effect");
				rules.movementRules.push_back(rule);
			}
		}

		// sustainedSpeedRules entries: speedFraction in (0, 1] and holdSeconds >= 0 are required. primarySpeedMultiplier
		// and primaryDamageMultiplier (the latter a scalar or per-level array) are >= 1, primaryPierceBonus and
		// lingerSeconds are >= 0 and optional. An entry with no Primary effect is rejected, as is any unknown field.
		void ParseSustainedSpeedRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json entries = object.value("sustainedSpeedRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".sustainedSpeedRules must be an array");
			const std::set<std::string> allowedKeys{ "speedFraction", "holdSeconds", "primarySpeedMultiplier", "primaryPierceBonus", "primaryDamageMultiplier", "lingerSeconds" };
			const auto atLeastOne = [](float value) { return value >= 1.f; };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			for (const Json& entry : entries)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " sustainedSpeedRules entry must be an object");
				if (sideFeature)
				{
					// Side modifiers patch only this placement's sonic state.
					const std::set<std::string> sideKeys{ "lingerSeconds", "turnCapabilityMultiplier" };
					for (const auto& field : entry.items())
						if (!sideKeys.count(field.key())) throw std::runtime_error(context + ".sustainedSpeedRules side entry allows only lingerSeconds and turnCapabilityMultiplier");
					ModuleSustainedSpeedRule modifier;
					if (entry.contains("lingerSeconds"))
					{
						modifier.lingerSeconds = ParseFiniteFloat(entry.at("lingerSeconds"), context + ".sustainedSpeedRules.lingerSeconds");
						if (modifier.lingerSeconds <= 0.f) throw std::runtime_error(context + ".sustainedSpeedRules.lingerSeconds must be positive");
					}
					if (entry.contains("turnCapabilityMultiplier"))
					{
						modifier.turnCapabilityMultiplier = ParseFiniteFloat(entry.at("turnCapabilityMultiplier"), context + ".sustainedSpeedRules.turnCapabilityMultiplier");
						if (modifier.turnCapabilityMultiplier <= 1.f) throw std::runtime_error(context + ".sustainedSpeedRules.turnCapabilityMultiplier must exceed 1");
					}
					if (modifier.lingerSeconds <= 0.f && modifier.turnCapabilityMultiplier <= 1.f) throw std::runtime_error(context + ".sustainedSpeedRules side entry has no effect");
					rules.sustainedSpeedRules.push_back(modifier);
					continue;
				}
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(context + " unknown sustainedSpeedRules field: " + field.key());
				if (!entry.contains("speedFraction")) throw std::runtime_error(context + ".sustainedSpeedRules.speedFraction is required");
				if (!entry.contains("holdSeconds")) throw std::runtime_error(context + ".sustainedSpeedRules.holdSeconds is required");
				ModuleSustainedSpeedRule rule;
				rule.speedFraction = ParseFiniteFloat(entry.at("speedFraction"), context + ".sustainedSpeedRules.speedFraction");
				if (rule.speedFraction <= 0.f || rule.speedFraction > 1.f) throw std::runtime_error(context + ".sustainedSpeedRules.speedFraction must be in (0, 1]");
				rule.holdSeconds = ParseFiniteFloat(entry.at("holdSeconds"), context + ".sustainedSpeedRules.holdSeconds");
				if (rule.holdSeconds < 0.f) throw std::runtime_error(context + ".sustainedSpeedRules.holdSeconds must be non-negative");
				if (entry.contains("primarySpeedMultiplier"))
				{
					rule.primarySpeedMultiplier = ParseFiniteFloat(entry.at("primarySpeedMultiplier"), context + ".sustainedSpeedRules.primarySpeedMultiplier");
					if (!atLeastOne(rule.primarySpeedMultiplier)) throw std::runtime_error(context + ".sustainedSpeedRules.primarySpeedMultiplier must be at least 1");
				}
				if (entry.contains("primaryPierceBonus"))
				{
					rule.primaryPierceBonus = ParseFiniteFloat(entry.at("primaryPierceBonus"), context + ".sustainedSpeedRules.primaryPierceBonus");
					if (!nonnegative(rule.primaryPierceBonus)) throw std::runtime_error(context + ".sustainedSpeedRules.primaryPierceBonus must be non-negative");
				}
				if (entry.contains("primaryDamageMultiplier"))
				{
					const std::string fieldContext = context + ".sustainedSpeedRules.primaryDamageMultiplier";
					rule.primaryDamageMultiplier = ParseLevelFloat(entry.at("primaryDamageMultiplier"), fieldContext, minimumLevelCount, rule.primaryDamageMultiplierByLevel);
					if (!atLeastOne(rule.primaryDamageMultiplier)) throw std::runtime_error(fieldContext + " must be at least 1");
					ValidateTable(rule.primaryDamageMultiplierByLevel, fieldContext, atLeastOne, "must be at least 1");
				}
				if (entry.contains("lingerSeconds"))
				{
					rule.lingerSeconds = ParseFiniteFloat(entry.at("lingerSeconds"), context + ".sustainedSpeedRules.lingerSeconds");
					if (rule.lingerSeconds < 0.f) throw std::runtime_error(context + ".sustainedSpeedRules.lingerSeconds must be non-negative");
				}
				bool hasEffect = rule.primarySpeedMultiplier > 1.f || rule.primaryPierceBonus > 0.f || rule.primaryDamageMultiplier > 1.f;
				for (float value : rule.primaryDamageMultiplierByLevel.values) hasEffect = hasEffect || value > 1.f;
				if (!hasEffect) throw std::runtime_error(context + ".sustainedSpeedRules entry has no Primary effect");
				rules.sustainedSpeedRules.push_back(rule);
			}
		}

		// Thermal and Kinetic status stacks cap at 4 (damage_status_balance.json maxStacks); status thresholds may not exceed it.
		constexpr int kStatusRuleStackCap = 4;

		// Side-feature thermal modifier (Area Blast, Smoldering Remains). It has no damage of its own, so it carries only area and smolder
		// fields; the resolver merges them into the placement's burst rule. Smolder needs both smolderSeconds and smolderRadius.
		void ParseThermalBurstModifier(const Json& entry, const std::string& entryContext, ModuleRulePackage& rules)
		{
			ModuleThermalBurstRule modifier;
			modifier.areaRadius = entry.contains("areaRadius") ? ParseFiniteFloat(entry.at("areaRadius"), entryContext + ".areaRadius") : 0.f;
			modifier.areaDamageFraction = entry.contains("areaDamageFraction") ? ParseFiniteFloat(entry.at("areaDamageFraction"), entryContext + ".areaDamageFraction") : 0.f;
			modifier.smolderSeconds = entry.contains("smolderSeconds") ? ParseFiniteFloat(entry.at("smolderSeconds"), entryContext + ".smolderSeconds") : 0.f;
			modifier.smolderRadius = entry.contains("smolderRadius") ? ParseFiniteFloat(entry.at("smolderRadius"), entryContext + ".smolderRadius") : 0.f;
			if (modifier.areaRadius < 0.f) throw std::runtime_error(entryContext + ".areaRadius must be non-negative");
			if (modifier.areaDamageFraction < 0.f || modifier.areaDamageFraction > 1.f) throw std::runtime_error(entryContext + ".areaDamageFraction must be in [0, 1]");
			if ((modifier.areaRadius > 0.f) != (modifier.areaDamageFraction > 0.f)) throw std::runtime_error(entryContext + " area needs both areaRadius and areaDamageFraction");
			if (modifier.smolderSeconds < 0.f || modifier.smolderRadius < 0.f) throw std::runtime_error(entryContext + " smolder fields must be non-negative");
			if ((modifier.smolderSeconds > 0.f) != (modifier.smolderRadius > 0.f)) throw std::runtime_error(entryContext + " smolder needs both smolderSeconds and smolderRadius");
			if (entry.contains("smolderStacksPerSecond"))
			{
				if (modifier.smolderSeconds <= 0.f) throw std::runtime_error(entryContext + ".smolderStacksPerSecond needs smolderSeconds");
				modifier.smolderStacksPerSecond = ParsePositiveInt(entry.at("smolderStacksPerSecond"), entryContext + ".smolderStacksPerSecond");
				if (modifier.smolderStacksPerSecond > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".smolderStacksPerSecond must be 1 to 4");
			}
			else if (modifier.smolderSeconds > 0.f) modifier.smolderStacksPerSecond = 1;
			if (modifier.areaRadius <= 0.f && modifier.smolderSeconds <= 0.f) throw std::runtime_error(entryContext + " side modifier needs an area or a smolder field");
			rules.statusRules.thermalBursts.push_back(modifier);
		}

		// statusRules.thermalBursts entries: stackThreshold (required, 1..4), damageFraction (required, positive scalar or per-level),
		// optional areaRadius (non-negative) and areaDamageFraction ([0,1]); the area needs both or neither. Unknown fields are rejected.
		void ParseThermalBurstRules(const Json& statusRules, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json entries = statusRules.value("thermalBursts", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".statusRules.thermalBursts must be an array");
			const std::set<std::string> allowedKeys = sideFeature ? std::set<std::string>{ "areaRadius", "areaDamageFraction", "smolderSeconds", "smolderRadius", "smolderStacksPerSecond" }
								: std::set<std::string>{ "stackThreshold", "damageFraction", "areaRadius", "areaDamageFraction" };
			const auto positive = [](float value) { return value > 0.f; };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".statusRules.thermalBursts[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (sideFeature)
					{
						ParseThermalBurstModifier(entry, entryContext, rules);
						continue;
					}
				if (!entry.contains("stackThreshold") || !entry.contains("damageFraction")) throw std::runtime_error(entryContext + " requires stackThreshold and damageFraction");
				ModuleThermalBurstRule rule;
				rule.stackThreshold = ParsePositiveInt(entry.at("stackThreshold"), entryContext + ".stackThreshold");
				if (rule.stackThreshold > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".stackThreshold must be 1 to 4");
				rule.damageFraction = ParseLevelFloat(entry.at("damageFraction"), entryContext + ".damageFraction", minimumLevelCount, rule.damageFractionByLevel);
				if (!positive(rule.damageFraction)) throw std::runtime_error(entryContext + ".damageFraction must be positive");
				ValidateTable(rule.damageFractionByLevel, entryContext + ".damageFraction", positive, "must be positive");
				rule.areaRadius = entry.contains("areaRadius") ? ParseFiniteFloat(entry.at("areaRadius"), entryContext + ".areaRadius") : 0.f;
				rule.areaDamageFraction = entry.contains("areaDamageFraction") ? ParseFiniteFloat(entry.at("areaDamageFraction"), entryContext + ".areaDamageFraction") : 0.f;
				if (rule.areaRadius < 0.f) throw std::runtime_error(entryContext + ".areaRadius must be non-negative");
				if (rule.areaDamageFraction < 0.f || rule.areaDamageFraction > 1.f) throw std::runtime_error(entryContext + ".areaDamageFraction must be in [0, 1]");
				if ((rule.areaRadius > 0.f) != (rule.areaDamageFraction > 0.f)) throw std::runtime_error(entryContext + " area needs both areaRadius and areaDamageFraction");
				rules.statusRules.thermalBursts.push_back(std::move(rule));
			}
		}

		// Side-feature kinetic modifier (Hair Trigger, Armor Rend). It has no multiplier of its own; normalTargetThreshold and Armor Rend
		// merge into the placement's consume rule.
		void ParseKineticConsumeModifier(const Json& entry, const std::string& entryContext, ModuleRulePackage& rules)
		{
			ModuleKineticConsumeRule modifier;
			if (entry.contains("normalTargetThreshold"))
			{
				modifier.normalTargetThreshold = ParsePositiveInt(entry.at("normalTargetThreshold"), entryContext + ".normalTargetThreshold");
				if (modifier.normalTargetThreshold > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".normalTargetThreshold must be 1 to 4");
			}
			modifier.rendFraction = entry.contains("rendFraction") ? ParseFiniteFloat(entry.at("rendFraction"), entryContext + ".rendFraction") : 0.f;
			modifier.rendSeconds = entry.contains("rendSeconds") ? ParseFiniteFloat(entry.at("rendSeconds"), entryContext + ".rendSeconds") : 0.f;
			if (modifier.rendFraction < 0.f || modifier.rendFraction > 1.f) throw std::runtime_error(entryContext + ".rendFraction must be in [0, 1]");
			if (modifier.rendSeconds < 0.f) throw std::runtime_error(entryContext + ".rendSeconds must be non-negative");
			if ((modifier.rendFraction > 0.f) != (modifier.rendSeconds > 0.f)) throw std::runtime_error(entryContext + " Armor Rend needs both rendFraction and rendSeconds");
			if (modifier.normalTargetThreshold <= 0 && modifier.rendFraction <= 0.f) throw std::runtime_error(entryContext + " side modifier needs normalTargetThreshold or Armor Rend");
			rules.statusRules.kineticConsumes.push_back(modifier);
		}

		// statusRules.kineticConsumes entries: stackThreshold (required, 1..4), optional normalTargetThreshold (1..4), damageMultiplier
		// (required, positive scalar or per-level), optional rendFraction ([0,1]) and rendSeconds (non-negative); Armor Rend needs both or neither.
		void ParseKineticConsumeRules(const Json& statusRules, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			const Json entries = statusRules.value("kineticConsumes", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".statusRules.kineticConsumes must be an array");
			const std::set<std::string> allowedKeys = sideFeature ? std::set<std::string>{ "normalTargetThreshold", "rendFraction", "rendSeconds" }
								: std::set<std::string>{ "stackThreshold", "normalTargetThreshold", "damageMultiplier", "rendFraction", "rendSeconds" };
			const auto positive = [](float value) { return value > 0.f; };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".statusRules.kineticConsumes[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (sideFeature)
					{
						ParseKineticConsumeModifier(entry, entryContext, rules);
						continue;
					}
				if (!entry.contains("stackThreshold") || !entry.contains("damageMultiplier")) throw std::runtime_error(entryContext + " requires stackThreshold and damageMultiplier");
				ModuleKineticConsumeRule rule;
				rule.stackThreshold = ParsePositiveInt(entry.at("stackThreshold"), entryContext + ".stackThreshold");
				if (rule.stackThreshold > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".stackThreshold must be 1 to 4");
				if (entry.contains("normalTargetThreshold"))
				{
					rule.normalTargetThreshold = ParsePositiveInt(entry.at("normalTargetThreshold"), entryContext + ".normalTargetThreshold");
					if (rule.normalTargetThreshold > kStatusRuleStackCap) throw std::runtime_error(entryContext + ".normalTargetThreshold must be 1 to 4");
				}
				rule.damageMultiplier = ParseLevelFloat(entry.at("damageMultiplier"), entryContext + ".damageMultiplier", minimumLevelCount, rule.damageMultiplierByLevel);
				if (!positive(rule.damageMultiplier)) throw std::runtime_error(entryContext + ".damageMultiplier must be positive");
				ValidateTable(rule.damageMultiplierByLevel, entryContext + ".damageMultiplier", positive, "must be positive");
				rule.rendFraction = entry.contains("rendFraction") ? ParseFiniteFloat(entry.at("rendFraction"), entryContext + ".rendFraction") : 0.f;
				rule.rendSeconds = entry.contains("rendSeconds") ? ParseFiniteFloat(entry.at("rendSeconds"), entryContext + ".rendSeconds") : 0.f;
				if (rule.rendFraction < 0.f || rule.rendFraction > 1.f) throw std::runtime_error(entryContext + ".rendFraction must be in [0, 1]");
				if (rule.rendSeconds < 0.f) throw std::runtime_error(entryContext + ".rendSeconds must be non-negative");
				if ((rule.rendFraction > 0.f) != (rule.rendSeconds > 0.f)) throw std::runtime_error(entryContext + " Armor Rend needs both rendFraction and rendSeconds");
				rules.statusRules.kineticConsumes.push_back(std::move(rule));
			}
		}

		// runCounterRules entries: source "Distance" (unitsPerStack positive, damagePerStack and moveSpeedPerStack non-negative
		// scalars or per-level arrays, at least one positive) or "Kill" (damagePerStack per-level, positive; killWeights with normal,
		// elite, miniboss and boss; optional adjacentAbilityLimit 1-4). Unknown fields are rejected.
		void ParseRunCounterRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules)
		{
			const Json entries = object.value("runCounterRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".runCounterRules must be an array");
			const std::set<std::string> allowedKeys{ "source", "unitsPerStack", "damagePerStack", "moveSpeedPerStack", "killWeights", "adjacentAbilityLimit", "requiredFamily", "armorPerStack", "armorCap" };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			const auto positiveAnywhere = [](float scalar, const ModuleLevelTable<float>& table)
			{
				return scalar > 0.f || std::any_of(table.values.begin(), table.values.end(), [](float value) { return value > 0.f; });
			};
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".runCounterRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (!entry.contains("source")) throw std::runtime_error(entryContext + ".source is required");
				const std::string source = entry.at("source").get<std::string>();
				if (source != "Distance" && source != "Kill" && source != "AbilityUses") throw std::runtime_error(entryContext + ".source is not a known run counter source: " + source);
				const auto parsePerStack = [&entry, &entryContext, minimumLevelCount, nonnegative](const char* key, ModuleLevelTable<float>& table)
				{
					if (!entry.contains(key)) return 0.f;
					const std::string fieldContext = entryContext + "." + key;
					const float value = ParseLevelFloat(entry.at(key), fieldContext, minimumLevelCount, table);
					if (!nonnegative(value)) throw std::runtime_error(fieldContext + " must be non-negative");
					ValidateTable(table, fieldContext, nonnegative, "must be non-negative");
					return value;
				};
				if (source == "AbilityUses")
				{
					ModuleRunCounterRule rule; rule.source = ModuleRunCounterSource::AbilityUses;
					rule.requiredFamily = ParseTag(entry.at("requiredFamily"), GameplayTagKind::AbilityCategory, entryContext);
					rule.unitsPerStack = ParseFiniteFloat(entry.at("unitsPerStack"), entryContext);
					rule.armorPerStack = ParseFiniteFloat(entry.at("armorPerStack"), entryContext);
					rule.armorCap = ParseLevelFloat(entry.at("armorCap"), entryContext, minimumLevelCount, rule.armorCapByLevel);
					if (rule.requiredFamily == GameplayTags::Ability::Primary || rule.unitsPerStack <= 0.f || rule.armorPerStack <= 0.f || rule.armorCap <= 0.f) throw std::runtime_error(entryContext + " invalid ability-use Armor counter");
					ValidateTable(rule.armorCapByLevel, entryContext, [](float value) { return value > 0.f; }, "invalid Armor cap");
					if (entry.contains("damagePerStack") || entry.contains("moveSpeedPerStack") || entry.contains("killWeights")) throw std::runtime_error(entryContext + " mixed ability-use counter");
					rules.runCounterRules.push_back(rule); continue;
				}
				if (entry.contains("armorPerStack") || entry.contains("armorCap") || entry.contains("requiredFamily")) throw std::runtime_error(entryContext + " Armor fields require AbilityUses");
				if (source == "Kill")
				{
					if (entry.contains("unitsPerStack") || entry.contains("moveSpeedPerStack")) throw std::runtime_error(entryContext + " Kill takes only damagePerStack and killWeights");
					ModuleRunCounterRule kill;
					kill.source = ModuleRunCounterSource::Kill;
					kill.damagePerStack = parsePerStack("damagePerStack", kill.damagePerStackByLevel);
					if (!positiveAnywhere(kill.damagePerStack, kill.damagePerStackByLevel)) throw std::runtime_error(entryContext + ".damagePerStack must be positive");
					if (!entry.contains("killWeights") || !entry.at("killWeights").is_object()) throw std::runtime_error(entryContext + ".killWeights must be an object");
					const Json weights = entry.at("killWeights");
					for (const auto& field : weights.items())
						if (field.key() != "normal" && field.key() != "elite" && field.key() != "miniboss" && field.key() != "boss") throw std::runtime_error(entryContext + ".killWeights unknown field: " + field.key());
					for (const char* key : { "normal", "elite", "miniboss", "boss" })
						if (!weights.contains(key)) throw std::runtime_error(entryContext + ".killWeights." + key + " is required");
					const auto weight = [&weights, &entryContext, nonnegative](const char* key)
					{
						const float value = ParseFiniteFloat(weights.at(key), entryContext + ".killWeights." + key);
						if (!nonnegative(value)) throw std::runtime_error(entryContext + ".killWeights." + key + " must be non-negative");
						return value;
					};
					kill.killWeights.normal = weight("normal");
					kill.killWeights.elite = weight("elite");
					kill.killWeights.miniBoss = weight("miniboss");
					kill.killWeights.boss = weight("boss");
					if (!(kill.killWeights.normal > 0.f)) throw std::runtime_error(entryContext + ".killWeights.normal must be positive");
					if (entry.contains("adjacentAbilityLimit"))
					{
						if (!entry.at("adjacentAbilityLimit").is_number_integer()) throw std::runtime_error(entryContext + ".adjacentAbilityLimit must be an integer");
						kill.adjacentAbilityLimit = entry.at("adjacentAbilityLimit").get<int>();
						if (kill.adjacentAbilityLimit < 1 || kill.adjacentAbilityLimit > 4) throw std::runtime_error(entryContext + ".adjacentAbilityLimit must be in [1, 4]");
					}
					// A placement keeps one kill counter, so a package may carry at most one Kill rule.
					if (std::any_of(rules.runCounterRules.begin(), rules.runCounterRules.end(), [](const ModuleRunCounterRule& existing) { return existing.source == ModuleRunCounterSource::Kill; }))
						throw std::runtime_error(entryContext + " a package allows one Kill run counter rule");
					rules.runCounterRules.push_back(std::move(kill));
					continue;
				}
				if (entry.contains("killWeights") || entry.contains("adjacentAbilityLimit")) throw std::runtime_error(entryContext + " killWeights and adjacentAbilityLimit are for Kill only");
				if (!entry.contains("unitsPerStack")) throw std::runtime_error(entryContext + ".unitsPerStack is required");
				ModuleRunCounterRule rule;
				rule.source = ModuleRunCounterSource::Distance;
				rule.unitsPerStack = ParseFiniteFloat(entry.at("unitsPerStack"), entryContext + ".unitsPerStack");
				if (rule.unitsPerStack <= 0.f) throw std::runtime_error(entryContext + ".unitsPerStack must be positive");
				rule.damagePerStack = parsePerStack("damagePerStack", rule.damagePerStackByLevel);
				rule.moveSpeedPerStack = parsePerStack("moveSpeedPerStack", rule.moveSpeedPerStackByLevel);
				if (!positiveAnywhere(rule.damagePerStack, rule.damagePerStackByLevel) && !positiveAnywhere(rule.moveSpeedPerStack, rule.moveSpeedPerStackByLevel))
					throw std::runtime_error(entryContext + " has no effect");
				rules.runCounterRules.push_back(std::move(rule));
			}
		}

		// killStackCooldownRules entries (Bounty Mark): stacksPerStep (positive), reductionPerStep (non-empty fractions in (0,1)),
		// optional tailReduction (fraction in [0,1)), minimumSeconds (positive) and optional adjacentAbilityLimit 1-4. Unknown fields are rejected.
		void ParseKillStackCooldownRules(const Json& object, const std::string& context, ModuleRulePackage& rules)
		{
			const Json entries = object.value("killStackCooldownRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".killStackCooldownRules must be an array");
			const std::set<std::string> allowedKeys{ "stacksPerStep", "reductionPerStep", "tailReduction", "minimumSeconds", "adjacentAbilityLimit" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".killStackCooldownRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				ModuleKillStackCooldownRule rule;
				if (!entry.contains("stacksPerStep") || !entry.contains("reductionPerStep") || !entry.contains("minimumSeconds"))
					throw std::runtime_error(entryContext + " needs stacksPerStep, reductionPerStep and minimumSeconds");
				rule.stacksPerStep = ParseFiniteFloat(entry.at("stacksPerStep"), entryContext + ".stacksPerStep");
				if (!(rule.stacksPerStep > 0.f)) throw std::runtime_error(entryContext + ".stacksPerStep must be positive");
				const Json steps = entry.at("reductionPerStep");
				if (!steps.is_array() || steps.empty()) throw std::runtime_error(entryContext + ".reductionPerStep must be a non-empty array");
				for (const Json& value : steps)
				{
					const float reduction = ParseFiniteFloat(value, entryContext + ".reductionPerStep");
					if (!(reduction > 0.f && reduction < 1.f)) throw std::runtime_error(entryContext + ".reductionPerStep entries must be in (0, 1)");
					rule.reductionPerStep.push_back(reduction);
				}
				if (entry.contains("tailReduction"))
				{
					rule.tailReduction = ParseFiniteFloat(entry.at("tailReduction"), entryContext + ".tailReduction");
					if (!(rule.tailReduction >= 0.f && rule.tailReduction < 1.f)) throw std::runtime_error(entryContext + ".tailReduction must be in [0, 1)");
				}
				rule.minimumSeconds = ParseFiniteFloat(entry.at("minimumSeconds"), entryContext + ".minimumSeconds");
				if (!(rule.minimumSeconds > 0.f)) throw std::runtime_error(entryContext + ".minimumSeconds must be positive");
				if (entry.contains("adjacentAbilityLimit"))
				{
					if (!entry.at("adjacentAbilityLimit").is_number_integer()) throw std::runtime_error(entryContext + ".adjacentAbilityLimit must be an integer");
					rule.adjacentAbilityLimit = entry.at("adjacentAbilityLimit").get<int>();
					if (rule.adjacentAbilityLimit < 1 || rule.adjacentAbilityLimit > 4) throw std::runtime_error(entryContext + ".adjacentAbilityLimit must be in [1, 4]");
				}
				rules.killStackCooldownRules.push_back(std::move(rule));
			}
		}

		// killStackDamageRules entries (Trophy Wall): family (an AbilityCategory tag) and fraction in (0,1].
		void ParseKillStackDamageRules(const Json& object, const std::string& context, ModuleRulePackage& rules)
		{
			const Json entries = object.value("killStackDamageRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".killStackDamageRules must be an array");
			const std::set<std::string> allowedKeys{ "family", "fraction" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".killStackDamageRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (!entry.contains("family") || !entry.contains("fraction")) throw std::runtime_error(entryContext + " needs family and fraction");
				ModuleKillStackDamageRule rule;
				rule.family = ParseTag(entry.at("family"), GameplayTagKind::AbilityCategory, entryContext + " family");
				rule.fraction = ParseFiniteFloat(entry.at("fraction"), entryContext + ".fraction");
				if (!(rule.fraction > 0.f && rule.fraction <= 1.f)) throw std::runtime_error(entryContext + ".fraction must be in (0, 1]");
				rules.killStackDamageRules.push_back(std::move(rule));
			}
		}

		// chargeRules entries (Spare Capacitor). A main-package entry takes chargePerLevel only: a per-level array of positive
		// integers (charge capacity added to each adjacent Q/E/F/R ability). A side-feature entry takes overflowTransfer (bool)
		// and/or trickleSeconds (positive) with trickleCharges (positive integer), and never chargePerLevel. The adjacent
		// ability count is the cell count, so a variant with a charge rule holds at most 3 cells. Unknown fields are rejected.
		void ParseChargeRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleSizeVariant& variant, bool sideFeature)
		{
			const Json entries = object.value("chargeRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".chargeRules must be an array");
			const std::set<std::string> allowedKeys{ "chargePerLevel", "overflowTransfer", "trickleSeconds", "trickleCharges" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".chargeRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (variant.shape.cells.size() > 3) throw std::runtime_error(entryContext + " charge rules allow at most 3 cells (3 adjacent abilities)");
				ModuleChargeRule rule;
				const bool capacityRule = entry.contains("chargePerLevel");
				const bool sideRule = entry.contains("overflowTransfer") || entry.contains("trickleSeconds") || entry.contains("trickleCharges");
				if (capacityRule == sideRule) throw std::runtime_error(entryContext + " needs either chargePerLevel or overflowTransfer/trickleSeconds/trickleCharges");
				if (capacityRule)
				{
					if (sideFeature) throw std::runtime_error(entryContext + " chargePerLevel is main-package only");
					if (!entry.at("chargePerLevel").is_array()) throw std::runtime_error(entryContext + ".chargePerLevel must be a per-level array");
					ParsePositiveLevelInt(entry.at("chargePerLevel"), entryContext + ".chargePerLevel", minimumLevelCount, rule.chargePerLevel);
				}
				else
				{
					if (!sideFeature) throw std::runtime_error(entryContext + " overflowTransfer and trickle fields are side-feature only");
					if (entry.contains("overflowTransfer"))
					{
						if (!entry.at("overflowTransfer").is_boolean()) throw std::runtime_error(entryContext + ".overflowTransfer must be a boolean");
						rule.overflowTransfer = entry.at("overflowTransfer").get<bool>();
					}
					if (entry.contains("trickleSeconds") != entry.contains("trickleCharges")) throw std::runtime_error(entryContext + " trickleSeconds and trickleCharges go together");
					if (entry.contains("trickleSeconds"))
					{
						rule.trickleSeconds = ParseFiniteFloat(entry.at("trickleSeconds"), entryContext + ".trickleSeconds");
						if (rule.trickleSeconds <= 0.f) throw std::runtime_error(entryContext + ".trickleSeconds must be positive");
						rule.trickleCharges = ParsePositiveInt(entry.at("trickleCharges"), entryContext + ".trickleCharges");
					}
					if (!rule.overflowTransfer && rule.trickleSeconds <= 0.f) throw std::runtime_error(entryContext + " has no effect");
				}
				variant.chargeRules.push_back(std::move(rule));
			}
		}

		// targetSlowRules entries: minRatio (at least 1), slowFraction and bossSlowFraction (in (0,1)) and seconds (positive).
		// All four fields are required, not per-level, and unknown fields are rejected.
		void ParseTargetSlowRules(const Json& object, const std::string& context, ModuleRulePackage& rules)
		{
			const Json entries = object.value("targetSlowRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".targetSlowRules must be an array");
			const std::set<std::string> allowedKeys{ "minRatio", "slowFraction", "bossSlowFraction", "seconds" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".targetSlowRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				for (const char* key : { "minRatio", "slowFraction", "bossSlowFraction", "seconds" })
					if (!entry.contains(key)) throw std::runtime_error(entryContext + "." + key + " is required");
				ModuleTargetSlowRule rule;
				rule.minRatio = ParseFiniteFloat(entry.at("minRatio"), entryContext + ".minRatio");
				rule.slowFraction = ParseFiniteFloat(entry.at("slowFraction"), entryContext + ".slowFraction");
				rule.bossSlowFraction = ParseFiniteFloat(entry.at("bossSlowFraction"), entryContext + ".bossSlowFraction");
				rule.seconds = ParseFiniteFloat(entry.at("seconds"), entryContext + ".seconds");
				if (rule.minRatio < 1.f) throw std::runtime_error(entryContext + ".minRatio must be at least 1");
				if (rule.slowFraction <= 0.f || rule.slowFraction >= 1.f) throw std::runtime_error(entryContext + ".slowFraction must be between 0 and 1");
				if (rule.bossSlowFraction <= 0.f || rule.bossSlowFraction >= 1.f) throw std::runtime_error(entryContext + ".bossSlowFraction must be between 0 and 1");
				if (rule.seconds <= 0.f) throw std::runtime_error(entryContext + ".seconds must be positive");
				rules.targetSlowRules.push_back(rule);
			}
		}

		// primaryMountRules (Epic Primary mounts) are main-package only: mode Turret takes no multiplier, mode Broadside requires a positive
		// per-level damageMultiplier (the coefficient of each side volley). primaryExecutionPriority (Turret side feature) may only be true,
		// and only in a side package.
		void ParsePrimaryMountRules(const Json& object, const std::string& context, int minimumLevelCount, ModuleRulePackage& rules, bool sideFeature)
		{
			if (object.contains("primaryExecutionPriority"))
			{
				if (!sideFeature) throw std::runtime_error(context + ".primaryExecutionPriority is side-feature only");
				if (!object.at("primaryExecutionPriority").is_boolean() || !object.at("primaryExecutionPriority").get<bool>())
					throw std::runtime_error(context + ".primaryExecutionPriority must be true when present");
				rules.primaryExecutionPriority = true;
			}
			const Json entries = object.value("primaryMountRules", Json::array());
			if (!entries.is_array()) throw std::runtime_error(context + ".primaryMountRules must be an array");
			const std::set<std::string> allowedKeys{ "mode", "damageMultiplier" };
			std::size_t index = 0;
			for (const Json& entry : entries)
			{
				const std::string entryContext = context + ".primaryMountRules[" + std::to_string(index++) + "]";
				if (sideFeature) throw std::runtime_error(entryContext + " is main-package only");
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				for (const auto& field : entry.items())
					if (!allowedKeys.count(field.key())) throw std::runtime_error(entryContext + " unknown field: " + field.key());
				if (!entry.contains("mode") || !entry.at("mode").is_string()) throw std::runtime_error(entryContext + ".mode must be Turret or Broadside");
				const std::string mode = entry.at("mode").get<std::string>();
				ModulePrimaryMountRule rule;
				if (mode == "Turret")
				{
					if (entry.contains("damageMultiplier")) throw std::runtime_error(entryContext + " Turret takes no damageMultiplier");
					rule.mode = ModulePrimaryMountMode::Turret;
				}
				else if (mode == "Broadside")
				{
					if (!entry.contains("damageMultiplier")) throw std::runtime_error(entryContext + ".damageMultiplier is required for Broadside");
					rule.mode = ModulePrimaryMountMode::Broadside;
					rule.damageMultiplier = ParseLevelFloat(entry.at("damageMultiplier"), entryContext + ".damageMultiplier", minimumLevelCount, rule.damageMultiplierByLevel);
					if (!(rule.damageMultiplier > 0.f)) throw std::runtime_error(entryContext + ".damageMultiplier must be positive");
					for (const float value : rule.damageMultiplierByLevel.values)
						if (!(value > 0.f)) throw std::runtime_error(entryContext + ".damageMultiplier must be positive");
				}
				else throw std::runtime_error(entryContext + ".mode must be Turret or Broadside");
				rules.primaryMountRules.push_back(rule);
			}
		}

		// fragmentation (F1 Parçalama) is a side-feature switch: it may only be true, and only in a side package. A main package never splits.
		void ParseFragmentation(const Json& object, const std::string& context, ModuleRulePackage& rules, bool sideFeature)
		{
			if (!object.contains("fragmentation")) return;
			if (!sideFeature) throw std::runtime_error(context + ".fragmentation is side-feature only");
			if (!object.at("fragmentation").is_boolean() || !object.at("fragmentation").get<bool>())
				throw std::runtime_error(context + ".fragmentation must be true when present");
			rules.fragmentation = true;
		}
	}
}

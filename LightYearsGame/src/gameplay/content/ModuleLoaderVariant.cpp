#include "ModuleLoaderParsing.h"

#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/AttributeJsonParser.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <set>
#include <utility>

namespace vega::content
{
	namespace module_loader_detail
	{
		ModuleSizeVariant ParseVariant(const Json& object, const std::string& context, int minimumLevelCount, bool defaultAffectedByZoneBonus, bool sideFeature)
		{
			ValidateLevelGrowthFields(object, context);
			ModuleSizeVariant variant;
			variant.shape = ParseShape(object.at("shape"), context + ".shape");
			std::size_t index = 0;
			for (const Json& entry : object.value("attributeModifiers", Json::array()))
			{
				const std::string entryContext = context + ".attributeModifiers[" + std::to_string(index++) + "]";
				variant.attributeModifiers.push_back(ParseAttributeModifier(entry, entryContext, minimumLevelCount, defaultAffectedByZoneBonus));
			}
			index = 0;
			for (const Json& entry : object.value("abilityModifiers", Json::array()))
			{
				const std::string entryContext = context + ".abilityModifiers[" + std::to_string(index++) + "]";
				ModuleAbilityModifier modifier;
				modifier.modifier = ParseModifier(entry, entryContext, minimumLevelCount, modifier.magnitudeByLevel);
				modifier.affectedByZoneBonus = entry.value("affectedByZoneBonus", defaultAffectedByZoneBonus);
				variant.abilityModifiers.push_back(std::move(modifier));
			}
			index = 0;
			const Json triggerBuffs = object.value("triggerBuffs", Json::array());
			if (!triggerBuffs.is_array()) throw std::runtime_error(context + ".triggerBuffs must be an array");
			for (const Json& entry : triggerBuffs)
			{
				const std::string entryContext = context + ".triggerBuffs[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				ModuleTriggerBuff buff;
				buff.trigger = ParseTriggerKind(entry.at("trigger").get<std::string>());
				if (buff.trigger == ModuleTriggerKind::None) throw std::runtime_error(entryContext + " trigger must not be None");
				buff.durationSeconds = ParseLevelFloat(entry, "durationSeconds", entryContext + ".durationSeconds", minimumLevelCount, buff.durationSecondsByLevel);
				if (buff.durationSeconds <= 0.f) throw std::runtime_error(entryContext + " durationSeconds must be positive");
				ValidateTable(buff.durationSecondsByLevel, entryContext + ".durationSeconds", [](float value) { return value > 0.f; }, "must be positive");
				buff.movementSpeedMultiplier = entry.contains("movementSpeedMultiplier")
					? ParseLevelFloat(entry, "movementSpeedMultiplier", entryContext + ".movementSpeedMultiplier", minimumLevelCount,
						buff.movementSpeedMultiplierByLevel)
					: 1.f;
				if (buff.movementSpeedMultiplier <= 0.f) throw std::runtime_error(entryContext + " movementSpeedMultiplier must be positive");
				ValidateTable(buff.movementSpeedMultiplierByLevel, entryContext + ".movementSpeedMultiplier", [](float value) { return value > 0.f; }, "must be positive");
				buff.requiredEventTags = ParseRequiredEventTags(
					entry.value("requiredEventTags", Json::array()), buff.trigger, entryContext + ".requiredEventTags");
				const Json modifiers = entry.value("modifiers", Json::array());
				if (!modifiers.is_array()) throw std::runtime_error(entryContext + ".modifiers must be an array");
				std::size_t modifierIndex = 0;
				for (const Json& modifier : modifiers)
				{
					const std::string modifierContext = entryContext + ".modifiers[" + std::to_string(modifierIndex++) + "]";
					buff.modifiers.push_back(ParseAttributeModifier(modifier, modifierContext, minimumLevelCount, false));
				}
				bool changesMovementSpeed = buff.movementSpeedMultiplier != 1.f;
				for (float value : buff.movementSpeedMultiplierByLevel.values) changesMovementSpeed = changesMovementSpeed || value != 1.f;
				if (buff.modifiers.empty() && !changesMovementSpeed)
					throw std::runtime_error(entryContext + " must define a modifier or movementSpeedMultiplier");
				variant.triggerBuffs.push_back(std::move(buff));
			}
			index = 0;
			const Json damageRules = object.value("damageRules", Json::array());
			if (!damageRules.is_array()) throw std::runtime_error(context + ".damageRules must be an array");
			for (const Json& entry : damageRules)
			{
				const std::string entryContext = context + ".damageRules[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				ModuleDamageRule rule;
				rule.incoming = entry.value("incoming", false);
				rule.condition = ParseDamageCondition(entry.at("condition").get<std::string>());
				if (entry.contains("openingShotCount"))
				{
					if (rule.condition != ModuleDamageCondition::FullMagazineShot)
						throw std::runtime_error(entryContext + " openingShotCount requires FullMagazineShot and an integer");
					rule.openingShotCount = ParsePositiveLevelInt(entry, "openingShotCount", entryContext + ".openingShotCount", minimumLevelCount, rule.openingShotCountByLevel);
				}
				if (entry.contains("bonusCriticalPolicy"))
					rule.bonusCriticalPolicy = ParseCriticalPolicy(entry.at("bonusCriticalPolicy").get<std::string>());
				rule.conditionValue = entry.contains("conditionValue")
					? ParseLevelFloat(entry, "conditionValue", entryContext + ".conditionValue", minimumLevelCount, rule.conditionValueByLevel)
					: 0.f;
				rule.damageMultiplier = entry.contains("damageMultiplier")
					? ParseLevelFloat(entry, "damageMultiplier", entryContext + ".damageMultiplier", minimumLevelCount, rule.damageMultiplierByLevel)
					: 1.f;
				rule.bonusDamageFraction = entry.contains("bonusDamageFraction")
					? ParseLevelFloat(entry, "bonusDamageFraction", entryContext + ".bonusDamageFraction", minimumLevelCount,
						rule.bonusDamageFractionByLevel)
					: 0.f;
				if (rule.damageMultiplier <= 0.f) throw std::runtime_error(entryContext + " damageMultiplier must be positive");
				ValidateTable(rule.damageMultiplierByLevel, entryContext + ".damageMultiplier",
					[](float value) { return value > 0.f; }, "must be positive");
				if (rule.bonusDamageFraction < 0.f || rule.bonusDamageFraction > 1.f)
					throw std::runtime_error(entryContext + " bonusDamageFraction must be between 0 and 1");
				ValidateTable(rule.bonusDamageFractionByLevel, entryContext + ".bonusDamageFraction",
					[](float value) { return value >= 0.f && value <= 1.f; }, "must be between 0 and 1");
				if (entry.contains("perEmptyCellMultiplier"))
				{
					if (rule.condition != ModuleDamageCondition::Always || rule.incoming)
						throw std::runtime_error(entryContext + " perEmptyCellMultiplier requires an outgoing Always rule");
					if (entry.contains("damageMultiplier"))
						throw std::runtime_error(entryContext + " perEmptyCellMultiplier cannot be combined with damageMultiplier");
					rule.perEmptyCellMultiplier = ParseLevelFloat(entry, "perEmptyCellMultiplier", entryContext + ".perEmptyCellMultiplier", minimumLevelCount,
						rule.perEmptyCellMultiplierByLevel);
					if (rule.perEmptyCellMultiplier <= 0.f) throw std::runtime_error(entryContext + " perEmptyCellMultiplier must be positive");
					ValidateTable(rule.perEmptyCellMultiplierByLevel, entryContext + ".perEmptyCellMultiplier", [](float value) { return value > 0.f; }, "must be positive");
				}
				if (entry.contains("requiredEmptyAbilitySlots"))
				{
					if (rule.incoming) throw std::runtime_error(entryContext + " requiredEmptyAbilitySlots requires an outgoing rule");
					rule.requiredEmptyAbilitySlots = ParsePositiveInt(entry.at("requiredEmptyAbilitySlots"), entryContext + ".requiredEmptyAbilitySlots");
					if (rule.requiredEmptyAbilitySlots > 4) throw std::runtime_error(entryContext + " requiredEmptyAbilitySlots must be between 1 and 4");
				}
				if (entry.contains("ratioSlope"))
				{
					if (rule.condition != ModuleDamageCondition::TargetMaxHealthRatioAtLeast || rule.incoming)
						throw std::runtime_error(entryContext + " ratioSlope requires an outgoing TargetMaxHealthRatioAtLeast rule");
					rule.ratioSlope = ParseFiniteFloat(entry.at("ratioSlope"), entryContext + ".ratioSlope");
					if (rule.ratioSlope < 0.f) throw std::runtime_error(entryContext + " ratioSlope must not be negative");
				}
				bool hasBonusDamage = rule.bonusDamageFraction > 0.f;
				if (entry.contains("maxHealthDamageFraction"))
				{
					if (rule.condition != ModuleDamageCondition::AdjacentAbilityDirectHit || rule.incoming)
						throw std::runtime_error(entryContext + " MaxHealth damage requires outgoing adjacency");
					rule.maxHealthDamageFraction = ParseLevelFloat(entry, "maxHealthDamageFraction", entryContext, minimumLevelCount, rule.maxHealthDamageFractionByLevel);
					const auto positive = [](float value) { return value > 0.f; };
					if (!positive(rule.maxHealthDamageFraction)) throw std::runtime_error(entryContext + " invalid MaxHealth damage fraction");
					ValidateTable(rule.maxHealthDamageFractionByLevel, entryContext, positive, "invalid MaxHealth damage fraction");
				}
				else if (rule.condition == ModuleDamageCondition::AdjacentAbilityDirectHit)
					throw std::runtime_error(entryContext + " adjacency requires maxHealthDamageFraction");
				bool changesDamageMultiplier = rule.damageMultiplier != 1.f;
				for (float value : rule.bonusDamageFractionByLevel.values) hasBonusDamage = hasBonusDamage || value > 0.f;
				for (float value : rule.damageMultiplierByLevel.values) changesDamageMultiplier = changesDamageMultiplier || value != 1.f;
				if (GetModuleDamageConditionPhase(rule.condition) == ModuleDamageRulePhase::Fire && hasBonusDamage)
					throw std::runtime_error(entryContext + " fire-time conditions support damageMultiplier only");
				if (rule.condition == ModuleDamageCondition::TargetHealthAtMost || rule.condition == ModuleDamageCondition::OwnerHealthAtMost)
				{
					if (!entry.contains("conditionValue") || rule.conditionValue < 0.f || rule.conditionValue > 1.f)
						throw std::runtime_error(entryContext + " TargetHealthAtMost requires conditionValue in 0..1");
					ValidateTable(rule.conditionValueByLevel, entryContext + ".conditionValue",
						[](float value) { return value >= 0.f && value <= 1.f; }, "must be between 0 and 1");
				}
				else if (rule.condition == ModuleDamageCondition::TargetStatusKindsAtLeast)
				{
					const auto isStatusKindCount = [](float value) { return value >= 2.f && std::floor(value) == value; };
					if (!entry.contains("conditionValue") || !isStatusKindCount(rule.conditionValue))
						throw std::runtime_error(entryContext + " TargetStatusKindsAtLeast requires an integer conditionValue of at least 2");
					ValidateTable(rule.conditionValueByLevel, entryContext + ".conditionValue", isStatusKindCount, "must be an integer of at least 2");
				}
				else if (rule.condition == ModuleDamageCondition::TargetMaxHealthRatioAtLeast)
				{
					const auto atLeastOne = [](float value) { return value >= 1.f; };
					if (!entry.contains("conditionValue") || !atLeastOne(rule.conditionValue))
						throw std::runtime_error(entryContext + " TargetMaxHealthRatioAtLeast requires a conditionValue of at least 1");
					ValidateTable(rule.conditionValueByLevel, entryContext + ".conditionValue", atLeastOne, "must be at least 1");
				}
				else if (entry.contains("conditionValue"))
				{
					throw std::runtime_error(entryContext + " this damage condition does not accept conditionValue");
				}
				if (hasBonusDamage)
				{
					if (!entry.contains("bonusDamageType")) throw std::runtime_error(entryContext + " bonusDamageFraction requires bonusDamageType");
					rule.bonusDamageType = ParseTag(entry.at("bonusDamageType"), GameplayTagKind::DamageType, entryContext + ".bonusDamageType");
					if (rule.bonusCriticalPolicy != DamageCriticalPolicy::Disabled)
						throw std::runtime_error(entryContext + " bonus damage critical policy must be Disabled");
				}
				else if (entry.contains("bonusDamageType") || entry.contains("bonusCriticalPolicy"))
				{
					throw std::runtime_error(entryContext + " bonus damage fields require bonusDamageFraction");
				}
				if (entry.contains("activeWhileTrigger"))
					rule.activeWhileTrigger = ParseTriggerKind(entry.at("activeWhileTrigger").get<std::string>());
				if (rule.activeWhileTrigger != ModuleTriggerKind::None)
				{
					if (!entry.contains("activeDurationSeconds")) throw std::runtime_error(entryContext + " active trigger requires activeDurationSeconds");
					rule.activeDurationSeconds = ParseLevelFloat(entry, "activeDurationSeconds", entryContext + ".activeDurationSeconds", minimumLevelCount,
						rule.activeDurationSecondsByLevel);
					if (rule.activeDurationSeconds <= 0.f) throw std::runtime_error(entryContext + " activeDurationSeconds must be positive");
					ValidateTable(rule.activeDurationSecondsByLevel, entryContext + ".activeDurationSeconds",
						[](float value) { return value > 0.f; }, "must be positive");
					rule.activeWhileEventTags = ParseRequiredEventTags(
						entry.value("activeWhileEventTags", Json::array()), rule.activeWhileTrigger, entryContext + ".activeWhileEventTags");
				}
				else if (entry.contains("activeDurationSeconds") || entry.contains("activeWhileEventTags"))
				{
					throw std::runtime_error(entryContext + " activeDurationSeconds and activeWhileEventTags require activeWhileTrigger");
				}
				if (!changesDamageMultiplier && !hasBonusDamage && rule.maxHealthDamageFraction <= 0.f && rule.perEmptyCellMultiplier <= 0.f && rule.ratioSlope <= 0.f)
					throw std::runtime_error(entryContext + " must change outgoing damage");
				if (rule.incoming && ((rule.condition != ModuleDamageCondition::OwnerHealthAtMost && rule.condition != ModuleDamageCondition::SourceHasStatus && rule.condition != ModuleDamageCondition::Always && rule.condition != ModuleDamageCondition::SourceMaxHealthGreaterThanOwner && rule.condition != ModuleDamageCondition::OwnerShieldOverMax) || hasBonusDamage))
					throw std::runtime_error(entryContext + " incoming rules require an owner-health/source-status/always/source-max-health condition and a multiplier only");
				if (!rule.incoming && rule.condition == ModuleDamageCondition::SourceMaxHealthGreaterThanOwner)
					throw std::runtime_error(entryContext + " SourceMaxHealthGreaterThanOwner is an incoming condition");
				if (!rule.incoming && rule.condition == ModuleDamageCondition::SourceHasStatus)
					throw std::runtime_error(entryContext + " SourceHasStatus is an incoming condition");
				variant.damageRules.push_back(std::move(rule));
			}
			index = 0;
			std::set<std::string> seenZones;
			const Json zoneEffects = object.value("zoneEffects", Json::array());
			if (!zoneEffects.is_array()) throw std::runtime_error(context + ".zoneEffects must be an array");
			for (const Json& entry : zoneEffects)
			{
				const std::string entryContext = context + ".zoneEffects[" + std::to_string(index++) + "]";
				if (!entry.is_object()) throw std::runtime_error(entryContext + " must be an object");
				ModuleZoneEffect effect;
				effect.zone = ParseTag(entry.at("zone"), GameplayTagKind::ModuleZone, entryContext + ".zone");
				if (!seenZones.insert(effect.zone.name).second) throw std::runtime_error(entryContext + " duplicates zone effect: " + effect.zone.name);
				effect.scope = ParseCooldownScope(entry.at("scope").get<std::string>());
				effect.multiplier = ParseLevelFloat(entry, "multiplier", entryContext + ".multiplier", minimumLevelCount, effect.multiplierByLevel);
				if (effect.multiplier <= 0.f || effect.multiplier > 1.f)
					throw std::runtime_error(entryContext + " multiplier must be in (0, 1]");
				ValidateTable(effect.multiplierByLevel, entryContext + ".multiplier",
					[](float value) { return value > 0.f && value <= 1.f; }, "must be in (0, 1]");
				variant.zoneEffects.push_back(std::move(effect));
			}
			const Json actions = object.value("eventActions", Json::array());
			if (!actions.is_array()) throw std::runtime_error(context + ".eventActions must be an array");
			for (const Json& entry : actions)
			{
				ModuleEventAction action;
				action.trigger = ParseTriggerKind(entry.at("trigger").get<std::string>());
				const std::string kind = entry.at("kind").get<std::string>();
				if (kind == "ShieldOverflowToHealth") action.kind = ModuleEventActionKind::ShieldOverflowToHealth;
				else if (kind == "RestoreShield") action.kind = ModuleEventActionKind::RestoreShield;
				else if (kind == "GrantTemporaryShield") action.kind = ModuleEventActionKind::GrantTemporaryShield;
				else if (kind == "RestoreHealth") action.kind = ModuleEventActionKind::RestoreHealth;
				else if (kind == "RestoreHealthOverTime") action.kind = ModuleEventActionKind::RestoreHealthOverTime;
				else if (kind == "RestoreAfterburner") action.kind = ModuleEventActionKind::RestoreAfterburner;
				else if (kind == "ReduceAllCooldowns") action.kind = ModuleEventActionKind::ReduceAllCooldowns;
				else if (kind == "ReduceFamilyCooldowns") action.kind = ModuleEventActionKind::ReduceFamilyCooldowns;
				else if (kind == "ReduceLongestCooldown") action.kind = ModuleEventActionKind::ReduceLongestCooldown;
				else if (kind == "ReduceLongestCooldownSeconds") action.kind = ModuleEventActionKind::ReduceLongestCooldownSeconds;
				else if (kind == "RefundAllCooldownsFraction") action.kind = ModuleEventActionKind::RefundAllCooldownsFraction;
				else if (kind == "RefundUsedCooldown") action.kind = ModuleEventActionKind::RefundUsedCooldown;
				else if (kind == "ApplyNearbyCryo") action.kind = ModuleEventActionKind::ApplyNearbyCryo;
				else if (kind == "AwardGold") action.kind = ModuleEventActionKind::AwardGold;
				else throw std::runtime_error(context + " unknown event action: " + kind);
				if (action.kind == ModuleEventActionKind::GrantTemporaryShield || action.kind == ModuleEventActionKind::RestoreHealthOverTime)
				{
					action.durationSeconds = ParseFiniteFloat(entry.at("durationSeconds"), context);
					if (action.durationSeconds <= 0.f || action.trigger != ModuleTriggerKind::AbilityActivated) throw std::runtime_error(context + " invalid temporary shield timing");
				}
				const bool refund = action.kind == ModuleEventActionKind::RefundUsedCooldown;
				const bool cryo = action.kind == ModuleEventActionKind::ApplyNearbyCryo;
				const bool allFraction = action.kind == ModuleEventActionKind::RefundAllCooldownsFraction;
				const bool longestSeconds = action.kind == ModuleEventActionKind::ReduceLongestCooldownSeconds;
				const bool abilityRestore = action.trigger == ModuleTriggerKind::AbilityActivated && (action.kind == ModuleEventActionKind::RestoreShield || action.kind == ModuleEventActionKind::RestoreHealth || action.kind == ModuleEventActionKind::GrantTemporaryShield || action.kind == ModuleEventActionKind::RestoreHealthOverTime);
				const ModuleTriggerKind expectedTrigger = refund ? ModuleTriggerKind::AbilityActivated : (cryo || allFraction) ? ModuleTriggerKind::DamageTaken : longestSeconds ? ModuleTriggerKind::CriticalHit : ModuleTriggerKind::KillConfirmed;
				if (!abilityRestore && action.trigger != expectedTrigger)
					throw std::runtime_error(context + " incompatible event action trigger");
				action.requiredEventTags = ParseRequiredEventTags(entry.value("requiredEventTags", Json::array()), action.trigger, context + ".eventActions.requiredEventTags");
				action.amount = ParseLevelFloat(entry, "amount", context + ".eventActions.amount", minimumLevelCount, action.amountByLevel);
				const bool seconds = action.kind == ModuleEventActionKind::ReduceAllCooldowns || longestSeconds;
				const auto validAmount = [seconds, cryo, gold = action.kind == ModuleEventActionKind::AwardGold](float value) { return value > 0.f && ((cryo || gold) ? value <= static_cast<float>(std::numeric_limits<int>::max() / 2) && std::floor(value) == value : seconds || value <= 1.f); };
				if (!validAmount(action.amount)) throw std::runtime_error(context + " invalid event action amount");
				ValidateTable(action.amountByLevel, context, validAmount, "invalid event action amount");
				if (entry.contains("abilityFamily"))
				{
					if (action.kind != ModuleEventActionKind::ReduceFamilyCooldowns && action.kind != ModuleEventActionKind::ReduceLongestCooldown)
						throw std::runtime_error(context + " abilityFamily requires a family cooldown action");
					action.abilityFamily = ParseTag(entry.at("abilityFamily"), GameplayTagKind::AbilityCategory, context + ".abilityFamily");
					if (action.abilityFamily == GameplayTags::Ability::Primary) throw std::runtime_error(context + " cooldown refunds exclude Primary");
				}
				if (action.kind == ModuleEventActionKind::ReduceFamilyCooldowns && !action.abilityFamily.IsValid())
					throw std::runtime_error(context + " family cooldown refund requires abilityFamily");
				action.everyCount = entry.contains("everyCount") ? ParsePositiveInt(entry.at("everyCount"), context) : 1;
				if (action.kind == ModuleEventActionKind::ShieldOverflowToHealth && action.everyCount != 1)
					throw std::runtime_error(context + " shield overflow conversion must apply to every shield grant");
				action.overflowHealthFraction = entry.contains("overflowHealthFraction") ? ParseFiniteFloat(entry.at("overflowHealthFraction"), context) : 0.f;
				if (action.overflowHealthFraction < 0.f || action.overflowHealthFraction > 1.f ||
					(entry.contains("overflowHealthFraction") && action.kind != ModuleEventActionKind::RestoreShield))
					throw std::runtime_error(context + " invalid shield overflow conversion");
				if (entry.contains("chancePerLuck"))
				{
					if (!refund) throw std::runtime_error(context + " chancePerLuck requires RefundUsedCooldown");
					action.chancePerLuck = ParseLevelFloat(entry, "chancePerLuck", context, minimumLevelCount, action.chancePerLuckByLevel);
					if (action.chancePerLuck <= 0.f) throw std::runtime_error(context + " chancePerLuck must be positive");
					ValidateTable(action.chancePerLuckByLevel, context, [](float value) { return value > 0.f; }, "chancePerLuck must be positive");
				}
				else if (refund) throw std::runtime_error(context + " refund requires chancePerLuck");
				if (cryo)
				{
					action.radius = ParseFiniteFloat(entry.at("radius"), context + ".radius");
					action.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), context + ".cooldownSeconds");
					action.targetCount = ParsePositiveLevelInt(entry, "targetCount", context + ".targetCount", minimumLevelCount, action.targetCountByLevel);
					if (action.radius <= 0.f || action.cooldownSeconds <= 0.f || action.everyCount != 1)
						throw std::runtime_error(context + " nearby Cryo requires positive radius/cooldown and everyCount=1");
				}
				else if (action.kind == ModuleEventActionKind::RefundAllCooldownsFraction)
				{
					action.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), context + ".cooldownSeconds");
					if (action.cooldownSeconds <= 0.f || action.everyCount != 1)
						throw std::runtime_error(context + " all-cooldown refund requires positive cooldown and everyCount=1");
				}
				else if (action.kind == ModuleEventActionKind::RestoreHealthOverTime)
				{
					action.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), context);
					if (action.cooldownSeconds < action.durationSeconds) throw std::runtime_error(context + " healing cooldown must cover duration");
				}
				else if (entry.contains("radius") || entry.contains("cooldownSeconds") || entry.contains("targetCount"))
					throw std::runtime_error(context + " nearby targeting fields require ApplyNearbyCryo");
				variant.eventActions.push_back(std::move(action));
			}
			const Json ownerRules = object.value("ownerAttributeRules", Json::array());
			if (!ownerRules.is_array()) throw std::runtime_error(context + ".ownerAttributeRules must be an array");
			for (const Json& entry : ownerRules)
			{
				ModuleOwnerAttributeRule rule;
				const std::string source = entry.at("source").get<std::string>();
				if (source == "HealthThreshold") rule.source = ModuleOwnerAttributeSource::HealthThreshold;
				else if (source == "MovementSpeed") rule.source = ModuleOwnerAttributeSource::MovementSpeed;
				else if (source == "HigherPower") rule.source = ModuleOwnerAttributeSource::HigherPower;
				else if (source == "Luck") rule.source = ModuleOwnerAttributeSource::Luck;
				else if (source == "Armor") rule.source = ModuleOwnerAttributeSource::Armor;
				else if (source == "ArmorHigherPower") rule.source = ModuleOwnerAttributeSource::ArmorHigherPower;
				else if (source == "ArmorLowerPower") rule.source = ModuleOwnerAttributeSource::ArmorLowerPower;
				else if (source == "Gold") rule.source = ModuleOwnerAttributeSource::Gold;
				else if (source == "GoldArmorBonus") rule.source = ModuleOwnerAttributeSource::GoldArmorBonus;
				else if (source == "MaxHealthFraction") rule.source = ModuleOwnerAttributeSource::MaxHealthFraction;
				else if (source == "PowerEqualize") rule.source = ModuleOwnerAttributeSource::PowerEqualize;
				else if (source == "ShieldReserve") rule.source = ModuleOwnerAttributeSource::ShieldReserve;
				else throw std::runtime_error(context + " unknown owner attribute source: " + source);
				rule.attributeId = sas::AttributeId{ entry.at("attributeId").get<std::string>() };
				std::string failure;
				if (!AttributeIdSchema::Validate(rule.attributeId, &failure)) throw std::runtime_error(context + failure);
				rule.target = ParseTarget(entry.value("target", std::string{ "Owner" }));
				rule.operation = AttributeJsonParser::ParseOperation(entry.value("operation", std::string{ "Add" }), context);
				const bool ownerAttribute = rule.target == ModuleAttributeTarget::Owner && rule.operation == sas::AttributeModifierOperation::Add &&
					(rule.attributeId == OwnerAttributeIds::AttackSpeed || rule.attributeId == OwnerAttributeIds::AttackPower || rule.attributeId == OwnerAttributeIds::EnergyPower ||
						((rule.source == ModuleOwnerAttributeSource::Luck || rule.source == ModuleOwnerAttributeSource::HealthThreshold) && rule.attributeId == OwnerAttributeIds::CriticalChance) || (rule.source == ModuleOwnerAttributeSource::Gold && rule.attributeId == OwnerAttributeIds::Armor) || (rule.source == ModuleOwnerAttributeSource::GoldArmorBonus && rule.attributeId == OwnerAttributeIds::MaxHealth) || (rule.source == ModuleOwnerAttributeSource::MaxHealthFraction && rule.attributeId == OwnerAttributeIds::Armor));
				const bool shipAttribute = rule.target == ModuleAttributeTarget::Ship && rule.operation == sas::AttributeModifierOperation::Multiply && rule.source == ModuleOwnerAttributeSource::HealthThreshold && rule.attributeId == ShipAttributeIds::ShieldRegen;
				if (!ownerAttribute && !shipAttribute)
					throw std::runtime_error(context + " unsupported conditional owner attribute");
				rule.healthAtMost = entry.contains("healthAtMost") ? ParseFiniteFloat(entry.at("healthAtMost"), context) : 1.f;
				if ((rule.source == ModuleOwnerAttributeSource::MovementSpeed || rule.source == ModuleOwnerAttributeSource::Luck) && entry.contains("healthAtMost"))
					throw std::runtime_error(context + " movement scaling does not accept a health threshold");
				if (rule.healthAtMost <= 0.f || rule.healthAtMost > 1.f) throw std::runtime_error(context + " invalid owner health threshold");
				rule.amount = ParseLevelFloat(entry, "amount", context, minimumLevelCount, rule.amountByLevel);
				if (rule.amount <= 0.f) throw std::runtime_error(context + " owner attribute amount must be positive");
				ValidateTable(rule.amountByLevel, context, [](float value) { return value > 0.f; }, "owner attribute amount must be positive");
				if (entry.contains("maximumAmount"))
				{
					rule.maximumAmount = ParseLevelFloat(entry, "maximumAmount", context, minimumLevelCount, rule.maximumAmountByLevel);
					if (rule.source != ModuleOwnerAttributeSource::Gold || rule.maximumAmount <= 0.f) throw std::runtime_error(context + " maximumAmount requires positive Gold-based output");
					ValidateTable(rule.maximumAmountByLevel, context, [](float value) { return value > 0.f; }, "invalid Gold Armor cap");
				}
				if (rule.source == ModuleOwnerAttributeSource::Gold && (!entry.contains("maximumAmount") || rule.attributeId != OwnerAttributeIds::Armor || entry.contains("healthAtMost"))) throw std::runtime_error(context + " Gold requires capped Armor output without health threshold");
				if (rule.source == ModuleOwnerAttributeSource::GoldArmorBonus && (!sideFeature || entry.contains("maximumAmount") || entry.contains("healthAtMost"))) throw std::runtime_error(context + " GoldArmorBonus is a side-feature source without cap or health threshold");
				rule.fadeSeconds = entry.contains("fadeSeconds") ? ParseFiniteFloat(entry.at("fadeSeconds"), context) : 0.f;
				if (rule.fadeSeconds < 0.f || (entry.contains("fadeSeconds") && rule.source != ModuleOwnerAttributeSource::MovementSpeed))
					throw std::runtime_error(context + " invalid movement fade");
				rule.minimumSpeed = entry.contains("minimumSpeed") ? ParseFiniteFloat(entry.at("minimumSpeed"), context) : 0.f;
				rule.speedStep = entry.contains("speedStep") ? ParseFiniteFloat(entry.at("speedStep"), context) : 0.f;
				if (rule.minimumSpeed < 0.f || rule.speedStep < 0.f || ((entry.contains("minimumSpeed") || entry.contains("speedStep")) && rule.source != ModuleOwnerAttributeSource::MovementSpeed))
					throw std::runtime_error(context + " invalid movement speed steps");
				if (entry.contains("speedStep") && rule.speedStep <= 0.f) throw std::runtime_error(context + " movement speed step must be positive");
				variant.ownerAttributeRules.push_back(std::move(rule));
			}
			const Json movement = object.value("conditionalMovementRules", Json::array());
			if (!movement.is_array()) throw std::runtime_error(context + " conditionalMovementRules must be an array");
			for (const Json& entry : movement)
			{
				if (!entry.is_object() || entry.size() != 2 || !entry.contains("condition") || !entry.contains("multiplier")) throw std::runtime_error(context + " invalid conditional movement rule");
				ModuleConditionalMovementRule rule;
				if (entry.at("condition") != "OwnerShieldOverMax") throw std::runtime_error(context + " unsupported conditional movement condition");
				rule.multiplier = ParseLevelFloat(entry, "multiplier", context, minimumLevelCount, rule.multiplierByLevel);
				if (rule.multiplier <= 1.f) throw std::runtime_error(context + " movement multiplier must exceed one");
				ValidateTable(rule.multiplierByLevel, context, [](float value) { return value > 1.f; }, "invalid conditional movement multiplier");
				variant.conditionalMovementRules.push_back(rule);
			}
			const Json fades = object.value("movementFadeRules", Json::array());
			if (!fades.is_array()) throw std::runtime_error(context + ".movementFadeRules must be an array");
			for (const Json& entry : fades)
			{
				if (!entry.is_object() || entry.size() != 1 || !entry.contains("fadeSeconds")) throw std::runtime_error(context + " invalid movement fade rule");
				const float seconds = ParseFiniteFloat(entry.at("fadeSeconds"), context);
				if (seconds <= 0.f) throw std::runtime_error(context + " movement fade must be positive");
				variant.movementFadeRules.push_back({ seconds });
			}
			const Json reflections = object.value("shieldReflections", Json::array());
			if (!reflections.is_array()) throw std::runtime_error(context + ".shieldReflections must be an array");
			for (const Json& entry : reflections)
			{
				ModuleRulePackage::ShieldReflection rule;
				rule.fraction = ParseLevelFloat(entry, "fraction", context + ".reflection.fraction", minimumLevelCount, rule.fractionByLevel);
				rule.durationSeconds = ParseFiniteFloat(entry.at("durationSeconds"), context + ".reflection.durationSeconds");
					rule.damageType = ParseTag(entry.at("damageType"), GameplayTagKind::DamageType, context + " shield reflection damageType");
				rule.electricStacks = entry.contains("electricStacks") ? ParsePositiveInt(entry.at("electricStacks"), context + " shield reflection electricStacks") : 0;
				if (rule.electricStacks > 4 || (rule.electricStacks > 0 && rule.damageType != GameplayTags::Damage::Type::Electric)) throw std::runtime_error(context + " invalid shield reflection electric stacks");
				const auto valid = [](float value) { return value > 0.f; };
				if (!valid(rule.fraction) || rule.durationSeconds <= 0.f) throw std::runtime_error(context + " invalid shield reflection");
				ValidateTable(rule.fractionByLevel, context, valid, "invalid shield reflection fraction");
				variant.shieldReflections.push_back(rule);
			}
			ParseShieldRules(object, context, minimumLevelCount, variant);
			ParseShieldBatteryRules(object, context, minimumLevelCount, sideFeature, variant);
			ParseShieldRuptureRules(object, context, minimumLevelCount, sideFeature, variant);
			ParseMovementRules(object, context, minimumLevelCount, variant);
			ParseRunCounterRules(object, context, minimumLevelCount, variant);
			ParseSustainedSpeedRules(object, context, minimumLevelCount, variant, sideFeature);
			ParseOwnerBurstRules(object, context, minimumLevelCount, variant);
			ParseOwnerBurstModifiers(object, context, variant);
			ParseTargetSlowRules(object, context, variant);
			ParseKillStackCooldownRules(object, context, variant);
			ParseKillStackDamageRules(object, context, variant);
			ParseChargeRules(object, context, minimumLevelCount, variant, sideFeature);
			ParseFamilyStatRules(object, context, minimumLevelCount, variant, sideFeature);
			ParseFragmentation(object, context, variant, sideFeature);
			ParseEpicDefenseRules(object, context, minimumLevelCount, variant, sideFeature);
			ParsePrimaryMountRules(object, context, minimumLevelCount, variant, sideFeature);
			ParseEpicOffenseRules(object, context, minimumLevelCount, variant, sideFeature);
			ParseEpicUtilityMovementRules(object, context, minimumLevelCount, variant, sideFeature);
			const Json statusRules = object.value("statusRules", Json::object());
			if (!statusRules.is_object()) throw std::runtime_error(context + ".statusRules must be an object");
			const std::set<std::string> statusKeys{ "electricOverrides", "electricSpreads", "primaryStatusPrimers", "cryoFreezes", "thermalCryoDurationBonuses", "freezeSpreads", "durationBonuses", "thermalBursts", "kineticConsumes" };
			for (const auto& field : statusRules.items())
				if (!statusKeys.count(field.key()) || !field.value().is_array()) throw std::runtime_error(context + " invalid status rule list: " + field.key());
			const auto positive = [](float value) { return value > 0.f; };
			const auto nonnegative = [](float value) { return value >= 0.f; };
			const auto validateStatusFields = [&context](const Json& entry, std::initializer_list<const char*> allowed)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " status rule must be an object");
				for (const auto& field : entry.items())
					if (std::none_of(allowed.begin(), allowed.end(), [&field](const char* name) { return field.key() == name; })) throw std::runtime_error(context + " unknown status rule field: " + field.key());
			};
			for (const Json& entry : statusRules.value("electricOverrides", Json::array()))
			{
				validateStatusFields(entry, { "damageTakenByStack", "damageTakenByStackGrowth" });
				ModuleElectricStatusRule rule;
				const auto& values = entry.at("damageTakenByStack");
				if (!values.is_array() || values.empty()) throw std::runtime_error(context + " electric stack table must not be empty");
				for (const auto& value : values)
				{
					ModuleLevelTable<float> table;
					const float base = ParseLevelFloat(value, context + ".electric.damageTakenByStack", minimumLevelCount, table);
					ParseLevelGrowth(entry, "damageTakenByStack", context, table);
					if (base < 0.f) throw std::runtime_error(context + " invalid electric bonus");
					ValidateTable(table, context, nonnegative, "invalid electric bonus");
					rule.damageTakenByStack.push_back(base);
					rule.damageTakenByStackByLevel.push_back(std::move(table));
				}
				variant.statusRules.electricOverrides.push_back(std::move(rule));
			}
			for (const Json& entry : statusRules.value("primaryStatusPrimers", Json::array()))
			{
				validateStatusFields(entry, { "cooldownSeconds", "cooldownSecondsGrowth" });
				ModulePrimaryStatusPrimerRule rule; rule.cooldownSeconds = ParseLevelFloat(entry, "cooldownSeconds", context, minimumLevelCount, rule.cooldownSecondsByLevel);
				ValidateTable(rule.cooldownSecondsByLevel, context, positive, "invalid status primer cooldown");
				if (rule.cooldownSeconds <= 0.f) throw std::runtime_error(context + " invalid status primer cooldown");
				variant.statusRules.primaryStatusPrimers.push_back(rule);
			}
			for (const Json& entry : statusRules.value("electricSpreads", Json::array()))
			{
				validateStatusFields(entry, { "threshold", "radius", "stacks", "targetCount", "targetCountGrowth", "cooldownSeconds", "cooldownSecondsGrowth" });
				ModuleElectricSpreadRule rule;
				rule.threshold = ParsePositiveInt(entry.at("threshold"), context);
				rule.radius = ParseFiniteFloat(entry.at("radius"), context);
				rule.stacks = ParsePositiveInt(entry.at("stacks"), context);
				rule.targetCount = ParsePositiveInt(entry.at("targetCount"), context);
				rule.cooldownSeconds = ParseFiniteFloat(entry.at("cooldownSeconds"), context);
				if (rule.threshold > 5 || !positive(rule.radius) || !positive(rule.cooldownSeconds)) throw std::runtime_error(context + " invalid electric spread");
				variant.statusRules.electricSpreads.push_back(rule);
			}
			for (const Json& entry : statusRules.value("cryoFreezes", Json::array()))
			{
				ModuleCryoFreezeRule rule;
				validateStatusFields(entry, { "threshold", "durationSeconds", "durationSecondsGrowth", "cooldownSeconds", "cooldownSecondsGrowth", "normalTargetsOnly" });
				rule.threshold = ParsePositiveInt(entry.at("threshold"), context + ".freeze.threshold");
				rule.durationSeconds = ParseLevelFloat(entry, "durationSeconds", context, minimumLevelCount, rule.durationSecondsByLevel);
				rule.cooldownSeconds = ParseLevelFloat(entry, "cooldownSeconds", context, minimumLevelCount, rule.cooldownSecondsByLevel);
				rule.normalTargetsOnly = entry.value("normalTargetsOnly", true);
				if (!positive(rule.durationSeconds) || !positive(rule.cooldownSeconds)) throw std::runtime_error(context + " invalid freeze timing");
				ValidateTable(rule.durationSecondsByLevel, context, positive, "invalid freeze duration");
				ValidateTable(rule.cooldownSecondsByLevel, context, positive, "invalid freeze cooldown");
				variant.statusRules.cryoFreezes.push_back(std::move(rule));
			}
			for (const char* key : { "thermalCryoDurationBonuses", "durationBonuses" })
			{
				for (const Json& entry : statusRules.value(key, Json::array()))
				{
					validateStatusFields(entry, { "bonusSeconds", "bonusSecondsGrowth", "durationMultiplier", "durationMultiplierGrowth" });
					ModuleStatusDurationRule rule;
					if (entry.contains("bonusSeconds")) rule.bonusSeconds = ParseLevelFloat(entry, "bonusSeconds", context, minimumLevelCount, rule.bonusSecondsByLevel);
					if (entry.contains("durationMultiplier"))
					{
						if (std::string{ key } != "durationBonuses") throw std::runtime_error(context + " duration multiplier is general-only");
						rule.durationMultiplier = ParseLevelFloat(entry, "durationMultiplier", context, minimumLevelCount, rule.durationMultiplierByLevel);
						ValidateTable(rule.durationMultiplierByLevel, context, [](float value) { return value >= 1.f; }, "invalid status duration multiplier");
					}
					if (rule.bonusSeconds < 0.f || rule.durationMultiplier < 1.f || (rule.bonusSeconds == 0.f && rule.durationMultiplier == 1.f)) throw std::runtime_error(context + " invalid status duration bonus");
					ValidateTable(rule.bonusSecondsByLevel, context, positive, "invalid status duration bonus");
					(std::string{ key } == "durationBonuses" ? variant.statusRules.durationBonuses : variant.statusRules.thermalCryoDurationBonuses).push_back(std::move(rule));
				}
			}
			for (const Json& entry : statusRules.value("freezeSpreads", Json::array()))
			{
				validateStatusFields(entry, { "radius", "stacks" });
				ModuleFreezeSpreadRule rule;
				rule.radius = ParseFiniteFloat(entry.at("radius"), context);
				rule.stacks = ParsePositiveInt(entry.at("stacks"), context);
				if (!positive(rule.radius)) throw std::runtime_error(context + " invalid freeze spread radius");
				variant.statusRules.freezeSpreads.push_back(rule);
			}
			ParseThermalBurstRules(statusRules, context, minimumLevelCount, variant, sideFeature);
			ParseKineticConsumeRules(statusRules, context, minimumLevelCount, variant, sideFeature);
			const Json generated = object.value("generatedRules", Json::object());
			if (!generated.is_object()) throw std::runtime_error(context + ".generatedRules must be an object");
			const std::set<std::string> generatedKeys{ "ricochets", "guidedSalvos", "delayedExplosions", "trailMines", "killExplosions", "speedLances", "annularBursts" };
			for (const auto& field : generated.items())
				if (!generatedKeys.count(field.key()) || !field.value().is_array() || field.value().size() > 1) throw std::runtime_error(context + " generated rule lists accept at most one rule per family");
			const auto fields = [&context](const Json& entry, std::initializer_list<const char*> allowed)
			{
				if (!entry.is_object()) throw std::runtime_error(context + " generated rule must be an object");
				for (const auto& field : entry.items())
					if (std::none_of(allowed.begin(), allowed.end(), [&field](const char* name) { return field.key() == name; })) throw std::runtime_error(context + " unknown generated rule field: " + field.key());
			};
			const auto number = [&context](const Json& entry, const char* key)
			{
				if (!entry.contains(key)) return 0.f;
				const float value = ParseFiniteFloat(entry.at(key), context + ".generatedRules." + key);
				if (value <= 0.f) throw std::runtime_error(context + " generated value must be positive: " + key);
				return value;
			};
			const auto count = [&context](const Json& entry, const char* key)
			{
				const int value = entry.contains(key) ? ParsePositiveInt(entry.at(key), context + ".generatedRules." + key) : 0;
				if (value > 1024) throw std::runtime_error(context + " generated count exceeds technical ceiling: " + key);
				return value;
			};
			const auto fraction = [&context, minimumLevelCount](const Json& entry, const char* key, ModuleLevelTable<float>& table)
			{
				if (!entry.contains(key)) return 0.f;
				const float value = ParseLevelFloat(entry, key, context + ".generatedRules." + key, minimumLevelCount, table);
				const auto valid = [](float v) { return v > 0.f && v <= 1.f; };
				if (!valid(value)) throw std::runtime_error(context + " generated damage fraction must be in (0,1]");
				ValidateTable(table, context, valid, "generated fraction must be in (0,1]");
				return value;
			};
			for (const auto& entry : generated.value("ricochets", Json::array()))
			{
				fields(entry, { "damageFraction", "damageFractionGrowth", "range", "secondaryFraction", "transferStatus" });
				ModuleRicochetRule rule;
				rule.damageFraction = fraction(entry, "damageFraction", rule.damageFractionByLevel);
				rule.range = number(entry, "range");
				rule.secondaryFraction = number(entry, "secondaryFraction");
				rule.transferStatus = entry.value("transferStatus", false);
				if ((rule.damageFraction > 0.f && rule.range <= 0.f) || rule.secondaryFraction > 1.f || (rule.damageFraction <= 0.f && rule.secondaryFraction <= 0.f && !rule.transferStatus)) throw std::runtime_error(context + " incomplete ricochet rule");
				variant.generatedRules.ricochets.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("guidedSalvos", Json::array()))
			{
				fields(entry, { "damageFraction", "damageFractionGrowth", "projectileCount", "speed", "lifetime", "targetRange", "maxActive", "maxActiveGrowth", "damageType" });
				ModuleGuidedSalvoRule rule;
				rule.damageFraction = fraction(entry, "damageFraction", rule.damageFractionByLevel);
				rule.projectileCount = count(entry, "projectileCount"); rule.speed = number(entry, "speed"); rule.lifetime = number(entry, "lifetime"); rule.targetRange = number(entry, "targetRange"); rule.maxActive = count(entry, "maxActive");
				if (rule.damageFraction > 0.f) rule.damageType = ParseTag(entry.at("damageType"), GameplayTagKind::DamageType, context + " guided salvo damageType");
				else if (entry.contains("damageType")) throw std::runtime_error(context + " guided salvo damageType requires damageFraction");
				if (rule.projectileCount <= 0 || (rule.damageFraction > 0.f && (rule.speed <= 0.f || rule.lifetime <= 0.f || rule.targetRange <= 0.f || rule.maxActive <= 0))) throw std::runtime_error(context + " incomplete guided salvo rule");
				variant.generatedRules.guidedSalvos.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("delayedExplosions", Json::array()))
			{
				fields(entry, { "damageFraction", "damageFractionGrowth", "delaySeconds", "radius", "maxActive", "maxActiveGrowth" });
				ModuleDelayedExplosionRule rule;
				rule.damageFraction = fraction(entry, "damageFraction", rule.damageFractionByLevel);
				rule.delaySeconds = number(entry, "delaySeconds"); rule.radius = number(entry, "radius"); rule.maxActive = count(entry, "maxActive");
				if (rule.damageFraction <= 0.f || rule.delaySeconds <= 0.f || rule.radius <= 0.f || rule.maxActive <= 0) throw std::runtime_error(context + " incomplete delayed explosion rule");
				variant.generatedRules.delayedExplosions.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("trailMines", Json::array()))
			{
				fields(entry, { "damage", "damageGrowth", "attackPowerCoefficient", "attackPowerCoefficientGrowth", "spacing", "windowSeconds", "fuseSeconds", "radius", "maxActive", "maxActiveGrowth", "additionalLimit", "cryoStacks", "damageType" });
				ModuleTrailMineRule rule;
				if (entry.contains("damage"))
				{
					rule.damage = ParseLevelFloat(entry, "damage", context, minimumLevelCount, rule.damageByLevel);
					if (!positive(rule.damage)) throw std::runtime_error(context + " mine damage must be positive");
					ValidateTable(rule.damageByLevel, context, positive, "mine damage must be positive");
				}
				if (entry.contains("attackPowerCoefficient"))
				{
					rule.attackPowerCoefficient = ParseLevelFloat(entry, "attackPowerCoefficient", context, minimumLevelCount, rule.attackPowerCoefficientByLevel);
					const auto nonNegative = [](float value) { return std::isfinite(value) && value >= 0.f; };
					if (rule.damage <= 0.f || !nonNegative(rule.attackPowerCoefficient)) throw std::runtime_error(context + " mine AP coefficient requires damage and must be non-negative");
					ValidateTable(rule.attackPowerCoefficientByLevel, context, nonNegative, "mine AP coefficient must be non-negative");
				}
				rule.spacing = number(entry, "spacing"); rule.windowSeconds = number(entry, "windowSeconds"); rule.fuseSeconds = number(entry, "fuseSeconds"); rule.radius = number(entry, "radius");
				if (entry.contains("maxActive")) rule.maxActive = ParsePositiveLevelInt(entry, "maxActive", context, minimumLevelCount, rule.maxActiveByLevel);
				rule.additionalLimit = count(entry, "additionalLimit"); rule.cryoStacks = count(entry, "cryoStacks");
				if (rule.damage > 0.f) rule.damageType = ParseTag(entry.at("damageType"), GameplayTagKind::DamageType, context + " trail mine damageType");
				else if (entry.contains("damageType")) throw std::runtime_error(context + " trail mine damageType requires damage");
				if ((rule.damage > 0.f && (rule.spacing <= 0.f || rule.windowSeconds <= 0.f || rule.fuseSeconds <= 0.f || rule.radius <= 0.f || rule.maxActive <= 0)) || (rule.damage <= 0.f && rule.additionalLimit <= 0 && rule.cryoStacks <= 0)) throw std::runtime_error(context + " incomplete trail mine rule");
				if (rule.maxActive > 1024) throw std::runtime_error(context + " mine limit exceeds technical ceiling");
				ValidateTable(rule.maxActiveByLevel, context, [](int v) { return v <= 1024; }, "mine limit exceeds technical ceiling");
				variant.generatedRules.trailMines.push_back(std::move(rule));
			}
			for (const auto& entry : generated.value("annularBursts", Json::array()))
			{
				fields(entry, { "innerRadius", "outerRadius", "baseDamage", "baseDamageGrowth", "energyPowerCoefficient", "energyPowerCoefficientGrowth" });
				ModuleAnnularBurstRule rule;
				rule.innerRadius = number(entry, "innerRadius");
				rule.outerRadius = number(entry, "outerRadius");
				rule.baseDamage = entry.contains("baseDamage") ? ParseLevelFloat(entry, "baseDamage", context + ".generatedRules.baseDamage", minimumLevelCount, rule.baseDamageByLevel) : 0.f;
				rule.energyPowerCoefficient = entry.contains("energyPowerCoefficient") ? ParseLevelFloat(entry, "energyPowerCoefficient", context + ".generatedRules.energyPowerCoefficient", minimumLevelCount, rule.energyPowerCoefficientByLevel) : 0.f;
				ValidateTable(rule.baseDamageByLevel, context, [](float value) { return value >= 0.f; }, "annular burst base damage must not be negative");
				ValidateTable(rule.energyPowerCoefficientByLevel, context, [](float value) { return value > 0.f; }, "annular burst Energy Power coefficient must be positive");
				if (sideFeature || rule.innerRadius <= 0.f || rule.outerRadius <= rule.innerRadius || (rule.baseDamage <= 0.f && rule.energyPowerCoefficient <= 0.f)) throw std::runtime_error(context + " annular burst needs an ordered positive ring and damage; main sources only");
				variant.generatedRules.annularBursts.push_back(rule);
			}
			for (const auto& entry : generated.value("speedLances", Json::array()))
			{
				fields(entry, { "speedDamageCoefficient", "speedDamageCoefficientGrowth", "sampleSeconds", "projectileSpeed", "range", "echoDelaySeconds", "echoFraction", "firstTargetElectricStacks" });
				ModuleSpeedLanceRule rule;
				rule.speedDamageCoefficient = entry.contains("speedDamageCoefficient") ? ParseLevelFloat(entry, "speedDamageCoefficient", context, minimumLevelCount, rule.speedDamageCoefficientByLevel) : 0.f;
				ValidateTable(rule.speedDamageCoefficientByLevel, context, positive, "invalid speed damage coefficient");
				rule.sampleSeconds = number(entry, "sampleSeconds"); rule.projectileSpeed = number(entry, "projectileSpeed"); rule.range = number(entry, "range");
				rule.echoDelaySeconds = number(entry, "echoDelaySeconds"); rule.echoFraction = number(entry, "echoFraction"); rule.firstTargetElectricStacks = count(entry, "firstTargetElectricStacks");
				if ((rule.speedDamageCoefficient > 0.f && (rule.sampleSeconds <= 0.f || rule.projectileSpeed <= 0.f || rule.range <= 0.f)) || ((rule.echoDelaySeconds > 0.f) != (rule.echoFraction > 0.f)) || rule.echoFraction > 1.f || (rule.speedDamageCoefficient == 0.f && rule.echoFraction == 0.f && rule.firstTargetElectricStacks == 0)) throw std::runtime_error(context + " incomplete Speed Lance rule");
				variant.generatedRules.speedLances.push_back(rule);
			}
			for (const auto& entry : generated.value("killExplosions", Json::array()))
			{
				fields(entry, { "maxHealthFraction", "maxHealthFractionGrowth", "radius", "maxActive", "maxActiveGrowth", "damageType", "presentationId", "requiredTargetStatuses" });
				ModuleKillExplosionRule rule;
				rule.maxHealthFraction = fraction(entry, "maxHealthFraction", rule.maxHealthFractionByLevel);
				rule.radius = number(entry, "radius"); rule.maxActive = count(entry, "maxActive");
				rule.damageType = ParseTag(entry.at("damageType"), GameplayTagKind::DamageType, context + " kill explosion damageType");
				rule.presentationId = ParseContentId(entry.at("presentationId"), context + " kill explosion presentationId");
				const auto& statuses = entry.at("requiredTargetStatuses");
				if (!statuses.is_array() || statuses.empty()) throw std::runtime_error(context + " kill explosion requires target statuses");
				for (const auto& tag : statuses) rule.requiredTargetStatuses.push_back(ParseTag(tag, GameplayTagKind::Status, context));
				if (rule.maxHealthFraction <= 0.f || rule.radius <= 0.f || rule.maxActive <= 0) throw std::runtime_error(context + " incomplete kill explosion rule");
				variant.generatedRules.killExplosions.push_back(std::move(rule));
			}
			return variant;
		}
	}
}

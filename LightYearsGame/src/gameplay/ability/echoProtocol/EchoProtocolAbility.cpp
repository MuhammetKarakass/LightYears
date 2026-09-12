#include "gameplay/ability/echoProtocol/EchoProtocolAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/echoProtocol/EchoProtocolContracts.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		const sas::GameplayAttribute* FindAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}
	}

	bool EchoProtocolAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::EchoProtocol::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			definition.maxCharges != 1 ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			definition.recordInAbilityHistory)
		{
			if (failureReason)
			{
				*failureReason =
					"Echo Protocol requires a loadout, pressed, instant, one-charge definition and must not record itself.";
			}
			return false;
		}

		for (const sas::AttributeId& attributeId : {
			AbilityData::EchoProtocol::Attribute::PowerBase,
			AbilityData::EchoProtocol::Attribute::PowerPerLevel,
			AbilityData::EchoProtocol::Attribute::AttackPowerScale,
			AbilityData::EchoProtocol::Attribute::MaxHealthScale,
			AbilityData::EchoProtocol::Attribute::EnergyPowerScale,
			AbilityData::EchoProtocol::Attribute::AttackSpeedScale,
			AbilityData::EchoProtocol::Attribute::LuckScale,
			AbilityData::EchoProtocol::Attribute::MovementScale
		})
		{
			const sas::GameplayAttribute* attribute = FindAttribute(definition, attributeId);
			if (!attribute || !std::isfinite(attribute->baseValue) || attribute->baseValue < 0.f)
			{
				if (failureReason)
				{
					*failureReason = "Echo Protocol must declare eight non-negative runtime attributes.";
				}
				return false;
			}
		}
		if (definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason = "Echo Protocol requires fourteen cooldown progression steps.";
			}
			return false;
		}
		return true;
	}

	float EchoProtocolAbility::ResolveValue(
		GameAbilityBehaviorContext& context,
		const sas::AttributeId& attributeId,
		float fallback
	) const
	{
		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		return sas::FindAttributeValue(values, attributeId, fallback);
	}

	float EchoProtocolAbility::ResolveScalingCoefficient(
		GameAbilityBehaviorContext& context,
		const sas::AttributeId& sourceAttributeId
	) const
	{
		using Attribute = AbilityData::EchoProtocol::Attribute;
		if (sourceAttributeId == OwnerAttributeIds::AttackPower)
		{
			return ResolveValue(context, Attribute::AttackPowerScale, 1.30f);
		}
		if (sourceAttributeId == OwnerAttributeIds::MaxHealth)
		{
			return ResolveValue(context, Attribute::MaxHealthScale, 0.20f);
		}
		if (sourceAttributeId == OwnerAttributeIds::EnergyPower)
		{
			return ResolveValue(context, Attribute::EnergyPowerScale, 0.10f);
		}
		if (sourceAttributeId == OwnerAttributeIds::AttackSpeed)
		{
			return ResolveValue(context, Attribute::AttackSpeedScale, 0.20f);
		}
		if (sourceAttributeId == OwnerAttributeIds::Luck)
		{
			return ResolveValue(context, Attribute::LuckScale, 0.25f);
		}
		if (sourceAttributeId == OwnerAttributeIds::MoveSpeedHorizontal ||
			sourceAttributeId == OwnerAttributeIds::MoveSpeedVertical)
		{
			return ResolveValue(context, Attribute::MovementScale, 0.15f);
		}
		return 0.f;
	}

	bool EchoProtocolAbility::Activate(GameAbilityBehaviorContext& context)
	{
		AbilityUseHistory& history = context.abilitySystem.GetAbilityUseHistory();
		// Do not erase consumed records here. Other future systems may need the
		// shared ten-record window; this ability's cursor is the temporary Echo
		// policy and is intentionally isolated from that shared history.
		// Ability-system Clear() resets the shared history sequence. Do not let a
		// stale Echo cursor block the first ability recorded after that reset.
		if (history.GetRecords().empty())
		{
			mLastEchoedSequence = 0;
		}
		const AbilityUseRecord* record = history.FindLatestUnconsumed(
			[&](const AbilityUseRecord& candidate)
			{
				return candidate.sequence > mLastEchoedSequence;
			}
		);
		if (!record || record->abilityId == AbilityData::EchoProtocol::AbilityId::Basic)
		{
			return false;
		}

		const float echoPower = std::max(
			0.f,
			ResolveValue(context, AbilityData::EchoProtocol::Attribute::PowerBase, 0.60f) +
			static_cast<float>(std::max(0, context.instance.GetLevel() - 1)) *
			ResolveValue(context, AbilityData::EchoProtocol::Attribute::PowerPerLevel, 0.03f)
		);
		List<sas::AttributeScalingRule> scalingRules;
		for (const AbilityScalingChannel& channel : record->scalingChannels)
		{
			// A single Echo coefficient is safe when a source attribute drives one
			// target. If it drives multiple targets, the source coefficients are
			// target-specific balance data and must be preserved. Replacing both
			// Gravity Radius (0.20) and Duration (0.0025) with one MaxHealth scale
			// would make the field last orders of magnitude too long.
			const std::size_t sourceChannelCount = std::count_if(
				record->scalingChannels.begin(),
				record->scalingChannels.end(),
				[&](const AbilityScalingChannel& other)
				{
					return other.sourceAttributeId == channel.sourceAttributeId;
				}
			);
			const bool sourceAttributeIsShared = sourceChannelCount > 1;
			const float echoCoefficient = ResolveScalingCoefficient(
				context,
				channel.sourceAttributeId
			);
			// Echo only declares replacement coefficients for its supported owner
			// channels. Unsupported channels (for example Armor in Shield) must
			// retain their source tuning instead of disappearing from the replay.
			const float coefficient =
				(sourceAttributeIsShared || echoCoefficient <= 0.f) &&
				channel.sourceCoefficient > 0.f
				? channel.sourceCoefficient
				: echoCoefficient;
			if (coefficient <= 0.f)
			{
				continue;
			}
			scalingRules.push_back(sas::AttributeScalingRule{
				channel.targetAttributeId,
				channel.sourceAttributeId,
				channel.operation,
				coefficient
			});
		}

		std::string failureReason;
		if (!context.abilitySystem.InvokeRecordedAbility(
			*record,
			scalingRules,
			echoPower,
			context.definition.slot,
			context.instance.IsInputHeld(),
			&failureReason
		))
		{
			return false;
		}
		const std::uint64_t echoedSequence = record->sequence;
		if (!history.Consume(echoedSequence))
		{
			return false;
		}
		mLastEchoedSequence = echoedSequence;
		return true;
	}
}

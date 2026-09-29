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
			AbilityData::EchoProtocol::Attribute::EnergyPowerScale,
			AbilityData::EchoProtocol::Attribute::EnergyPowerScalePerLevel
		})
		{
			const sas::GameplayAttribute* attribute = FindAttribute(definition, attributeId);
			if (!attribute || !std::isfinite(attribute->baseValue) || attribute->baseValue < 0.f)
			{
				if (failureReason)
				{
					*failureReason = "Echo Protocol must declare four non-negative runtime attributes.";
				}
				return false;
			}
		}
		if ((definition.levelProgression.empty() && definition.repeatingLevelProgression.empty()))
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

		const int levelDelta = std::max(0, context.instance.GetLevel() - 1);
		const float energyPower = std::max(0.f, context.abilitySystem.GetAttributes().GetCurrentValue(
			OwnerAttributeIds::EnergyPower
		));
		const float replayPower = std::max(
			0.f,
			ResolveValue(context, AbilityData::EchoProtocol::Attribute::PowerBase, 0.60f) +
			static_cast<float>(levelDelta) * ResolveValue(
				context, AbilityData::EchoProtocol::Attribute::PowerPerLevel, 0.02f
			) +
			(energyPower / 100.f) * (
				ResolveValue(context, AbilityData::EchoProtocol::Attribute::EnergyPowerScale, 0.12f) +
				static_cast<float>(levelDelta) * ResolveValue(
					context,
					AbilityData::EchoProtocol::Attribute::EnergyPowerScalePerLevel,
					0.02f
				)
			)
		);
		List<sas::AttributeScalingRule> scalingRules;
		for (const AbilityScalingChannel& channel : record->scalingChannels)
		{
			scalingRules.push_back(sas::AttributeScalingRule{
				channel.targetAttributeId,
				channel.sourceAttributeId,
				channel.operation,
				channel.sourceCoefficient
			});
		}

		std::string failureReason;
		if (!context.abilitySystem.InvokeRecordedAbility(
			*record,
			scalingRules,
			replayPower,
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

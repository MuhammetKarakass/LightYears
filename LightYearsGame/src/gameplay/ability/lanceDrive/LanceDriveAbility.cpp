#include "gameplay/ability/lanceDrive/LanceDriveAbility.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/ironcladProtocol/IroncladProtocolContracts.h"
#include "gameplay/ability/lanceDrive/LanceDriveActor.h"
#include "gameplay/ability/lanceDrive/LanceDriveContracts.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/movement/MovementPolicyService.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/lanceDrive/LanceDrivePresentationIds.h"
#include "presentation/ability/lanceDrive/LanceDrivePresentationProfile.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& id,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}
	}

	sas::GameplayAttributeList LanceDriveAbility::ResolveValues(
		GameAbilityBehaviorContext& context
	) const
	{
		return AbilityActionAttributeResolver::ResolveAbilityAttributes(
			AbilityExecutionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			}
		);
	}

	bool LanceDriveAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::LanceDrive::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::LanceDrive;
		const bool validLifecycle =
			definition.abilityId == AbilityData::LanceDrive::AbilityId::Basic &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.cooldownStartPolicy == sas::AbilityCooldownStartPolicy::OnActivation &&
			definition.duration > 0.f && definition.cooldown > 0.f &&
			definition.maxCharges == 1;
		const bool validTags =
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::LanceDrive::CategoryTag);
				}) &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::LanceDrive::FamilyTag);
				});

		const List<sas::AttributeId> required{
			AbilityData::LanceDrive::Attribute::BaseDamage,
			AbilityData::LanceDrive::Attribute::SpeedDamageConversion,
			AbilityData::LanceDrive::Attribute::EnergyPowerReference,
			AbilityData::LanceDrive::Attribute::EnergyPowerConversionPerPoint,
			AbilityData::LanceDrive::Attribute::TopSpeedBonus,
			AbilityData::LanceDrive::Attribute::ThrustBonus,
			AbilityData::LanceDrive::Attribute::TurnCapabilityMultiplier,
			AbilityData::LanceDrive::Attribute::SameTargetHitCooldown,
			AbilityData::LanceDrive::Attribute::Length,
			AbilityData::LanceDrive::Attribute::EdgeThickness,
			AbilityData::LanceDrive::Attribute::OpeningAngleDegrees,
			AbilityData::LanceDrive::Attribute::LateralKnockback
		};
		const bool hasAttributes = std::all_of(
			required.begin(),
			required.end(),
			[&definition](const sas::AttributeId& id)
			{
				const sas::GameplayAttribute* attribute =
					sas::FindAttribute(definition.attributes, id);
				return attribute && std::isfinite(attribute->baseValue);
			}
		);
		if (!validLifecycle || !validIdentity || !validTags || !hasAttributes ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Kinetic)
		{
			if (failureReason)
			{
				*failureReason =
					"Lance Drive requires a timed OnPressed lifecycle, offense/family tags, all runtime attributes, and Kinetic damage.";
			}
			return false;
		}

		const auto value = [&definition](const sas::AttributeId& id)
		{
			return sas::FindAttributeValue(definition.attributes, id, 0.f);
		};
		if (value(AbilityData::LanceDrive::Attribute::BaseDamage) < 0.f ||
			value(AbilityData::LanceDrive::Attribute::SpeedDamageConversion) < 0.f ||
			value(AbilityData::LanceDrive::Attribute::EnergyPowerReference) < 0.f ||
			value(AbilityData::LanceDrive::Attribute::EnergyPowerConversionPerPoint) < 0.f ||
			value(AbilityData::LanceDrive::Attribute::TopSpeedBonus) < 0.f ||
			value(AbilityData::LanceDrive::Attribute::ThrustBonus) < 0.f ||
			value(AbilityData::LanceDrive::Attribute::TurnCapabilityMultiplier) <= 0.f ||
			value(AbilityData::LanceDrive::Attribute::SameTargetHitCooldown) <= 0.f ||
			value(AbilityData::LanceDrive::Attribute::Length) <= 0.f ||
			value(AbilityData::LanceDrive::Attribute::EdgeThickness) <= 0.f ||
			value(AbilityData::LanceDrive::Attribute::OpeningAngleDegrees) <= 0.f ||
			value(AbilityData::LanceDrive::Attribute::OpeningAngleDegrees) >= 179.f ||
			value(AbilityData::LanceDrive::Attribute::LateralKnockback) < 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Lance Drive contains invalid geometry, damage, or movement values.";
			}
			return false;
		}
		return true;
	}

	bool LanceDriveAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mActive || !dynamic_cast<SpaceShip*>(&context.owner) ||
			context.abilitySystem.HasOwnedTag(
				AbilityData::IroncladProtocol::State::Active
			))
		{
			return false;
		}

		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		World* world = context.owner.GetWorld();
		const LanceDrivePresentationProfile* profile =
			PresentationProfileRegistry<LanceDrivePresentationProfile>::Find(
				LanceDrivePresentationIds::LanceBasic.ToString()
			);
		if (!ship || !world || !profile)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		movement::MovementPolicyRequest policy;
		policy.sourceId = context.definition.abilityId;
		// Lance Drive is a constant-speed charge. Normal thrust-drift damping
		// must not bleed speed while the ability is active.
		policy.dampingRetentionOverride = 1.f;
		policy.speedCapFlatBonus = std::max(0.f, FindValue(
			values, AbilityData::LanceDrive::Attribute::TopSpeedBonus, 300.f
		));
		if (!movement::MovementPolicyService::SetPolicy(context.owner, policy))
		{
			return false;
		}
		ShipRuntimeModifier modifier;
		modifier.thrustBonus = std::max(0.f, FindValue(
			values, AbilityData::LanceDrive::Attribute::ThrustBonus, 0.25f
		));
		modifier.turnCapabilityMultiplier = std::max(0.f, FindValue(
			values,
			AbilityData::LanceDrive::Attribute::TurnCapabilityMultiplier,
			0.25f
		));
		ship->GetRuntimeModifiers().Set(context.definition.abilityId, modifier);

		const weak_ptr<LanceDriveActor> lanceHandle = world->SpawnActor<LanceDriveActor>(
			&context.owner,
			*profile
		);
		const shared_ptr<LanceDriveActor> lance = lanceHandle.lock();
		if (!lance)
		{
			ship->GetRuntimeModifiers().Remove(context.definition.abilityId);
			movement::MovementPolicyService::ReleasePolicy(
				context.owner,
				context.definition.abilityId,
				movement::MovementPolicyReleaseMode::Immediate,
				0.f
			);
			return false;
		}
		lance->ConfigureFromAttributes(values);
		lance->SetSourceAbility(
			sas::ContentId{ context.definition.abilityId },
			context.definition.abilityTags
		);
		lance->SetSourceAbilityInstance(&context.instance);
		lance->SetLifeTime(context.definition.duration);
		mLance = lance;

		const std::string ownAbilityId = context.definition.abilityId;
		mActivationGuard = context.abilitySystem.RegisterAbilityActivationGuard(
			[ownAbilityId](const sas::AbilityLifecycleEvent& event)
			{
				return event.abilityId == ownAbilityId;
			}
		);
		mContactGuard = ship->GetCombatRuntime().GetContactDamageGuardRegistry().Register(
			[this](const Actor& source, const Actor& target)
			{
				const shared_ptr<LanceDriveActor> lance = mLance.lock();
				return !lance || !lance->ConsumesFrontalContact(source, target);
			}
		);
		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::LanceDrive::State::Active);
		context.abilitySystem.AddOwnedTag(GameplayTags::State::ActionLock::AbilityActivation);
		context.abilitySystem.AddOwnedTag(GameplayTags::State::ActionLock::PrimaryWeaponFire);
		if (GameAbility* primary = context.abilitySystem.GetAbility(sas::AbilitySlot::PrimaryFire);
			primary && primary->IsActive())
		{
			primary->Cancel(sas::AbilityEndReason::Interrupted);
		}
		EmitEvent(context, AbilityData::LanceDrive::Event::Started);
		return true;
	}

	void LanceDriveAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		ClearRuntimeState(context);
		EmitEvent(context, AbilityData::LanceDrive::Event::Ended);
	}

	void LanceDriveAbility::ClearRuntimeState(GameAbilityBehaviorContext& context)
	{
		if (!mActive && !mLance.lock())
		{
			return;
		}
		if (const shared_ptr<LanceDriveActor> lance = mLance.lock())
		{
			lance->Destroy();
		}
		mLance.reset();
		if (mContactGuard.IsValid())
		{
			if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
			{
				ship->GetCombatRuntime().GetContactDamageGuardRegistry().Unregister(mContactGuard);
			}
			mContactGuard = {};
		}
		if (mActivationGuard.IsValid())
		{
			context.abilitySystem.UnregisterAbilityActivationGuard(mActivationGuard);
			mActivationGuard = {};
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetRuntimeModifiers().Remove(context.definition.abilityId);
		}
		movement::MovementPolicyService::ReleasePolicy(
			context.owner,
			context.definition.abilityId,
			movement::MovementPolicyReleaseMode::Immediate,
			0.f
		);
		context.abilitySystem.RemoveOwnedTag(AbilityData::LanceDrive::State::Active);
		context.abilitySystem.RemoveOwnedTag(GameplayTags::State::ActionLock::AbilityActivation);
		context.abilitySystem.RemoveOwnedTag(GameplayTags::State::ActionLock::PrimaryWeaponFire);
		mActive = false;
	}

	void LanceDriveAbility::EmitEvent(
		GameAbilityBehaviorContext& context,
		const GameplayTag& eventTag
	) const
	{
		sas::AbilityEvent event;
		event.eventTag = eventTag;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(&context.owner);
		context.abilitySystem.HandleGameplayEvent(event);
	}
}

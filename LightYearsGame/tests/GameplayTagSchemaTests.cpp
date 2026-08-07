#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/ability/dash/DashContracts.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyContracts.h"
#include "gameplay/ability/infernoSpray/InfernoSprayContracts.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/ability/rocket/RocketContracts.h"
#include "gameplay/ability/shield/ShieldContracts.h"
#include "gameplay/ability/sunBeam/SunBeamContracts.h"

#include <iostream>

namespace
{
	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}

	bool IsValid(
		const ly::GameplayTag& tag,
		ly::GameplayTagKind kind
	)
	{
		return ly::GameplayTagSchema::Validate(tag, kind);
	}
}

int main()
{
	using namespace ly;

	std::string failureReason;
	if (!GameplayTagSchema::Validate(
		GameplayTag{ "Ability.Offense.OverdriveCore" },
		GameplayTagKind::Ability,
		&failureReason
	))
	{
		return Fail("Feature-local ability leaf was rejected by the shared tag schema");
	}
	if (GameplayTagSchema::Validate(
		GameplayTag{ "Ability.Offense-OverdriveCore" },
		GameplayTagKind::Ability,
		&failureReason
	))
	{
		return Fail("Malformed gameplay tag was accepted by the shared tag schema");
	}
	if (GameplayTagSchema::Validate(
		GameplayTag{ "Damage.Type.Kinetic" },
		GameplayTagKind::Ability,
		&failureReason
	))
	{
		return Fail("Gameplay tag was accepted for the wrong semantic domain");
	}
	if (!GameplayTagSchema::Validate(
		GameplayTagSchema::BlockAbilityActivation,
		GameplayTagKind::ActionLock,
		&failureReason
	) || !GameplayTagSchema::Validate(
		GameplayTagSchema::BlockPrimaryWeaponFire,
		GameplayTagKind::ActionLock,
		&failureReason
	))
	{
		return Fail("Registered shared action locks were rejected");
	}
	if (GameplayTagSchema::Validate(
		GameplayTag{ "State.ActionLock.Custom" },
		GameplayTagKind::Any,
		&failureReason
	))
	{
		return Fail("Unregistered shared action lock was accepted");
	}
	if (!GameplayTagSchema::Validate(
		GameplayTag{ "PrimaryWeapon.Projectile.Standard" },
		GameplayTagKind::PrimaryWeaponType,
		&failureReason
	) || !GameplayTagSchema::Validate(
		GameplayTag{ "PrimaryWeapon.Feature.Heat" },
		GameplayTagKind::PrimaryWeaponFeature,
		&failureReason
	))
	{
		return Fail("Primary weapon tag domains were rejected");
	}
	if (!content::ContentIdSchema::ValidateAttachmentId(
		"Attachment.Thermal.Converter.Basic",
		&failureReason
	) || !GameplayTagSchema::Validate(
		GameplayTag{ "Attachment.Capability.Damage" },
		GameplayTagKind::AttachmentCapability,
		&failureReason
	))
	{
		return Fail("Attachment tag domains were rejected");
	}
	if (!GameplayTagSchema::Validate(
		GameplayTagSchema::AbilityOffense,
		GameplayTagKind::AbilityCategory,
		&failureReason
	) || !GameplayTagSchema::Validate(
		GameplayTag{ "Status.Damage.Ignite" },
		GameplayTagKind::Status,
		&failureReason
	) || !GameplayTagSchema::Validate(
		GameplayTag{ "State.Effect.Movement.Slow" },
		GameplayTagKind::EffectState,
		&failureReason
	) || !GameplayTagSchema::Validate(
		CombatEventSchema::OwnerDamageTaken,
		GameplayTagKind::Event,
		&failureReason
	))
	{
		return Fail("Shared category, status, effect, or combat-event tag was rejected");
	}
	if (!content::ContentIdSchema::ValidateAbilityId(
			"Ability.Offense.SunBeam.Strike.Basic",
			&failureReason
		) || !content::ContentIdSchema::ValidateEffectId(
			"Effect.Status.Damage.Cryo.Buildup",
			&failureReason
		) || !content::ContentIdSchema::ValidateAbilityActorDefinitionId(
			"Actor.Ability.SunBeam.Strike.Basic",
			&failureReason
		) || !content::ContentIdSchema::ValidateAbilityAttributeProfileId(
			"AttributeProfile.SunBeam.Strike.Basic",
			&failureReason
		) || !content::ContentIdSchema::ValidateAbilityPresentationProfileId(
			"Presentation.Ability.SunBeam.Strike.Basic",
			&failureReason
		) || !content::ContentIdSchema::ValidateWeaponId(
			"Weapon.Projectile.RapidShotgun.Basic",
			&failureReason
		) || !content::ContentIdSchema::ValidateShipId(
			"Ship.Player.Fighter.Basic",
			&failureReason
		) || !content::ContentIdSchema::ValidateAttachmentId(
			"Attachment.Thermal.Converter.Basic",
			&failureReason
		) || !content::ContentIdSchema::ValidateGameplayEffectVisualId(
			"Visual.Effect.Shield.Basic",
			&failureReason
		))
	{
		return Fail("Valid content ID formats were rejected");
	}
	if (content::ContentIdSchema::ValidateAbilityId(
			"Ability.Offense.SunBeam",
			&failureReason
		) || content::ContentIdSchema::ValidateEffectId(
			"Effect.Barrier",
			&failureReason
		) || content::ContentIdSchema::ValidateAbilityActorDefinitionId(
			"Actor.SunBeam.Strike.Basic",
			&failureReason
		) || content::ContentIdSchema::ValidateAbilityPresentationProfileId(
			"Presentation.Rocket.Basic",
			&failureReason
		) || content::ContentIdSchema::ValidateWeaponId(
			"Weapon.RapidShotgun",
			&failureReason
		) || content::ContentIdSchema::ValidateShipId(
			"Ship.Fighter",
			&failureReason
		) || content::ContentIdSchema::ValidateAttachmentId(
			"Attachment.Thermal.Converter",
			&failureReason
		) || content::ContentIdSchema::ValidateGameplayEffectVisualId(
			"Visual.Shield.Basic",
			&failureReason
		))
	{
		return Fail("Malformed content IDs were accepted");
	}
	if (GameplayTagSchema::ValidateEffectGrantedTag(
			GameplayTag{ "Ability.Offense.Rocket" },
			&failureReason
		) || !GameplayTagSchema::ValidateEffectGrantedTag(
			GameplayTag{ "State.Effect.Movement.Slow" },
			&failureReason
		) || GameplayTagSchema::ValidateEffectGrantedTag(
			GameplayTag{ "Effect.Movement.Slow" },
			&failureReason
		) || !GameplayTagSchema::ValidateEffectGrantedTag(
			GameplayTag{ "Status.Damage.Ignite" },
			&failureReason
		) || GameplayTagSchema::ValidateEffectApplicationTag(
			GameplayTag{ "Attribute.Owner.Health" },
			&failureReason
		))
	{
		return Fail("Effect tag domain validation is inconsistent");
	}
	if (!GameplayTagSchema::ValidateAbilityOwnerConditionTag(
			GameplayTag{ "State.ActionLock.AbilityActivation" },
			&failureReason
		) || GameplayTagSchema::ValidateAbilityOwnerConditionTag(
			GameplayTag{ "Event.Ability.Rocket.Start" },
			&failureReason
		))
	{
		return Fail("Ability owner-condition tag domain validation is inconsistent");
	}

	// Every ability family exposes the same mandatory contract surface. Optional
	// state/event/actor members are validated below only where the behavior owns
	// those mechanics; this keeps future families extensible without fake leaves.
	if (!IsValid(GameplayTag{ AbilityData::Dash::AbilityId::Basic }, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::Dash::CategoryTag, GameplayTagKind::AbilityCategory) ||
		!IsValid(AbilityData::Dash::FamilyTag, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::Dash::BehaviorTag, GameplayTagKind::AbilityBehavior) ||
		!IsValid(AbilityData::Dash::State::Active, GameplayTagKind::AbilityState) ||
		!IsValid(AbilityData::Dash::Event::Started, GameplayTagKind::AbilityEvent) ||
		!IsValid(AbilityData::Dash::Event::Ended, GameplayTagKind::AbilityEvent) ||
		!IsValid(GameplayTag{ AbilityData::Shield::AbilityId::Basic }, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::Shield::CategoryTag, GameplayTagKind::AbilityCategory) ||
		!IsValid(AbilityData::Shield::FamilyTag, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::Shield::BehaviorTag, GameplayTagKind::AbilityBehavior) ||
		!IsValid(GameplayTag{ AbilityData::Rocket::AbilityId::Basic }, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::Rocket::CategoryTag, GameplayTagKind::AbilityCategory) ||
		!IsValid(AbilityData::Rocket::FamilyTag, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::Rocket::BehaviorTag, GameplayTagKind::AbilityBehavior) ||
		!IsValid(AbilityData::Rocket::Actor::Projectile::TypeTag, GameplayTagKind::AbilityActorType) ||
		!IsValid(AbilityData::Rocket::Actor::Projectile::ProjectileSpeed, GameplayTagKind::Attribute) ||
		!IsValid(GameplayTag{ AbilityData::InfernoSpray::AbilityId::Basic }, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::InfernoSpray::CategoryTag, GameplayTagKind::AbilityCategory) ||
		!IsValid(AbilityData::InfernoSpray::FamilyTag, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::InfernoSpray::BehaviorTag, GameplayTagKind::AbilityBehavior) ||
		!IsValid(AbilityData::InfernoSpray::State::Active, GameplayTagKind::AbilityState) ||
		!IsValid(AbilityData::InfernoSpray::Event::Started, GameplayTagKind::AbilityEvent) ||
		!IsValid(AbilityData::InfernoSpray::Actor::FlameCone::TypeTag, GameplayTagKind::AbilityActorType) ||
		!IsValid(AbilityData::InfernoSpray::Actor::FlameCone::Range, GameplayTagKind::Attribute) ||
		!IsValid(GameplayTag{ AbilityData::SunBeam::AbilityId::Strike::Basic }, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::SunBeam::CategoryTag, GameplayTagKind::AbilityCategory) ||
		!IsValid(AbilityData::SunBeam::FamilyTag, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::SunBeam::BehaviorTag, GameplayTagKind::AbilityBehavior) ||
		!IsValid(AbilityData::SunBeam::Actor::Strike::TypeTag, GameplayTagKind::AbilityActorType) ||
		!IsValid(AbilityData::SunBeam::Actor::Shared::Width, GameplayTagKind::Attribute) ||
		!IsValid(GameplayTag{ AbilityData::GravityAnomaly::AbilityId::Basic }, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::GravityAnomaly::CategoryTag, GameplayTagKind::AbilityCategory) ||
		!IsValid(AbilityData::GravityAnomaly::FamilyTag, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::GravityAnomaly::BehaviorTag, GameplayTagKind::AbilityBehavior) ||
		!IsValid(AbilityData::GravityAnomaly::Actor::Projectile::TypeTag, GameplayTagKind::AbilityActorType) ||
		!IsValid(AbilityData::GravityAnomaly::Actor::Field::TypeTag, GameplayTagKind::AbilityActorType) ||
		!IsValid(AbilityData::GravityAnomaly::Actor::Field::PullStrength, GameplayTagKind::Attribute) ||
		!IsValid(AbilityData::GravityAnomaly::Effect::BehaviorTag, GameplayTagKind::EffectBehavior) ||
		!IsValid(GameplayTag{ AbilityData::OverdriveCore::AbilityId::Basic }, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::OverdriveCore::CategoryTag, GameplayTagKind::AbilityCategory) ||
		!IsValid(AbilityData::OverdriveCore::FamilyTag, GameplayTagKind::Ability) ||
		!IsValid(AbilityData::OverdriveCore::BehaviorTag, GameplayTagKind::AbilityBehavior) ||
		!IsValid(AbilityData::OverdriveCore::State::Firing, GameplayTagKind::AbilityState) ||
		!IsValid(AbilityData::OverdriveCore::State::AttackSpeedBoost, GameplayTagKind::AbilityState) ||
		!IsValid(AbilityData::OverdriveCore::Event::Started, GameplayTagKind::AbilityEvent) ||
		!IsValid(AbilityData::OverdriveCore::Event::Ended, GameplayTagKind::AbilityEvent) ||
		!IsValid(AbilityData::OverdriveCore::Event::AttackSpeedBoostEnded, GameplayTagKind::AbilityEvent))
	{
		return Fail("Ability contract tags violate the shared gameplay tag schema");
	}

	return 0;
}

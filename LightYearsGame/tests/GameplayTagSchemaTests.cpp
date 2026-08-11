#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/damage/DamageContext.h"

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

	return 0;
}

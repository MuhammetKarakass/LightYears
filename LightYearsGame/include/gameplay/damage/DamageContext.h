#pragma once

#include "framework/Core.h"
#include "content/ContentId.h"
#include "gameplay/tags/GameplayTags.h"

namespace ly
{
	class Actor;

	// CombatRuntime emits this shared event after damage has been resolved. It
	// belongs to the combat pipeline rather than to an ability-family contract.
	struct CombatEventSchema
	{
		inline static const GameplayTag& OwnerDamageTaken = GameplayTags::Event::Owner::DamageTaken;
	};

	// Numeric damage-type behavior is carried with each hit. Values are resolved
	// once by DamageTypeSystem, so the combat pipeline does not need to know the
	// weapon or ability that produced the hit.
	struct DamagePayload
	{
		float shieldDamageMultiplier = 1.f;
		float shieldRegenerationDelay = 0.f;
		float armorPenetration = 0.f;
		int igniteStacks = 0;
		float burnDamagePerSecond = 0.f;
		// Optional fixed-period Burn mode. Legacy sources continue to use
		// burnDamagePerSecond; Scorch Drive supplies these snapshot values.
		float burnDamagePerTick = 0.f;
		float burnTickInterval = 0.f;
		float burnDuration = 0.f;
		int burnMaxStacks = 1;
		int cryoBuildupPerHit = 0;
		int cryoBuildupRequired = 1;
		float cryoBuildupDuration = 0.f;
		float cryoSlowPercent = 0.f;
		float cryoSlowDuration = 0.f;
		int electricStacks = 0;
		float electricDamageTakenMultiplierPerStack = 0.f;
		float electricDuration = 0.f;
		int electricMaxStacks = 1;
		bool canCrit = true;
		float criticalDamageMultiplier = 2.f;
	};

	// Delivery kind is shared metadata for incoming-damage defenses. It keeps
	// projectile-only protection out of weapon/ability-specific conditionals and
	// leaves beams, fields, contact, and direct damage distinguishable.
	enum class DamageDeliveryType
	{
		Direct,
		Projectile,
		Beam,
		Area,
		Contact
	};

	struct DamageContext
	{
		Actor* source = nullptr;
		Actor* target = nullptr;
		DamageDeliveryType deliveryType = DamageDeliveryType::Direct;
		Actor* deliveryActor = nullptr;
		sas::ContentId sourceAbilityId;
		List<GameplayTag> sourceAbilityTags;
		float originalDamage = 0.f;
		float remainingDamage = 0.f;
		float absorbedDamage = 0.f;
		float mitigatedDamage = 0.f;
		float modifiedDamage = 0.f;
		float appliedDamage = 0.f;
		bool wasCritical = false;
		// This is true only after health/shield resolution confirms that the hit
		// actually reduced the target's health to zero.
		bool targetWasKilled = false;
		// Snapshot taken after this hit's status application but before health
		// death callbacks can clear the target's CombatRuntime. This lets global
		// kill mechanics evaluate Cryo at the actual kill boundary.
		bool targetWasCryoAffected = false;
		// Immutable target snapshots keep kill reactions safe when the target's
		// death callback destroys the actor before the source receives KillConfirmed.
		bool targetWasEnemyCombatant = false;
		struct TargetLocationSnapshot
		{
			float x = 0.f;
			float y = 0.f;
		};
		TargetLocationSnapshot targetLocationAtResolution{};
		List<GameplayTag> damageTags;
		DamagePayload payload;
	};
}



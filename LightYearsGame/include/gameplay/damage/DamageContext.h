#pragma once

#include "framework/Core.h"
#include "content/ContentId.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/damage/DamageHitGroup.h"
#include "effects/GameplayEffectSpec.h"
#include "gameplay/module/ModuleFirstHitHistory.h"

namespace vega
{
	class Actor;

	// CombatRuntime emits this shared event after damage has been resolved. It
	// belongs to the combat pipeline rather than to an ability-family contract.
	struct CombatEventSchema
	{
		inline static const GameplayTag& OwnerDamageTaken = GameplayTags::Event::Owner::DamageTaken;
	};

	enum class DamageCriticalPolicy
	{
		Random,
		Guaranteed,
		Disabled
	};
	enum class DamageModuleTriggerPolicy { Normal, Suppressed };
	enum class DamageSourceScalingPolicy { Normal, AlreadyResolved };

	// Numeric damage-type behavior is carried with each hit. Values are resolved
	// once by DamageTypeSystem, so the combat pipeline does not need to know the
	// weapon or ability that produced the hit.
	struct DamagePayload
	{
		float shieldDamageMultiplier = 1.f;
		float shieldRegenerationDelay = 0.f;
		float armorPenetration = 0.f;
		int igniteStacks = 0;
		int cryoBuildupPerHit = 0;
		int electricStacks = 0;
		int kineticStacks = 1;
		DamageCriticalPolicy criticalPolicy = DamageCriticalPolicy::Random;
		bool roundDamageUp = false;
		// Prevents an outgoing module bonus from recursively creating more bonus hits.
		bool skipModuleDamageRules = false;
		DamageModuleTriggerPolicy moduleTriggerPolicy = DamageModuleTriggerPolicy::Normal;
		DamageSourceScalingPolicy sourceScalingPolicy = DamageSourceScalingPolicy::Normal;
		std::shared_ptr<DamageHitGroup> hitGroup;
	};

	// Delivery kind is shared metadata for incoming-damage defenses. It keeps
	// projectile-only protection out of weapon/ability-specific conditionals and
	// leaves beams, fields, contact, direct damage and periodic ticks distinguishable.
	enum class DamageDeliveryType
	{
		Direct,
		Projectile,
		Beam,
		Area,
		Contact,
		Periodic
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
		// Shield resource lost by this hit, excluding barrier/other absorption.
		float shieldDamage = 0.f;
		// Damage arriving at an active shield before shield Armor sharing (Impact Redirector).
		float incomingShieldDamage = 0.f;
		float mitigatedDamage = 0.f;
		float modifiedDamage = 0.f;
		float appliedDamage = 0.f;
		bool wasCritical = false;
		bool deferredDamage = false;
		// This is true only after health/shield resolution confirms that the hit
		// actually reduced the target's health to zero.
		bool targetWasKilled = false;
		// Snapshot taken after this hit's status application but before health
		// death callbacks can clear the target's CombatRuntime. This lets global
		// kill mechanics evaluate Cryo at the actual kill boundary.
		bool targetWasCryoAffected = false;
		int targetCryoStacksAtResolution = 0;
		// Immutable target snapshots keep kill reactions safe when the target's
		// death callback destroys the actor before the source receives KillConfirmed.
		bool targetWasEnemyCombatant = false;
		struct TargetLocationSnapshot
		{
			float x = 0.f;
			float y = 0.f;
		};
		TargetLocationSnapshot targetLocationAtResolution{};
		float targetMaxHealthAtResolution = 0.f;
		List<GameplayTag> targetStatusesAtResolution;
		List<sas::GameplayEffectSpec> targetStatusEffectsAtResolution;
		// Armor fraction removed by the target's Kinetic stacks, captured before
		// this hit applies its own statuses, so a new stack shreds later hits only.
		float targetArmorShred = 0.f;
		List<GameplayTag> damageTags;
		DamagePayload payload;
		std::weak_ptr<ModuleFirstHitHistory::Claim> moduleFirstHitClaim;
	};
}



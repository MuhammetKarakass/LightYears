#pragma once

#include "framework/Core.h"
#include "abilities/AbilityCooldownTags.h"

#include "gameplay/tags/ability/DashTags.h"
#include "gameplay/tags/ability/GravityAnomalyTags.h"
#include "gameplay/tags/ability/HullShockTags.h"
#include "gameplay/tags/ability/InfernoSprayTags.h"
#include "gameplay/tags/ability/OverdriveCoreTags.h"
#include "gameplay/tags/ability/NullPulseTags.h"
#include "gameplay/tags/ability/OrbitalDronesTags.h"
#include "gameplay/tags/ability/ExecutionDriveTags.h"
#include "gameplay/tags/ability/PhaseDriftTags.h"
#include "gameplay/tags/ability/RelayPrismTags.h"
#include "gameplay/tags/ability/EchoProtocolTags.h"
#include "gameplay/tags/ability/RocketTags.h"
#include "gameplay/tags/ability/RailBurstTags.h"
#include "gameplay/tags/ability/MineLayerTags.h"
#include "gameplay/tags/ability/EnergySpearTags.h"
#include "gameplay/tags/ability/ShieldTags.h"
#include "gameplay/tags/ability/ShieldHarvestTags.h"
#include "gameplay/tags/ability/SunBeamTags.h"
#include "gameplay/tags/ability/ScorchDriveTags.h"
#include "gameplay/tags/ability/CrescentReaverTags.h"
#include "gameplay/tags/ability/DirectionalBarrierTags.h"
#include "gameplay/tags/ability/IonStormTags.h"
#include "gameplay/tags/ability/ChainLightningTags.h"
#include "gameplay/tags/ability/CryostasisTags.h"
#include "gameplay/tags/ability/GlacialPressureTags.h"
#include "gameplay/tags/ability/VoidGateTags.h"
#include "gameplay/tags/ability/FrostMaelstromTags.h"
#include "gameplay/tags/ability/FrozenThrongTags.h"
#include "gameplay/tags/ability/CombatSentryTags.h"
#include "gameplay/tags/ability/NanoPlagueTags.h"
#include "gameplay/tags/ability/AstralSurgeTags.h"
#include "gameplay/tags/ability/WingSentinelsTags.h"
#include "gameplay/tags/ability/ReturnProtocolTags.h"
#include "gameplay/tags/ability/CrystalBarricadeTags.h"

namespace ly::GameplayTags
{
	namespace Ability
	{
		inline const GameplayTag Root{ "Ability" };
		inline const GameplayTag Primary{ "Ability.Primary" };
		inline const GameplayTag Offense{ "Ability.Offense" };
		inline const GameplayTag Defense{ "Ability.Defense" };
		inline const GameplayTag Movement{ "Ability.Movement" };
		inline const GameplayTag Control{ "Ability.Control" };
		inline const GameplayTag Utility{ "Ability.Utility" };

	}

	namespace State
	{
		namespace Ability
		{
			inline const GameplayTag Root{ "State.Ability" };
		}

		namespace Effect
		{
			inline const GameplayTag Root{ "State.Effect" };
			namespace Defense
			{
				inline const GameplayTag Barrier{ "State.Effect.Defense.Barrier" };
			}
			namespace Movement
			{
				inline const GameplayTag Boost{ "State.Effect.Movement.Boost" };
				inline const GameplayTag Slow{ "State.Effect.Movement.Slow" };
			}
			namespace Test
			{
				inline const GameplayTag PassiveValidation{ "State.Effect.Test.PassiveValidation" };
			}
			namespace Immunity
			{
				namespace Movement
				{
					inline const GameplayTag Slow{
						"State.Effect.Immunity.Movement.Slow"
					};
				}
			}
		}

			namespace ActionLock
			{
			inline const GameplayTag Root{ "State.ActionLock" };
			inline const GameplayTag AbilityActivation{
				"State.ActionLock.AbilityActivation"
			};
			inline const GameplayTag PrimaryWeaponFire{
				"State.ActionLock.PrimaryWeaponFire"
			};
				inline const GameplayTag MovementInput{
					"State.ActionLock.MovementInput"
				};
				// A hard stasis may reject external impulses as well as player input.
				inline const GameplayTag ExternalMovement{
					"State.ActionLock.ExternalMovement"
				};
			}
	}

	namespace Event
	{
		inline const GameplayTag Root{ "Event" };
		namespace Ability
		{
			inline const GameplayTag Root{ "Event.Ability" };
			inline const GameplayTag Activated{ "Event.Ability.Activated" };
			inline const GameplayTag Ended{ "Event.Ability.Ended" };
		}

		namespace Owner
		{
			inline const GameplayTag Root{ "Event.Owner" };
			inline const GameplayTag DamageTaken{ "Event.Owner.DamageTaken" };
			inline const GameplayTag BarrierBroken{ "Event.Owner.BarrierBroken" };
		}

		namespace Source
		{
			inline const GameplayTag Root{ "Event.Source" };
			inline const GameplayTag DamageDealt{ "Event.Source.DamageDealt" };
			namespace StatusApplied
			{
				inline const GameplayTag Ignite{
					"Event.Source.StatusApplied.Ignite"
				};
			}
		}

			namespace Combat
		{
			inline const GameplayTag Root{ "Event.Combat" };
			inline const GameplayTag DamageReceived{ "Event.Combat.DamageReceived" };
			inline const GameplayTag DamageDealt{ "Event.Combat.DamageDealt" };
			inline const GameplayTag KillConfirmed{ "Event.Combat.KillConfirmed" };
		}
	}

	namespace Status
	{
		inline const GameplayTag Root{ "Status" };
		namespace Damage
		{
			inline const GameplayTag Ignite{ "Status.Damage.Ignite" };
			namespace Cryo
			{
				inline const GameplayTag Buildup{ "Status.Damage.Cryo.Buildup" };
				inline const GameplayTag Slowed{ "Status.Damage.Cryo.Slowed" };
			}
			inline const GameplayTag Electric{ "Status.Damage.Electric" };
		}
	}

	namespace Damage
	{
		namespace Type
		{
			inline const GameplayTag Root{ "Damage.Type" };
			inline const GameplayTag Photonic{ "Damage.Type.Photonic" };
			inline const GameplayTag Energy{ "Damage.Type.Energy" };
			inline const GameplayTag Kinetic{ "Damage.Type.Kinetic" };
			inline const GameplayTag Thermal{ "Damage.Type.Thermal" };
			inline const GameplayTag Cryo{ "Damage.Type.Cryo" };
			inline const GameplayTag Electric{ "Damage.Type.Electric" };
		}
	}

	namespace Attachment
	{
		inline const GameplayTag Root{ "Attachment" };
		namespace Capability
		{
			inline const GameplayTag Root{ "Attachment.Capability" };
			inline const GameplayTag Damage{ "Attachment.Capability.Damage" };
			inline const GameplayTag Cooldown{ "Attachment.Capability.Cooldown" };
			inline const GameplayTag FireRate{ "Attachment.Capability.FireRate" };
			inline const GameplayTag Projectile{ "Attachment.Capability.Projectile" };
			inline const GameplayTag Beam{ "Attachment.Capability.Beam" };
			inline const GameplayTag Area{ "Attachment.Capability.Area" };
		}
	}

	namespace Cooldown
	{
		inline const GameplayTag& Root = sas::AbilityCooldownTags::Root;
		inline const GameplayTag& Ability = sas::AbilityCooldownTags::Ability;
		inline const GameplayTag& PrimaryWeapon = sas::AbilityCooldownTags::PrimaryWeapon;
	}
}

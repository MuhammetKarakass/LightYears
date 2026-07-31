#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/attachment/AttachmentDefinition.h"

// One shared catalog. Host compatibility is declared by each definition; it is not split into pools.
namespace AttachmentData
{
	namespace Definitions
	{
		static const ly::AttachmentDefinition ThermalConverter{
			ly::GameplayTag{ "Attachment.Thermal.Converter" },
			"Thermal Converter",
			{ ly::AttachmentHostKind::Ability, ly::AttachmentHostKind::PrimaryWeapon },
			{ ly::AttachmentSchema::Capability::Damage },
			{
				sas::GameplayAttribute{
					ly::AttachmentSchema::Attribute::CooldownReductionOnIgnite,
					0.4f,
					0.f
				}
			},
			{},
			{
				ly::ConditionalAttributeModifier{
					ly::AttachmentCondition{
						ly::AttachmentConditionType::HasDamageTag,
						ly::DamageTypeSchema::Thermal
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Multiply,
						1.2f
					}
				}
			},
			ly::DamageTypeSchema::Thermal,
			0,
			{
				ly::AttachmentEventRule{
					ly::AttachmentSchema::Event::SourceIgniteApplied,
					ly::AttachmentEventAction::ReduceCooldown,
					ly::AttachmentCooldownTarget::AllNonPrimaryAbilities,
					ly::AttachmentSchema::Attribute::CooldownReductionOnIgnite,
					0.f,
					true,
					{ ly::DamageTypeSchema::Thermal }
				}
			}
		};

		static const ly::AttachmentDefinition EnergyCoupler{
			ly::GameplayTag{ "Attachment.Energy.Coupler" },
			"Energy Coupler",
			{ ly::AttachmentHostKind::Ability, ly::AttachmentHostKind::PrimaryWeapon },
			{ ly::AttachmentSchema::Capability::Damage },
			{
				sas::GameplayAttribute{ ly::DamageAttributeIds::ShieldDamageMultiplier, 1.25f, 0.f },
				sas::GameplayAttribute{ ly::DamageAttributeIds::ShieldRegenerationDelay, 0.75f, 0.f }
			},
			{},
			{
				ly::ConditionalAttributeModifier{
					ly::AttachmentCondition{
						ly::AttachmentConditionType::HasDamageTag,
						ly::DamageTypeSchema::Energy
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Multiply,
						1.15f
					}
				}
			},
			ly::DamageTypeSchema::Energy,
			0,
			{}
		};

		static const ly::AttachmentDefinition KineticBore{
			ly::GameplayTag{ "Attachment.Kinetic.Bore" },
			"Kinetic Bore",
			{ ly::AttachmentHostKind::Ability, ly::AttachmentHostKind::PrimaryWeapon },
			{ ly::AttachmentSchema::Capability::Damage },
			{
				sas::GameplayAttribute{ ly::DamageAttributeIds::ArmorPenetration, 0.10f, 0.f, 0.25f }
			},
			{},
			{
				ly::ConditionalAttributeModifier{
					ly::AttachmentCondition{
						ly::AttachmentConditionType::HasDamageTag,
						ly::DamageTypeSchema::Kinetic
					},
					sas::AttributeModifier{
						ly::DamageAttributeIds::ArmorPenetration,
						sas::AttributeModifierOperation::Add,
						0.05f
					}
				}
			},
			ly::DamageTypeSchema::Kinetic,
			0,
			{}
		};

		static const ly::AttachmentDefinition CryoConduit{
			ly::GameplayTag{ "Attachment.Cryo.Conduit" },
			"Cryo Conduit",
			{ ly::AttachmentHostKind::Ability, ly::AttachmentHostKind::PrimaryWeapon },
			{ ly::AttachmentSchema::Capability::Damage },
			{
				sas::GameplayAttribute{ ly::DamageAttributeIds::CryoBuildupPerHit, 1.f, 1.f },
				sas::GameplayAttribute{ ly::DamageAttributeIds::CryoBuildupRequired, 4.f, 1.f },
				sas::GameplayAttribute{ ly::DamageAttributeIds::CryoBuildupDuration, 2.5f, 0.f },
				sas::GameplayAttribute{ ly::DamageAttributeIds::CryoSlowPercent, 0.25f, 0.f, 0.30f },
				sas::GameplayAttribute{ ly::DamageAttributeIds::CryoSlowDuration, 1.5f, 0.f }
			},
			{},
			{
				ly::ConditionalAttributeModifier{
					ly::AttachmentCondition{
						ly::AttachmentConditionType::HasDamageTag,
						ly::DamageTypeSchema::Cryo
					},
					sas::AttributeModifier{
						ly::DamageAttributeIds::CryoSlowPercent,
						sas::AttributeModifierOperation::Add,
						0.05f
					}
				}
			},
			ly::DamageTypeSchema::Cryo,
			0,
			{}
		};

		static const ly::AttachmentDefinition ElectricConduit{
			ly::GameplayTag{ "Attachment.Electric.Conduit" },
			"Electric Conduit",
			{ ly::AttachmentHostKind::Ability, ly::AttachmentHostKind::PrimaryWeapon },
			{ ly::AttachmentSchema::Capability::Damage },
			{
				sas::GameplayAttribute{
					ly::DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
					0.04f,
					0.f
				},
				sas::GameplayAttribute{ ly::DamageAttributeIds::ElectricDuration, 3.f, 0.f },
				sas::GameplayAttribute{ ly::DamageAttributeIds::ElectricMaxStacks, 4.f, 1.f }
			},
			{},
			{
				ly::ConditionalAttributeModifier{
					ly::AttachmentCondition{
						ly::AttachmentConditionType::HasDamageTag,
						ly::DamageTypeSchema::Electric
					},
					sas::AttributeModifier{
						ly::DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
						sas::AttributeModifierOperation::Add,
						0.01f
					}
				}
			},
			ly::DamageTypeSchema::Electric,
			0,
			{}
		};

		static const ly::AttachmentDefinition HeavyCapacitor{
			ly::GameplayTag{ "Attachment.Heavy.Capacitor" },
			"Heavy Capacitor",
			{ ly::AttachmentHostKind::Ability },
			{
				ly::AttachmentSchema::Capability::Damage,
				ly::AttachmentSchema::Capability::Cooldown
			},
			{},
			{
				sas::AttributeModifier{
					ly::CommonAttributeIds::Damage,
					sas::AttributeModifierOperation::Multiply,
					1.4f
				},
				sas::AttributeModifier{
					ly::CommonAttributeIds::Cooldown,
					sas::AttributeModifierOperation::Multiply,
					1.25f
				}
			},
			{},
			{},
			0,
			{}
		};

		static const ly::AttachmentDefinition EmergencySalvo{
			ly::GameplayTag{ "Attachment.Projectile.EmergencySalvo" },
			"Emergency Salvo",
			{ ly::AttachmentHostKind::PrimaryWeapon },
			{
				ly::AttachmentSchema::Capability::FireRate,
				ly::AttachmentSchema::Capability::Projectile
			},
			{
				sas::GameplayAttribute{
					PrimaryWeaponSchema::Projectile::Delivery::AdditionalProjectileCount,
					0.f,
					0.f
				}
			},
			{},
			{
				ly::ConditionalAttributeModifier{
					ly::AttachmentCondition{
						ly::AttachmentConditionType::AttributeLessThan,
						ly::CommonAttributeIds::FireRate,
						4.f
					},
					sas::AttributeModifier{
						PrimaryWeaponSchema::Projectile::Delivery::AdditionalProjectileCount,
						sas::AttributeModifierOperation::Add,
						1.f
					}
				}
			},
			{},
			0,
			{}
		};
	}

	inline const ly::List<const ly::AttachmentDefinition*>& GetDefinitions()
	{
		static const ly::List<const ly::AttachmentDefinition*> definitions{
			&Definitions::ThermalConverter,
			&Definitions::EnergyCoupler,
			&Definitions::KineticBore,
			&Definitions::CryoConduit,
			&Definitions::ElectricConduit,
			&Definitions::HeavyCapacitor,
			&Definitions::EmergencySalvo
		};
		return definitions;
	}

	inline const ly::AttachmentDefinition* FindAttachmentDefinition(const ly::GameplayTag& attachmentId)
	{
		for (const ly::AttachmentDefinition* definition : GetDefinitions())
		{
			if (definition && definition->attachmentId == attachmentId)
			{
				return definition;
			}
		}
		return nullptr;
	}
}

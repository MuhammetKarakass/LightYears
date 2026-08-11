#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "../internal/PrimaryWeaponBuiltIns.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/weapon/wave/ExpandingWaveWeaponActor.h"

#include <algorithm>

namespace ly
{
	namespace
	{
		class ExpandingWaveWeaponHandler final : public PrimaryWeaponHandler
		{
		public:
			PrimaryWeaponType GetType() const override
			{
				return PrimaryWeaponType::WaveExpanding;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return PrimaryWeaponBuiltIns::WaveDeliveryAttributeRoots();
			}

			PrimaryWeaponValidationResult ValidateDefinition(
				const PrimaryWeaponDefinition& definition
			) const override
			{
				for (const sas::AttributeId& required : {
					CommonAttributeIds::Damage,
					CommonAttributeIds::Range,
					PrimaryWeaponSchema::Wave::Delivery::Speed,
					PrimaryWeaponSchema::Wave::Delivery::InitialWidth,
					PrimaryWeaponSchema::Wave::Delivery::MaximumWidth,
					PrimaryWeaponSchema::Wave::Delivery::Thickness
				})
				{
					const PrimaryWeaponValidationResult result =
						PrimaryWeaponBuiltIns::RequireAttribute(
							definition,
							required,
							"Expanding wave weapon"
						);
					if (!result.isValid)
					{
						return result;
					}
				}

				const float initialWidth = sas::FindAttributeValue(
					definition.attributes,
					PrimaryWeaponSchema::Wave::Delivery::InitialWidth,
					0.f
				);
				const float maximumWidth = sas::FindAttributeValue(
					definition.attributes,
					PrimaryWeaponSchema::Wave::Delivery::MaximumWidth,
					0.f
				);
				if (initialWidth <= 0.f || maximumWidth < initialWidth)
				{
					return {
						false,
						"Expanding wave weapon requires a positive initial width "
						"and a maximum width no smaller than it."
					};
				}
				return { true, {} };
			}

			void FireOnce(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
				World* world = context.owner.GetWorld();
				if (!world)
				{
					return;
				}

				const auto spawnFromMuzzle =
					[&](const WeaponMuzzleDefinition& muzzle)
				{
					const weak_ptr<ExpandingWaveWeaponActor> wave =
						world->SpawnActor<ExpandingWaveWeaponActor>(
							&context.owner,
							context.definition.presentationDefinition,
							context.attributes,
							context.damageTags
						);
					if (const shared_ptr<ExpandingWaveWeaponActor> spawned =
						wave.lock())
					{
						spawned->SetActorLocation(
							context.owner.GetActorLocation() +
							context.owner.TransformLocalToWorld(muzzle.offset)
						);
						spawned->SetActorRotation(
							context.owner.GetActorRotation() +
							muzzle.rotationOffset
						);
					}
				};

				if (context.definition.muzzleDefinitions.empty())
				{
					spawnFromMuzzle(WeaponMuzzleDefinition{});
					return;
				}
				for (const WeaponMuzzleDefinition& muzzle :
					context.definition.muzzleDefinitions)
				{
					spawnFromMuzzle(muzzle);
				}
			}
		};
	}

	namespace PrimaryWeaponBuiltIns
	{
		unique_ptr<PrimaryWeaponHandler> CreateExpandingWaveWeaponHandler()
		{
			return std::make_unique<ExpandingWaveWeaponHandler>();
		}
	}
}

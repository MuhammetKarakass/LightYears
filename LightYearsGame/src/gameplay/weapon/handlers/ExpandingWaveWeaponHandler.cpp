#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

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

			bool FireOnce(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
				World* world = context.owner.GetWorld();
				if (!world)
				{
					return false;
				}

				bool anySpawned = false;
				const auto spawnFromMuzzle =
					[&](const WeaponMuzzleDefinition& muzzle) -> bool
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
						return true;
					}
					return false;
				};

				if (context.definition.muzzleDefinitions.empty())
				{
					return spawnFromMuzzle(WeaponMuzzleDefinition{});
				}
				for (const WeaponMuzzleDefinition& muzzle :
					context.definition.muzzleDefinitions)
				{
					if (spawnFromMuzzle(muzzle))
					{
						anySpawned = true;
					}
				}
				return anySpawned;
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

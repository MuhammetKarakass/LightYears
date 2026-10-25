#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/weapon/wave/ExpandingWaveWeaponActor.h"

#include <algorithm>

namespace vega
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

			bool FireOnce(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState&) const override
			{
				World* world = context.owner.GetWorld();
				if (!world)
				{
					return false;
				}

				// Empowered waves reach further and grow wider.
				sas::GameplayAttributeList waveAttributes = context.attributes;
				if (context.shotMetadata.isEmpowered)
				{
					const float rangeBonus = sas::FindAttributeValue(
						waveAttributes, PrimaryWeaponSchema::Wave::Empowered::RangeBonus, 0.f);
					const float widthMultiplier = std::max(0.f, sas::FindAttributeValue(
						waveAttributes, PrimaryWeaponSchema::Wave::Empowered::WidthMultiplier, 1.f));
					const auto scale = [&](const sas::AttributeId& id, float multiplier, float add)
					{
						if (sas::GameplayAttribute* attribute = sas::FindAttribute(waveAttributes, id))
						{
							attribute->currentValue = attribute->currentValue * multiplier + add;
						}
					};
					scale(CommonAttributeIds::Range, 1.f, rangeBonus);
					scale(PrimaryWeaponSchema::Wave::Delivery::InitialWidth, widthMultiplier, 0.f);
					scale(PrimaryWeaponSchema::Wave::Delivery::MaximumWidth, widthMultiplier, 0.f);
				}

				bool anySpawned = false;
				const auto spawnFromMuzzle =
					[&](const PrimaryWeaponEmission& emission) -> bool
				{
					const weak_ptr<ExpandingWaveWeaponActor> wave =
						world->SpawnActor<ExpandingWaveWeaponActor>(
							&context.owner,
							context.definition.presentationDefinition,
							waveAttributes,
							context.damageTags
						);
					if (const shared_ptr<ExpandingWaveWeaponActor> spawned =
						wave.lock())
					{
						spawned->SetDamage(ResolvePrimaryWeaponShotDamage(context) * emission.damageMultiplier);
						spawned->SetShotMetadata(context.shotMetadata);
						spawned->SetActorLocation(emission.origin);
						spawned->SetActorRotation(emission.rotation);
						return true;
					}
					return false;
				};

				for (const auto& emission : ResolvePrimaryWeaponEmissions(context))
				{
					if (!context.ShouldContinue()) break;
					anySpawned = spawnFromMuzzle(emission) || anySpawned;
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

#include "../internal/PrimaryWeaponBuiltIns.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace ly
{
	namespace
	{
		constexpr float ElectricMaximumBonusChainChance = 0.35f;

		float GetElectricBonusChainChance(const Actor& owner)
		{
			const auto* combatant = dynamic_cast<const Combatant*>(&owner);
			return combatant
				? ElectricMaximumBonusChainChance * combatant->GetCombatRuntime().GetCombatLuckFactor()
				: 0.f;
		}

		bool IsElectricArcTarget(const Actor& source, const Actor& candidate)
		{
			if (&candidate == &source || candidate.GetIsPendingDestroy())
			{
				return false;
			}

			CollisionLayer expectedTargetLayer = CollisionLayer::None;
			CollisionLayer virtualProjectileLayer = CollisionLayer::None;
			switch (source.GetCollisionLayer())
			{
			case CollisionLayer::Player:
				expectedTargetLayer = CollisionLayer::Enemy;
				virtualProjectileLayer = CollisionLayer::PlayerBullet;
				break;
			case CollisionLayer::Enemy:
				expectedTargetLayer = CollisionLayer::Player;
				virtualProjectileLayer = CollisionLayer::EnemyBullet;
				break;
			default:
				return false;
			}

			return HasCollisionLayer(expectedTargetLayer, candidate.GetCollisionLayer()) &&
				HasCollisionLayer(candidate.GetCollisionMask(), virtualProjectileLayer);
		}

		Actor* FindClosestElectricArcTarget(
			World& world,
			const Actor& source,
			const sf::Vector2f& origin,
			float range,
			const std::unordered_set<Actor*>& excludedTargets
		)
		{
			Actor* closestTarget = nullptr;
			float closestDistanceSquared = std::numeric_limits<float>::max();
			const float rangeSquared = std::max(0.f, range) * std::max(0.f, range);
			for (const weak_ptr<Actor>& targetWeak : world.GetActorsByType<Actor>())
			{
				const shared_ptr<Actor> candidate = targetWeak.lock();
				if (!candidate || excludedTargets.count(candidate.get()) > 0 ||
					!IsElectricArcTarget(source, *candidate))
				{
					continue;
				}

				const sf::Vector2f offset = candidate->GetActorLocation() - origin;
				const float distanceSquared = offset.x * offset.x + offset.y * offset.y;
				if (distanceSquared <= rangeSquared && distanceSquared < closestDistanceSquared)
				{
					closestTarget = candidate.get();
					closestDistanceSquared = distanceSquared;
				}
			}
			return closestTarget;
		}

		void ResolveElectricArc(
			const PrimaryWeaponExecutionContext& context,
			const sf::Vector2f& origin,
			int additionalChainCount,
			float targetRange,
			float chainRange,
			float damageMultiplierPerChain
		)
		{
			World* world = context.owner.GetWorld();
			const float baseDamage = std::max(0.f, FindGameplayAttributeValue(
				context.attributes,
				CommonAttributeIds::Damage,
				0.f
			));
			if (!world || baseDamage <= 0.f || targetRange <= 0.f)
			{
				return;
			}

			std::unordered_set<Actor*> struckTargets;
			Actor* currentTarget = FindClosestElectricArcTarget(
				*world,
				context.owner,
				origin,
				targetRange,
				struckTargets
			);
			if (!currentTarget)
			{
				const float placeholderLength = std::min(targetRange, 420.f);
				world->SpawnActor<ElectricArcVisualActor>(
					origin,
					origin + context.owner.GetActorForwardDirection() * placeholderLength,
					context.definition.presentationDefinition.pointLightDef.color
				);
				return;
			}

			const DamagePayload payload =
				DamageTypeSystem::BuildPayload(context.damageTags, context.attributes);
			sf::Vector2f arcStart = origin;
			float damage = baseDamage;
			int remainingNormalChains = std::max(0, additionalChainCount);
			bool bonusChainResolved = false;
			while (currentTarget)
			{
				const sf::Vector2f targetLocation = currentTarget->GetActorLocation();
				ApplyCombatDamage(
					*currentTarget,
					damage,
					&context.owner,
					context.damageTags,
					payload
				);
				world->SpawnActor<ElectricArcVisualActor>(
					arcStart,
					targetLocation,
					context.definition.presentationDefinition.pointLightDef.color
				);
				struckTargets.insert(currentTarget);
				arcStart = targetLocation;
				damage *= std::clamp(damageMultiplierPerChain, 0.f, 1.f);
				if (damage <= 0.f)
				{
					return;
				}
				Actor* nextTarget = FindClosestElectricArcTarget(
					*world,
					context.owner,
					arcStart,
					chainRange,
					struckTargets
				);
				if (!nextTarget)
				{
					return;
				}
				if (remainingNormalChains > 0)
				{
					--remainingNormalChains;
					currentTarget = nextTarget;
					continue;
				}
				if (bonusChainResolved || RandRange(0.f, 1.f) >= GetElectricBonusChainChance(context.owner))
				{
					return;
				}

				bonusChainResolved = true;
				currentTarget = nextTarget;
			}
		}

		class ElectricArcWeaponHandler final : public PrimaryWeaponHandler
		{
		public:
			const GameplayTag& GetTypeTag() const override
			{
				return PrimaryWeaponSchema::Arc::Electric::TypeId;
			}

			const List<GameplayTag>& GetOwnedAttributeRoots() const override
			{
				return PrimaryWeaponBuiltIns::ArcAttributeRoots();
			}

			PrimaryWeaponValidationResult ValidateDefinition(
				const PrimaryWeaponDefinition& definition
			) const override
			{
				for (const GameplayTag& required : {
					CommonAttributeIds::Damage,
					CommonAttributeIds::Range,
					PrimaryWeaponSchema::Arc::Electric::ChainCount,
					PrimaryWeaponSchema::Arc::Electric::ChainRange,
					PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain
				})
				{
					const PrimaryWeaponValidationResult result =
						PrimaryWeaponBuiltIns::RequireAttribute(
							definition,
							required,
							"Electric arc weapon"
						);
					if (!result.isValid)
					{
						return result;
					}
				}

				const float chainCount = PrimaryWeaponBuiltIns::FindDefinitionAttribute(
					definition,
					PrimaryWeaponSchema::Arc::Electric::ChainCount
				)->baseValue;
				if (chainCount < 0.f || std::round(chainCount) != chainCount)
				{
					return { false, "Arc chain count must be a non-negative integer." };
				}
				const float chainRange = PrimaryWeaponBuiltIns::FindDefinitionAttribute(
					definition,
					PrimaryWeaponSchema::Arc::Electric::ChainRange
				)->baseValue;
				if (PrimaryWeaponBuiltIns::FindDefinitionAttribute(
						definition,
						CommonAttributeIds::Range
					)->baseValue <= 0.f ||
					chainRange <= 0.f)
				{
					return {
						false,
						"Electric arc target and chain ranges must be greater than zero."
					};
				}
				const float damageMultiplier =
					PrimaryWeaponBuiltIns::FindDefinitionAttribute(
						definition,
						PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain
					)->baseValue;
				return damageMultiplier > 0.f && damageMultiplier <= 1.f
					? PrimaryWeaponValidationResult{ true, {} }
					: PrimaryWeaponValidationResult{
						false,
						"Arc damage multiplier per chain must be greater than zero and at most one."
					};
			}

			void FireOnce(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponTypeRuntimeState&
			) const override
			{
				const int chainCount = std::max(
					0,
					static_cast<int>(std::round(FindGameplayAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Arc::Electric::ChainCount,
						0.f
					)))
				);
				const float chainRange = std::max(0.f, FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Arc::Electric::ChainRange,
					0.f
				));
				const float damageMultiplier = std::clamp(FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain,
					1.f
				), 0.f, 1.f);
				const float targetRange = std::max(0.f, FindGameplayAttributeValue(
					context.attributes,
					CommonAttributeIds::Range,
					0.f
				));
				const auto fireFromMuzzle = [&](const WeaponMuzzleDefinition& muzzle)
				{
					const sf::Vector2f origin = context.owner.GetActorLocation() +
						context.owner.TransformLocalToWorld(muzzle.offset);
					ResolveElectricArc(
						context,
						origin,
						chainCount,
						targetRange,
						chainRange,
						damageMultiplier
					);
				};
				if (context.definition.muzzleDefinitions.empty())
				{
					fireFromMuzzle(WeaponMuzzleDefinition{});
					return;
				}
				for (const WeaponMuzzleDefinition& muzzle : context.definition.muzzleDefinitions)
				{
					fireFromMuzzle(muzzle);
				}
			}
		};
	}

	namespace PrimaryWeaponBuiltIns
	{
		unique_ptr<PrimaryWeaponHandler> CreateElectricArcWeaponHandler()
		{
			return std::make_unique<ElectricArcWeaponHandler>();
		}
	}
}

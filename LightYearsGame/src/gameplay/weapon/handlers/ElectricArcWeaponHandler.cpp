#include "gameConfigs/CollisionLayers.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/module/rules/EpicPrimaryRules.h"
#include "gameplay/targeting/CombatTargetPriority.h"
#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <unordered_set>

namespace vega
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

		void ResolveElectricArc(const PrimaryWeaponExecutionContext& context, const PrimaryWeaponEmission& emission, int additionalChainCount, float targetRange, float chainRange, float damageMultiplierPerChain, bool& hitReported)
		{
			World* world = context.owner.GetWorld();
			const sf::Vector2f origin = emission.origin;
			const float baseDamage = ResolvePrimaryWeaponShotDamage(context) * emission.damageMultiplier;
			if (!world || baseDamage <= 0.f || targetRange <= 0.f)
			{
				return;
			}

			vega::CollisionLayer expectedTargetLayer = vega::CollisionLayer::None;
			vega::CollisionLayer virtualProjectileLayer = vega::CollisionLayer::None;
			switch (context.owner.GetCollisionLayer())
			{
			case GameCollision::Player:
				expectedTargetLayer = GameCollision::Enemy;
				virtualProjectileLayer = GameCollision::PlayerBullet;
				break;
			case GameCollision::Enemy:
				expectedTargetLayer =
					GameCollision::Player | GameCollision::FriendlySummon;
				virtualProjectileLayer = GameCollision::EnemyBullet;
				break;
			default:
				return;
			}

			auto findClosestTarget = [&](
				const sf::Vector2f& searchOrigin,
				float searchRange,
				const std::unordered_set<Actor*>& excludedTargets,
				const std::optional<sf::Vector2f>& preferNearCursor = std::nullopt
			) -> shared_ptr<Actor>
			{
				targeting::TargetingQuery query;
				query.source = &context.owner;
				query.origin = searchOrigin;
				query.range = searchRange;
				query.maxTargets = 1;
				query.requiredTargetLayers = expectedTargetLayer;
				query.requiredCandidateCollisionMask = virtualProjectileLayer;
				query.comparator = targeting::MakeCombatTargetPriorityComparator(
					context.owner.GetCollisionLayer()
				);
				if (preferNearCursor)
				{
					// The range filter above still decides who is reachable from the
					// muzzle; the cursor only picks among those candidates.
					const sf::Vector2f cursor = *preferNearCursor;
					const vega::CollisionLayer sourceLayer = context.owner.GetCollisionLayer();
					query.comparator = [cursor, sourceLayer](
						const targeting::TargetingCandidate& left,
						const targeting::TargetingCandidate& right
					)
					{
						const int leftPriority =
							targeting::GetCombatTargetPriority(sourceLayer, *left.actor);
						const int rightPriority =
							targeting::GetCombatTargetPriority(sourceLayer, *right.actor);
						if (leftPriority != rightPriority)
						{
							return leftPriority < rightPriority;
						}
						const sf::Vector2f leftOffset = left.actor->GetActorLocation() - cursor;
						const sf::Vector2f rightOffset = right.actor->GetActorLocation() - cursor;
						const float leftCursorDistanceSquared =
							leftOffset.x * leftOffset.x + leftOffset.y * leftOffset.y;
						const float rightCursorDistanceSquared =
							rightOffset.x * rightOffset.x + rightOffset.y * rightOffset.y;
						return leftCursorDistanceSquared != rightCursorDistanceSquared
							? leftCursorDistanceSquared < rightCursorDistanceSquared
							: left.distanceSquared < right.distanceSquared;
					};
				}
				if (excludedTargets.empty() && (context.mountProfile.turret || context.mountProfile.broadside))
				{
					query.comparator = [&](const targeting::TargetingCandidate& left, const targeting::TargetingCandidate& right)
					{
						const float leftHealth = context.mountProfile.executionPriority ? EpicPrimaryMounts::GetHealthFraction(*left.actor) : 0.f;
						const float rightHealth = context.mountProfile.executionPriority ? EpicPrimaryMounts::GetHealthFraction(*right.actor) : 0.f;
						return leftHealth != rightHealth ? leftHealth < rightHealth : left.distanceSquared < right.distanceSquared;
					};
					if (context.mountProfile.broadside)
					{
						query.shape = targeting::TargetingShape::Cone;
						query.direction = RotationToVector(emission.rotation - 90.f);
						query.coneHalfAngleRadians = 1.57079632679f;
					}
				}
				query.excludedTargets.reserve(excludedTargets.size());
				for (Actor* excludedTarget : excludedTargets)
				{
					query.excludedTargets.push_back(excludedTarget);
				}
				return targeting::AutoTargeting::FindTarget(*world, query).lock();
			};

			std::unordered_set<Actor*> struckTargets;
			shared_ptr<Actor> currentTarget = findClosestTarget(
				origin,
				targetRange,
				struckTargets,
				// Only the first target follows the cursor; chain jumps stay nearest-first.
				context.aimWorldPosition
			);
			if (!currentTarget)
			{
				const float placeholderLength = std::min(targetRange, 420.f);
				world->SpawnActor<ElectricArcVisualActor>(
					origin,
					origin + RotationToVector(emission.rotation - 90.f) * placeholderLength,
					context.definition.presentationDefinition.pointLightDef.color
				);
				return;
			}

			DamagePayload payload =
				DamageTypeSystem::BuildPayload(context.damageTags, context.attributes);
			// Chain targets share the shot's policy, so an empowered arc's status
			// stacks reach every target it jumps to.
			ApplyShotMetadataToPayload(payload, context.shotMetadata);
			if (!hitReported)
			{
				hitReported = context.shotMetadata.ReportHit();
			}
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
				struckTargets.insert(currentTarget.get());
				arcStart = targetLocation;
				damage *= std::clamp(damageMultiplierPerChain, 0.f, 1.f);
				if (damage <= 0.f)
				{
					return;
				}
				shared_ptr<Actor> nextTarget = findClosestTarget(
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
			PrimaryWeaponType GetType() const override
			{
				return PrimaryWeaponType::ArcElectric;
			}

			bool FireOnce(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState&) const override
			{
				if (!context.owner.GetWorld())
				{
					return false;
				}

				const int chainCount = std::max(
					0,
					static_cast<int>(std::round(sas::FindAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Arc::Electric::ChainCount,
						0.f
					)))
				);
				const float chainRange = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Arc::Electric::ChainRange,
					0.f
				));
				const float damageMultiplier = std::clamp(sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain,
					1.f
				), 0.f, 1.f);
				const float targetRange = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					CommonAttributeIds::Range,
					0.f
				));
				bool hitReported = false;
				for (const auto& emission : ResolvePrimaryWeaponEmissions(context))
				{
					if (!context.ShouldContinue()) break;
					ResolveElectricArc(context, emission, chainCount, targetRange, chainRange, damageMultiplier, hitReported);
				}
				return true;
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

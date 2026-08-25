#include "gameplay/ability/combatSentry/CombatSentryActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/combatSentry/CombatSentryContracts.h"
#include "gameplay/ability/combatSentry/CombatSentryProjectileActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> TurretCommonAttributes{
			CommonAttributeIds::Duration,
			CollisionAttributeIds::Radius
		};

		const List<sas::AttributeId> TurretAttributeRoots{
			AbilityData::CombatSentry::Actor::Turret::Root
		};

		class CombatSentryTurretActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::CombatSentryTurret;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return TurretAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return TurretCommonAttributes;
			}

			AbilityActorValidationResult ValidateDefinition(
				const AbilityActorDefinition& definition
			) const override
			{
				const AbilityActorValidationResult baseResult =
					AbilityActorTypeHandler::ValidateDefinition(definition);
				if (!baseResult.isValid)
				{
					return baseResult;
				}
				for (const sas::AttributeId& required : {
					CommonAttributeIds::Duration,
					CollisionAttributeIds::Radius,
					AbilityData::CombatSentry::Actor::Turret::MaxHealth,
					AbilityData::CombatSentry::Actor::Turret::OwnerMaxHealthScale,
					AbilityData::CombatSentry::Actor::Turret::Armor,
					AbilityData::CombatSentry::Actor::Turret::OwnerArmorScale,
					AbilityData::CombatSentry::Actor::Turret::BaseDamage,
					AbilityData::CombatSentry::Actor::Turret::OwnerAttackPowerScale,
					AbilityData::CombatSentry::Actor::Turret::TargetingRange,
					AbilityData::CombatSentry::Actor::Turret::AttackRate
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || !std::isfinite(attribute->baseValue) ||
						attribute->baseValue < 0.f)
					{
						return { false, "Combat Sentry turret requires finite combat attributes." };
					}
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<CombatSentryTurretPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Combat Sentry turret requires a typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const auto* profile =
					PresentationProfileRegistry<CombatSentryTurretPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<CombatSentryActor>(
						&context.owner,
						*profile,
						CombatSentryActor::Configuration{
							context.abilitySystem,
							context.abilityDefinition
								? std::make_shared<GameAbilityDefinition>(
									*context.abilityDefinition
								)
								: nullptr
						}
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	CombatSentryActor::CombatSentryActor(
		World* world,
		Actor* owner,
		const CombatSentryTurretPresentationProfile& profile,
		Configuration configuration
	)
		: SummonedCombatantActor(world, owner)
		, mProfile(profile)
		, mConfiguration(configuration)
	{
		SetRenderLayer(RenderLayer::World);
	}

	void CombatSentryActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		const auto value = [&attributes](const sas::AttributeId& id, float fallback)
		{
			return sas::FindAttributeValue(attributes, id, fallback);
		};
		const float ownerMaxHealthScale = std::max(0.f, value(
			AbilityData::CombatSentry::Actor::Turret::OwnerMaxHealthScale,
			AbilityData::CombatSentry::DefaultOwnerMaxHealthScale
		));
		ConfigureCombatant(CombatantConfiguration{
			std::max(1.f, value(
				AbilityData::CombatSentry::Actor::Turret::MaxHealth,
				AbilityData::CombatSentry::DefaultMaxHealth
			) + GetOwnerCombatAttribute(OwnerAttributeIds::MaxHealth) * ownerMaxHealthScale),
			std::max(0.f, value(
				AbilityData::CombatSentry::Actor::Turret::Armor,
				AbilityData::CombatSentry::DefaultArmor
			)),
			std::max(0.f, value(
				AbilityData::CombatSentry::Actor::Turret::OwnerArmorScale,
				AbilityData::CombatSentry::DefaultOwnerArmorScale
			)),
			1.f
		});
		mTargetingRange = std::max(0.f, value(
			AbilityData::CombatSentry::Actor::Turret::TargetingRange,
			AbilityData::CombatSentry::DefaultTargetingRange
		));
		mBaseDamage = std::max(0.f, value(
			AbilityData::CombatSentry::Actor::Turret::BaseDamage,
			AbilityData::CombatSentry::DefaultDamage
		));
		mOwnerAttackPowerScale = std::max(0.f, value(
			AbilityData::CombatSentry::Actor::Turret::OwnerAttackPowerScale,
			AbilityData::CombatSentry::DefaultOwnerAttackPowerScale
		));
		mBaseAttackRate = std::max(0.f, value(
			AbilityData::CombatSentry::Actor::Turret::AttackRate,
			AbilityData::CombatSentry::DefaultAttackRate
		));
	}

	void CombatSentryActor::Tick(float deltaTime)
	{
		SummonedCombatantActor::Tick(deltaTime);
		if (GetIsPendingDestroy())
		{
			return;
		}
		mVisualAge += std::max(0.f, deltaTime);
		TryFire(std::max(0.f, deltaTime));
	}

	shared_ptr<Actor> CombatSentryActor::FindTarget() const
	{
		World* world = GetWorld();
		if (!world)
		{
			return {};
		}
		targeting::TargetingQuery query;
		query.source = this;
		query.origin = GetActorLocation();
		query.range = mTargetingRange;
		query.maxTargets = 1;
		query.requiredTargetLayers = CollisionLayer::Enemy;
		query.requireCollisionCompatibility = true;
		query.filter = [](const Actor*, const Actor& candidate, const targeting::TargetingCandidate&)
		{
			return dynamic_cast<const Combatant*>(&candidate) != nullptr;
		};
		return targeting::AutoTargeting::FindTarget(*world, query).lock();
	}

	void CombatSentryActor::TryFire(float deltaTime)
	{
		mFireCooldown = std::max(0.f, mFireCooldown - deltaTime);
		if (mFireCooldown > 0.f || !mConfiguration.abilitySystem ||
			!mConfiguration.abilityDefinition)
		{
			return;
		}
		const shared_ptr<Actor> target = FindTarget();
		if (!target)
		{
			return;
		}
		sf::Vector2f direction = target->GetActorLocation() - GetActorLocation();
		if (GetVectorLength(direction) <= 0.001f)
		{
			return;
		}
		NormalizeVector(direction);
		SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);

		AbilityExecutionContext context{
			mConfiguration.abilitySystem,
			mConfiguration.abilityDefinition.get(),
			nullptr,
			nullptr
		};
		const weak_ptr<AbilityWorldActor> spawned = AbilityActorSpawner::SpawnAtLocation(
			AbilityData::CombatSentry::Actor::Projectile::BasicDefinitionId,
			context,
			*this,
			GetActorLocation() + direction * (mProfile.bodyRadius + 4.f),
			direction
		);
		if (const shared_ptr<CombatSentryProjectileActor> projectile =
			std::dynamic_pointer_cast<CombatSentryProjectileActor>(spawned.lock()))
		{
			projectile->SetShotDamage(ResolveProjectileDamage());
		}
		mFireCooldown = 1.f / std::max(0.01f, ResolveAttackRate());
	}

	float CombatSentryActor::ResolveProjectileDamage() const
	{
		return std::max(
			0.f,
			mBaseDamage + GetOwnerCombatAttribute(OwnerAttributeIds::AttackPower) *
				mOwnerAttackPowerScale
		);
	}

	float CombatSentryActor::ResolveAttackRate() const
	{
		return mBaseAttackRate * GetOwnerAttackSpeedMultiplier();
	}

	void CombatSentryActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		const float pulse = 0.80f + 0.20f * std::sin(mVisualAge * 6.f);
		sf::CircleShape body(mProfile.bodyRadius, 6);
		body.setOrigin({ mProfile.bodyRadius, mProfile.bodyRadius });
		body.setPosition(GetActorLocation());
		body.setRotation(sf::degrees(GetActorRotation()));
		body.setFillColor(mProfile.bodyColor);
		window.draw(body);

		sf::RectangleShape barrel({ mProfile.barrelWidth, mProfile.barrelLength });
		barrel.setOrigin({ mProfile.barrelWidth * 0.5f, mProfile.barrelLength * 0.72f });
		barrel.setPosition(GetActorLocation());
		barrel.setRotation(sf::degrees(GetActorRotation()));
		barrel.setFillColor(mProfile.barrelColor);
		window.draw(barrel);

		sf::CircleShape core(mProfile.bodyRadius * 0.32f * pulse, 16);
		core.setOrigin({ mProfile.bodyRadius * 0.32f * pulse, mProfile.bodyRadius * 0.32f * pulse });
		core.setPosition(GetActorLocation());
		core.setFillColor(mProfile.coreColor);
		window.draw(core);
	}

	bool RegisterCombatSentryTurretActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<CombatSentryTurretActorTypeHandler>()
		);
	}
}

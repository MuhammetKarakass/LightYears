#include "gameplay/ability/mineLayer/MineLayerMineActor.h"

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/mineLayer/MineLayerContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"
#include "gameplay/targeting/TargetingTypes.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <SFML/Graphics/CircleShape.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> MineCommonAttributes{
			CommonAttributeIds::Duration,
			CommonAttributeIds::Damage,
			CommonAttributeIds::Radius
		};

		const List<sas::AttributeId> MineAttributeRoots{
			AbilityData::MineLayer::Actor::Mine::Root
		};

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& value)
		{
			const float length = std::sqrt(value.x * value.x + value.y * value.y);
			return length > 0.001f
				? value / length
				: sf::Vector2f{ 0.f, -1.f };
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * std::clamp(multiplier, 0.f, 1.f),
				0.f,
				255.f
			));
			return result;
		}

		class MineLayerMineActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::MineLayerMine;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return MineAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return MineCommonAttributes;
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
					CommonAttributeIds::Damage,
					CommonAttributeIds::Radius,
					AbilityData::MineLayer::Actor::Mine::TriggerRadius,
					AbilityData::MineLayer::Actor::Mine::LaunchSpeed,
					AbilityData::MineLayer::Actor::Mine::StunDuration,
					AbilityData::MineLayer::Actor::Mine::KnockbackStrength
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || !std::isfinite(attribute->baseValue))
					{
						return {
							false,
							"Mine actor is missing required attribute '" +
								std::string{ required.GetName() } + "'."
						};
					}
				}

				if (definition.lifeTime <= 0.f ||
					sas::FindAttributeValue(definition.attributes, CommonAttributeIds::Damage) <= 0.f ||
					sas::FindAttributeValue(definition.attributes, CommonAttributeIds::Radius) <= 0.f ||
					sas::FindAttributeValue(
						definition.attributes,
						AbilityData::MineLayer::Actor::Mine::TriggerRadius
					) <= 0.f ||
					sas::FindAttributeValue(
						definition.attributes,
						AbilityData::MineLayer::Actor::Mine::LaunchSpeed
					) <= 0.f ||
					sas::FindAttributeValue(
						definition.attributes,
						AbilityData::MineLayer::Actor::Mine::StunDuration
					) <= 0.f ||
					sas::FindAttributeValue(
						definition.attributes,
						AbilityData::MineLayer::Actor::Mine::KnockbackStrength
					) < 0.f)
				{
					return { false, "Mine actor requires positive damage, area, trigger, launch and stun values." };
				}

				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<MineLayerPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Mine actor requires a registered typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const MineLayerPresentationProfile* profile =
					PresentationProfileRegistry<MineLayerPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<MineLayerMineActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	MineLayerMineActor::MineLayerMineActor(
		World* world,
		Actor* owner,
		const MineLayerPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile{ presentationProfile }
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		// A mine is a gameplay query volume, not a moving physics projectile. The
		// trigger and explosion radii are evaluated through reusable AutoTargeting.
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void MineLayerMineActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void MineLayerMineActor::BeginPortalTransit()
	{
		mPortalRemainingDeploymentDistance = mIsDeploying
			? std::max(
				0.f,
				GetVectorLength(mDeploymentTarget - GetActorLocation())
			)
			: 0.f;
		AbilityWorldActor::BeginPortalTransit();
	}

	void MineLayerMineActor::RebasePortalDestination(
		const sf::Vector2f& exitLocation
	)
	{
		if (mIsDeploying)
		{
			// Mine placement is a delivery flight. Preserve only the remaining
			// flight distance and continue it from the portal exit, never toward
			// the old side of the portal pair.
			mDeploymentTarget = portal::RebaseForwardDestination(
				exitLocation,
				mLaunchVelocity,
				mPortalRemainingDeploymentDistance
			);
		}
		mPortalRemainingDeploymentDistance = 0.f;
	}

	void MineLayerMineActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mTriggerRadius = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::MineLayer::Actor::Mine::TriggerRadius,
				mTriggerRadius
			)
		);
		mExplosionRadius = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				CommonAttributeIds::Radius,
				mExplosionRadius
			)
		);
		mLaunchSpeed = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::MineLayer::Actor::Mine::LaunchSpeed,
				mLaunchSpeed
			)
		);
		mStunDuration = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::MineLayer::Actor::Mine::StunDuration,
				mStunDuration
			)
		);
		mKnockbackStrength = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::MineLayer::Actor::Mine::KnockbackStrength,
				mKnockbackStrength
			)
		);
	}

	void MineLayerMineActor::LaunchTo(const sf::Vector2f& targetLocation)
	{
		mDeploymentTarget = targetLocation;
		mPortalRemainingDeploymentDistance = 0.f;
		const sf::Vector2f delta = mDeploymentTarget - GetActorLocation();
		const float distance = GetVectorLength(delta);
		if (distance <= 0.001f || mLaunchSpeed <= 0.f)
		{
			SetActorLocation(mDeploymentTarget);
			SetVelocity({});
			mLaunchVelocity = {};
			mIsDeploying = false;
			return;
		}

		const sf::Vector2f direction = delta / distance;
		mLaunchVelocity = direction * mLaunchSpeed;
		SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
		SetVelocity(mLaunchVelocity);
		mIsDeploying = true;
	}

	void MineLayerMineActor::MoveTowardDeploymentTarget(float deltaTime)
	{
		if (!mIsDeploying)
		{
			return;
		}

		const sf::Vector2f delta = mDeploymentTarget - GetActorLocation();
		const float remainingDistance = GetVectorLength(delta);
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (remainingDistance <= 0.001f || mLaunchSpeed <= 0.f)
		{
			SetActorLocation(mDeploymentTarget);
			SetVelocity({});
			mIsDeploying = false;
			return;
		}

		const sf::Vector2f direction = delta / remainingDistance;
		const float travelDistance = std::min(
			mLaunchSpeed * safeDeltaTime,
			remainingDistance
		);
		mLaunchVelocity = direction * mLaunchSpeed;
		SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
		SetVelocity(mLaunchVelocity);
		AddActorLocationOffset(direction * travelDistance);
		if (travelDistance >= remainingDistance - 0.001f)
		{
			SetActorLocation(mDeploymentTarget);
			SetVelocity({});
			mIsDeploying = false;
		}
	}

	List<shared_ptr<Actor>> MineLayerMineActor::FindTargets(float radius) const
	{
		List<shared_ptr<Actor>> targets;
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		const CollisionLayer opposingLayer = owner
			? targeting::ResolveOpposingLayer(*owner)
			: CollisionLayer::None;
		if (!world || !owner || opposingLayer == CollisionLayer::None || radius <= 0.f)
		{
			return targets;
		}

		targeting::TargetingQuery query;
		query.source = owner;
		query.origin = GetActorLocation();
		query.range = radius;
		query.requiredTargetLayers = opposingLayer;
		query.requireCollisionCompatibility = true;
		query.filter = [](
			const Actor*,
			const Actor& candidate,
			const targeting::TargetingCandidate&
		)
		{
			return dynamic_cast<const Combatant*>(&candidate) != nullptr;
		};

		for (const targeting::TargetingCandidate& candidate :
			targeting::AutoTargeting::FindTargets(*world, query))
		{
			if (candidate.actor)
			{
				targets.push_back(candidate.actor);
			}
		}
		return targets;
	}

	void MineLayerMineActor::ApplyControl(Actor& target)
	{
		Combatant* combatant = dynamic_cast<Combatant*>(&target);
		if (!combatant)
		{
			return;
		}

		const ControlResponse response = combatant->ResolveControlResponse(
			GameplayTags::State::Effect::Control::Stunned
		);
		if (response.mode == ControlResponseMode::Immune ||
			response.mode == ControlResponseMode::InterruptOnly)
		{
			return;
		}

		const sf::Vector2f direction = NormalizeOrDefault(
			target.GetActorLocation() - GetActorLocation()
		);
		// All external displacement enters through the shared movement influence
		// boundary. Ships receive collision-aware decaying impulses; legacy
		// movable combatants use the service's velocity compatibility path.
		movement::MovementInfluenceService::ApplyImpulse(
			target,
			movement::ImpulseRequest{ direction * mKnockbackStrength }
		);

		const sas::GameplayEffectDefinition* stunDefinition =
			EffectData::FindGameplayEffectDefinition(AbilityData::MineLayer::Effect::StunId);
		if (!stunDefinition)
		{
			return;
		}

		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*stunDefinition);
		spec.duration = std::max(
			0.f,
			mStunDuration * std::max(0.f, response.durationMultiplier)
		);
		spec.maxStacks = 1;
		if (spec.duration > 0.f)
		{
			combatant->GetAbilitySystemComponent().ApplyGameplayEffect(
				spec,
				sas::GameplayEffectSourceContext{ this }
			);
		}
	}

	void MineLayerMineActor::Trigger()
	{
		if (mTriggered)
		{
			return;
		}
		mTriggered = true;
		mExplosionAge = 0.f;
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);

		const List<shared_ptr<Actor>> targets = FindTargets(mExplosionRadius);
		for (const shared_ptr<Actor>& target : targets)
		{
			if (!target)
			{
				continue;
			}
			ApplyCombatDamage(
				*target,
				GetDamage(),
				GetOwnerActor(),
				GetDamageTags(),
				GetDamagePayload(),
				GetSourceAbilityId(),
				GetSourceAbilityTags()
			);
			ApplyControl(*target);
			++mAffectedTargetCount;
		}
	}

	void MineLayerMineActor::Tick(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(safeDeltaTime);
			return;
		}
		mVisualAge += safeDeltaTime;
		if (mTriggered)
		{
			mExplosionAge += safeDeltaTime;
			if (mExplosionAge >= mPresentationProfile.visual.explosionDuration)
			{
				Destroy();
			}
		}
		else if (mIsDeploying)
		{
			MoveTowardDeploymentTarget(safeDeltaTime);
		}
		else if (!GetIsPendingDestroy() && !FindTargets(mTriggerRadius).empty())
		{
			Trigger();
		}
		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void MineLayerMineActor::DrawIdle(sf::RenderWindow& window) const
	{
		const MineLayerVisualDefinition& visual = mPresentationProfile.visual;
		const sf::Vector2f location = GetActorLocation();
		const float pulse = 0.5f + 0.5f * std::sin(mVisualAge * visual.pulseSpeed);

		if (!mIsDeploying)
		{
			sf::CircleShape triggerRing(mTriggerRadius, 64);
			triggerRing.setOrigin({ mTriggerRadius, mTriggerRadius });
			triggerRing.setPosition(location);
			triggerRing.setFillColor(sf::Color::Transparent);
			triggerRing.setOutlineThickness(visual.triggerRingThickness);
			triggerRing.setOutlineColor(WithAlpha(visual.triggerRingColor, 0.65f + pulse * 0.35f));
			window.draw(triggerRing);
		}

		sf::CircleShape body(visual.bodyRadius, 24);
		body.setOrigin({ visual.bodyRadius, visual.bodyRadius });
		body.setPosition(location);
		body.setFillColor(WithAlpha(visual.bodyColor, 0.85f + pulse * 0.15f));
		window.draw(body);

		sf::CircleShape core(visual.coreRadius, 16);
		core.setOrigin({ visual.coreRadius, visual.coreRadius });
		core.setPosition(location);
		core.setFillColor(visual.coreColor);
		window.draw(core);
	}

	void MineLayerMineActor::DrawExplosion(sf::RenderWindow& window) const
	{
		const MineLayerVisualDefinition& visual = mPresentationProfile.visual;
		const float duration = std::max(0.001f, visual.explosionDuration);
		const float progress = std::clamp(mExplosionAge / duration, 0.f, 1.f);
		const float radius = std::max(visual.bodyRadius, mExplosionRadius) *
			(0.25f + 0.75f * progress) * visual.explosionEndScale;
		const sf::Vector2f location = GetActorLocation();

		sf::CircleShape flash(radius, 48);
		flash.setOrigin({ radius, radius });
		flash.setPosition(location);
		flash.setFillColor(WithAlpha(visual.explosionColor, 1.f - progress));
		window.draw(flash);

		sf::CircleShape ring(radius, 48);
		ring.setOrigin({ radius, radius });
		ring.setPosition(location);
		ring.setFillColor(sf::Color::Transparent);
		ring.setOutlineThickness(visual.explosionRingThickness);
		ring.setOutlineColor(WithAlpha(visual.explosionRingColor, 1.f - progress));
		window.draw(ring);
	}

	void MineLayerMineActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		Actor::Render(window);
		if (mTriggered)
		{
			DrawExplosion(window);
		}
		else
		{
			DrawIdle(window);
		}
	}

	bool RegisterMineLayerMineActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<MineLayerMineActorTypeHandler>()
		);
		return registered;
	}
}

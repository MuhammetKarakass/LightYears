#include "gameplay/ability/ionStorm/IonStormFieldActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/ionStorm/IonStormContracts.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include "gameplay/targeting/TargetingTypes.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> FieldCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius
		};

		const List<sas::AttributeId> FieldAttributeRoots{
			AbilityData::IonStorm::Actor::Field::Root
		};

		bool IsFiniteVector(const sf::Vector2f& value)
		{
			return std::isfinite(value.x) && std::isfinite(value.y);
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			return sf::Color{
				color.r,
				color.g,
				color.b,
				static_cast<std::uint8_t>(std::clamp(
					static_cast<float>(color.a) * std::max(0.f, multiplier),
					0.f,
					255.f
				))
			};
		}

		weak_ptr<Actor> MakeWeakActor(Actor* actor)
		{
			if (!actor)
			{
				return {};
			}
			const shared_ptr<Object> object = actor->GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<Actor>(object)
				: weak_ptr<Actor>{};
		}

		class IonStormFieldActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::IonStormField;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return FieldAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return FieldCommonAttributes;
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
					CommonAttributeIds::Duration,
					CommonAttributeIds::Radius,
					AbilityData::IonStorm::Attribute::TickInterval,
					AbilityData::IonStorm::Attribute::InnerCoreRadius,
					AbilityData::IonStorm::Attribute::OuterMinRadius,
					AbilityData::IonStorm::Attribute::OuterMaxRadius,
					AbilityData::IonStorm::Attribute::BoundaryPointCount
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Ion Storm field requires positive damage, lifetime, and boundary attributes."
						};
					}
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<
						IonStormFieldPresentationProfile
					>::Find(definition.presentationProfileId.ToString()) == nullptr)
				{
					return {
						false,
						"Ion Storm field requires a valid typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const IonStormFieldPresentationProfile* profile =
					PresentationProfileRegistry<
						IonStormFieldPresentationProfile
					>::Find(context.definition.presentationProfileId.ToString());
				return world && profile
					? world->SpawnActor<IonStormFieldActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	IonStormFieldActor::IonStormFieldActor(
		World* world,
		Actor* owner,
		const IonStormFieldPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
		, mOwnerReference(MakeWeakActor(owner))
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void IonStormFieldActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mDuration = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Duration,
			mDuration
		));
		mTickInterval = std::max(0.001f, sas::FindAttributeValue(
			attributes,
			AbilityData::IonStorm::Attribute::TickInterval,
			mTickInterval
		));
		const float radius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Radius,
			335.f
		));
		const float innerCoreRadius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::IonStorm::Attribute::InnerCoreRadius,
			250.f
		));
		const float outerMinRadius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::IonStorm::Attribute::OuterMinRadius,
			250.f
		));
		const float outerMaxRadius = std::max(
			radius,
			sas::FindAttributeValue(
				attributes,
				AbilityData::IonStorm::Attribute::OuterMaxRadius,
				335.f
			)
		);
		const int boundaryPointCount = std::max(
			3,
			static_cast<int>(std::lround(sas::FindAttributeValue(
				attributes,
				AbilityData::IonStorm::Attribute::BoundaryPointCount,
				20.f
			)))
		);

		mBoundary = IonStormBoundary::Generate(
			boundaryPointCount,
			innerCoreRadius,
			std::min(outerMinRadius, outerMaxRadius),
			outerMaxRadius
		);
		mDuration = std::max(0.f, mDuration);
		mMaximumTickCount = mDuration > 0.f
			? std::max(1, static_cast<int>(std::ceil(mDuration / mTickInterval)))
			: 0;
		mTickAccumulator = 0.f;
		mFieldAge = 0.f;
		mTickCount = 0;
		mVisualAge = 0.f;
		SetLifeTime(mDuration);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void IonStormFieldActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const shared_ptr<Actor> owner = mOwnerReference.lock();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mFieldAge += safeDeltaTime;
		mVisualAge += safeDeltaTime;
		const int remainingTicks = std::max(0, mMaximumTickCount - mTickCount);
		const int tickCount = time::ConsumePeriodicTicks(
			mTickAccumulator,
			safeDeltaTime,
			mTickInterval,
			remainingTicks
		);
		for (int tickIndex = 0; tickIndex < tickCount; ++tickIndex)
		{
			ApplyDamageTick(*owner);
			++mTickCount;
		}

		// Base actor lifetime handling runs after the last due tick, preserving
		// the intended 4.0 / 0.25 = 16 tick result at the lifetime boundary.
		AbilityWorldActor::Tick(deltaTime);
	}

	void IonStormFieldActor::ApplyDamageTick(Actor& owner)
	{
		World* world = GetWorld();
		const CollisionLayer opposingLayer = targeting::ResolveOpposingLayer(owner);
		if (!world || opposingLayer == CollisionLayer::None || GetDamage() <= 0.f)
		{
			return;
		}

		targeting::TargetingQuery query;
		query.source = &owner;
		query.origin = GetActorLocation();
		query.range = mBoundary.GetOuterMaxRadius();
		query.shape = targeting::TargetingShape::Radius;
		query.requiredTargetLayers = opposingLayer;
		query.requireCollisionCompatibility = true;
		query.filter = [this](
			const Actor*,
			const Actor& candidate,
			const targeting::TargetingCandidate&)
		{
			return IsEligibleTarget(candidate) &&
				mBoundary.Contains(candidate.GetActorLocation() - GetActorLocation());
		};

		for (const targeting::TargetingCandidate& candidate :
			targeting::AutoTargeting::FindTargets(*world, query))
		{
			const shared_ptr<Actor>& target = candidate.actor;
			if (!target || target->GetIsPendingDestroy())
			{
				continue;
			}
			ApplyCombatDamage(
				*target,
				GetDamage(),
				&owner,
				GetDamageTags(),
				GetDamagePayload(),
				GetSourceAbilityId(),
				GetSourceAbilityTags()
			);
		}
	}

	bool IonStormFieldActor::IsEligibleTarget(const Actor& actor) const
	{
		return !actor.GetIsPendingDestroy() &&
			dynamic_cast<const Combatant*>(&actor) != nullptr;
	}

	void IonStormFieldActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const std::size_t pointCount = static_cast<std::size_t>(std::max(
			3,
			mPresentationProfile.visual.renderPointCount
		));
		const List<sf::Vector2f> boundaryPoints = mBoundary.BuildBoundaryPoints(
			pointCount
		);
		const float pulse = 0.90f + 0.10f * std::sin(
			mVisualAge * mPresentationProfile.visual.pulseSpeed
		);
		const sf::Vector2f center = GetActorLocation();

		// One TriangleFan is deliberate: the gameplay boundary remains irregular,
		// but the presentation has no separate inner fill, perimeter overlay, or
		// internal energy lines.
		sf::VertexArray fieldShape(sf::PrimitiveType::TriangleFan, pointCount + 2);
		const sf::Color fillColor = WithAlpha(
			mPresentationProfile.visual.fillColor,
			pulse
		);

		fieldShape[0].position = center;
		fieldShape[0].color = fillColor;

		for (std::size_t index = 0; index <= pointCount; ++index)
		{
			const std::size_t boundaryIndex = index % pointCount;
			const sf::Vector2f outerPoint = center + boundaryPoints[boundaryIndex];
			fieldShape[index + 1].position = outerPoint;
			fieldShape[index + 1].color = fillColor;
		}

		window.draw(fieldShape);
	}

	void IonStormFieldActor::Destroy()
	{
		AbilityWorldActor::Destroy();
	}

	bool RegisterIonStormFieldActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<IonStormFieldActorTypeHandler>()
		);
	}
}

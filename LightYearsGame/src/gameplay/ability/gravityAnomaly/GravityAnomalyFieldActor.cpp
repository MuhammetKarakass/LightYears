#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h"

#include "framework/World.h"
#include "gameConfigs/ability/offensive/GravityAnomalyConfig.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"
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
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius
		};

		const List<sas::AttributeId> GravityAnomalyAttributeRoots{
			AbilityData::GravityAnomaly::Actor::Field::Root
		};

		class GravityAnomalyFieldActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::GravityAnomalyField;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return GravityAnomalyAttributeRoots;
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
					CommonAttributeIds::Duration,
					CommonAttributeIds::Radius,
					AbilityData::GravityAnomaly::Actor::Field::PullStrength,
					AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude,
					AbilityData::GravityAnomaly::Actor::Field::InsideEffectDuration
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(definition.attributes, required);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return { false, "Gravity Anomaly field requires a positive '" + std::string{ required.GetName() } + "' attribute." };
					}
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<GravityAnomalyFieldPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Gravity Anomaly field requires a valid typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const GravityAnomalyFieldPresentationProfile* profile =
					PresentationProfileRegistry<GravityAnomalyFieldPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<GravityAnomalyFieldActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	GravityAnomalyFieldActor::GravityAnomalyFieldActor(
		World* world,
		Actor* owner,
		const GravityAnomalyFieldPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
		, mCenter(1.f, 48)
		, mInnerRing(1.f, 48)
		, mOuterRing(1.f, 48)
		, mBoundary(1.f, 64)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		mInnerRing.setFillColor(sf::Color::Transparent);
		mOuterRing.setFillColor(sf::Color::Transparent);
		mBoundary.setFillColor(sf::Color::Transparent);
	}

	void GravityAnomalyFieldActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mDuration = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Duration,
			mDuration
		));
		mRadius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Radius,
			mRadius
		));
		mPullStrength = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::Actor::Field::PullStrength,
			mPullStrength
		));
		mSlowMagnitude = std::clamp(sas::FindAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude,
			mSlowMagnitude
		), 0.f, 0.95f);
		mInsideEffectDuration = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::Actor::Field::InsideEffectDuration,
			mInsideEffectDuration
		));
		SetLifeTime(mDuration);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		mRuntimeContext = std::make_shared<GravityAnomalyRuntimeContext>();
		mRuntimeContext->center = GetActorLocation();
		mRuntimeContext->resolvedRadius = mRadius;
		mRuntimeContext->resolvedPullStrength = mPullStrength;
		mRuntimeContext->sourceFieldScope = this;
		if (const sas::GameplayEffectDefinition* insideEffect =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::GravityAnomaly::Effect::InsideEffectId
			))
		{
			mInsideEffectSpec = sas::MakeGameplayEffectSpec(*insideEffect);
			mInsideEffectSpec.duration = mInsideEffectDuration;
			mInsideEffectSpec.modifiers = {
				sas::AttributeModifier{
					OwnerAttributeIds::MovementSlow,
					sas::AttributeModifierOperation::Add,
					mSlowMagnitude
				}
			};
		}
		else
		{
			mInsideEffectSpec = {};
		}
		ConfigureVisualGeometry();
	}

	void GravityAnomalyFieldActor::Tick(float deltaTime)
	{
		AbilityWorldActor::Tick(deltaTime);
		if (GetIsPendingDestroy())
		{
			return;
		}
		mVisualAge += std::max(0.f, deltaTime);
		if (mRuntimeContext)
		{
			mRuntimeContext->center = GetActorLocation();
		}
		UpdateAffectedTargets();
	}

	void GravityAnomalyFieldActor::Destroy()
	{
		ClearAppliedEffects();
		AbilityWorldActor::Destroy();
	}

	bool GravityAnomalyFieldActor::IsEligibleTarget(const Actor& actor) const
	{
		return !actor.GetIsPendingDestroy() && dynamic_cast<const Combatant*>(&actor) != nullptr;
	}

	void GravityAnomalyFieldActor::UpdateAffectedTargets()
	{
		LY_PROFILE_SCOPE("GravityAnomaly.TargetDiscovery");
		World* world = GetWorld();
		if (!world || !mRuntimeContext || mRadius <= 0.f)
		{
			return;
		}

		List<shared_ptr<Actor>> targetsInside;
		const float radiusSquared = mRadius * mRadius;
		for (const weak_ptr<Actor>& actorWeak : world->GetActorsByType<Actor>())
		{
			const shared_ptr<Actor> actor = actorWeak.lock();
			if (!actor || !IsEligibleTarget(*actor))
			{
				continue;
			}
			const sf::Vector2f delta = actor->GetActorLocation() - GetActorLocation();
			const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
			if (std::isfinite(distanceSquared) && distanceSquared <= radiusSquared)
			{
				targetsInside.push_back(actor);
			}
		}

		const auto updatePullContext = [this](sas::GameplayEffectRuntimeContext& baseContext)
		{
			auto* context = dynamic_cast<GravityAnomalyRuntimeContext*>(&baseContext);
			if (!context)
			{
				return;
			}
			context->center = GetActorLocation();
			context->resolvedRadius = mRadius;
			context->resolvedPullStrength = mPullStrength;
			context->pullEnabled = true;
		};
		const auto disablePull = [](sas::GameplayEffectRuntimeContext& baseContext)
		{
			if (auto* context =
				dynamic_cast<GravityAnomalyRuntimeContext*>(&baseContext))
			{
				context->pullEnabled = false;
			}
		};
		mEffectApplicator.Update(
			targetsInside,
			[](Actor& target, sas::GameplayEffectHandle handle)
			{
				auto* combatant = dynamic_cast<Combatant*>(&target);
				return combatant &&
					combatant->GetAbilitySystemComponent()
						.FindGameplayEffect(handle);
			},
			[](Actor& target, sas::GameplayEffectHandle handle)
			{
				if (auto* combatant = dynamic_cast<Combatant*>(&target))
				{
					combatant->GetAbilitySystemComponent()
						.RefreshGameplayEffectDuration(handle);
				}
			},
			[this](
				Actor& target,
				std::shared_ptr<sas::GameplayEffectRuntimeContext> runtimeContext
			)
			{
				auto* combatant = dynamic_cast<Combatant*>(&target);
				return combatant
					? combatant->GetAbilitySystemComponent()
						.ApplyGameplayEffect(
						mInsideEffectSpec,
						sas::GameplayEffectSourceContext{
							this,
							mRuntimeContext->sourceFieldScope,
							std::move(runtimeContext)
						}
					)
					: sas::GameplayEffectHandle{};
			},
			[this](Actor&)
			{
				return std::make_shared<GravityAnomalyRuntimeContext>(
					*mRuntimeContext
				);
			},
			updatePullContext,
			disablePull
		);
	}

	void GravityAnomalyFieldActor::ClearAppliedEffects()
	{
		mEffectApplicator.Clear([](sas::GameplayEffectRuntimeContext& baseContext)
		{
			if (auto* context =
				dynamic_cast<GravityAnomalyRuntimeContext*>(&baseContext))
			{
				context->pullEnabled = false;
			}
		});
	}

	void GravityAnomalyFieldActor::ConfigureVisualGeometry()
	{
		const GravityAnomalyFieldVisualDefinition& visual = mPresentationProfile.visual;
		const float innerRadius = std::max(1.f, mRadius * visual.innerRadiusRatio);
		for (sf::CircleShape* shape : { &mCenter, &mInnerRing, &mOuterRing, &mBoundary })
		{
			shape->setRadius(shape == &mInnerRing ? innerRadius : std::max(1.f, mRadius));
			shape->setOrigin({ shape->getRadius(), shape->getRadius() });
		}
		mInnerRing.setOutlineThickness(visual.ringThickness);
		mOuterRing.setOutlineThickness(visual.ringThickness);
		mBoundary.setOutlineThickness(visual.boundaryThickness);
		mInwardParticles.clear();
		mInwardParticles.reserve(static_cast<size_t>(std::max(0, visual.particleCount)));
		for (int index = 0; index < visual.particleCount; ++index)
		{
			sf::CircleShape particle{ 2.f + static_cast<float>(index % 3), 10 };
			particle.setOrigin({ particle.getRadius(), particle.getRadius() });
			mInwardParticles.push_back(std::move(particle));
		}
	}

	void GravityAnomalyFieldActor::Render(sf::RenderWindow& window)
	{
		Actor::Render(window);
		if (GetIsPendingDestroy() || mRadius <= 0.f)
		{
			return;
		}

		const GravityAnomalyFieldVisualDefinition& visual = mPresentationProfile.visual;
		const float expansion = visual.expansionDuration > 0.f
			? std::clamp(mVisualAge / visual.expansionDuration, 0.f, 1.f)
			: 1.f;
		const float remaining = std::max(0.f, mDuration - GetAge());
		const float collapse = visual.collapseDuration > 0.f
			? std::clamp(remaining / visual.collapseDuration, 0.f, 1.f)
			: 1.f;
		const float scale = expansion * collapse;
		const float pulse = 0.86f + 0.14f * std::sin(mVisualAge * 6.f);
		const sf::Vector2f location = GetActorLocation();

		mCenter.setPosition(location);
		mCenter.setFillColor(visual.centerColor);
		mCenter.setScale({ scale * pulse, scale * pulse });
		mInnerRing.setPosition(location);
		mInnerRing.setOutlineColor(visual.innerRingColor);
		mInnerRing.setRotation(sf::degrees(mVisualAge * visual.rotationSpeed));
		mInnerRing.setScale({ scale, scale });
		mOuterRing.setPosition(location);
		mOuterRing.setOutlineColor(visual.outerRingColor);
		mOuterRing.setRotation(sf::degrees(-mVisualAge * visual.rotationSpeed * 0.65f));
		mOuterRing.setScale({ scale * (0.96f + 0.04f * pulse), scale * (0.96f + 0.04f * pulse) });
		mBoundary.setPosition(location);
		mBoundary.setOutlineColor(visual.boundaryColor);
		mBoundary.setScale({ scale, scale });

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mCenter, additive);
		window.draw(mInnerRing, additive);
		window.draw(mOuterRing, additive);
		window.draw(mBoundary, additive);
		for (size_t index = 0; index < mInwardParticles.size(); ++index)
		{
			const float phase = std::fmod(
				mVisualAge * 0.72f + static_cast<float>(index) /
					static_cast<float>(mInwardParticles.size()),
				1.f
			);
			const float angle = mVisualAge * 1.8f +
				static_cast<float>(index) * 2.f * 3.1415926535f /
				static_cast<float>(mInwardParticles.size());
			const float orbitRadius = mRadius * (0.92f - 0.76f * phase) * scale;
			sf::CircleShape& particle = mInwardParticles[index];
			particle.setPosition(location + sf::Vector2f{
				std::cos(angle) * orbitRadius,
				std::sin(angle) * orbitRadius
			});
			particle.setFillColor(sf::Color{
				visual.outerRingColor.r,
				visual.outerRingColor.g,
				visual.outerRingColor.b,
				static_cast<std::uint8_t>(120.f * (1.f - phase) * collapse)
			});
			window.draw(particle, additive);
		}
	}

	bool RegisterGravityAnomalyFieldActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<GravityAnomalyFieldActorTypeHandler>()
		);
		return registered;
	}
}

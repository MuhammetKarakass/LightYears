#include "gameplay/ability/temporalConvergence/TemporalConvergenceFieldActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ly
{
	namespace
	{
		constexpr float Epsilon = 0.001f;

		const List<sas::AttributeId> FieldCommonAttributes{
			CommonAttributeIds::Duration
		};

		class TemporalConvergenceFieldActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::TemporalConvergenceField;
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
				const sas::GameplayAttribute* duration = sas::FindAttribute(
					definition.attributes, CommonAttributeIds::Duration
				);
				if (!duration || !std::isfinite(duration->baseValue) ||
					duration->baseValue <= 0.f ||
					!definition.presentationProfileId.IsValid() ||
					!PresentationProfileRegistry<
						TemporalConvergencePresentationProfile
					>::Find(definition.presentationProfileId.ToString()))
				{
					return {
						false,
						"Temporal Convergence field requires a duration and registered typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const auto* profile = PresentationProfileRegistry<
					TemporalConvergencePresentationProfile
				>::Find(context.definition.presentationProfileId.ToString());
				return world && profile
					? world->SpawnActor<TemporalConvergenceFieldActor>(
						&context.owner, *profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& id,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}

		sf::Color WithAlpha(const sf::Color& color, float alpha)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
			return result;
		}

		void ConfigureCircle(sf::CircleShape& circle, float radius)
		{
			const float safeRadius = std::max(1.f, radius);
			circle.setRadius(safeRadius);
			circle.setOrigin({ safeRadius, safeRadius });
		}
	}

	TemporalConvergenceFieldActor::TemporalConvergenceFieldActor(
		World* world,
		Actor* owner,
		const TemporalConvergencePresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
		, mDormantCore(1.f, 24)
		, mDormantGlow(1.f, 36)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void TemporalConvergenceFieldActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mActorLifetime = std::max(0.01f, FindValue(
			attributes, CommonAttributeIds::Duration, mActorLifetime
		));
		SetLifeTime(mActorLifetime);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void TemporalConvergenceFieldActor::ConfigureFromAbilityValues(
		const sas::GameplayAttributeList& values
	)
	{
		mInitialDelay = std::max(0.f, FindValue(
			values, AbilityData::TemporalConvergence::Attribute::InitialDelay, 2.f
		));
		mTravelSpeed = std::max(1.f, FindValue(
			values, AbilityData::TemporalConvergence::Attribute::TravelSpeed, 1000.f
		));
		mRadius = std::max(1.f, FindValue(
			values, AbilityData::TemporalConvergence::Attribute::Radius, 300.f
		));
		mFieldDuration = std::max(0.01f, FindValue(
			values, AbilityData::TemporalConvergence::Attribute::FieldDuration, 2.f
		));
		mSlowFraction = std::clamp(FindValue(
			values, AbilityData::TemporalConvergence::Attribute::SlowFraction, 0.40f
		), 0.f, 0.95f);
		mStunDuration = std::max(0.f, FindValue(
			values, AbilityData::TemporalConvergence::Attribute::StunDuration, 2.f
		));
		mGrantedShield = std::max(0.f, FindValue(
			values, AbilityData::TemporalConvergence::Attribute::BaseShield, 120.f
		));
		mOvershieldHoldDuration = std::max(0.f, FindValue(
			values,
			AbilityData::TemporalConvergence::Attribute::OvershieldHoldDuration,
			4.f
		));
		mOvershieldDecayPerSecond = std::max(0.f, FindValue(
			values,
			AbilityData::TemporalConvergence::Attribute::OvershieldDecayPerSecond,
			100.f
		));
		ConfigureCircle(mDormantCore, mPresentationProfile.dormantCoreRadius);
		ConfigureCircle(mDormantGlow, mPresentationProfile.dormantGlowRadius);
		mPhase = Phase::Dormant;
		mPhaseAge = 0.f;
		mVisualAge = 0.f;
		mTriggered = false;
	}

	void TemporalConvergenceFieldActor::SetSnapshotTarget(
		const sf::Vector2f& targetLocation
	)
	{
		mSnapshotTarget = targetLocation;
		mTargetConfigured = true;
		SpawnTargetTelegraph();
	}

	void TemporalConvergenceFieldActor::Tick(float deltaTime)
	{
		AbilityWorldActor::Tick(deltaTime);
		if (GetIsPendingDestroy())
		{
			return;
		}
		const float safeDeltaTime = std::max(0.f, deltaTime);
		mVisualAge += safeDeltaTime;
		mPhaseAge += safeDeltaTime;
		if (!mTargetConfigured)
		{
			return;
		}

		if (mPhase == Phase::Dormant)
		{
			if (mPhaseAge >= mInitialDelay)
			{
				BeginTravel();
			}
			return;
		}

		if (mPhase == Phase::Travelling)
		{
			const sf::Vector2f offset = mSnapshotTarget - GetActorLocation();
			const float distance = GetVectorLength(offset);
			const float step = mTravelSpeed * safeDeltaTime;
			if (distance <= std::max(Epsilon, step))
			{
				SetActorLocation(mSnapshotTarget);
				BeginField();
			}
			else
			{
				SetActorLocation(GetActorLocation() + offset / distance * step);
			}
			return;
		}

		UpdateSlowedTargets();
		TryTriggerPlayerEntry();
		if (mPhaseAge >= mFieldDuration)
		{
			Destroy();
		}
	}

	void TemporalConvergenceFieldActor::BeginTravel()
	{
		mPhase = Phase::Travelling;
		mPhaseAge = 0.f;
	}

	void TemporalConvergenceFieldActor::BeginField()
	{
		mPhase = Phase::Field;
		mPhaseAge = 0.f;
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTargetTelegraph.lock())
		{
			// Arrival confirms that the marked point is now an active field without
			// ending the warning before the player crosses into it.
			telegraph->SetExternalProgress(1.f);
		}
		UpdateSlowedTargets();
		TryTriggerPlayerEntry();
	}

	void TemporalConvergenceFieldActor::SpawnTargetTelegraph()
	{
		World* world = GetWorld();
		if (!world || !mTargetConfigured || mTargetTelegraph.lock())
		{
			return;
		}
		// External progress prevents a countdown timeout: this warning must stay at
		// the fixed snapshot location until the player actually enters the field.
		mTargetTelegraph = world->SpawnActor<AreaTelegraphActor>(
			AreaTelegraphActor::SpawnParams{
				mSnapshotTarget,
				mRadius,
				0.f,
				mPresentationProfile.targetTelegraph,
				AreaTelegraphAnchorMode::FixedLocation,
				AreaTelegraphProgressDriver::External,
				nullptr
			}
		);
	}

	void TemporalConvergenceFieldActor::CompleteTargetTelegraph()
	{
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTargetTelegraph.lock())
		{
			// The reusable telegraph owns the requested 0.5s small light explosion
			// and cleans itself after that feedback window.
			telegraph->Complete();
		}
	}

	bool TemporalConvergenceFieldActor::IsInsideField(const Actor& actor) const
	{
		const sf::Vector2f delta = actor.GetActorLocation() - GetActorLocation();
		return delta.x * delta.x + delta.y * delta.y <= mRadius * mRadius;
	}

	bool TemporalConvergenceFieldActor::IsSlowTracked(const Actor* target) const
	{
		return std::any_of(mSlowedTargets.begin(), mSlowedTargets.end(),
			[target](const SlowedTarget& tracked)
			{
				const shared_ptr<Actor> current = tracked.target.lock();
				return current && current.get() == target;
			});
	}

	void TemporalConvergenceFieldActor::UpdateSlowedTargets()
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner || mSlowFraction <= 0.f)
		{
			return;
		}

		const List<shared_ptr<Actor>> candidates = targeting::FindOpposingCombatants(
			*world, *owner, GetActorLocation(), mRadius
		);
		for (auto tracked = mSlowedTargets.begin(); tracked != mSlowedTargets.end();)
		{
			const shared_ptr<Actor> target = tracked->target.lock();
			const bool stillInside = target && !target->GetIsPendingDestroy() &&
				IsInsideField(*target);
			if (stillInside)
			{
				++tracked;
				continue;
			}
			if (target)
			{
				if (auto* combatant = dynamic_cast<Combatant*>(target.get()))
				{
					combatant->GetAbilitySystemComponent().RemoveGameplayEffect(
						tracked->handle
					);
				}
			}
			tracked = mSlowedTargets.erase(tracked);
		}

		const sas::GameplayEffectDefinition* slowDefinition =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::TemporalConvergence::Effect::SlowId
			);
		if (!slowDefinition)
		{
			return;
		}
		for (const shared_ptr<Actor>& candidate : candidates)
		{
			auto* combatant = candidate
				? dynamic_cast<Combatant*>(candidate.get())
				: nullptr;
			if (!combatant || IsSlowTracked(candidate.get()))
			{
				continue;
			}
			sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*slowDefinition);
			spec.duration = mFieldDuration;
			spec.maxStacks = 1;
			spec.modifiers = {
				sas::AttributeModifier{
					OwnerAttributeIds::MovementSlow,
					sas::AttributeModifierOperation::Add,
					mSlowFraction
				}
			};
			const sas::GameplayEffectHandle handle =
				combatant->GetAbilitySystemComponent().ApplyGameplayEffect(
					spec, sas::GameplayEffectSourceContext{ this, this }
				);
			if (handle.IsValid())
			{
				mSlowedTargets.push_back(SlowedTarget{ candidate, handle });
			}
		}
	}

	void TemporalConvergenceFieldActor::ClearSlowedTargets()
	{
		for (const SlowedTarget& tracked : mSlowedTargets)
		{
			if (const shared_ptr<Actor> target = tracked.target.lock())
			{
				if (auto* combatant = dynamic_cast<Combatant*>(target.get()))
				{
					combatant->GetAbilitySystemComponent().RemoveGameplayEffect(tracked.handle);
				}
			}
		}
		mSlowedTargets.clear();
	}

	void TemporalConvergenceFieldActor::TryTriggerPlayerEntry()
	{
		Actor* owner = GetOwnerActor();
		if (!mTriggered && owner && IsInsideField(*owner))
		{
			Trigger();
		}
	}

	void TemporalConvergenceFieldActor::ApplyStun(Combatant& target)
	{
		const ControlResponse response = target.ResolveControlResponse(
			GameplayTags::State::Effect::Control::Stunned
		);
		if (mStunDuration <= 0.f || response.mode == ControlResponseMode::Immune ||
			response.mode == ControlResponseMode::InterruptOnly)
		{
			return;
		}
		const sas::GameplayEffectDefinition* stunDefinition =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::TemporalConvergence::Effect::StunId
			);
		if (!stunDefinition)
		{
			return;
		}
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*stunDefinition);
		spec.duration = mStunDuration * std::max(0.f, response.durationMultiplier);
		spec.maxStacks = 1;
		if (spec.duration > 0.f)
		{
			target.GetAbilitySystemComponent().ApplyGameplayEffect(
				spec, sas::GameplayEffectSourceContext{ this }
			);
		}
	}

	void TemporalConvergenceFieldActor::Trigger()
	{
		if (mTriggered)
		{
			return;
		}
		mTriggered = true;
		CompleteTargetTelegraph();

		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return;
		}
		for (const shared_ptr<Actor>& candidate : targeting::FindOpposingCombatants(
			*world, *owner, GetActorLocation(), mRadius
		))
		{
			if (auto* target = candidate
				? dynamic_cast<Combatant*>(candidate.get())
				: nullptr)
			{
				ApplyStun(*target);
			}
		}

		if (SpaceShip* player = dynamic_cast<SpaceShip*>(owner))
		{
			// A 100 shield/sec decay rate makes the duration data-driven from actual
			// excess at hold end: 120 excess takes 1.2s, 10 takes 0.1s. The shared
			// ShieldComponent reconciles damage first, so lost shield never refills.
			player->GetShieldComponent().GrantTemporaryOvershield(
				GetSourceAbilityId().ToString(),
				mGrantedShield,
				mOvershieldHoldDuration,
				mOvershieldDecayPerSecond
			);
		}
	}

	void TemporalConvergenceFieldActor::Destroy()
	{
		ClearSlowedTargets();
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTargetTelegraph.lock())
		{
			telegraph->Destroy();
		}
		mTargetTelegraph.reset();
		AbilityWorldActor::Destroy();
	}

	void TemporalConvergenceFieldActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		const sf::Vector2f location = GetActorLocation();
		const float pulse = 0.80f + 0.20f * std::sin(mVisualAge * 7.f);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		if (mPhase == Phase::Dormant)
		{
			mDormantGlow.setPosition(location);
			mDormantGlow.setScale({ pulse, pulse });
			mDormantGlow.setFillColor(WithAlpha(
				mPresentationProfile.dormantGlowColor, 0.45f + pulse * 0.30f
			));
			mDormantCore.setPosition(location);
			mDormantCore.setScale({ 0.9f + 0.1f * pulse, 0.9f + 0.1f * pulse });
			mDormantCore.setFillColor(mPresentationProfile.dormantCoreColor);
			window.draw(mDormantGlow, additive);
			window.draw(mDormantCore, additive);
			return;
		}

		if (mPhase == Phase::Travelling)
		{
			const sf::Vector2f direction = mSnapshotTarget - location;
			const float length = GetVectorLength(direction);
			if (length > Epsilon)
			{
				const sf::Vector2f normalized = direction / length;
				const sf::Vertex line[]{
					sf::Vertex{ location - normalized * 65.f,
						WithAlpha(mPresentationProfile.travelTrailColor, 0.f) },
					sf::Vertex{ location,
						mPresentationProfile.travelTrailColor }
				};
				window.draw(line, 2, sf::PrimitiveType::Lines, additive);
			}
			mDormantCore.setPosition(location);
			mDormantCore.setFillColor(mPresentationProfile.dormantCoreColor);
			window.draw(mDormantCore, additive);
			return;
		}

		// The fixed AreaTelegraphActor is the sole field boundary. It has the
		// exact gameplay radius and owns the entry pulse, avoiding a duplicate
		// actor-local ring at the same location.
	}

	bool RegisterTemporalConvergenceFieldActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<TemporalConvergenceFieldActorTypeHandler>()
		);
	}
}

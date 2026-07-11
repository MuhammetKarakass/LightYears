#include "gameplay/ability/actors/sunBeam/SunBeamStrikeActor.h"

#include "framework/World.h"
#include "gameConfigs/VisualConfig.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"

#include <algorithm>
#include <memory>

namespace ly
{
	namespace
	{
		const List<GameplayTag> SunBeamStrikeCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Radius
		};

		const List<GameplayTag> SunBeamStrikeAttributeRoots{
			AbilityActorSchema::SunBeam::SharedAttributeRoot,
			AbilityActorSchema::SunBeam::Strike::AttributeRoot
		};

		class SunBeamStrikeActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			const GameplayTag& GetActorTypeTag() const override
			{
				return AbilityActorSchema::SunBeam::Strike::TypeId;
			}

			const List<GameplayTag>& GetOwnedAttributeRoots() const override
			{
				return SunBeamStrikeAttributeRoots;
			}

			const List<GameplayTag>& GetAllowedCommonAttributeIds() const override
			{
				return SunBeamStrikeCommonAttributes;
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

				for (const GameplayTag& required : {
					CommonAttributeIds::Damage,
					CommonAttributeIds::Radius,
					AbilityActorSchema::SunBeam::Width,
					AbilityActorSchema::SunBeam::Length
				})
				{
					const GameplayAttribute* attribute = FindGameplayAttribute(
						definition.attributes,
						required
					);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Sun Beam strike requires a positive '" + required.ToString() + "' attribute."
						};
					}
				}

				if (definition.telegraphVisualId.empty()
					|| VisualData::FindAreaTelegraphVisualDefinition(definition.telegraphVisualId) == nullptr)
				{
					return { false, "Sun Beam strike requires a valid circular telegraph visual." };
				}
				if (definition.visualId.empty()
					|| VisualData::FindSunBeamVisualDefinition(definition.visualId) == nullptr)
				{
					return { false, "Sun Beam strike requires a valid Sun Beam visual." };
				}

				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const AreaTelegraphVisualDefinition* telegraphVisualDefinition =
					VisualData::FindAreaTelegraphVisualDefinition(context.definition.telegraphVisualId);
				const SunBeamVisualDefinition* sunBeamVisualDefinition =
					VisualData::FindSunBeamVisualDefinition(context.definition.visualId);
				return world
					&& telegraphVisualDefinition
					&& sunBeamVisualDefinition
					? world->SpawnActor<SunBeamStrikeActor>(
						&context.owner,
						*telegraphVisualDefinition,
						*sunBeamVisualDefinition
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	SunBeamStrikeActor::SunBeamStrikeActor(
		World* world,
		Actor* owner,
		const AreaTelegraphVisualDefinition& telegraphVisualDefinition,
		const SunBeamVisualDefinition& sunBeamVisualDefinition
	)
		: SunBeamActorBase(world, owner, sunBeamVisualDefinition),
		mTelegraphVisualDefinition(telegraphVisualDefinition)
	{
	}

	void SunBeamStrikeActor::Destroy()
	{
		DestroyTelegraph();
		SunBeamActorBase::Destroy();
	}

	void SunBeamStrikeActor::OnSunBeamBeginPlay()
	{
		SetActorRotation(0.f);
		SpawnTelegraph();

		if (mPhase == StrikePhase::Telegraph)
		{
			return;
		}

		BeginArrival();
	}

	void SunBeamStrikeActor::ConfigureSunBeam(
		const GameplayAttributeList& attributes
	)
	{
		SetAbilityPhysicsEnabled(false);
		SetLifeTime(0.f);

		mImpactLocation = GetActorLocation();
		mImpactRadius = std::max(
			1.f,
			FindGameplayAttributeValue(attributes, CommonAttributeIds::Radius, 1.f)
		);
		mTelegraphDuration = std::max(
			0.f,
			FindGameplayAttributeValue(
				attributes,
				AbilityActorSchema::SunBeam::Strike::TelegraphDuration,
				0.f
			)
		);
		mArrivalDuration = std::max(
			0.f,
			FindGameplayAttributeValue(
				attributes,
				AbilityActorSchema::SunBeam::Strike::ArrivalDuration,
				0.f
			)
		);
		mImpactVisualDuration = std::max(
			0.f,
			FindGameplayAttributeValue(
				attributes,
				AbilityActorSchema::SunBeam::Strike::ImpactVisualDuration,
				0.f
			)
		);

		mPhaseElapsed = 0.f;
		mHasImpacted = false;
		mPhase = mTelegraphDuration > 0.f
			? StrikePhase::Telegraph
			: StrikePhase::Arrival;
	}

	void SunBeamStrikeActor::TickSunBeam(float deltaTime)
	{
		if (mPhase == StrikePhase::Impact)
		{
			mPhaseElapsed += deltaTime;
			if (mPhaseElapsed >= mImpactVisualDuration)
			{
				Destroy();
			}
			return;
		}

		if (mPhase == StrikePhase::Telegraph)
		{
			mPhaseElapsed += deltaTime;
			if (mPhaseElapsed >= mTelegraphDuration)
			{
				BeginArrival();
			}
			return;
		}

		if (mArrivalDuration <= 0.f)
		{
			BeginImpact();
			return;
		}

		mPhaseElapsed += deltaTime;
		const float progress = std::clamp(
			mPhaseElapsed / mArrivalDuration,
			0.f,
			1.f
		);
		if (progress >= 1.f)
		{
			BeginImpact();
		}
	}

	void SunBeamStrikeActor::BeginArrival()
	{
		mPhase = StrikePhase::Arrival;
		mPhaseElapsed = 0.f;

		if (mArrivalDuration <= 0.f)
		{
			BeginImpact();
			return;
		}

	}

	void SunBeamStrikeActor::BeginImpact()
	{
		if (mHasImpacted)
		{
			return;
		}

		mHasImpacted = true;
		mPhase = StrikePhase::Impact;
		mPhaseElapsed = 0.f;

		ApplyCombatDamageInRadius(mImpactLocation, mImpactRadius);
		DestroyTelegraph();

		if (mImpactVisualDuration <= 0.f)
		{
			Destroy();
		}
	}

	SunBeamVisualFrame SunBeamStrikeActor::BuildSunBeamVisualFrame() const
	{
		SunBeamVisualFrame frame;
		const SunBeamVisualDefinition& definition = GetSunBeamVisualDefinition();

		if (mPhase == StrikePhase::Telegraph)
		{
			return frame;
		}

		if (mPhase == StrikePhase::Arrival)
		{
			const float rawProgress = mArrivalDuration > 0.f
				? std::clamp(mPhaseElapsed / mArrivalDuration, 0.f, 1.f)
				: 1.f;
			const float progress = rawProgress * rawProgress * (3.f - 2.f * rawProgress);

			frame.overhead = SunBeamOverheadVisualState{
				SunBeamOverheadVisualStage::Arrival,
				progress,
				0.35f + 0.65f * progress
			};
			frame.groundGlow = SunBeamGroundGlowVisualState{
				definition.groundGlowStartScale
					+ (definition.groundGlowEndScale - definition.groundGlowStartScale) * progress,
				0.25f + 0.75f * progress
			};
			return frame;
		}

		const float impactProgress = mImpactVisualDuration > 0.f
			? std::clamp(mPhaseElapsed / mImpactVisualDuration, 0.f, 1.f)
			: 1.f;
		frame.overhead = SunBeamOverheadVisualState{
			SunBeamOverheadVisualStage::Impact,
			impactProgress,
			1.f
		};
		frame.groundGlow = SunBeamGroundGlowVisualState{
			definition.groundGlowEndScale + 0.35f * impactProgress,
			1.f - 0.8f * impactProgress
		};
		frame.radialPulse = SunBeamRadialPulseVisualState{
			impactProgress,
			1.f
		};
		return frame;
	}

	void SunBeamStrikeActor::SpawnTelegraph()
	{
		World* world = GetWorld();
		const float warningDuration = mTelegraphDuration + mArrivalDuration;
		if (world == nullptr || warningDuration <= 0.f)
		{
			return;
		}

		mTelegraph = world->SpawnActor<AreaTelegraphActor>(
			mImpactLocation,
			mImpactRadius,
			warningDuration + 0.1f,
			mTelegraphVisualDefinition
		);
	}

	void SunBeamStrikeActor::DestroyTelegraph()
	{
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Destroy();
		}
		mTelegraph.reset();
	}

	bool RegisterSunBeamStrikeActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<SunBeamStrikeActorTypeHandler>()
		);
		return registered;
	}
}

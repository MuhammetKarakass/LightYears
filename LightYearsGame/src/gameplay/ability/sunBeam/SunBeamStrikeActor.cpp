#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/sunBeam/SunBeamStrikeActor.h"

#include "framework/World.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"

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
			AbilityData::SunBeam::ActorSchema::SharedAttributeRoot,
			AbilityData::SunBeam::ActorSchema::Strike::AttributeRoot
		};

		class SunBeamStrikeActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			const GameplayTag& GetActorTypeTag() const override
			{
				return AbilityData::SunBeam::ActorSchema::Strike::TypeId;
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
					AbilityData::SunBeam::ActorSchema::Width,
					AbilityData::SunBeam::ActorSchema::Length
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindGameplayAttribute(
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

				if (definition.presentationProfileId.empty()
					|| PresentationProfileRegistry<SunBeamPresentationProfile>::Find(
						definition.presentationProfileId
					) == nullptr)
				{
					return { false, "Sun Beam strike requires a valid presentation profile." };
				}

				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const SunBeamPresentationProfile* presentationProfile =
					PresentationProfileRegistry<SunBeamPresentationProfile>::Find(
						context.definition.presentationProfileId
					);
				return world && presentationProfile
					? world->SpawnActor<SunBeamStrikeActor>(
						&context.owner,
						*presentationProfile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	SunBeamStrikeActor::SunBeamStrikeActor(
		World* world,
		Actor* owner,
		const SunBeamPresentationProfile& presentationProfile
	)
		: SunBeamActorBase(world, owner, presentationProfile.visual),
		mTelegraphVisualDefinition(presentationProfile.telegraph)
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
		UpdatePreImpactTimeline();
	}

	void SunBeamStrikeActor::ConfigureSunBeam(
		const sas::GameplayAttributeList& attributes
	)
	{
		SetAbilityPhysicsEnabled(false);
		ConfigureCollisionFromOwner();
		SetLifeTime(0.f);

		mImpactLocation = GetActorLocation();
		mImpactRadius = std::max(
			1.f,
			sas::FindGameplayAttributeValue(attributes, CommonAttributeIds::Radius, 1.f)
		);
		mTelegraphDuration = std::max(
			0.f,
			sas::FindGameplayAttributeValue(
				attributes,
				AbilityData::SunBeam::ActorSchema::Strike::TelegraphDuration,
				0.f
			)
		);
		mArrivalDuration = std::max(
			0.f,
			sas::FindGameplayAttributeValue(
				attributes,
				AbilityData::SunBeam::ActorSchema::Strike::ArrivalDuration,
				0.f
			)
		);
		mImpactDelay = std::max(
			0.f,
			sas::FindGameplayAttributeValue(
				attributes,
				AbilityData::SunBeam::ActorSchema::Strike::ImpactDelay,
				0.f
			)
		);
		mImpactVisualDuration = std::max(
			0.f,
			sas::FindGameplayAttributeValue(
				attributes,
				AbilityData::SunBeam::ActorSchema::Strike::ImpactVisualDuration,
				0.f
			)
		);

		mTimelineElapsed = 0.f;
		mPhaseElapsed = 0.f;
		mHasImpacted = false;
		mPhase = StrikePhase::Telegraph;
	}

	void SunBeamStrikeActor::TickSunBeam(float deltaTime)
	{
		if (mPhase == StrikePhase::Impact)
		{
			mPhaseElapsed += std::max(0.f, deltaTime);
			if (mPhaseElapsed >= mImpactVisualDuration)
			{
				Destroy();
			}
			return;
		}

		mTimelineElapsed += std::max(0.f, deltaTime);
		UpdatePreImpactTimeline();
	}

	void SunBeamStrikeActor::UpdatePreImpactTimeline()
	{
		const float arrivalStart = mTelegraphDuration;
		const float settleStart = arrivalStart + mArrivalDuration;
		const float impactStart = settleStart + mImpactDelay;

		if (mTimelineElapsed < arrivalStart)
		{
			mPhase = StrikePhase::Telegraph;
			mPhaseElapsed = mTimelineElapsed;
			SynchronizeTelegraph();
			return;
		}

		if (mTimelineElapsed < settleStart)
		{
			mPhase = StrikePhase::Arrival;
			mPhaseElapsed = mTimelineElapsed - arrivalStart;
			SynchronizeTelegraph();
			return;
		}

		if (mTimelineElapsed < impactStart)
		{
			mPhase = StrikePhase::Settle;
			mPhaseElapsed = mTimelineElapsed - settleStart;
			SynchronizeTelegraph();
			return;
		}

		BeginImpact(mTimelineElapsed - impactStart);
	}

	void SunBeamStrikeActor::BeginImpact(float impactElapsed)
	{
		if (mHasImpacted)
		{
			return;
		}

		mHasImpacted = true;
		mPhase = StrikePhase::Impact;
		mPhaseElapsed = std::max(0.f, impactElapsed);

		ApplyCombatDamageInRadius(mImpactLocation, mImpactRadius);
		DestroyTelegraph();
		if (World* world = GetWorld())
		{
			const SunBeamVisualDefinition& definition = GetSunBeamVisualDefinition();
			world->PlayCameraShake(
				definition.screenShakeAmplitude,
				definition.screenShakeDuration,
				definition.screenShakeFrequency
			);
		}

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

		if (mPhase == StrikePhase::Arrival || mPhase == StrikePhase::Settle)
		{
			const float rawProgress = mPhase == StrikePhase::Settle
				? 1.f
				: mArrivalDuration > 0.f
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
		const float flashProgress = definition.impactFlashDuration > 0.f
			? std::clamp(mPhaseElapsed / definition.impactFlashDuration, 0.f, 1.f)
			: 1.f;
		const float flashFade = 1.f - flashProgress * flashProgress * (3.f - 2.f * flashProgress);
		const float recoveryFade = 1.f - impactProgress * impactProgress * (3.f - 2.f * impactProgress);
		frame.overhead = SunBeamOverheadVisualState{
			SunBeamOverheadVisualStage::Impact,
			impactProgress,
			recoveryFade,
			flashFade
		};
		frame.groundGlow = SunBeamGroundGlowVisualState{
			definition.groundGlowEndScale + 0.35f * impactProgress,
			recoveryFade
		};
		frame.radialPulse = SunBeamRadialPulseVisualState{
			impactProgress,
			recoveryFade
		};
		return frame;
	}

	void SunBeamStrikeActor::SpawnTelegraph()
	{
		World* world = GetWorld();
		const float warningDuration = mTelegraphDuration + mArrivalDuration + mImpactDelay;
		if (world == nullptr || warningDuration <= 0.f)
		{
			return;
		}

		mTelegraph = world->SpawnActor<AreaTelegraphActor>(
			mImpactLocation,
			mImpactRadius,
			warningDuration + 0.25f,
			mTelegraphVisualDefinition
		);
		SynchronizeTelegraph();
	}

	void SunBeamStrikeActor::SynchronizeTelegraph()
	{
		const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock();
		if (!telegraph)
		{
			return;
		}

		const float convergenceDuration = mTelegraphDuration + mArrivalDuration;
		const float progress = convergenceDuration > 0.f
			? std::clamp(mTimelineElapsed / convergenceDuration, 0.f, 1.f)
			: 1.f;
		telegraph->SetCountdownProgress(progress);
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

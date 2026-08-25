#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/portal/PortalTransferParticipant.h"

#include <algorithm>
#include <memory>

namespace ly
{
	namespace
	{
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
	}

	AreaTelegraphActor::AreaTelegraphActor(
		World* world,
		const SpawnParams& params
	)
		: Actor(world),
		mDefinition(params.visual),
		mFill(std::max(1.f, params.radius), 64),
		mCountdownRing(std::max(1.f, params.radius), 64),
		mOutline(std::max(1.f, params.radius), 64),
		mDuration(std::max(0.f, params.duration)),
		mAnchor(params.anchor),
		mProgressDriver(params.progress),
		mTargetActor(MakeWeakActor(params.targetActor))
	{
		SetActorLocation(params.targetActor ? params.targetActor->GetActorLocation() : params.location);
		SetRenderLayer(RenderLayer::GroundDecal);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);

		const float effectiveRadius = std::max(1.f, params.radius);
		mFill.setOrigin({ effectiveRadius, effectiveRadius });
		mCountdownRing.setOrigin({ effectiveRadius, effectiveRadius });
		mOutline.setOrigin({ effectiveRadius, effectiveRadius });
		UpdateVisuals();
	}

	AreaTelegraphActor::AreaTelegraphActor(
		World* world,
		const sf::Vector2f& worldLocation,
		float radius,
		float lifeTime,
		const AreaTelegraphVisualDefinition& definition,
		Actor* targetActor
	)
		: AreaTelegraphActor(
			world,
			SpawnParams{
				worldLocation,
				radius,
				lifeTime,
				definition,
				targetActor ? AreaTelegraphAnchorMode::FollowActor : AreaTelegraphAnchorMode::FixedLocation,
				AreaTelegraphProgressDriver::Timed,
				targetActor
			}
		)
	{
	}

	void AreaTelegraphActor::Tick(float deltaTime)
	{
		if (mAnchor == AreaTelegraphAnchorMode::FollowActor)
		{
			const shared_ptr<Actor> targetActor = mTargetActor.lock();
			if (!targetActor || targetActor->GetIsPendingDestroy())
			{
				Destroy();
				return;
			}
			if (const auto* participant = dynamic_cast<const PortalTransferParticipant*>(
				targetActor.get()); participant && participant->IsInPortalTransit())
			{
				// A follow telegraph is owner-bound feedback, not a world effect.
				// Hide and pause it until its owner has fully left the portal.
				SetRenderEnabled(false);
				return;
			}

			SetRenderEnabled(true);
			SetActorLocation(targetActor->GetActorLocation());
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mAge += safeDeltaTime;
		if (mPhase == AreaTelegraphPhase::CompletionFeedback)
		{
			mCompletionAge += safeDeltaTime;
			if (mDefinition.completionFeedbackDuration <= 0.f ||
				mCompletionAge >= mDefinition.completionFeedbackDuration)
			{
				Destroy();
				return;
			}

			UpdateVisuals();
			Actor::Tick(deltaTime);
			return;
		}

		// Timed actors own their lifetime. External actors are ended explicitly by
		// their ability, which keeps impact and completion frames deterministic.
		if (mProgressDriver == AreaTelegraphProgressDriver::Timed &&
			mDuration > 0.f && mAge >= mDuration)
		{
			Destroy();
			return;
		}

		UpdateVisuals();
		Actor::Tick(deltaTime);
	}

	void AreaTelegraphActor::SetExternalProgress(float normalizedProgress)
	{
		if (GetIsPendingDestroy() || mPhase != AreaTelegraphPhase::Countdown)
		{
			return;
		}

		mProgressDriver = AreaTelegraphProgressDriver::External;
		mCountdownProgress = SaturateAreaTelegraphProgress(normalizedProgress);
		UpdateVisuals();
	}

	void AreaTelegraphActor::Complete(float normalizedProgress)
	{
		if (GetIsPendingDestroy() || mPhase != AreaTelegraphPhase::Countdown)
		{
			return;
		}

		const float feedbackDuration = std::max(0.f, mDefinition.completionFeedbackDuration);
		if (feedbackDuration <= 0.f)
		{
			Destroy();
			return;
		}

		mProgressDriver = AreaTelegraphProgressDriver::External;
		mCountdownProgress = SaturateAreaTelegraphProgress(normalizedProgress);
		mCompletionProgress = mCountdownProgress;
		mCompletionAge = 0.f;
		mPhase = AreaTelegraphPhase::CompletionFeedback;
		UpdateVisuals();
	}

	bool AreaTelegraphActor::IsInCompletionFeedback() const
	{
		return mPhase == AreaTelegraphPhase::CompletionFeedback && !GetIsPendingDestroy();
	}

	void AreaTelegraphActor::SetCountdownProgress(float normalizedProgress)
	{
		SetExternalProgress(normalizedProgress);
	}

	void AreaTelegraphActor::SetCompleted()
	{
		Complete();
	}

	void AreaTelegraphActor::SetCompleted(float normalizedProgress)
	{
		Complete(normalizedProgress);
	}

	bool AreaTelegraphActor::IsShowingCompletionFeedback() const
	{
		return IsInCompletionFeedback();
	}

	void AreaTelegraphActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || !IsRenderEnabled())
		{
			return;
		}

		Actor::Render(window);

		const sf::Vector2f location = GetActorLocation();
		mFill.setPosition(location);
		mCountdownRing.setPosition(location);
		mOutline.setPosition(location);
		if (mDefinition.drawInteriorFill)
		{
			window.draw(mFill);
		}
		if (mDefinition.drawCountdownRing)
		{
			window.draw(mCountdownRing);
		}
		window.draw(mOutline);
	}

	void AreaTelegraphActor::UpdateVisuals()
	{
		const float countdownProgress = mProgressDriver == AreaTelegraphProgressDriver::External
			? mCountdownProgress
			: (mDuration > 0.f ? SaturateAreaTelegraphProgress(mAge / mDuration) : 0.f);
		const AreaTelegraphRuntimeState runtime{
			mPhase,
			mAge,
			countdownProgress,
			mCompletionProgress,
			mCompletionAge
		};
		const AreaTelegraphVisualState state = ResolveAreaTelegraphVisualState(mDefinition, runtime);

		mFill.setFillColor(state.fillColor);
		mFill.setScale({ state.fillScale, state.fillScale });

		mCountdownRing.setFillColor(sf::Color::Transparent);
		mCountdownRing.setOutlineColor(state.outlineColor);
		mCountdownRing.setOutlineThickness(std::max(0.f, mDefinition.countdownRingThickness));
		mCountdownRing.setScale({ state.countdownRingScale, state.countdownRingScale });

		mOutline.setFillColor(sf::Color::Transparent);
		mOutline.setOutlineColor(state.outlineColor);
		mOutline.setOutlineThickness(mDefinition.outlineThickness);
		mOutline.setScale({ state.outlineScale, state.outlineScale });
	}
}

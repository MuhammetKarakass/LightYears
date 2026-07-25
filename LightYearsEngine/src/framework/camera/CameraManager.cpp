#include "framework/camera/CameraManager.h"
#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		float SmoothDampScalar(
			float current,
			float target,
			float& currentVelocity,
			float smoothTime,
			float deltaTime)
		{
			if (deltaTime <= 0.f)
			{
				return current;
			}

			const float safeSmoothTime = std::max(0.0001f, smoothTime);
			const float angularFrequency = 2.f / safeSmoothTime;
			const float scaledDelta = angularFrequency * deltaTime;
			const float decay = 1.f / (
				1.f +
				scaledDelta +
				0.48f * scaledDelta * scaledDelta +
				0.235f * scaledDelta * scaledDelta * scaledDelta
			);

			const float originalTarget = target;
			const float displacement = current - target;
			const float velocityStep =
				(currentVelocity + angularFrequency * displacement) * deltaTime;
			currentVelocity =
				(currentVelocity - angularFrequency * velocityStep) * decay;
			float result = target + (displacement + velocityStep) * decay;

			// Numerical protection only; a critically damped response should not cross its target.
			if ((originalTarget - current) * (result - originalTarget) > 0.f)
			{
				result = originalTarget;
				currentVelocity = 0.f;
			}
			return result;
		}
	}

	void CameraManager::SetSettings(const CameraSettings& settings)
	{
		mSettings = settings;
	}

	void CameraManager::SetFollowTarget(weak_ptr<Actor> target)
	{
		mFollowTarget = target;
		mPreviousFollowTargetLocation.reset();
		mCurrentZoomVelocity = 0.f;
	}

	void CameraManager::ClearFollowTarget()
	{
		mFollowTarget.reset();
		mPreviousFollowTargetLocation.reset();
		mPreserveFollowTargetOffset = false;
		mCurrentZoomVelocity = 0.f;
		ClearExternalVelocity();
		SetAdditionalZoomOut(0.f);
		SetRelativeAdditionalZoomOut(0.f);
		ClearLookAheadWorldPosition();
	}

	void CameraManager::SetExternalVelocity(const sf::Vector2f& velocity)
	{
		mExternalVelocity = velocity;
	}

	void CameraManager::ClearExternalVelocity()
	{
		mExternalVelocity = sf::Vector2f{};
	}

	void CameraManager::SetAdditionalZoomOut(float zoomOut)
	{
		mAdditionalZoomOut = std::max(0.f, zoomOut);
	}

	void CameraManager::SetRelativeAdditionalZoomOut(float zoomOutRatio)
	{
		mRelativeAdditionalZoomOut = std::max(0.f, zoomOutRatio);
	}

	void CameraManager::SetLookAheadWorldPosition(const std::optional<sf::Vector2f>& worldPosition)
	{
		mLookAheadWorldPosition = worldPosition;
	}

	void CameraManager::ClearLookAheadWorldPosition()
	{
		mLookAheadWorldPosition.reset();
	}

	void CameraManager::SetWorldBounds(const sf::FloatRect& bounds)
	{
		mWorldBounds = bounds;
	}

	void CameraManager::ClearWorldBounds()
	{
		mWorldBounds.reset();
	}

	void CameraManager::PlayShake(float amplitude, float duration, float frequency)
	{
		const float clampedAmplitude = std::max(0.f, amplitude);
		const float clampedDuration = std::max(0.f, duration);
		if (clampedAmplitude <= 0.f || clampedDuration <= 0.f)
		{
			return;
		}

		const float remainingDuration = std::max(0.f, mShakeDuration - mShakeElapsed);
		mShakeAmplitude = std::max(mShakeAmplitude, clampedAmplitude);
		mShakeDuration = std::max(remainingDuration, clampedDuration);
		mShakeFrequency = std::max(mShakeFrequency, std::max(0.f, frequency));
		mShakeElapsed = 0.f;
	}

	void CameraManager::Update(float deltaTime, const sf::View& defaultView)
	{
		if (mShakeDuration > 0.f)
		{
			mShakeElapsed = std::min(mShakeDuration, mShakeElapsed + std::max(0.f, deltaTime));
			if (mShakeElapsed >= mShakeDuration)
			{
				mShakeAmplitude = 0.f;
				mShakeDuration = 0.f;
				mShakeElapsed = 0.f;
				mShakeFrequency = 0.f;
			}
		}

		auto followTarget = mFollowTarget.lock();
		if (!followTarget || followTarget->GetIsPendingDestroy())
		{
			mPreviousFollowTargetLocation.reset();
			if (!mHasState)
			{
				mCurrentCenter = defaultView.getCenter();
				mCurrentZoom = 1.f;
				mCurrentZoomVelocity = 0.f;
				mHasState = true;
			}
			return;
		}

		const float desiredZoom = GetDesiredZoom();
		const sf::Vector2f followTargetLocation = followTarget->GetActorLocation();

		if (!mHasState)
		{
			mCurrentZoom = desiredZoom;
			mCurrentZoomVelocity = 0.f;
			mCurrentCenter = ClampCenterToWorldBounds(
				GetDesiredCenter(*followTarget, deltaTime),
				defaultView.getSize() * mCurrentZoom
			);
			mHasState = true;
			mPreviousFollowTargetLocation = followTargetLocation;
			return;
		}

		const float positionAlpha = 1.f - std::exp(-std::max(0.f, mSettings.positionSmoothingSpeed) * deltaTime);
		const float zoomSmoothingSpeed = std::max(0.f, mSettings.zoomSmoothingSpeed);
		if (zoomSmoothingSpeed > 0.f)
		{
			mCurrentZoom = SmoothDampScalar(
				mCurrentZoom,
				desiredZoom,
				mCurrentZoomVelocity,
				1.f / zoomSmoothingSpeed,
				deltaTime
			);
		}
		else
		{
			mCurrentZoomVelocity = 0.f;
		}
		const sf::Vector2f currentViewSize = defaultView.getSize() * mCurrentZoom;

		if (mPreserveFollowTargetOffset && mPreviousFollowTargetLocation)
		{
			mCurrentCenter += followTargetLocation - *mPreviousFollowTargetLocation;
		}
		const sf::Vector2f desiredCenter = ClampCenterToWorldBounds(
			GetDesiredCenter(*followTarget, deltaTime),
			currentViewSize
		);
		mCurrentCenter = ClampCenterToWorldBounds(
			LerpVector(mCurrentCenter, desiredCenter, positionAlpha),
			currentViewSize
		);
		mPreviousFollowTargetLocation = followTargetLocation;
	}

	sf::View CameraManager::GetView(const sf::View& defaultView) const
	{
		sf::View view = defaultView;
		if (mHasState)
		{
			view.setCenter(mCurrentCenter);
			view.setSize(defaultView.getSize() * mCurrentZoom);
		}
		view.setCenter(view.getCenter() + GetShakeOffset());
		return view;
	}

	sf::Vector2f CameraManager::GetShakeOffset() const
	{
		if (mShakeDuration <= 0.f || mShakeElapsed >= mShakeDuration)
		{
			return {};
		}

		const float remaining = 1.f - std::clamp(mShakeElapsed / mShakeDuration, 0.f, 1.f);
		const float envelope = remaining * remaining;
		const float phase = mShakeElapsed * mShakeFrequency;
		return {
			std::sin(phase) * mShakeAmplitude * envelope,
			std::sin(phase * 1.37f + 1.1f) * mShakeAmplitude * 0.65f * envelope
		};
	}

	sf::Vector2f CameraManager::GetDesiredCenter(const Actor& followTarget, float deltaTime)
	{
		const sf::Vector2f targetLocation = followTarget.GetActorLocation();
		sf::Vector2f cursorLookAheadOffset{};
		sf::Vector2f movementLookAheadOffset{};

		const float velocityLength = GetVectorLength(mExternalVelocity);
		if (velocityLength > 0.001f)
		{
			const float speedForMaxLookAhead = std::max(0.01f, mSettings.speedForMaxZoomOut);
			const float speedAlphaRaw = std::clamp(velocityLength / speedForMaxLookAhead, 0.f, 1.f);
			const float speedAlpha = mSettings.useSmoothSpeedZoom
				? speedAlphaRaw * speedAlphaRaw * (3.f - 2.f * speedAlphaRaw)
				: speedAlphaRaw;

			const sf::Vector2f velocityDirection = mExternalVelocity / velocityLength;
			movementLookAheadOffset =
				velocityDirection *
				std::max(0.f, mSettings.movementLookAheadMaxDistance) *
				std::clamp(mSettings.movementLookAheadStrength, 0.f, 1.f) *
				speedAlpha;
		}

		if (mLookAheadWorldPosition)
		{
			const sf::Vector2f lookAheadDelta = *mLookAheadWorldPosition - targetLocation;
			const float lookAheadDistance = GetVectorLength(lookAheadDelta);
			const float lookAheadDeadZone = std::max(0.f, mSettings.lookAheadDeadZone);

			if (lookAheadDistance > lookAheadDeadZone)
			{
				const float maxLookAheadDistance = std::max(lookAheadDeadZone, mSettings.lookAheadMaxDistance);
				const float effectiveLookAheadDistance = std::min(
					lookAheadDistance - lookAheadDeadZone,
					maxLookAheadDistance - lookAheadDeadZone
				);
				const sf::Vector2f lookAheadDirection = lookAheadDelta / lookAheadDistance;
				const sf::Vector2f clampedLookAheadDelta = lookAheadDirection * effectiveLookAheadDistance;

				const float lookAheadStrength = std::clamp(mSettings.lookAheadStrength, 0.f, 1.f);
				cursorLookAheadOffset = clampedLookAheadDelta * lookAheadStrength;
			}
		}

		const sf::Vector2f desiredLookAheadOffset = ClampVectorLength(
			cursorLookAheadOffset + movementLookAheadOffset,
			std::max(0.f, mSettings.combinedLookAheadMaxOffset)
		);

		const float lookAheadAlpha = 1.f - std::exp(-std::max(0.f, mSettings.lookAheadSmoothingSpeed) * deltaTime);
		mCurrentLookAheadOffset = LerpVector(mCurrentLookAheadOffset, desiredLookAheadOffset, lookAheadAlpha);
		return targetLocation + mCurrentLookAheadOffset;
	}

	sf::Vector2f CameraManager::ClampCenterToWorldBounds(const sf::Vector2f& center, const sf::Vector2f& viewSize) const
	{
		if (!mWorldBounds)
		{
			return center;
		}

		const float boundsPadding = std::max(0.f, mSettings.worldBoundsPadding);
		const sf::Vector2f paddedBoundsPosition = mWorldBounds->position - sf::Vector2f{ boundsPadding, boundsPadding };
		const sf::Vector2f paddedBoundsSize = mWorldBounds->size + sf::Vector2f{ boundsPadding * 2.f, boundsPadding * 2.f };
		const sf::Vector2f boundsMin = paddedBoundsPosition;
		const sf::Vector2f boundsMax = paddedBoundsPosition + paddedBoundsSize;
		const sf::Vector2f boundsCenter = paddedBoundsPosition + paddedBoundsSize * 0.5f;
		const sf::Vector2f halfViewSize = viewSize * 0.5f;

		sf::Vector2f clampedCenter = center;

		if (viewSize.x >= paddedBoundsSize.x)
		{
			clampedCenter.x = boundsCenter.x;
		}
		else
		{
			clampedCenter.x = std::clamp(center.x, boundsMin.x + halfViewSize.x, boundsMax.x - halfViewSize.x);
		}

		if (viewSize.y >= paddedBoundsSize.y)
		{
			clampedCenter.y = boundsCenter.y;
		}
		else
		{
			clampedCenter.y = std::clamp(center.y, boundsMin.y + halfViewSize.y, boundsMax.y - halfViewSize.y);
		}

		return clampedCenter;
	}

	float CameraManager::GetDesiredZoom() const
	{
		const float baseZoom = std::max(0.01f, mSettings.baseZoom);
		const float speedForMaxZoomOut = std::max(0.01f, mSettings.speedForMaxZoomOut);
		const float speedAlphaRaw = std::clamp(GetVectorLength(mExternalVelocity) / speedForMaxZoomOut, 0.f, 1.f);
		const float speedAlpha = mSettings.useSmoothSpeedZoom
			? speedAlphaRaw * speedAlphaRaw * (3.f - 2.f * speedAlphaRaw)
			: speedAlphaRaw;

		const float composedZoom =
			baseZoom +
			speedAlpha * std::max(0.f, mSettings.maxSpeedZoomOut) +
			mAdditionalZoomOut;
		return composedZoom + composedZoom * mRelativeAdditionalZoomOut;
	}
}

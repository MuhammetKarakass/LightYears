#include "framework/camera/CameraManager.h"
#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	void CameraManager::SetSettings(const CameraSettings& settings)
	{
		mSettings = settings;
	}

	void CameraManager::SetFollowTarget(weak_ptr<Actor> target)
	{
		mFollowTarget = target;
	}

	void CameraManager::ClearFollowTarget()
	{
		mFollowTarget.reset();
		ClearExternalVelocity();
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

	void CameraManager::Update(float deltaTime, const sf::View& defaultView)
	{
		auto followTarget = mFollowTarget.lock();
		if (!followTarget || followTarget->GetIsPendingDestroy())
		{
			if (!mHasState)
			{
				mCurrentCenter = defaultView.getCenter();
				mCurrentZoom = 1.f;
				mHasState = true;
			}
			return;
		}

		const float desiredZoom = GetDesiredZoom();
		const sf::Vector2f desiredViewSize = defaultView.getSize() * desiredZoom;
		const sf::Vector2f desiredCenter = ClampCenterToWorldBounds(
			GetDesiredCenter(*followTarget, deltaTime),
			desiredViewSize
		);

		if (!mHasState)
		{
			mCurrentCenter = desiredCenter;
			mCurrentZoom = desiredZoom;
			mHasState = true;
			return;
		}

		const float positionAlpha = 1.f - std::exp(-std::max(0.f, mSettings.positionSmoothingSpeed) * deltaTime);
		const float zoomAlpha = 1.f - std::exp(-std::max(0.f, mSettings.zoomSmoothingSpeed) * deltaTime);

		mCurrentCenter = LerpVector(mCurrentCenter, desiredCenter, positionAlpha);
		mCurrentZoom = Lerp(mCurrentZoom, desiredZoom, zoomAlpha);
	}

	sf::View CameraManager::GetView(const sf::View& defaultView) const
	{
		if (!mHasState)
		{
			return defaultView;
		}

		sf::View view = defaultView;
		view.setCenter(mCurrentCenter);
		view.setSize(defaultView.getSize() * mCurrentZoom);
		return view;
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

		return baseZoom + speedAlpha * std::max(0.f, mSettings.maxSpeedZoomOut);
	}
}

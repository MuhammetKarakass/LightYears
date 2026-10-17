#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "framework/WorldRenderPipeline.h"
#include "framework/RenderViewport.h"
#include "gameplay/GameStage.h"
#include "widget/HUD.h"
#include "framework/PerfMonitor.h"
#include "framework/PhysicsScene.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <typeinfo>
#include <cstdint>
#include <iterator>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vega{
	void World::SetViewTarget(weak_ptr<Actor> target)
	{
		mViewTarget = target;
		mCameraManager.SetFollowTarget(target);
	}

	void World::ClearViewTarget()
	{
		mViewTarget.reset();
		mCameraManager.ClearFollowTarget();
	}

	void World::SetCameraSettings(const CameraSettings& settings)
	{
		mCameraManager.SetSettings(settings);
	}

	void World::SetCameraExternalVelocity(const sf::Vector2f& velocity)
	{
		mCameraManager.SetExternalVelocity(velocity);
	}

	void World::ClearCameraExternalVelocity()
	{
		mCameraManager.ClearExternalVelocity();
	}

	void World::SetCameraPreserveFollowTargetOffset(bool preserve)
	{
		mCameraManager.SetPreserveFollowTargetOffset(preserve);
	}

	void World::SetCameraAdditionalZoomOut(float zoomOut)
	{
		mCameraManager.SetAdditionalZoomOut(zoomOut);
	}

	void World::SetCameraRelativeAdditionalZoomOut(float zoomOutRatio)
	{
		mCameraManager.SetRelativeAdditionalZoomOut(zoomOutRatio);
	}

	void World::SetCameraLookAheadWorldPosition(const std::optional<sf::Vector2f>& worldPosition)
	{
		mCameraManager.SetLookAheadWorldPosition(worldPosition);
	}

	void World::ClearCameraLookAheadWorldPosition()
	{
		mCameraManager.ClearLookAheadWorldPosition();
	}

	void World::SetCameraWorldBounds(const sf::FloatRect& bounds)
	{
		mCameraManager.SetWorldBounds(bounds);
	}

	void World::ClearCameraWorldBounds()
	{
		mCameraManager.ClearWorldBounds();
	}

	void World::PlayCameraShake(float amplitude, float duration, float frequency)
	{
		mCameraManager.PlayShake(amplitude, duration, frequency);
	}

	void World::SetRenderViewport(RenderViewport* viewport)
	{
		mRenderViewport = viewport;
	}

	RenderViewport* World::GetRenderViewport()
	{
		return mRenderViewport ? mRenderViewport : (mOwningApp ? &mOwningApp->GetWindowViewport() : nullptr);
	}

	const RenderViewport* World::GetRenderViewport() const
	{
		return mRenderViewport ? mRenderViewport : (mOwningApp ? &mOwningApp->GetWindowViewport() : nullptr);
	}

	sf::Vector2u World::GetWindowSize()
	{
		const RenderViewport* viewport = GetRenderViewport();
		return viewport ? viewport->GetSize() : sf::Vector2u{};
	}

	sf::View World::GetWorldView() const
	{
		if (mViewOverride) return *mViewOverride;
		const RenderViewport* viewport = GetRenderViewport();
		if (!viewport)
		{
			return mCameraManager.GetView(sf::View{});
		}
		return mCameraManager.GetView(viewport->GetDefaultView());
	}

	sf::Vector2f World::GetMouseWorldPosition() const
	{
		const RenderViewport* viewport = GetRenderViewport();
		if (!viewport)
		{
			// No viewport to read the cursor from; aim at the view center.
			return GetWorldView().getCenter();
		}
		return viewport->MapPixelToCoords(viewport->GetMousePixelPosition(), GetWorldView());
	}

	bool World::HasInputFocus() const
	{
		const RenderViewport* viewport = GetRenderViewport();
		return viewport && viewport->HasInputFocus();
	}


	void World::UpdateCamera(float deltaTime)
	{
		if (const RenderViewport* viewport = GetRenderViewport())
		{
			mCameraManager.Update(deltaTime, viewport->GetDefaultView());
		}
	}

}

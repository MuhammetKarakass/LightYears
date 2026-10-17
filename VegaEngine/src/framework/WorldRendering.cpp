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

#include "WorldInternal.h"

namespace vega{
	using world_internal::Intersects;
	using world_internal::SlowActorTrace;

	void World::Render(sf::RenderTarget& window)
	{
		VEGA_PROFILE_SCOPE("World Render");
		sf::View previousView = window.getView();
		sf::View worldView = GetWorldView();
		WorldRenderPipeline* pipeline = mOwningApp ? mOwningApp->GetWorldRenderPipeline() : nullptr;
		if (pipeline && !pipeline->IsActive())
		{
			pipeline = nullptr;
		}
		if (pipeline)
		{
			VEGA_PROFILE_SCOPE("World.PipelineBegin");
			pipeline->BeginFrame(*this, worldView);
		}
		else
		{
			window.setView(worldView);
		}
		const sf::Vector2f viewSize = worldView.getSize();
		const sf::Vector2f viewCenter = worldView.getCenter();
		const sf::FloatRect viewBounds{
			{ viewCenter.x - viewSize.x * 0.5f, viewCenter.y - viewSize.y * 0.5f },
			viewSize
		};
		int renderCandidates = 0;
		int renderCulled = 0;
		int renderSubmitted = 0;

		for (List<Actor*>& bucket : mRenderBuckets)
		{
			bucket.clear();
		}

		{
			VEGA_PROFILE_SCOPE("World.RenderCull");
		for (const shared_ptr<Actor>& actor : mActors)
		{
			if (actor->GetIsPendingDestroy())
			{
				continue;
			}
			++renderCandidates;
			const bool mayCull = actor->GetRenderLayer() != RenderLayer::Background &&
				actor->GetRenderLayer() != RenderLayer::Foreground;
			const std::optional<sf::FloatRect> renderBounds =
				mayCull ? actor->GetRenderBounds() : std::nullopt;
			if (renderBounds && !Intersects(*renderBounds, viewBounds))
			{
				++renderCulled;
				continue;
			}

			const std::size_t layerIndex = static_cast<std::size_t>(actor->GetRenderLayer());
			if (layerIndex < mRenderBuckets.size())
			{
				mRenderBuckets[layerIndex].push_back(actor.get());
				++renderSubmitted;
			}
		}
		}
		vega::perf::SetRenderStats(renderCandidates, renderCulled, renderSubmitted);

		{
			VEGA_PROFILE_SCOPE("World.RenderDraw");
		for (std::size_t layerIndex = 0; layerIndex < mRenderBuckets.size(); ++layerIndex)
		{
			const RenderLayer layer = static_cast<RenderLayer>(layerIndex);
			if (pipeline && pipeline->ReplacesLayer(layer))
			{
				continue;
			}
			sf::RenderTarget& target = pipeline ? pipeline->GetTarget(layer) : static_cast<sf::RenderTarget&>(window);
			for (Actor* actor : mRenderBuckets[layerIndex])
			{
				if (pipeline && pipeline->HidesActor(*actor))
				{
					continue;
				}
				const SlowActorTrace trace{ *actor };
				actor->Render(target);
			}
		}
		}

		if (pipeline)
		{
			VEGA_PROFILE_SCOPE("World.PipelineCompose");
			pipeline->Compose(window);
		}
		window.setView(window.getDefaultView());
		{
			VEGA_PROFILE_SCOPE("World.RenderHUD");
		RenderHUD(window);
		}

		window.setView(previousView);
	}

	void World::RenderHUD(sf::RenderTarget& window)
	{
		if(mHUD)
		{
			if (mHUD->HasInit())
			{
				mHUD->Draw(window);
			}
		}
		if(mOverlayHUD)
		{
			if (mOverlayHUD->HasInit())
			{
				mOverlayHUD->Draw(window);
			}
		}
	}
}

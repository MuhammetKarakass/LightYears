#include "editor/EditorApplication.h"
#include "editor/EditorIcons.h"
#include "editor/PropertyWidgets.h"
#include "editor/EditorPrefabs.h"
#include <framework/World.h>
#include <framework/Actor.h>
#include <framework/scene/SceneEntityComponent.h>
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>
#include <algorithm>
#include <cctype>
#include <typeinfo>
#include <typeindex>
#include <string_view>

namespace vega
{
	void EditorApplication::BuildViewportPanel()
	{
		LY_PROFILE_SCOPE("Editor Viewport");
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		if (ImGui::Begin("Viewport", &mShowViewport))
		{
			const ImVec2 available = ImGui::GetContentRegionAvail();
			if (available.x > 0.f && available.y > 0.f)
			{
				const sf::Vector2f textureSize{ mViewport.GetSize() };
				const float scale = std::min(available.x / textureSize.x, available.y / textureSize.y);
				const sf::Vector2f imageSize = textureSize * scale;
				const ImVec2 cursor = ImGui::GetCursorScreenPos();
				ImGui::SetCursorScreenPos({ cursor.x + (available.x - imageSize.x) * 0.5f, cursor.y + (available.y - imageSize.y) * 0.5f });
				// The RenderTexture overload also flips the UVs to display the image upright.
				ImGui::Image(mViewport.GetTexture(), imageSize);
				if (!mContext.playMode && !mExtensions.Active() && ImGui::BeginDragDropTarget())
				{
					if (const auto* payload = ImGui::AcceptDragDropPayload("VEGA_ACTOR"))
					{
						const auto actor = mContext.Resolve(*static_cast<const unsigned int*>(payload->Data));
						if (actor)
						{
							const auto origin = ImGui::GetItemRectMin();
							const auto mouse = ImGui::GetMousePos();
							const sf::Vector2i pixel{ static_cast<int>((mouse.x - origin.x) / scale), static_cast<int>((mouse.y - origin.y) / scale) };
							const auto location = mViewport.MapPixelToCoords(pixel, mContext.world->GetWorldView());
							TransformSelection(mContext, mHistory, CaptureSelectionTransform(mSelection), mSelection.Centre(), location - actor->GetActorLocation());
						}
					}
					if (const auto* payload = ImGui::AcceptDragDropPayload("VEGA_ASSET"))
					{
						const auto origin = ImGui::GetItemRectMin();
						const auto mouse = ImGui::GetMousePos();
						const sf::Vector2i pixel{ static_cast<int>((mouse.x - origin.x) / scale), static_cast<int>((mouse.y - origin.y) / scale) };
						const auto path = static_cast<const char*>(payload->Data);
						if (IsPrefabPath(path)) PlacePrefab(mContext, mHistory, path, mViewport.MapPixelToCoords(pixel, mContext.world->GetWorldView()));
					}
					ImGui::EndDragDropTarget();
				}
				const ImVec2 topLeft = ImGui::GetItemRectMin();
				const ImVec2 bottomRight = ImGui::GetItemRectMax();
				auto* draw = ImGui::GetWindowDrawList();
				if (mContext.playMode) draw->AddRect(topLeft, bottomRight, IM_COL32(255, 158, 56, 255), 0.f, 0, 4.f);
				if (!mContext.playMode && mContext.world)
				{
					draw->PushClipRect(topLeft, bottomRight, true);
					for (const auto& weak : mContext.world->GetActorsByTypeIncludingPending<ly::Actor>())
					{
						const auto start = weak.lock();
						if (!start || start->GetIsPendingDestroy()) continue;
						const auto* provider = QueryCapability<IActorDisplayInfo>(mModule.get());
						const auto display = provider ? provider->DisplayInfo(*start) : ActorDisplayInfo{};
						if (!display.spawnMarker) continue;
						const auto pixel = mViewport.GetTexture().mapCoordsToPixel(start->GetActorLocation(), mContext.world->GetWorldView());
						const ImVec2 p{ topLeft.x + pixel.x * scale, topLeft.y + pixel.y * scale };
						draw->AddTriangleFilled({ p.x, p.y - 14.f }, { p.x - 11.f, p.y + 10.f }, { p.x + 11.f, p.y + 10.f }, IM_COL32(75, 224, 219, 245));
						draw->AddText({ p.x + 18.f, p.y - 8.f }, IM_COL32(230, 242, 250, 255), display.label.c_str());
					}
					draw->PopClipRect();
				}
				mViewportImageRect = { { topLeft.x - mainViewport->Pos.x, topLeft.y - mainViewport->Pos.y },
					{ bottomRight.x - topLeft.x, bottomRight.y - topLeft.y } };
				mViewportImageHovered = ImGui::IsItemHovered();
				mViewportWindowFocused = ImGui::IsWindowFocused();
				mViewport.SetImagePresentation(mViewportImageRect, mViewportImageHovered, ImGui::IsWindowFocused());
				EditorViewportExtensionContext extensionContext{ mContext, mHistory,
					{ { topLeft.x, topLeft.y }, { bottomRight.x - topLeft.x, bottomRight.y - topLeft.y } }, mViewport.GetSize(), mViewportImageHovered };
				for (const auto& drawExtension : mExtensions.viewports) drawExtension(extensionContext);
				if (!mExtensions.Active()) BuildGizmo();
			}
		}
		ImGui::End();
	}

	void EditorApplication::PickViewportActor()
	{
		ly::World* world = GetCurrentWorld();
		if (!world)
		{
			return;
		}
		const auto mouse = ImGui::GetMousePos();
		const auto mainPos = ImGui::GetMainViewport()->Pos;
		const auto pixel = MapWindowToTexturePixel({ static_cast<int>(mouse.x - mainPos.x), static_cast<int>(mouse.y - mainPos.y) }, mViewportImageRect, mViewport.GetSize());
		const sf::Vector2f point = mViewport.MapPixelToCoords(pixel, world->GetWorldView());
		const auto actors = world->GetActorsByTypeIncludingPending<ly::Actor>();
		std::vector<ActorPickBounds> candidates;
		for (const auto& weakActor : actors)
		{
			if (const auto actor = weakActor.lock(); actor && !actor->GetIsPendingDestroy())
			{
				auto bounds = actor->GetActorGlobalBounds();
				const auto* provider = QueryCapability<IActorDisplayInfo>(mModule.get());
				if (provider && provider->DisplayInfo(*actor).spawnMarker && !mContext.playMode)
					bounds = { actor->GetActorLocation() - sf::Vector2f{ 24.f, 24.f }, { 48.f, 48.f } };
				candidates.emplace_back(actor->GetUniqueID(), bounds);
			}
		}
		const auto best = PickBestActor(point, candidates);
		if (mReferencePick)
		{
			const auto request = *mReferencePick;
			mReferencePick.reset();
			const auto owner = mContext.Resolve(request.actor), targetActor = best ? mContext.Resolve(*best) : nullptr;
			if (!owner || !targetActor) return;
			const auto target = ResolvePropertyTarget(*owner, request.component);
			const auto* property = target.type ? target.type->FindProperty(request.property) : nullptr;
			if (!property) return;
			auto value = property->getter(target.object);
			if (request.index < 0) value = ly::ActorRef{ *targetActor };
			else
			{
				auto* list = std::get_if<ly::reflect::PropertyList>(&value);
				if (!list || static_cast<std::size_t>(request.index) >= list->size()) return;
				(*list)[request.index] = ly::ActorRef{ *targetActor };
			}
			mHistory.Execute(std::make_unique<SetPropertyCommand>(request.actor, request.component, request.property, value));
			return;
		}
		const auto& io = ImGui::GetIO();
		if (best)
		{
			const auto actor = mContext.Resolve(*best);
			if (io.KeyCtrl) mSelection.Toggle(actor);
			else if (io.KeyShift) mSelection.Add(actor);
			else if (!mSelection.Contains(actor)) mSelection.Select(actor);
			else mSelection.Add(actor);
		}
		else
		{
			if (!io.KeyCtrl && !io.KeyShift) mSelection.Clear();
			if (!mContext.playMode)
			{
				mMarquee = true;
				mMarqueeStart = point;
				mMarqueeScreen = { mouse.x, mouse.y };
				mMarqueeAdd = io.KeyShift;
				mMarqueeRemove = io.KeyCtrl;
			}
		}
	}
}

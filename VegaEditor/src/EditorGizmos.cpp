#include "editor/EditorApplication.h"
#include <framework/World.h>
#include <framework/PhysicsScene.h>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <unordered_set>
#include <framework/scene/SceneEntityComponent.h>

namespace vega
{
	bool EditorApplication::BuildBoundsOverlay()
	{
		const auto bounds = vega::ReadSceneBounds(mSceneSettings, GetEngineContext().SceneSettings());
		if (!bounds || !mContext.world || mPrefabWorld) { mBoundsCorner = -1; return false; }
		const auto image = mViewportImageRect;
		const auto main = ImGui::GetMainViewport()->Pos;
		const auto screen = [&](sf::Vector2f point)
		{
			const auto pixel = mViewport.GetTexture().mapCoordsToPixel(point, mContext.world->GetWorldView());
			return ImVec2{ main.x + image.position.x + pixel.x * image.size.x / mViewport.GetSize().x,
				main.y + image.position.y + pixel.y * image.size.y / mViewport.GetSize().y };
		};
		auto* draw = ImGui::GetWindowDrawList();
		draw->PushClipRect({ main.x + image.position.x, main.y + image.position.y },
			{ main.x + image.position.x + image.size.x, main.y + image.position.y + image.size.y }, true);
		draw->AddRect(screen(bounds->position), screen(bounds->position + bounds->size), IM_COL32(100, 230, 170, 220), 0.f, 0, 2.f);
		const auto mouse = ImGui::GetMousePos();
		for (int corner = 0; corner < 4; ++corner)
		{
			const auto p = screen(bounds->position + sf::Vector2f{ (corner & 1) ? bounds->size.x : 0.f, (corner & 2) ? bounds->size.y : 0.f });
			if (!mContext.playMode)
			{
				draw->AddRectFilled({ p.x - 5.f, p.y - 5.f }, { p.x + 5.f, p.y + 5.f }, IM_COL32(100, 230, 170, 255));
				if (mBoundsCorner < 0 && mViewportImageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && std::hypot(mouse.x - p.x, mouse.y - p.y) < 10.f)
				{
					mBoundsCorner = corner; mBoundsStart = mSceneSettings.at("bounds"); mBoundsMergeGroup = mHistory.BeginMergeGroup();
				}
			}
		}
		draw->PopClipRect();
		const bool consumed = mBoundsCorner >= 0;
		if (consumed)
		{
			if (mContext.playMode || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) mBoundsCorner = -1;
			else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				mHistory.Execute(std::make_unique<SetSceneSettingCommand>("bounds", mBoundsStart, mBoundsMergeGroup)); mBoundsCorner = -1;
			}
			else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.f))
			{
				const auto pixel = MapWindowToTexturePixel({ int(mouse.x - main.x), int(mouse.y - main.y) }, image, mViewport.GetSize());
				const auto point = mViewport.MapPixelToCoords(pixel, mContext.world->GetWorldView());
				const auto start = vega::ReadSceneBounds(nlohmann::json{ { "bounds", mBoundsStart } }, GetEngineContext().SceneSettings()).value();
				const sf::Vector2f opposite = start.position + sf::Vector2f{ (mBoundsCorner & 1) ? 0.f : start.size.x, (mBoundsCorner & 2) ? 0.f : start.size.y };
				const nlohmann::json value{ std::min(point.x, opposite.x), std::min(point.y, opposite.y), std::max(1.f, std::abs(point.x - opposite.x)), std::max(1.f, std::abs(point.y - opposite.y)) };
				if (value != mSceneSettings.at("bounds")) mHistory.Execute(std::make_unique<SetSceneSettingCommand>("bounds", value, mBoundsMergeGroup));
			}
		}
		return consumed;
	}

	bool EditorApplication::BuildPropertyOverlays()
	{
		if (!mContext.world || mContext.playMode || mPrefabWorld) { mPointDrag.reset(); return false; }
		if (mReferencePick)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Escape)) mReferencePick.reset();
			else
			{
				ImGui::SetTooltip("Pick a target actor in the viewport (Esc cancels)");
				if (mSelectionClickPending && mViewportImageHovered) PickViewportActor();
				return true;
			}
		}
		// Cache property values and descriptors by edit/selection revision. No scene JSON
		// or repeated list getter copies are needed while merely drawing the viewport.
		if (mAuthoringRevision != mContext.sceneRevision || mAuthoringSelection != mSelection.Revision() || mAuthoringWorld != mContext.world->GetUniqueID())
		{
			if (mAuthoringSelection != mSelection.Revision() || mAuthoringWorld != mContext.world->GetUniqueID()) mPointDrag.reset();
			mAuthoringProperties.clear();
			for (const auto& actor : mSelection.GetAll())
			{
				const auto collect = [&](const ReflectedTarget& target)
				{
					if (!target.type) return;
					for (const auto* property : target.type->AllProperties())
						if (property->kind == vega::reflect::PropertyKind::ActorRef || property->kind == vega::reflect::PropertyKind::List)
							mAuthoringProperties.push_back({ actor->GetUniqueID(), target.componentIdentity, property, property->getter(target.object) });
				};
				collect(ResolvePropertyTarget(*actor, {}));
				actor->ForEachComponent([&](vega::ActorComponent& component) { collect(ResolvePropertyTarget(*actor, vega::IdentifyComponent(*actor, component))); });
			}
			mAuthoringRevision = mContext.sceneRevision; mAuthoringSelection = mSelection.Revision(); mAuthoringWorld = mContext.world->GetUniqueID();
		}
		const auto main = ImGui::GetMainViewport()->Pos;
		const auto image = mViewportImageRect;
		const auto screen = [&](sf::Vector2f point)
		{
			const auto pixel = mViewport.GetTexture().mapCoordsToPixel(point, mContext.world->GetWorldView());
			return ImVec2{ image.position.x + main.x + pixel.x * image.size.x / mViewport.GetSize().x,
				image.position.y + main.y + pixel.y * image.size.y / mViewport.GetSize().y };
		};
		auto* draw = ImGui::GetWindowDrawList();
		draw->PushClipRect({ image.position.x + main.x, image.position.y + main.y }, { image.position.x + main.x + image.size.x, image.position.y + main.y + image.size.y }, true);
		const auto mouse = ImGui::GetMousePos();
		bool consumed = mPointDrag.has_value();
		for (std::size_t row = 0; row < mAuthoringProperties.size(); ++row)
		{
			auto& entry = mAuthoringProperties[row];
			const auto actor = mContext.Resolve(entry.actor);
			if (!actor) continue;
			const auto line = [&](const vega::ActorRef& ref)
			{
				const auto target = ref.Resolve(*mContext.world);
				if (!target) return;
				const auto a = screen(actor->GetActorLocation()), b = screen(target->GetActorLocation());
				const float distance = std::hypot(b.x - a.x, b.y - a.y);
				for (float d = 0; d < distance; d += 12.f)
				{
					const float end = std::min(d + 6.f, distance);
					draw->AddLine({ a.x + (b.x - a.x) * d / distance, a.y + (b.y - a.y) * d / distance },
						{ a.x + (b.x - a.x) * end / distance, a.y + (b.y - a.y) * end / distance }, IM_COL32(100, 210, 245, 230), 1.5f);
				}
			};
			if (entry.property->kind == vega::reflect::PropertyKind::ActorRef) line(std::get<vega::ActorRef>(entry.value));
			else
			{
				auto& list = std::get<vega::reflect::PropertyList>(entry.value);
				for (std::size_t i = 0; i < list.size(); ++i)
				{
					if (const auto* ref = std::get_if<vega::ActorRef>(&list[i])) line(*ref);
					if (const auto* point = std::get_if<sf::Vector2f>(&list[i]))
					{
						const auto p = screen(*point);
						if (i) { const auto previous = screen(std::get<sf::Vector2f>(list[i - 1])); draw->AddLine(previous, p, IM_COL32(245, 195, 70, 180)); }
						draw->AddCircleFilled(p, 5.f, IM_COL32(245, 195, 70, 240));
						if (!mPointDrag && entry.property->setter && mViewportImageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && std::hypot(mouse.x - p.x, mouse.y - p.y) < 9.f)
						{
							mPointDrag = { { row, i } }; mPointMergeGroup = mHistory.BeginMergeGroup(); consumed = true;
						}
					}
				}
			}
		}
		draw->PopClipRect();
		if (mPointDrag)
		{
			const auto [row, index] = *mPointDrag;
			if (row >= mAuthoringProperties.size() || !ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_Escape)) mPointDrag.reset();
			else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.f))
			{
				auto& entry = mAuthoringProperties[row];
				auto value = entry.value;
				auto& list = std::get<vega::reflect::PropertyList>(value);
				if (index >= list.size()) mPointDrag.reset();
				else
				{
					const auto pixel = MapWindowToTexturePixel({ int(mouse.x - main.x), int(mouse.y - main.y) }, image, mViewport.GetSize());
					list[index] = mViewport.MapPixelToCoords(pixel, mContext.world->GetWorldView());
					if (value != entry.value) mHistory.Execute(std::make_unique<SetPropertyCommand>(entry.actor, entry.component, entry.property->name, value, mPointMergeGroup));
				}
			}
		}
		return consumed;
	}

	void EditorApplication::BuildGizmo()
	{
		if (BuildBoundsOverlay() || BuildPropertyOverlays()) return;
		auto* world = GetCurrentWorld();
		const auto actor = mSelection.Get();
		if (mContext.playMode)
		{
			if (mSelectionClickPending && mViewportImageHovered) PickViewportActor();
			mMoveDragActor.reset();
			return;
		}
		if (!world || (!world->IsPaused() && !world->GetViewOverride())) { mMoveDragActor.reset(); return; }
		const auto view = world->GetWorldView();
		const auto mainPos = ImGui::GetMainViewport()->Pos;
		const auto image = mViewportImageRect;
		const sf::Vector2f textureSize{ mViewport.GetSize() };
		const auto screen = [&](sf::Vector2f point)
		{
			const sf::Vector2f pixel{ mViewport.GetTexture().mapCoordsToPixel(point, view) };
			return sf::Vector2f{ image.position.x + mainPos.x + pixel.x * image.size.x / textureSize.x,
				image.position.y + mainPos.y + pixel.y * image.size.y / textureSize.y };
		};
		const auto mouse = ImGui::GetMousePos();
		const sf::Vector2f mouseScreen{ mouse.x, mouse.y };
		const auto pixel = MapWindowToTexturePixel({ static_cast<int>(mouse.x - mainPos.x), static_cast<int>(mouse.y - mainPos.y) }, image, mViewport.GetSize());
		const auto mouseWorld = mViewport.MapPixelToCoords(pixel, view);
		GizmoHandle hover = GizmoHandle::None;
		if (actor && actor->GetWorld() == world && mViewportImageHovered)
		{
			const auto origin = screen(mSelection.Centre());
			hover = mContext.viewSettings.tool == EditorTool::Move ? HitMoveHandle(mouseScreen, origin) : HitRotateHandle(mouseScreen, origin);
		}
		if (mSelectionClickPending && mViewportImageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if (hover != GizmoHandle::None && actor)
			{
				mMoveDragActor = actor->GetUniqueID();
				mGizmoHandle = hover;
				mMoveDragMouse = mouseWorld;
				mGizmoStartLocation = mSelection.Centre();
				mGizmoStarts = CaptureSelectionTransform(mSelection);
				mGizmoStartRotation = actor->GetActorRotation();
				mGizmoPreviousAngle = GizmoAngle(mouseScreen, screen(mGizmoStartLocation));
				mGizmoRotationDelta = 0.f;
				mGizmoDragStarted = false;
				mMoveMergeGroup = mHistory.BeginMergeGroup();
			}
			else PickViewportActor();
		}
		if (mMoveDragActor)
		{
			const auto target = mContext.Resolve(*mMoveDragActor);
			if (!target || target != actor || !GetRenderWindow().hasFocus()) mMoveDragActor.reset();
			else
			{
				// Merely clicking an off-grid handle must not snap the actor or add history.
				mGizmoDragStarted = mGizmoDragStarted || ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.f);
				const bool snap = mContext.viewSettings.snap || ImGui::GetIO().KeyCtrl;
				if (mGizmoDragStarted && mGizmoHandle == GizmoHandle::Ring)
				{
					const float angle = GizmoAngle(mouseScreen, screen(mGizmoStartLocation));
					mGizmoRotationDelta += GizmoAngleDelta(mGizmoPreviousAngle, angle);
					mGizmoPreviousAngle = angle;
					float value = mGizmoStartRotation + mGizmoRotationDelta;
					if (snap) value = SnapGizmoValue(value, 15.f);
					if (std::abs(value - target->GetActorRotation()) > .001f)
						TransformSelection(mContext, mHistory, mGizmoStarts, mGizmoStartLocation, {}, value - mGizmoStartRotation, mMoveMergeGroup);
				}
				else if (mGizmoDragStarted)
				{
					auto value = mGizmoStartLocation + ProjectGizmoDrag(mouseWorld - mMoveDragMouse, mGizmoHandle);
					if (snap) value = SnapGizmoPosition(value, mGizmoHandle, mContext.viewSettings.gridSize);
					TransformSelection(mContext, mHistory, mGizmoStarts, mGizmoStartLocation, value - mGizmoStartLocation, {}, mMoveMergeGroup);
				}
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) mMoveDragActor.reset();
			}
		}
		if (mMarquee)
		{
			auto* draw = ImGui::GetWindowDrawList();
			draw->PushClipRect({ image.position.x + mainPos.x, image.position.y + mainPos.y },
				{ image.position.x + mainPos.x + image.size.x, image.position.y + mainPos.y + image.size.y }, true);
			const ImVec2 low{ std::min(mMarqueeScreen.x, mouse.x), std::min(mMarqueeScreen.y, mouse.y) };
			const ImVec2 high{ std::max(mMarqueeScreen.x, mouse.x), std::max(mMarqueeScreen.y, mouse.y) };
			draw->AddRectFilled(low, high, IM_COL32(51, 199, 202, 35));
			draw->AddRect(low, high, IM_COL32(51, 199, 202, 230));
			draw->PopClipRect();
			if (!GetRenderWindow().hasFocus()) mMarquee = false;
			else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				if (std::abs(mouse.x - mMarqueeScreen.x) + std::abs(mouse.y - mMarqueeScreen.y) > 3.f)
					SelectRectangle(mContext, mMarqueeStart, mouseWorld, mMarqueeAdd, mMarqueeRemove);
				mMarquee = false;
			}
		}
		const auto selected = mSelection.Get();
		if (!selected || selected->GetWorld() != world) return;
		const auto origin = screen(mSelection.Centre());
		const ImVec2 o{ origin.x, origin.y };
		auto* d = ImGui::GetWindowDrawList();
		d->PushClipRect({ image.position.x + mainPos.x, image.position.y + mainPos.y },
			{ image.position.x + mainPos.x + image.size.x, image.position.y + mainPos.y + image.size.y }, true);
		const auto active = mMoveDragActor ? mGizmoHandle : hover;
		const ImU32 highlight = IM_COL32(255, 222, 116, 255);
		if (mContext.viewSettings.tool == EditorTool::Move)
		{
			const ImU32 x = active == GizmoHandle::X ? highlight : IM_COL32(243, 100, 108, 255);
			const ImU32 y = active == GizmoHandle::Y ? highlight : IM_COL32(111, 221, 157, 255);
			// Dark backing keeps handles readable against bright sprites and HUD text.
			d->AddLine({ o.x + 11.f, o.y }, { o.x + 77.f, o.y }, IM_COL32(10, 18, 25, 230), 6.f);
			d->AddLine({ o.x, o.y - 11.f }, { o.x, o.y - 77.f }, IM_COL32(10, 18, 25, 230), 6.f);
			d->AddLine({ o.x + 11.f, o.y }, { o.x + 77.f, o.y }, x, 3.f);
			d->AddTriangleFilled({ o.x + 86.f, o.y }, { o.x + 73.f, o.y - 6.f }, { o.x + 73.f, o.y + 6.f }, x);
			d->AddLine({ o.x, o.y - 11.f }, { o.x, o.y - 77.f }, y, 3.f);
			d->AddTriangleFilled({ o.x, o.y - 86.f }, { o.x - 6.f, o.y - 73.f }, { o.x + 6.f, o.y - 73.f }, y);
			d->AddText({ o.x + 91.f, o.y - 10.f }, x, "X");
			d->AddText({ o.x - 5.f, o.y - 109.f }, y, "Y");
			d->AddRectFilled({ o.x - 8.f, o.y - 8.f }, { o.x + 8.f, o.y + 8.f }, IM_COL32(18, 48, 58, 240), 2.f);
			d->AddRect({ o.x - 8.f, o.y - 8.f }, { o.x + 8.f, o.y + 8.f }, active == GizmoHandle::Centre ? highlight : IM_COL32(77, 220, 222, 255), 2.f, 0, 2.f);
		}
		else
		{
			const ImU32 colour = active == GizmoHandle::Ring ? highlight : IM_COL32(77, 220, 222, 255);
			d->AddCircle(o, 64.f, IM_COL32(10, 18, 25, 230), 96, 6.f);
			d->AddCircle(o, 64.f, colour, 96, 2.5f);
			d->AddCircleFilled(o, 3.f, colour);
			const float radians = selected->GetActorRotation() * 3.141592654f / 180.f;
			d->AddLine(o, { o.x + std::cos(radians) * 64.f, o.y + std::sin(radians) * 64.f }, colour, 1.5f);
			char label[48];
			std::snprintf(label, sizeof(label), "%.1f deg", selected->GetActorRotation());
			d->AddText({ o.x - 30.f, o.y + 76.f }, colour, label);
		}
		if (active != GizmoHandle::None) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		d->PopClipRect();
	}

	void EditorApplication::DrawWorldOverlays(sf::RenderTexture& texture, vega::World& world)
	{
		const auto previous = texture.getView();
		const auto view = world.GetWorldView();
		texture.setView(view);
		const float pixel = view.getSize().x / (texture.getSize().x * view.getViewport().size.x);
		if (mContext.viewSettings.grid && world.GetViewOverride())
		{
			const float spacing = mContext.viewSettings.gridSize;
			const auto min = view.getCenter() - view.getSize() * .5f;
			const auto max = min + view.getSize();
			sf::VertexArray lines{ sf::PrimitiveType::Lines };
			for (int axis = 0; axis < 2; ++axis)
			{
				const float low = axis == 0 ? min.x : min.y;
				const float high = axis == 0 ? max.x : max.y;
				const int start = static_cast<int>(std::floor(low / spacing));
				const int end = static_cast<int>(std::ceil(high / spacing));
				for (int i = start; i <= end; ++i)
				{
					const sf::Color colour = i == 0 ? sf::Color{ 62, 142, 155, 125 } : i % 8 == 0 ? sf::Color{ 79, 112, 133, 100 } : sf::Color{ 69, 96, 120, 42 };
					const float value = i * spacing;
					lines.append({ axis == 0 ? sf::Vector2f{ value, min.y } : sf::Vector2f{ min.x, value }, colour });
					lines.append({ axis == 0 ? sf::Vector2f{ value, max.y } : sf::Vector2f{ max.x, value }, colour });
				}
			}
			texture.draw(lines);
		}
		if (mContext.viewSettings.colliders)
		{
			std::unordered_set<const vega::Actor*> fallbackActors;
			for (const auto& weak : world.GetActorsByType<vega::Actor>())
			{
				const auto actor = weak.lock();
				if (!actor || actor->GetIsPendingDestroy() || !actor->HasPhysicsBody()) continue;
				const float radians = actor->GetActorRotation() * 3.141592654f / 180.f;
				bool hasBox = false;
				for (std::size_t i = 0; i < actor->GetPhysicsCollisionBoxCount(); ++i)
				{
					const auto box = actor->GetPhysicsCollisionBox(i);
					if (box.halfExtents.x <= 0.f || box.halfExtents.y <= 0.f) continue;
					hasBox = true;
					const sf::Vector2f centre{ box.localCenter.x * std::cos(radians) - box.localCenter.y * std::sin(radians),
						box.localCenter.x * std::sin(radians) + box.localCenter.y * std::cos(radians) };
					sf::RectangleShape shape{ box.halfExtents * 2.f };
					shape.setOrigin(box.halfExtents);
					shape.setPosition(actor->GetActorLocation() + centre);
					shape.setRotation(sf::degrees(actor->GetActorRotation() + box.localRotationDegrees));
					shape.setFillColor(sf::Color::Transparent);
					shape.setOutlineColor({ 112, 229, 158, 200 });
					shape.setOutlineThickness(pixel * 1.5f);
					texture.draw(shape);
				}
				const float radius = actor->GetPhysicsCollisionRadius();
				if (radius > 0.f && !hasBox)
				{
					sf::CircleShape shape{ radius, 64 };
					shape.setOrigin({ radius, radius });
					shape.setPosition(actor->GetActorLocation());
					shape.setFillColor(sf::Color::Transparent);
					shape.setOutlineColor({ 112, 229, 158, 200 });
					shape.setOutlineThickness(pixel * 1.5f);
					texture.draw(shape);
				}
				else if (!hasBox) fallbackActors.insert(actor.get());
			}
			if (!fallbackActors.empty() && world.GetPhysicsScene().IsInitialized())
			{
				// Sprite-derived fixtures are sized when the body is created, and the Actor
				// shape providers intentionally return zero for them. Read their actual
				// vertices so rotating an actor cannot change the displayed fixture size.
				struct DrawContext
				{
					sf::RenderTexture& target;
					const std::unordered_set<const vega::Actor*>& actors;
					float rate;
					float thickness;
				};
				const float rate = world.GetPhysicsScene().GetPhysicsRate();
				DrawContext context{ texture, fallbackActors, rate, pixel * 1.5f };
				const auto min = (view.getCenter() - view.getSize() * .5f) * rate;
				const auto max = (view.getCenter() + view.getSize() * .5f) * rate;
				auto filter = b2DefaultQueryFilter();
				filter.categoryBits = filter.maskBits = ~std::uint64_t{ 0 };
				b2World_OverlapAABB(world.GetPhysicsScene().GetWorldId(), { { min.x, min.y }, { max.x, max.y } }, filter,
					[](b2ShapeId id, void* data)
					{
						auto& draw = *static_cast<DrawContext*>(data);
						const auto body = b2Shape_GetBody(id);
						const auto* actor = static_cast<const vega::Actor*>(b2Body_GetUserData(body));
						if (draw.actors.find(actor) == draw.actors.end()) return true;
						if (b2Shape_GetType(id) == b2_polygonShape)
						{
							const auto polygon = b2Shape_GetPolygon(id);
							sf::ConvexShape shape{ static_cast<std::size_t>(polygon.count) };
							for (int i = 0; i < polygon.count; ++i)
							{
								const auto point = b2Body_GetWorldPoint(body, polygon.vertices[i]);
								shape.setPoint(static_cast<std::size_t>(i), { point.x / draw.rate, point.y / draw.rate });
							}
							shape.setFillColor(sf::Color::Transparent);
							shape.setOutlineColor({ 112, 229, 158, 220 });
							shape.setOutlineThickness(draw.thickness);
							draw.target.draw(shape);
						}
						else if (b2Shape_GetType(id) == b2_circleShape)
						{
							const auto circle = b2Shape_GetCircle(id);
							const auto centre = b2Body_GetWorldPoint(body, circle.center);
							const float radius = circle.radius / draw.rate;
							sf::CircleShape shape{ radius, 64 };
							shape.setOrigin({ radius, radius });
							shape.setPosition({ centre.x / draw.rate, centre.y / draw.rate });
							shape.setFillColor(sf::Color::Transparent);
							shape.setOutlineColor({ 112, 229, 158, 220 });
							shape.setOutlineThickness(draw.thickness);
							draw.target.draw(shape);
						}
						return true;
					}, &context);
			}
		}
		texture.setView(previous);
	}
}

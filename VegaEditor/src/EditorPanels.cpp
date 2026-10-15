#include "editor/EditorApplication.h"
#include <framework/World.h>
#include <framework/Actor.h>
#include <imgui.h>
#include <imgui-SFML.h>
#include <algorithm>
#include <cctype>
#include <typeinfo>

namespace vega
{
	void EditorApplication::BuildViewportPanel()
	{
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		if (ImGui::Begin("Viewport", &mShowViewport))
		{
			ly::World* world = GetCurrentWorld();
			ImGui::BeginDisabled(world == nullptr);
			const bool paused = world && world->IsPaused();
			if (ImGui::Button(paused ? "Play" : "Pause") && world)
			{
				SetPaused{ !paused }.Execute(mContext);
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::TextUnformatted(world ? (world->IsPaused() ? "Paused" : "Playing") : "No world");
			ImGui::SetItemTooltip("Paused: click an actor in the viewport to select it; the editor consumes the click.\n"
				"Drag on the selected actor's bounds to move it.\n"
				"Playing: clicks go to the game; select actors in the Hierarchy.");
			ImGui::Separator();

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
				const ImVec2 topLeft = ImGui::GetItemRectMin();
				const ImVec2 bottomRight = ImGui::GetItemRectMax();
				mViewportImageRect = { { topLeft.x - mainViewport->Pos.x, topLeft.y - mainViewport->Pos.y },
					{ bottomRight.x - topLeft.x, bottomRight.y - topLeft.y } };
				mViewportImageHovered = ImGui::IsItemHovered();
				mViewport.SetImagePresentation(mViewportImageRect, mViewportImageHovered, ImGui::IsWindowFocused());
				if (world && world->IsPaused() && mSelectionClickPending && mViewportImageHovered &&
					!ImGui::IsAnyItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					const auto selected = mSelection.Get();
					const sf::Vector2f point = world->GetMouseWorldPosition();
					if (selected && selected->GetActorGlobalBounds().contains(point))
					{
						mMoveDragActor = selected->GetUniqueID();
						mMoveDragMouse = point;
						mMoveMergeGroup = mHistory.BeginMergeGroup();
					}
					else PickViewportActor();
				}
				if (mMoveDragActor)
				{
					const auto actor = mContext.Resolve(*mMoveDragActor);
					if (actor && world && world->IsPaused() && mSelection.Get() == actor)
					{
						const sf::Vector2f point = world->GetMouseWorldPosition();
						const sf::Vector2f delta = point - mMoveDragMouse;
						if (delta != sf::Vector2f{})
						{
							mHistory.Execute(std::make_unique<SetActorLocation>(*mMoveDragActor,
								actor->GetActorLocation() + delta, mMoveMergeGroup));
						}
						mMoveDragMouse = point;
					}
					else mMoveDragActor.reset();
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) mMoveDragActor.reset();
				}
			}
		}
		ImGui::End();
	}

	void EditorApplication::BuildHierarchyPanel()
	{
		if (ImGui::Begin("Hierarchy", &mShowHierarchy))
		{
			ImGui::InputTextWithHint("##ActorFilter", "Filter by type name", mHierarchyFilter, sizeof(mHierarchyFilter));
			ImGui::Separator();
			struct Row
			{
				std::shared_ptr<ly::Actor> actor;
				std::string name;
			};
			std::vector<Row> rows;
			if (const ly::World* world = GetCurrentWorld())
			{
				const std::string filter = mHierarchyFilter;
				for (const auto& weakActor : world->GetActorsByType<ly::Actor>())
				{
					if (auto actor = weakActor.lock(); actor && !actor->GetIsPendingDestroy())
					{
						std::string name = ActorTypeName(*actor);
						if (filter.empty() || std::search(name.begin(), name.end(), filter.begin(), filter.end(),
							[](unsigned char first, unsigned char second) { return std::tolower(first) == std::tolower(second); }) != name.end())
						{
							rows.push_back({ std::move(actor), std::move(name) });
						}
					}
				}
			}
			const auto selected = mSelection.Get();
			// Filter before clipping: every displayed index must correspond to one visible row.
			if (ImGui::BeginChild("ActorList"))
			{
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(rows.size()));
				while (clipper.Step())
				{
					for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index)
					{
						const Row& row = rows[static_cast<std::size_t>(index)];
						const std::string label = row.name + "  #" + std::to_string(row.actor->GetUniqueID());
						ImGui::PushID(row.actor.get());
						if (ImGui::Selectable(label.c_str(), row.actor == selected))
						{
							SelectActor{ row.actor->GetUniqueID() }.Execute(mContext);
						}
						ImGui::PopID();
					}
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void EditorApplication::BuildInspectorPanel()
	{
		if (ImGui::Begin("Inspector", &mShowInspector))
		{
			if (const auto actor = mSelection.Get())
			{
				ImGui::TextWrapped("%s  #%u", ActorTypeName(*actor).c_str(), actor->GetUniqueID());
				if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				{
					const sf::Vector2f location = actor->GetActorLocation();
					float values[]{ location.x, location.y };
					const bool locationChanged = ImGui::DragFloat2("Location", values, 1.f);
					// Include the final value committed on deactivation during Ctrl-click text entry.
					if (ImGui::IsItemActivated()) mLocationMergeGroup = mHistory.BeginMergeGroup();
					if (locationChanged)
					{
						mHistory.Execute(std::make_unique<SetActorLocation>(actor->GetUniqueID(),
							sf::Vector2f{ values[0], values[1] },
							(ImGui::IsItemActive() || ImGui::IsItemDeactivated()) ? mLocationMergeGroup : 0));
					}
					float rotation = actor->GetActorRotation();
					const bool rotationChanged = ImGui::DragFloat("Rotation", &rotation, 0.5f);
					if (ImGui::IsItemActivated()) mRotationMergeGroup = mHistory.BeginMergeGroup();
					if (rotationChanged)
					{
						mHistory.Execute(std::make_unique<SetActorRotation>(actor->GetUniqueID(), rotation,
							(ImGui::IsItemActive() || ImGui::IsItemDeactivated()) ? mRotationMergeGroup : 0));
					}
					const sf::Vector2f velocity = actor->GetVelocity();
					ImGui::Text("Velocity: %.2f, %.2f", velocity.x, velocity.y);
				}
				if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
				{
					static const char* layers[]{ "Background", "GroundDecal", "World", "Projectile", "WorldVfx", "Foreground" };
					static_assert(IM_ARRAYSIZE(layers) == static_cast<int>(ly::RenderLayer::Count), "Update editor render layers");
					int layer = static_cast<int>(actor->GetRenderLayer());
					if (ImGui::Combo("RenderLayer", &layer, layers, IM_ARRAYSIZE(layers)))
					{
						mHistory.Execute(std::make_unique<SetRenderLayer>(actor->GetUniqueID(), static_cast<ly::RenderLayer>(layer)));
					}
					bool enabled = actor->IsRenderEnabled();
					if (ImGui::Checkbox("Render enabled", &enabled))
					{
						mHistory.Execute(std::make_unique<SetRenderEnabled>(actor->GetUniqueID(), enabled));
					}
				}
				if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Physics enabled: %s", actor->IsPhysicsEnabled() ? "Yes" : "No");
					ImGui::Text("Has body: %s", actor->HasPhysicsBody() ? "Yes" : "No");
					ImGui::Text("Body type: %s", actor->GetPhysicsBodyType() == ly::PhysicsBodyType::Static ? "Static" : "Dynamic");
					ImGui::Text("Collision layer: 0x%02X", static_cast<unsigned int>(actor->GetCollisionLayer()));
					ImGui::Text("Collision mask: 0x%02X", static_cast<unsigned int>(actor->GetCollisionMask()));
				}
				if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen))
				{
					const sf::FloatRect bounds = actor->GetActorGlobalBounds();
					ImGui::Text("Position: %.2f, %.2f", bounds.position.x, bounds.position.y);
					ImGui::Text("Size: %.2f, %.2f", bounds.size.x, bounds.size.y);
				}
				ImGui::Separator();
				if (ImGui::Button("Destroy"))
				{
					mDestroyActor = actor->GetUniqueID();
					ImGui::OpenPopup("Destroy actor?");
				}
			}
			else
			{
				ImGui::TextWrapped("Select an actor in the Hierarchy or paused viewport.");
			}
			if (ImGui::BeginPopupModal("Destroy actor?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextUnformatted("Destroy this actor? This cannot be undone.");
				if (ImGui::Button("Destroy permanently"))
				{
					if (mDestroyActor) mHistory.Execute(std::make_unique<DestroyActor>(*mDestroyActor));
					mDestroyActor.reset();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					mDestroyActor.reset();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
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
		const sf::Vector2f point = world->GetMouseWorldPosition();
		const auto actors = world->GetActorsInBounds({ point - sf::Vector2f{ 2.f, 2.f }, { 4.f, 4.f } });
		std::vector<ActorPickBounds> candidates;
		for (const auto& weakActor : actors)
		{
			if (const auto actor = weakActor.lock(); actor && !actor->GetIsPendingDestroy())
			{
				candidates.emplace_back(actor->GetUniqueID(), actor->GetActorGlobalBounds());
			}
		}
		const auto best = PickBestActor(point, candidates);
		SelectActor{ best }.Execute(mContext);
	}

	void EditorApplication::BuildStatsPanel()
	{
		if (ImGui::Begin("Stats", &mShowStats))
		{
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			const ly::World* world = GetCurrentWorld();
			ImGui::TextWrapped("World: %s", world ? typeid(*world).name() : "None");
		}
		ImGui::End();
	}

}

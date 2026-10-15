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
	namespace
	{
		bool DrawProperty(const ly::reflect::PropertyInfo& property, ly::reflect::PropertyValue& value)
		{
			using ly::reflect::PropertyKind;
			const char* label = property.name.c_str();
			const float min = property.min.value_or(0.f);
			const float max = property.max.value_or(0.f);
			switch (property.kind)
			{
			case PropertyKind::Bool: return ImGui::Checkbox(label, &std::get<bool>(value));
			case PropertyKind::Int: return ImGui::DragInt(label, &std::get<int>(value), property.speed, static_cast<int>(min), static_cast<int>(max));
			case PropertyKind::Float: return ImGui::DragFloat(label, &std::get<float>(value), property.speed, min, max);
			case PropertyKind::Vector2:
			{
				auto& vector = std::get<sf::Vector2f>(value);
				float values[]{ vector.x, vector.y };
				const bool changed = ImGui::DragFloat2(label, values, property.speed, min, max);
				vector = { values[0], values[1] };
				return changed;
			}
			case PropertyKind::Color:
			{
				auto& color = std::get<sf::Color>(value);
				float channels[]{ color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f };
				const bool changed = ImGui::ColorEdit4(label, channels);
				if (changed) color = { static_cast<std::uint8_t>(std::clamp(channels[0], 0.f, 1.f) * 255.f + 0.5f),
					static_cast<std::uint8_t>(std::clamp(channels[1], 0.f, 1.f) * 255.f + 0.5f),
					static_cast<std::uint8_t>(std::clamp(channels[2], 0.f, 1.f) * 255.f + 0.5f),
					static_cast<std::uint8_t>(std::clamp(channels[3], 0.f, 1.f) * 255.f + 0.5f) };
				return changed;
			}
			case PropertyKind::String:
			{
				auto& text = std::get<std::string>(value);
				return ImGui::InputText(label, text.data(), text.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
					[](ImGuiInputTextCallbackData* data)
					{
						auto& buffer = *static_cast<std::string*>(data->UserData);
						buffer.resize(static_cast<std::size_t>(data->BufTextLen));
						data->Buf = buffer.data();
						return 0;
					}, &text);
			}
			case PropertyKind::Enum:
			{
				auto& number = std::get<int>(value);
				std::string preview = std::to_string(number);
				for (const auto& entry : property.enumLabels) if (entry.first == number) preview = entry.second;
				bool changed = false;
				if (ImGui::BeginCombo(label, preview.c_str()))
				{
					for (const auto& entry : property.enumLabels)
					{
						if (ImGui::Selectable(entry.second.c_str(), number == entry.first)) { number = entry.first; changed = true; }
					}
					ImGui::EndCombo();
				}
				return changed;
			}
			}
			return false;
		}
	}

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
			ImGui::BeginDisabled(world == nullptr);
			const bool editorCamera = world && world->GetViewOverride().has_value();
			if (ImGui::Button(editorCamera ? "Editor camera" : "Game camera")) mContext.SetEditorCamera(!editorCamera);
			ImGui::SetItemTooltip("Toggle camera. Editor camera: wheel zooms at cursor; middle/right drag pans. F focuses selection.");
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
				mViewportWindowFocused = ImGui::IsWindowFocused();
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
				if (ImGui::Button("Focus")) mContext.FocusSelection();
				const auto drawSection = [&](const ReflectedTarget& target)
				{
					if (!target.type) return;
					ImGui::PushID(target.object);
					if (ImGui::CollapsingHeader(target.type->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						const auto properties = target.type->AllProperties();
						std::vector<std::string> categories;
						for (const auto* property : properties)
							if (std::find(categories.begin(), categories.end(), property->category) == categories.end()) categories.push_back(property->category);
						for (const auto& category : categories)
						{
							if (!category.empty()) ImGui::SeparatorText(category.c_str());
							for (const auto* property : properties)
							{
								if (property->category != category) continue;
								auto value = property->getter(target.object);
								ImGui::BeginDisabled(!property->setter);
								// Reflected names can be longer than the old fixed labels.
								const float labelWidth = ImGui::CalcTextSize(property->name.c_str()).x;
								ImGui::SetNextItemWidth(std::max(100.f,
									ImGui::GetContentRegionAvail().x - labelWidth - ImGui::GetStyle().ItemInnerSpacing.x));
								const bool changed = DrawProperty(*property, value);
								if (ImGui::IsItemActivated()) mPropertyMergeGroup = mHistory.BeginMergeGroup();
								if (changed && property->setter)
								{
									mHistory.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIndex,
										property->name, std::move(value), (ImGui::IsItemActive() || ImGui::IsItemDeactivated()) ? mPropertyMergeGroup : 0));
								}
								ImGui::EndDisabled();
								if (!property->tooltip.empty()) ImGui::SetItemTooltip("%s", property->tooltip.c_str());
							}
						}
					}
					ImGui::PopID();
				};
				drawSection(ResolvePropertyTarget(*actor, {}));
				std::size_t componentIndex = 0;
				actor->ForEachComponent([&](ly::ActorComponent& component)
				{
					drawSection({ &component, ly::reflect::TypeRegistry::Get().FindMostDerived(component), componentIndex++ });
				});
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

	void EditorApplication::BuildExportDialog()
	{
		if (mExportDialogRequested)
		{
			ImGui::OpenPopup("Export Game");
			mExportDialogRequested = false;
		}
		ImGui::SetNextWindowSize({ 560.f, 0.f }, ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal("Export Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
		const bool idle = mExportPhase == ExportPhase::Idle;
		ImGui::BeginDisabled(!idle);
		ImGui::TextUnformatted("Output folder");
		ImGui::SetNextItemWidth(420.f);
		ImGui::InputText("##ExportDirectory", mExportDirectory, sizeof(mExportDirectory));
		ImGui::Checkbox("Overwrite a previous export", &mExportOverwrite);
		ImGui::SetItemTooltip("Removes only the files that an earlier export recorded in vega_export_manifest.txt. Other files stay.");
		ImGui::Checkbox("Run smoke test after export", &mExportSmokeTest);
		ImGui::EndDisabled();

		// Export is blocking. Its start is deferred one frame so the "Exporting..." line is presented first.
		if (mExportPhase == ExportPhase::Running)
		{
			GameExportRequest request;
			request.outputDir = std::filesystem::u8path(mExportDirectory);
			request.overwrite = mExportOverwrite;
			request.runSmokeTest = mExportSmokeTest;
			mExportResult = ExportGame(mExportLayout, request);
			mExportPhase = ExportPhase::Idle;
		}
		ImGui::BeginDisabled(!idle || mExportDirectory[0] == '\0');
		if (ImGui::Button("Export")) mExportPhase = ExportPhase::Starting;
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!idle);
		if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
		ImGui::EndDisabled();
		if (mExportPhase == ExportPhase::Starting)
		{
			ImGui::TextUnformatted("Exporting...");
			mExportPhase = ExportPhase::Running;
		}
		if (mExportResult)
		{
			ImGui::Separator();
			const bool passed = mExportResult->success && (!mExportResult->smokeTestRan || mExportResult->smokeTestPassed);
			ImGui::PushStyleColor(ImGuiCol_Text, passed ? ImVec4{ 0.45f, 0.85f, 0.45f, 1.f } : ImVec4{ 0.95f, 0.45f, 0.40f, 1.f });
			ImGui::TextWrapped("%s", DescribeGameExport(*mExportResult).c_str());
			ImGui::PopStyleColor();
		}
		ImGui::EndPopup();
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

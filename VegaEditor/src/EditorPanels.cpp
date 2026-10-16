#include "editor/EditorApplication.h"
#include "editor/EditorIcons.h"
#include "editor/PropertyWidgets.h"
#include "editor/EditorPrefabs.h"
#include <framework/World.h>
#include <framework/Actor.h>
#include <framework/scene/SceneEntityComponent.h>
#include <level/PlayerStart.h>
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
	namespace
	{
		bool DrawProperty(const ly::reflect::PropertyInfo& property, ly::reflect::PropertyValue& value, bool mixed = false)
		{
			using ly::reflect::PropertyKind;
			const char* label = property.name.c_str();
			const float min = property.min.value_or(0.f);
			const float max = property.max.value_or(0.f);
			switch (property.kind)
			{
			case PropertyKind::Bool: return ImGui::Checkbox(label, &std::get<bool>(value));
			case PropertyKind::Int: return ImGui::DragInt(label, &std::get<int>(value), property.speed, static_cast<int>(min), static_cast<int>(max), mixed ? "\xe2\x80\x94" : "%d");
			case PropertyKind::Float: return ImGui::DragFloat(label, &std::get<float>(value), property.speed, min, max, mixed ? "\xe2\x80\x94" : "%.3f");
			case PropertyKind::Vector2:
			{
				auto& vector = std::get<sf::Vector2f>(value);
				float values[]{ vector.x, vector.y };
				const bool changed = ImGui::DragFloat2(label, values, property.speed, min, max, mixed ? "\xe2\x80\x94" : "%.3f");
				vector = { values[0], values[1] };
				return changed;
			}
			case PropertyKind::Color:
			{
				auto& color = std::get<sf::Color>(value);
				float channels[]{ color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f };
				// ColorEdit has no mixed numeric format; keep a stable ID while the
				// mixed preview hides channels and labels its swatch with a dash.
				const std::string colorLabel = (mixed ? std::string{ "\xe2\x80\x94  " } : std::string{}) + property.name + "###" + property.name;
				const bool changed = ImGui::ColorEdit4(colorLabel.c_str(), channels, mixed ? ImGuiColorEditFlags_NoInputs : ImGuiColorEditFlags_None);
				if (changed) color = { static_cast<std::uint8_t>(std::clamp(channels[0], 0.f, 1.f) * 255.f + 0.5f),
					static_cast<std::uint8_t>(std::clamp(channels[1], 0.f, 1.f) * 255.f + 0.5f),
					static_cast<std::uint8_t>(std::clamp(channels[2], 0.f, 1.f) * 255.f + 0.5f),
					static_cast<std::uint8_t>(std::clamp(channels[3], 0.f, 1.f) * 255.f + 0.5f) };
				return changed;
			}
			case PropertyKind::String:
				return InputStringProperty(label, std::get<std::string>(value), mixed);
			case PropertyKind::Flags:
			{
				auto& bits = std::get<int>(value);
				std::string preview;
				for (const auto& bit : property.enumLabels)
					if (bits & bit.first) { if (!preview.empty()) preview += " | "; preview += bit.second; }
				bool changed = false;
				if (ImGui::BeginCombo(label, mixed ? "\xe2\x80\x94" : preview.empty() ? "None" : preview.c_str()))
				{
					for (const auto& bit : property.enumLabels)
					{
						bool enabled = (bits & bit.first) != 0;
						if (ImGui::Checkbox(bit.second.c_str(), &enabled))
						{
							bits = enabled ? bits | bit.first : bits & ~bit.first;
							changed = true;
						}
					}
					ImGui::EndCombo();
				}
				return changed;
			}
			case PropertyKind::Enum:
			{
				auto& number = std::get<int>(value);
				std::string preview = std::to_string(number);
				for (const auto& entry : property.enumLabels) if (entry.first == number) preview = entry.second;
				bool changed = false;
				if (ImGui::BeginCombo(label, mixed ? "\xe2\x80\x94" : preview.c_str()))
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
				if (!mContext.playMode && ImGui::BeginDragDropTarget())
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
					if (const auto* payload = ImGui::AcceptDragDropPayload("VEGA_PREFAB"))
					{
						const auto origin = ImGui::GetItemRectMin();
						const auto mouse = ImGui::GetMousePos();
						const sf::Vector2i pixel{ static_cast<int>((mouse.x - origin.x) / scale), static_cast<int>((mouse.y - origin.y) / scale) };
						PlacePrefab(mContext, mHistory, static_cast<const char*>(payload->Data), mViewport.MapPixelToCoords(pixel, mContext.world->GetWorldView()));
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
					for (const auto& weak : mContext.world->GetActorsByTypeIncludingPending<ly::PlayerStart>())
					{
						const auto start = weak.lock();
						if (!start || start->GetIsPendingDestroy()) continue;
						const auto pixel = mViewport.GetTexture().mapCoordsToPixel(start->GetActorLocation(), mContext.world->GetWorldView());
						const ImVec2 p{ topLeft.x + pixel.x * scale, topLeft.y + pixel.y * scale };
						draw->AddTriangleFilled({ p.x, p.y - 14.f }, { p.x - 11.f, p.y + 10.f }, { p.x + 11.f, p.y + 10.f }, IM_COL32(75, 224, 219, 245));
						draw->AddText({ p.x + 18.f, p.y - 8.f }, IM_COL32(230, 242, 250, 255), start->GetShip().c_str());
					}
					draw->PopClipRect();
				}
				mViewportImageRect = { { topLeft.x - mainViewport->Pos.x, topLeft.y - mainViewport->Pos.y },
					{ bottomRight.x - topLeft.x, bottomRight.y - topLeft.y } };
				mViewportImageHovered = ImGui::IsItemHovered();
				mViewportWindowFocused = ImGui::IsWindowFocused();
				mViewport.SetImagePresentation(mViewportImageRect, mViewportImageHovered, ImGui::IsWindowFocused());
				BuildGizmo();
			}
		}
		ImGui::End();
	}

	void EditorApplication::BuildHierarchyPanel()
	{
		LY_PROFILE_SCOPE("Editor Hierarchy");
		if (ImGui::Begin("Hierarchy", &mShowHierarchy))
		{
			if (ImGui::Button("World Settings")) { mShowWorldSettings = true; ImGui::SetWindowFocus("World Settings"); }
			ImGui::SameLine();
			ImGui::BeginDisabled(mContext.playMode || mContext.transitioning);
			if (ImGui::Button("+")) ImGui::OpenPopup("Create entity");
			if (ImGui::BeginPopup("Create entity")) { DrawCreateMenu(); ImGui::EndPopup(); }
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1.f);
			ImGui::InputTextWithHint("##ActorFilter", "Filter name or type", mHierarchyFilter, sizeof(mHierarchyFilter));
			ImGui::Separator();
			if (ImGui::BeginChild("ActorList"))
			{
				struct Row
				{
					std::shared_ptr<ly::Actor> actor;
					const ly::SceneEntityComponent* identity;
					const std::string* type;
				};
				// RTTI names are immutable. Cache once per type, not once per actor/frame.
				static std::map<std::type_index, std::string> typeNames;
				std::vector<Row> rows;
				if (mContext.world) rows.reserve(mContext.world->GetActorCountIncludingPending());
				const std::string_view filter = mHierarchyFilter;
				const auto matches = [&](std::string_view name, std::string_view type)
				{
					// Search the same "name type" text without allocating a concatenation.
					const auto character = [&](std::size_t i) { return i < name.size() ? name[i] : i == name.size() ? ' ' : type[i - name.size() - 1]; };
					const auto length = name.size() + 1 + type.size();
					for (std::size_t start = 0; start + filter.size() <= length; ++start)
					{
						std::size_t i = 0;
						while (i < filter.size() && std::tolower(static_cast<unsigned char>(character(start + i))) ==
							std::tolower(static_cast<unsigned char>(filter[i]))) ++i;
						if (i == filter.size()) return true;
					}
					return false;
				};
				if (mContext.world) for (const auto& weak : mContext.world->GetActorsByTypeIncludingPending<ly::Actor>())
				{
					const auto actor = weak.lock();
					if (!actor || actor->GetIsPendingDestroy()) continue;
					const auto* identity = actor->FindComponent<ly::SceneEntityComponent>();
					auto entry = typeNames.find(typeid(*actor));
					if (entry == typeNames.end()) entry = typeNames.emplace(typeid(*actor), ActorTypeName(*actor)).first;
					const auto& type = entry->second;
					if (!filter.empty())
					{
						const std::string fallback = identity ? std::string{} : type + " #" + std::to_string(actor->GetUniqueID());
						if (!matches(identity ? identity->name : fallback, type)) continue;
					}
					rows.push_back({ actor, identity, &type });
				}
				if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
				{
					mSelection.Clear();
					for (const auto& row : rows) mSelection.Add(row.actor);
				}
				const float rowHeight = 2.f * ImGui::GetTextLineHeightWithSpacing();
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(rows.size()), rowHeight);
				for (int index = 0; index < static_cast<int>(rows.size()); ++index)
				{
					const auto& row = rows[index];
					// Keep an active rename or context menu alive when scrolling clips its row.
					if ((row.identity && row.identity->guid == mRenameGuid) || mHierarchyPopupActor == row.actor->GetUniqueID()) clipper.IncludeItemByIndex(index);
				}
				while (clipper.Step()) for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index)
				{
					const auto& row = rows[index];
					const auto& actor = row.actor;
					if (actor->GetIsPendingDestroy()) { ImGui::Dummy({ 0.f, rowHeight - ImGui::GetStyle().ItemSpacing.y }); continue; }
					const auto* identity = row.identity;
					const auto& type = *row.type;
					const std::string fallback = identity ? std::string{} : type + " #" + std::to_string(actor->GetUniqueID());
					const auto& name = identity ? identity->name : fallback;
					const float rowStart = ImGui::GetCursorPosY();
					ImGui::PushID(static_cast<int>(actor->GetUniqueID()));
					if (identity && identity->guid == mRenameGuid)
					{
						if (mRenameFocus) { ImGui::SetKeyboardFocusHere(); mRenameFocus = false; }
						const bool enter = ImGui::InputText("##Rename", mRenameInput, sizeof(mRenameInput), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
						if (enter || ImGui::IsItemDeactivatedAfterEdit())
						{
							mSelection.Select(actor);
							RenameSceneEntity(mContext, mHistory, mRenameInput);
							mRenameGuid.clear();
						}
						if (ImGui::IsKeyPressed(ImGuiKey_Escape)) mRenameGuid.clear();
					}
					else
					{
						const bool prefab = identity && !identity->prefab.empty();
						if (prefab) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ .35f, .75f, 1.f, 1.f });
						if (ImGui::Selectable(name.c_str(), mSelection.Contains(actor), ImGuiSelectableFlags_AllowDoubleClick))
						{
							const auto& io = ImGui::GetIO();
							if (io.KeyShift && mHierarchyAnchor)
							{
								const auto anchor = std::find_if(rows.begin(), rows.end(), [&](const Row& entry) { return entry.actor->GetUniqueID() == *mHierarchyAnchor; });
								if (!io.KeyCtrl) mSelection.Clear();
								const int start = anchor == rows.end() ? index : static_cast<int>(anchor - rows.begin());
								for (int i = std::min(start, index); i <= std::max(start, index); ++i) mSelection.Add(rows[i].actor);
								mSelection.Add(actor);
							}
							else
							{
								if (io.KeyCtrl) mSelection.Toggle(actor); else mSelection.Select(actor);
								mHierarchyAnchor = actor->GetUniqueID();
							}
							if (mSelection.Count() == 1 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) BeginRename();
						}
						if (prefab) ImGui::PopStyleColor();
						if (!mContext.playMode && ImGui::BeginDragDropSource())
						{
							if (!mSelection.Contains(actor)) mSelection.Select(actor); else mSelection.Add(actor);
							const auto id = actor->GetUniqueID();
							ImGui::SetDragDropPayload("VEGA_ACTOR", &id, sizeof(id));
							ImGui::TextUnformatted(name.c_str());
							ImGui::EndDragDropSource();
						}
						if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
						{
							ImGui::OpenPopup("Entity actions");
							mHierarchyPopupActor = actor->GetUniqueID();
						}
						ImGui::TextDisabled("%s", type.c_str());
					}
					if (ImGui::BeginPopup("Entity actions"))
					{
						if (!mSelection.Contains(actor)) mSelection.Select(actor);
						ImGui::BeginDisabled(mContext.playMode || mContext.transitioning);
						DrawCreateMenu();
						if (ImGui::MenuItem("Create Prefab...", nullptr, false, identity != nullptr))
						{
							mVariantParent.clear();
							std::snprintf(mPrefabName, sizeof(mPrefabName), "%s", identity->name.c_str());
							mCreatePrefabDialog = true;
						}
						if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, identity != nullptr)) DuplicateSceneEntity(mContext, mHistory);
						if (ImGui::MenuItem("Rename", "F2", false, identity != nullptr)) BeginRename();
						if (ImGui::MenuItem("Delete", "Del", false, identity != nullptr)) DeleteSelection(mContext, mHistory);
						ImGui::EndDisabled();
						ImGui::EndPopup();
					}
					if (mHierarchyPopupActor == actor->GetUniqueID() && !ImGui::IsPopupOpen("Entity actions")) mHierarchyPopupActor.reset();
					ImGui::PopID();
					ImGui::SetCursorPosY(rowStart + rowHeight);
				}
				if (ImGui::BeginPopupContextWindow("Hierarchy actions", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
				{
					ImGui::BeginDisabled(mContext.playMode || mContext.transitioning);
					DrawCreateMenu();
					ImGui::EndDisabled();
					ImGui::EndPopup();
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}
	void EditorApplication::BuildInspectorPanel()
	{
		LY_PROFILE_SCOPE("Editor Inspector");
		auto& context = mPrefabWorld ? mPrefabContext : mContext;
		auto& selection = mPrefabWorld ? mPrefabSelection : mSelection;
		auto& history = mPrefabWorld ? mPrefabHistory : mHistory;
		if (ImGui::Begin("Inspector", &mShowInspector))
		{
			if (mPrefabWorld)
			{
				ImGui::Text("Prefab: %s%s", mPrefabPath.generic_string().c_str(), PrefabDirty() ? " *" : "");
				if (ImGui::Button("Save Prefab")) SavePrefab();
				ImGui::SameLine();
				if (ImGui::Button("Back to Scene")) RequestPrefab({});
			}
			if (selection.Count() > 1)
			{
				ImGui::Text("%zu actors selected", selection.Count());
				std::size_t prefabCount = 0;
				std::string prefab;
				bool mixed = false;
				for (const auto& selected : selection.GetAll())
				{
					const auto* identity = selected->FindComponent<ly::SceneEntityComponent>();
					if (!identity || identity->prefab.empty()) continue;
					if (prefabCount && prefab != identity->prefab) mixed = true;
					prefab = identity->prefab;
					++prefabCount;
				}
				if (prefabCount)
				{
					ImGui::TextDisabled("%zu prefab instances%s", prefabCount, mixed || prefabCount != selection.Count() ? " / mixed prefabs" : "");
				}
				if (ImGui::Button("Focus")) context.FocusSelection();
				ImGui::BeginDisabled(context.playMode || context.transitioning);
				std::string section;
				int index = 0;
				for (const auto& row : mMultiInspector.Get(context))
				{
					if (row.section != section) { section = row.section; ImGui::SeparatorText(section.c_str()); }
					ImGui::PushID(index++);
					auto value = row.value;
					ImGui::BeginDisabled(!row.property->setter);
					ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, row.mixed);
					const bool changed = DrawProperty(*row.property, value, row.mixed);
					ImGui::PopItemFlag();
					if (ImGui::IsItemActivated()) mPropertyMergeGroup = history.BeginMergeGroup();
					if (changed) SetCommonProperty(context, history, row, value,
						(ImGui::IsItemActive() || ImGui::IsItemDeactivated()) ? mPropertyMergeGroup : 0);
					ImGui::EndDisabled();
					ImGui::PopID();
				}
				if (!context.propertyError.empty()) ImGui::TextWrapped("%s", context.propertyError.c_str());
				if (ImGui::Button("Delete selection")) DeleteSelection(context, history);
				ImGui::EndDisabled();
			}
			else if (const auto actor = selection.Get())
			{
				if (!mPrefabWorld)
					if (const auto* identity = actor->FindComponent<ly::SceneEntityComponent>(); identity && !identity->prefab.empty())
					{
						ImGui::Text("Prefab: %s", std::filesystem::path(identity->prefab).stem().stem().string().c_str());
						ImGui::SameLine();
						if (ImGui::SmallButton("open")) RequestPrefab(identity->prefab);
						if (ImGui::SmallButton("Revert all")) ChangeAllPrefab(context, history, false);
						ImGui::SameLine();
						if (ImGui::SmallButton("Apply all")) ChangeAllPrefab(context, history, true);
					}
				ImGui::TextWrapped("%s  #%u", ActorTypeName(*actor).c_str(), actor->GetUniqueID());
				if (ImGui::Button("Focus")) context.FocusSelection();
				if (context.playMode) ImGui::TextWrapped("Play inspection only. Runtime changes are discarded on Stop.");
				ImGui::BeginDisabled(context.playMode || context.transitioning);
				if (const auto* identity = actor->FindComponent<ly::SceneEntityComponent>())
				{
					ly::reflect::PropertyInfo nameProperty;
					nameProperty.name = "Name";
					nameProperty.kind = ly::reflect::PropertyKind::String;
					ly::reflect::PropertyValue value = identity->name;
					const bool changed = DrawProperty(nameProperty, value);
					if (ImGui::IsItemActivated()) mPropertyMergeGroup = history.BeginMergeGroup();
					if (changed) RenameSceneEntity(context, history, std::get<std::string>(value), mPropertyMergeGroup);
				}
				std::unique_ptr<EditorCommand> componentEdit;
				static ImGuiTextFilter componentFilter;
				if (ImGui::Button("Add Component"))
				{
					componentFilter.Clear();
					ImGui::OpenPopup("Add Component");
				}
				if (ImGui::BeginPopup("Add Component"))
				{
					componentFilter.Draw("Search");
					auto types = ly::reflect::TypeRegistry::Get().AddableComponents();
					std::sort(types.begin(), types.end(), [](const auto* a, const auto* b) { return a->name < b->name; });
					for (const auto* type : types)
						if (componentFilter.PassFilter(type->name.c_str()) && ImGui::Selectable(type->name.c_str()))
							componentEdit = std::make_unique<AddComponentCommand>(actor->GetUniqueID(), type->name);
					ImGui::EndPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Add Script")) ImGui::OpenPopup("Add Script");
				ImGui::SetItemTooltip("Attach a Lua script. Adding a script cannot be undone in this step.");
				if (ImGui::BeginPopup("Add Script"))
				{
					std::error_code error;
					const auto& root = script::ScriptRuntime::Get().Root();
					std::vector<std::string> paths;
					for (std::filesystem::recursive_directory_iterator it{ root / "scripts", error }, end; it != end && !error; it.increment(error))
					{
						if (it->is_regular_file(error) && it->path().extension() == ".lua")
							paths.push_back(it->path().lexically_relative(root).generic_string());
					}
					std::sort(paths.begin(), paths.end());
					for (const auto& path : paths)
						if (ImGui::Selectable(path.c_str()))
						{
							actor->AddComponent<script::ScriptComponent>(path).Prepare();
							context.SceneChanged();
						}
					if (paths.empty()) ImGui::TextUnformatted("No .lua files under scripts/");
					ImGui::EndPopup();
				}
				const auto drawSection = [&](const ReflectedTarget& target)
				{
					if (!target.type) return;
					ImGui::PushID(target.object);
					const bool open = ImGui::CollapsingHeader(target.type->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
					if (ImGui::BeginPopupContextItem("Component actions"))
					{
						if (!mPrefabWorld)
						{
							if (ImGui::MenuItem("Revert all")) ChangePrefabSection(context, history, target, false);
							if (ImGui::MenuItem("Apply all")) ChangePrefabSection(context, history, target, true);
						}
						const auto* actorType = ly::reflect::TypeRegistry::Get().FindMostDerived(*actor);
						const bool required = actorType && actorType->RequiresComponent(target.type->name);
						const bool removable = target.componentIdentity && target.type->removable && !required;
						if (ImGui::MenuItem("Remove Component", nullptr, false, removable))
							componentEdit = std::make_unique<RemoveComponentCommand>(actor->GetUniqueID(), *target.componentIdentity);
						if (!removable && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
							ImGui::SetTooltip("%s", required ? "Required by this actor type." : "This component is constructor-owned and cannot be removed.");
						ImGui::EndPopup();
					}
					if (open)
					{
						auto* scriptComponent = target.componentIdentity
							? dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object)) : nullptr;
						if (scriptComponent)
						{
							ImGui::TextWrapped("Script: %s", scriptComponent->ScriptPath().c_str());
							if (!scriptComponent->LastError().empty())
							{
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f, 0.3f, 0.3f, 1.f });
								ImGui::TextWrapped("%s", scriptComponent->LastError().c_str());
								ImGui::PopStyleColor();
							}
							if (ImGui::Button("Reload"))
							{
								scriptComponent->Reload();
								context.SceneChanged();
							}
							for (const auto& entry : scriptComponent->Properties())
							{
								ly::reflect::PropertyInfo property;
								property.name = entry.first;
								property.kind = ly::reflect::KindOf(entry.second);
								auto value = entry.second;
								const bool overridden = !mPrefabWorld && PrefabPropertyOverridden(context, *actor, target, entry.first);
								if (overridden) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ .35f, .75f, 1.f, 1.f });
								const bool changed = DrawProperty(property, value);
								if (overridden) ImGui::PopStyleColor();
								if (ImGui::IsItemActivated()) mPropertyMergeGroup = history.BeginMergeGroup();
								if (changed) history.Execute(std::make_unique<SetScriptPropertyCommand>(actor->GetUniqueID(),
									*target.componentIdentity, entry.first, value, (ImGui::IsItemActive() || ImGui::IsItemDeactivated()) ? mPropertyMergeGroup : 0));
								PrefabPropertyMenu(*actor, target, entry.first);
							}
						}
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
								if (mPrefabWorld && !target.componentIdentity && property->name == "Location") continue;
								if (scriptComponent && (property->name == "Script" || property->name == "Last Error")) continue;
								auto value = property->getter(target.object);
								ImGui::BeginDisabled(!property->setter);
								// Reflected names can be longer than the old fixed labels.
								const float labelWidth = ImGui::CalcTextSize(property->name.c_str()).x;
								ImGui::SetNextItemWidth(std::max(100.f,
									ImGui::GetContentRegionAvail().x - labelWidth - ImGui::GetStyle().ItemInnerSpacing.x));
								const bool overridden = !mPrefabWorld && PrefabPropertyOverridden(context, *actor, target, property->name);
								if (overridden) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ .35f, .75f, 1.f, 1.f });
								const bool changed = DrawProperty(*property, value);
								if (overridden) ImGui::PopStyleColor();
								if (ImGui::IsItemActivated()) mPropertyMergeGroup = history.BeginMergeGroup();
								if (changed && property->setter)
								{
									history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIdentity,
										property->name, std::move(value), (ImGui::IsItemActive() || ImGui::IsItemDeactivated()) ? mPropertyMergeGroup : 0));
								}
								ImGui::EndDisabled();
								PrefabPropertyMenu(*actor, target, property->name);
								if (!property->tooltip.empty()) ImGui::SetItemTooltip("%s", property->tooltip.c_str());
								if (!context.propertyError.empty() && context.propertyErrorActor == actor->GetUniqueID() &&
									context.propertyErrorComponent == target.componentIdentity && context.propertyErrorName == property->name)
								{
									ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f, 0.3f, 0.3f, 1.f });
									ImGui::TextWrapped("%s", context.propertyError.c_str());
									ImGui::PopStyleColor();
								}
							}
						}
					}
					ImGui::PopID();
				};
				drawSection(ResolvePropertyTarget(*actor, {}));

				actor->ForEachComponent([&](ly::ActorComponent& component)
				{
					if (!dynamic_cast<ly::SceneEntityComponent*>(&component))
						drawSection({ &component, ly::reflect::TypeRegistry::Get().FindMostDerived(component), ly::IdentifyComponent(*actor, component) });

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
				if (ImGui::Button("Delete entity") && actor->FindComponent<ly::SceneEntityComponent>())
					history.Execute(std::make_unique<DestroyActor>(actor->GetUniqueID()));
				ImGui::EndDisabled();
				// Recreation invalidates the enumerated component pointers; apply after drawing.
				if (componentEdit) history.Execute(std::move(componentEdit));
			}
			else
			{
				ImGui::TextWrapped("Select an actor in the Hierarchy or paused viewport.");
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
				if (dynamic_cast<ly::PlayerStart*>(actor.get()) && !mContext.playMode)
					bounds = { actor->GetActorLocation() - sf::Vector2f{ 24.f, 24.f }, { 48.f, 48.f } };
				candidates.emplace_back(actor->GetUniqueID(), bounds);
			}
		}
		const auto best = PickBestActor(point, candidates);
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

	void EditorApplication::BuildExportDialog()
	{
		if (mExportFuture.valid() && mExportFuture.wait_for(std::chrono::seconds{ 0 }) == std::future_status::ready)
		{
			try { mExportResult = mExportFuture.get(); }
			catch (const std::exception& error) { mExportResult = GameExportResult{}; mExportResult->error = error.what(); }
		}
		if (mExportDialogRequested)
		{
			ImGui::OpenPopup("Export Game");
			mExportDialogRequested = false;
		}
		ImGui::SetNextWindowSize({ 560.f, 0.f }, ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal("Export Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
		const bool idle = !mExportFuture.valid();
		ImGui::BeginDisabled(!idle);
		ImGui::TextUnformatted("Output folder");
		ImGui::SetNextItemWidth(420.f);
		ImGui::InputText("##ExportDirectory", mExportDirectory, sizeof(mExportDirectory));
		ImGui::Checkbox("Overwrite a previous export", &mExportOverwrite);
		ImGui::SetItemTooltip("Removes only the files that an earlier export recorded in vega_export_manifest.txt. Other files stay.");
		ImGui::Checkbox("Run smoke test after export", &mExportSmokeTest);
		ImGui::EndDisabled();

		ImGui::BeginDisabled(!idle || mExportDirectory[0] == '\0');
		if (ImGui::Button("Export"))
		{
			GameExportRequest request;
			request.outputDir = std::filesystem::u8path(mExportDirectory);
			request.overwrite = mExportOverwrite;
			request.runSmokeTest = mExportSmokeTest;
			mExportResult.reset();
			try { mExportFuture = ExportGameAsync(mExportLayout, request); }
			catch (const std::exception& error) { mExportResult = GameExportResult{}; mExportResult->error = error.what(); }
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(mExportFuture.valid());
		if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
		ImGui::EndDisabled();
		if (mExportFuture.valid())
		{
			ImGui::TextUnformatted("Exporting\xe2\x80\xa6");
			ImGui::ProgressBar(-static_cast<float>(ImGui::GetTime()), { -1.f, 0.f }, "");
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
			ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate);
			ImGui::SameLine();
			ImGui::TextDisabled(" / %.2f ms", 1000.f / std::max(1.f, ImGui::GetIO().Framerate));
			ImGui::Separator();
			const ly::World* world = GetCurrentWorld();
			std::string name = world ? typeid(*world).name() : "None";
			if (const auto scope = name.rfind("::"); scope != std::string::npos) name.erase(0, scope + 2);
			ImGui::TextWrapped("World: %s", name.c_str());
			if (world)
			{
				std::size_t live = 0;
				for (const auto& weak : world->GetActorsByType<ly::Actor>())
					if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++live;
				ImGui::Text("Live actors: %zu", live);
				ImGui::TextDisabled("%s camera", world->GetViewOverride() ? "Editor" : "Game");
			}
		}
		ImGui::End();
	}

}

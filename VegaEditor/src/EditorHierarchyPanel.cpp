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
	void EditorApplication::BuildHierarchyPanel()
	{
		LY_PROFILE_SCOPE("Editor Hierarchy");
		auto& context = mPrefabWorld ? mPrefabContext : mContext;
		auto& selection = mPrefabWorld ? mPrefabSelection : mSelection;
		auto& history = mPrefabWorld ? mPrefabHistory : mHistory;
		if (ImGui::Begin("Hierarchy", &mShowHierarchy))
		{
			if (ImGui::Button("World Settings")) { mShowWorldSettings = true; ImGui::SetWindowFocus("World Settings"); }
			ImGui::SameLine();
			ImGui::BeginDisabled(context.playMode || context.transitioning);
			if (ImGui::Button("+")) ImGui::OpenPopup("Create entity");
			if (ImGui::BeginPopup("Create entity"))
			{
				DrawCreateMenu();
				ImGui::EndPopup();
			}
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
					int depth = 0;
				};
				// RTTI names are immutable. Cache once per type, not once per actor/frame.
				static std::map<std::type_index, std::string> typeNames;
				std::vector<Row> rows;
				if (context.world) rows.reserve(context.world->GetActorCountIncludingPending());
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
				if (context.world) for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
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
				static std::unordered_set<unsigned int> collapsed;
				std::unordered_map<ly::Actor*, std::vector<std::size_t>> descendants;
				std::unordered_set<ly::Actor*> visible;
				for (const auto& row : rows) visible.insert(row.actor.get());
				for (std::size_t i = 0; i < rows.size(); ++i)
					descendants[visible.count(rows[i].actor->GetParent()) ? rows[i].actor->GetParent() : nullptr].push_back(i);
				std::vector<Row> ordered;
				ordered.reserve(rows.size());
				const auto append = [&](auto&& self, ly::Actor* parent, int depth) -> void
				{
					const auto found = descendants.find(parent);
					if (found == descendants.end()) return;
					for (auto i : found->second)
					{
						auto row = rows[i]; row.depth = depth; ordered.push_back(row);
						if (!collapsed.count(row.actor->GetUniqueID()) || !filter.empty()) self(self, row.actor.get(), depth + 1);
					}
				};
				append(append, nullptr, 0);
				rows = std::move(ordered);
				if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
				{
					selection.Clear();
					for (const auto& row : rows) selection.Add(row.actor);
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
					if (row.depth) ImGui::Indent(row.depth * 16.f);
					const float labelX = ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing();
					if (identity && identity->guid == mRenameGuid)
					{
						if (mRenameFocus) { ImGui::SetKeyboardFocusHere(); mRenameFocus = false; }
						const bool enter = ImGui::InputText("##Rename", mRenameInput, sizeof(mRenameInput), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
						if (enter || ImGui::IsItemDeactivatedAfterEdit())
						{
							selection.Select(actor);
							RenameSceneEntity(context, history, mRenameInput);
							mRenameGuid.clear();
						}
						if (ImGui::IsKeyPressed(ImGuiKey_Escape)) mRenameGuid.clear();
					}
					else
					{
						const bool prefab = identity && !identity->prefab.empty();
						if (prefab) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ .35f, .75f, 1.f, 1.f });
						ImGui::SetNextItemOpen(!collapsed.count(actor->GetUniqueID()), ImGuiCond_Always);
						const bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow |
							ImGuiTreeNodeFlags_SpanAvailWidth | (actor->GetChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0) |
							(selection.Contains(actor) ? ImGuiTreeNodeFlags_Selected : 0));
						if (open) collapsed.erase(actor->GetUniqueID()); else collapsed.insert(actor->GetUniqueID());
						if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
						{
							const auto& io = ImGui::GetIO();
							if (io.KeyShift && mHierarchyAnchor)
							{
								const auto anchor = std::find_if(rows.begin(), rows.end(), [&](const Row& entry) { return entry.actor->GetUniqueID() == *mHierarchyAnchor; });
								if (!io.KeyCtrl) selection.Clear();
								const int start = anchor == rows.end() ? index : static_cast<int>(anchor - rows.begin());
								for (int i = std::min(start, index); i <= std::max(start, index); ++i) selection.Add(rows[i].actor);
								selection.Add(actor);
							}
							else
							{
								if (io.KeyCtrl) selection.Toggle(actor); else selection.Select(actor);
								mHierarchyAnchor = actor->GetUniqueID();
							}
							if (selection.Count() == 1 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) BeginRename();
						}
						if (prefab) ImGui::PopStyleColor();
						if (!context.playMode && ImGui::BeginDragDropSource())
						{
							if (!selection.Contains(actor)) selection.Select(actor); else selection.Add(actor);
							const auto id = actor->GetUniqueID();
							ImGui::SetDragDropPayload("VEGA_ACTOR", &id, sizeof(id));
							ImGui::TextUnformatted(name.c_str());
							ImGui::EndDragDropSource();
						}
						if (!context.playMode && ImGui::BeginDragDropTarget())
						{
							if (const auto* payload = ImGui::AcceptDragDropPayload("VEGA_ACTOR"))
							{
								if (const auto child = context.Resolve(*static_cast<const unsigned int*>(payload->Data)))
									if (const auto* childIdentity = child->FindComponent<ly::SceneEntityComponent>(); childIdentity && identity)
										history.Execute(std::make_unique<ReparentActor>(childIdentity->guid, ImGui::GetIO().KeyAlt ? std::string{} : identity->guid));
							}
							ImGui::EndDragDropTarget();
						}
						if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
						{
							ImGui::OpenPopup("Entity actions");
							mHierarchyPopupActor = actor->GetUniqueID();
						}
						ImGui::SetCursorPosX(labelX);
						ImGui::TextDisabled("%s", type.c_str());
					}
					if (ImGui::BeginPopup("Entity actions"))
					{
						if (!selection.Contains(actor)) selection.Select(actor);
						ImGui::BeginDisabled(context.playMode || context.transitioning);
						DrawCreateMenu();
						if (ImGui::MenuItem("Create Prefab...", nullptr, false, identity != nullptr && !mPrefabWorld))
						{
							mVariantParent.clear();
							std::snprintf(mPrefabName, sizeof(mPrefabName), "%s", identity->name.c_str());
							mCreatePrefabDialog = true;
						}
						if (ImGui::MenuItem("Detach", nullptr, false, identity && actor->GetParent()))
							history.Execute(std::make_unique<ReparentActor>(identity->guid, std::string{}));
						if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, identity != nullptr)) DuplicateSceneEntity(context, history);
						if (ImGui::MenuItem("Rename", "F2", false, identity != nullptr)) BeginRename();
						if (ImGui::MenuItem("Delete", "Del", false, identity != nullptr)) DeleteSelection(context, history);
						ImGui::EndDisabled();
						ImGui::EndPopup();
					}
					if (mHierarchyPopupActor == actor->GetUniqueID() && !ImGui::IsPopupOpen("Entity actions")) mHierarchyPopupActor.reset();
					ImGui::PopID();
					if (row.depth) ImGui::Unindent(row.depth * 16.f);
					ImGui::SetCursorPosY(rowStart + rowHeight);
				}
				if (ImGui::BeginPopupContextWindow("Hierarchy actions", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
				{
					ImGui::BeginDisabled(context.playMode || context.transitioning);
					DrawCreateMenu();
					ImGui::EndDisabled();
					ImGui::EndPopup();
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}
}

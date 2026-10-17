#include "editor/EditorApplication.h"
#include "editor/EditorAssetField.h"
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
	namespace
	{
		struct PropertyGesture { bool activated = false; bool continuous = false; };
		bool DrawProperty(const ly::reflect::PropertyInfo& property, ly::reflect::PropertyValue& value, bool mixed = false,
			EditorContext* context = nullptr, const std::function<void(int)>& pick = {}, PropertyGesture* gesture = nullptr, const std::filesystem::path& assetRoot = {});
		bool DrawPropertyBody(const ly::reflect::PropertyInfo& property, ly::reflect::PropertyValue& value, bool mixed,
			EditorContext* context, const std::function<void(int)>& pick, PropertyGesture* gesture, const std::filesystem::path& assetRoot)
		{
			using ly::reflect::PropertyKind;
			const char* label = property.name.c_str();
			const float min = property.min.value_or(0.f);
			const float max = property.max.value_or(0.f);
			switch (property.kind)
			{
			case PropertyKind::ActorRef:
			{
				auto& ref = std::get<ly::ActorRef>(value);
				const auto target = context && context->world ? ref.Resolve(*context->world) : nullptr;
				const auto* identity = target ? target->FindComponent<ly::SceneEntityComponent>() : nullptr;
				const auto preview = mixed ? std::string{ "\xe2\x80\x94" } : identity ? identity->name : ref.Guid().empty() ? "None" : "Missing: " + ref.Guid();
				bool changed = false;
				ImGui::PushID(label);
				if (ImGui::BeginCombo(label, preview.c_str()))
				{
					static ImGuiTextFilter filter;
					filter.Draw("Search");
					if (ImGui::Selectable("None", ref.Guid().empty())) { ref = ly::ActorRef{}; changed = true; }
					if (context && context->world) for (const auto& weak : context->world->GetActorsByTypeIncludingPending<ly::Actor>())
					{
						const auto actor = weak.lock();
						if (!actor || actor->GetIsPendingDestroy()) continue;
						const auto* data = actor->FindComponent<ly::SceneEntityComponent>();
						const auto* type = property.requiredType.empty() ? nullptr : context->Engine().Types().Find(property.requiredType);
						if (!data || (!property.requiredType.empty() && (!type || type->root != ly::reflect::TypeRoot::Actor || !type->isInstance(actor.get()))) || !filter.PassFilter(data->name.c_str())) continue;
						ImGui::PushID(data->guid.c_str());
						if (ImGui::Selectable(data->name.c_str(), ref.Guid() == data->guid)) { ref = ly::ActorRef{ *actor }; changed = true; }
						ImGui::PopID();
					}
					ImGui::EndCombo();
				}
				if (pick && ImGui::SmallButton("Pick in viewport")) pick(-1);
				ImGui::SameLine();
				ImGui::BeginDisabled(!target);
				if (ImGui::SmallButton("Select target") && context) context->selection.Select(target);
				ImGui::EndDisabled();
				ImGui::PopID();
				return changed;
			}
			case PropertyKind::Struct:
			{
				// Struct editing comes later; until then a struct is a read-only summary line.
				if (mixed) ImGui::Text("%s: \xe2\x80\x94", label);
				else ImGui::Text("%s: %s {%zu fields}", label, property.structType.c_str(), std::get<ly::reflect::StructValue>(value).size());
				return false;
			}
			case PropertyKind::List:
			{
				ImGui::PushID(label);
				if (property.elementKind == PropertyKind::Struct)
				{
					// Struct lists are summarised too; Add/Remove and element editing come with the struct editor.
					if (mixed) ImGui::Text("%s: \xe2\x80\x94", label);
					else ImGui::Text("%s: %zu \xc3\x97 %s", label, std::get<ly::reflect::PropertyList>(value).size(), property.structType.c_str());
					ImGui::PopID();
					return false;
				}
				if (mixed) { ImGui::Text("%s: \xe2\x80\x94", label); ImGui::PopID(); return false; }
				auto& list = std::get<ly::reflect::PropertyList>(value);
				bool changed = false;
				if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (std::size_t i = 0; i < list.size(); ++i)
					{
						ImGui::PushID(static_cast<int>(i));
						auto element = property.Element(); element.name = "[" + std::to_string(i) + "]";
						auto item = ly::reflect::ToValue(list[i]);
						if (DrawProperty(element, item, false, context, pick ? std::function<void(int)>{ [&, i](int) { pick(static_cast<int>(i)); } } : std::function<void(int)>{}, gesture))
						{ list[i] = ly::reflect::ToScalar(item); changed = true; }
						if (ListRowActions(list, i)) { changed = true; if (gesture) *gesture = {}; ImGui::PopID(); break; }
						ImGui::PopID();
					}
					if (ImGui::SmallButton("Add") && list.size() < 4096)
					{
						if (gesture) *gesture = {};
						switch (property.elementKind)
						{
						case PropertyKind::Bool: list.emplace_back(false); break;
						case PropertyKind::Int: case PropertyKind::Flags: case PropertyKind::Enum: list.emplace_back(property.enumLabels.empty() ? 0 : property.enumLabels.front().first); break;
						case PropertyKind::Float: list.emplace_back(0.f); break;
						case PropertyKind::Vector2: list.emplace_back(sf::Vector2f{}); break;
						case PropertyKind::Color: list.emplace_back(sf::Color::White); break;
						case PropertyKind::String: list.emplace_back(std::string{}); break;
						case PropertyKind::ActorRef: list.emplace_back(ly::ActorRef{}); break;
						default: break;
						}
						changed = true;
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
				return changed;
			}
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
				if (property.assetKind != ly::reflect::AssetKind::None) return DrawAssetField(property, std::get<std::string>(value), mixed, assetRoot);
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
		bool DrawProperty(const ly::reflect::PropertyInfo& property, ly::reflect::PropertyValue& value, bool mixed,
			EditorContext* context, const std::function<void(int)>& pick, PropertyGesture* gesture, const std::filesystem::path& assetRoot)
		{
			const bool changed = DrawPropertyBody(property, value, mixed, context, pick, gesture, assetRoot);
			// Asset edits are discrete: a picker choice or a drop is one undo step, never a merged drag.
			if (property.assetKind != ly::reflect::AssetKind::None)
			{
				if (gesture) *gesture = {};
				return changed;
			}
			if (gesture && property.kind != ly::reflect::PropertyKind::List && (changed || ImGui::IsItemActivated()))
			{
				gesture->activated = ImGui::IsItemActivated();
				gesture->continuous = property.kind != ly::reflect::PropertyKind::ActorRef && (ImGui::IsItemActive() || ImGui::IsItemDeactivated());
			}
			return changed;
		}
	}

	void EditorApplication::BuildInspectorPanel()
	{
		LY_PROFILE_SCOPE("Editor Inspector");
		auto& context = mPrefabWorld ? mPrefabContext : mContext;
		auto& selection = mPrefabWorld ? mPrefabSelection : mSelection;
		auto& history = mPrefabWorld ? mPrefabHistory : mHistory;
		const auto assetRoot = context.document ? context.document->Root() : std::filesystem::path{};
		if (ImGui::Begin("Inspector", &mShowInspector))
		{
			if (mPrefabWorld)
			{
				ImGui::Text("Prefab: %s%s", mPrefabPath.generic_u8string().c_str(), PrefabDirty() ? " *" : "");
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
					PropertyGesture gesture;
					const bool changed = DrawProperty(*row.property, value, row.mixed, &context, {}, &gesture, assetRoot);
					ImGui::PopItemFlag();
					if (gesture.activated) mPropertyMergeGroup = history.BeginMergeGroup();
					if (changed) SetCommonProperty(context, history, row, value,
						gesture.continuous ? mPropertyMergeGroup : 0);
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
						ImGui::Text("Prefab: %s", std::filesystem::path(identity->prefab).stem().stem().u8string().c_str());
						ImGui::SameLine();
						if (ImGui::SmallButton("open")) RequestPrefab(identity->prefab);
						if (ImGui::SmallButton("Revert all")) ChangeAllPrefab(context, history, false);
						ImGui::SameLine();
						if (ImGui::SmallButton("Apply all")) ChangeAllPrefab(context, history, true);
					}
				ImGui::TextWrapped("%s  #%u", ActorTypeName(*actor).c_str(), actor->GetUniqueID());
				const auto* parentIdentity = actor->GetParent() ? actor->GetParent()->FindComponent<ly::SceneEntityComponent>() : nullptr;
				ImGui::Text("Parent: %s", parentIdentity ? parentIdentity->name.c_str() : "none");
				ImGui::Checkbox("Relative Location / Rotation", &context.relativeTransforms);
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
					auto types = context.Engine().Types().AddableComponents();
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
					const auto& root = script::ScriptRuntime::Get(GetEngineContext()).Root();
					std::vector<std::string> paths;
					for (std::filesystem::recursive_directory_iterator it{ root / "scripts", error }, end; it != end && !error; it.increment(error))
					{
						if (it->is_regular_file(error) && it->path().extension() == ".lua")
							paths.push_back(it->path().lexically_relative(root).generic_u8string());
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
						const auto* actorType = context.Engine().Types().FindMostDerived(*actor);
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
								const auto cacheRevision = std::make_tuple(context.world->GetUniqueID(), context.sceneRevision, selection.Revision());
								if (mInspectorCacheRevision != cacheRevision || context.playMode) { mInspectorValues.clear(); mInspectorCacheRevision = cacheRevision; }
								const auto key = std::make_pair(target.object, property);
								auto cached = mInspectorValues.find(key);
								if (cached == mInspectorValues.end()) cached = mInspectorValues.emplace(key, property->getter(target.object)).first;
								auto value = cached->second;
								if (context.relativeTransforms && !target.componentIdentity && property->name == "Location") value = actor->GetRelativeLocation();
								if (context.relativeTransforms && !target.componentIdentity && property->name == "Rotation") value = actor->GetRelativeRotation();
								ImGui::BeginDisabled(!property->setter);
								// Reflected names can be longer than the old fixed labels.
								const float labelWidth = ImGui::CalcTextSize(property->name.c_str()).x;
								ImGui::SetNextItemWidth(std::max(100.f,
									ImGui::GetContentRegionAvail().x - labelWidth - ImGui::GetStyle().ItemInnerSpacing.x));
								const bool overridden = !mPrefabWorld && PrefabPropertyOverridden(context, *actor, target, property->name);
								if (overridden) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ .35f, .75f, 1.f, 1.f });
								PropertyGesture gesture;
								const bool changed = DrawProperty(*property, value, false, &context, [&](int index)
								{
									mReferencePick = ReferencePick{ actor->GetUniqueID(), target.componentIdentity, property->name, index };
								}, &gesture, assetRoot);
								if (overridden) ImGui::PopStyleColor();
								if (gesture.activated) mPropertyMergeGroup = history.BeginMergeGroup();
								if (changed && property->setter)
								{
									history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIdentity,
										property->name, std::move(value), gesture.continuous ? mPropertyMergeGroup : 0, context.relativeTransforms));
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
						drawSection({ &component, context.Engine().Types().FindMostDerived(component), ly::IdentifyComponent(*actor, component) });

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
}

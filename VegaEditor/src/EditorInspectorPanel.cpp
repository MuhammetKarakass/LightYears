#include "editor/EditorApplication.h"
#include "editor/EditorAssetField.h"
#include "editor/EditorFunctionCommands.h"
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
		// headerMenu draws right after a struct or struct-list header, the item a revert menu must anchor to.
		bool DrawProperty(const vega::reflect::PropertyInfo& property, vega::reflect::PropertyValue& value, bool mixed = false,
			EditorContext* context = nullptr, const std::function<void(int)>& pick = {}, PropertyGesture* gesture = nullptr, const std::filesystem::path& assetRoot = {},
			const std::function<void()>& headerMenu = {});
		bool DrawStructFields(const vega::reflect::PropertyInfo& property, vega::reflect::StructValue& fields, EditorContext* context,
			PropertyGesture* gesture, const std::filesystem::path& assetRoot);
		bool DrawStructElements(const vega::reflect::PropertyInfo& property, vega::reflect::PropertyList& list, EditorContext* context,
			PropertyGesture* gesture, const std::filesystem::path& assetRoot);
		bool DrawPropertyBody(const vega::reflect::PropertyInfo& property, vega::reflect::PropertyValue& value, bool mixed,
			EditorContext* context, const std::function<void(int)>& pick, PropertyGesture* gesture, const std::filesystem::path& assetRoot,
			const std::function<void()>& headerMenu)
		{
			using vega::reflect::PropertyKind;
			const char* label = property.name.c_str();
			const float min = property.min.value_or(0.f);
			const float max = property.max.value_or(0.f);
			switch (property.kind)
			{
			case PropertyKind::ActorRef:
			{
				auto& ref = std::get<vega::ActorRef>(value);
				const auto target = context && context->world ? ref.Resolve(*context->world) : nullptr;
				const auto* identity = target ? target->FindComponent<vega::SceneEntityComponent>() : nullptr;
				const auto preview = mixed ? std::string{ "\xe2\x80\x94" } : identity ? identity->name : ref.Guid().empty() ? "None" : "Missing: " + ref.Guid();
				bool changed = false;
				ImGui::PushID(label);
				if (ImGui::BeginCombo(label, preview.c_str()))
				{
					static ImGuiTextFilter filter;
					filter.Draw("Search");
					if (ImGui::Selectable("None", ref.Guid().empty())) { ref = vega::ActorRef{}; changed = true; }
					if (context && context->world) for (const auto& weak : context->world->GetActorsByTypeIncludingPending<vega::Actor>())
					{
						const auto actor = weak.lock();
						if (!actor || actor->GetIsPendingDestroy()) continue;
						const auto* data = actor->FindComponent<vega::SceneEntityComponent>();
						const auto* type = property.requiredType.empty() ? nullptr : context->Engine().Types().Find(property.requiredType);
						if (!data || (!property.requiredType.empty() && (!type || type->root != vega::reflect::TypeRoot::Actor || !type->isInstance(actor.get()))) || !filter.PassFilter(data->name.c_str())) continue;
						ImGui::PushID(data->guid.c_str());
						if (ImGui::Selectable(data->name.c_str(), ref.Guid() == data->guid)) { ref = vega::ActorRef{ *actor }; changed = true; }
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
				// Without a registered struct there are no field descriptors, so only the summary can be shown.
				if (!property.structInfo)
				{
					if (mixed) ImGui::Text("%s: \xe2\x80\x94", label);
					else ImGui::Text("%s: %s {%zu fields}", label, property.structType.c_str(), std::get<vega::reflect::StructValue>(value).size());
					return false;
				}
				ImGui::PushID(label);
				bool changed = false;
				if (mixed) ImGui::Text("%s: \xe2\x80\x94", label);
				else
				{
					const bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen);
					if (headerMenu) headerMenu();
					if (open)
					{
						changed = DrawStructFields(property, std::get<vega::reflect::StructValue>(value), context, gesture, assetRoot);
						ImGui::TreePop();
					}
				}
				ImGui::PopID();
				return changed;
			}
			case PropertyKind::List:
			{
				ImGui::PushID(label);
				if (property.elementKind == PropertyKind::Struct)
				{
					bool changed = false;
					if (mixed) ImGui::Text("%s: \xe2\x80\x94", label);
					else
					{
						const bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen);
						if (headerMenu) headerMenu();
						if (open)
						{
							changed = DrawStructElements(property, std::get<vega::reflect::PropertyList>(value), context, gesture, assetRoot);
							ImGui::TreePop();
						}
					}
					ImGui::PopID();
					return changed;
				}
				if (mixed) { ImGui::Text("%s: \xe2\x80\x94", label); ImGui::PopID(); return false; }
				auto& list = std::get<vega::reflect::PropertyList>(value);
				bool changed = false;
				if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (std::size_t i = 0; i < list.size(); ++i)
					{
						ImGui::PushID(static_cast<int>(i));
						auto element = property.Element(); element.name = "[" + std::to_string(i) + "]";
						auto item = vega::reflect::ToValue(list[i]);
						if (DrawProperty(element, item, false, context, pick ? std::function<void(int)>{ [&, i](int) { pick(static_cast<int>(i)); } } : std::function<void(int)>{}, gesture))
						{ list[i] = vega::reflect::ToScalar(item); changed = true; }
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
						case PropertyKind::ActorRef: list.emplace_back(vega::ActorRef{}); break;
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
				if (property.assetKind != vega::reflect::AssetKind::None) return DrawAssetField(property, std::get<std::string>(value), mixed, assetRoot);
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
		bool DrawProperty(const vega::reflect::PropertyInfo& property, vega::reflect::PropertyValue& value, bool mixed,
			EditorContext* context, const std::function<void(int)>& pick, PropertyGesture* gesture, const std::filesystem::path& assetRoot,
			const std::function<void()>& headerMenu)
		{
			const bool changed = DrawPropertyBody(property, value, mixed, context, pick, gesture, assetRoot, headerMenu);
			// Asset edits are discrete: a picker choice or a drop is one undo step, never a merged drag.
			if (property.assetKind != vega::reflect::AssetKind::None)
			{
				if (gesture) *gesture = {};
				return changed;
			}
			// Struct gestures come from their fields, so the last widget of the header must not overwrite them.
			if (gesture && property.kind != vega::reflect::PropertyKind::List && property.kind != vega::reflect::PropertyKind::Struct &&
				(changed || ImGui::IsItemActivated()))
			{
				gesture->activated = ImGui::IsItemActivated();
				gesture->continuous = property.kind != vega::reflect::PropertyKind::ActorRef && (ImGui::IsItemActive() || ImGui::IsItemDeactivated());
			}
			return changed;
		}
		// Names a list element by its first non-empty text field, such as a prefab path, else by its struct type.
		std::string StructSummary(const vega::reflect::PropertyInfo& property, const vega::reflect::StructValue& fields)
		{
			for (const auto& entry : fields)
				if (const auto* text = std::get_if<std::string>(&entry.second); text && !text->empty()) return *text;
			return property.structType;
		}
		// Each field is drawn from its own descriptor, so ranges, labels, asset kinds, type filters and tooltips
		// match top-level properties. Field values are copies: the caller stores the whole struct once a field changes.
		bool DrawStructFields(const vega::reflect::PropertyInfo& property, vega::reflect::StructValue& fields, EditorContext* context,
			PropertyGesture* gesture, const std::filesystem::path& assetRoot)
		{
			bool changed = false;
			if (!property.structInfo) return false;
			for (const auto& field : property.structInfo->properties)
			{
				// Read-only fields are absent from a struct value, so they are not drawn.
				const auto entry = std::find_if(fields.begin(), fields.end(), [&](const auto& item) { return item.first == field.name; });
				if (entry == fields.end()) continue;
				ImGui::PushID(field.name.c_str());
				auto fieldValue = vega::reflect::ToPropertyValue(entry->second);
				// A field's own gesture decides merging; the struct keeps the activation and the last change's continuity.
				PropertyGesture local;
				const bool fieldChanged = DrawProperty(field, fieldValue, false, context, {}, &local, assetRoot);
				if (!field.tooltip.empty()) ImGui::SetItemTooltip("%s", field.tooltip.c_str());
				ImGui::PopID();
				if (gesture && local.activated) gesture->activated = true;
				if (fieldChanged)
				{
					entry->second = vega::reflect::ToLeaf(fieldValue);
					changed = true;
					if (gesture) gesture->continuous = local.continuous;
				}
			}
			return changed;
		}
		// One sub-tree per element. Remove, Up and Down edit the list; the loop ends after a structural edit
		// because the element indices it holds are then stale.
		bool DrawStructElements(const vega::reflect::PropertyInfo& property, vega::reflect::PropertyList& list, EditorContext* context,
			PropertyGesture* gesture, const std::filesystem::path& assetRoot)
		{
			bool changed = false;
			for (std::size_t i = 0; i < list.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				auto item = vega::reflect::ToValue(list[i]);
				auto& fields = std::get<vega::reflect::StructValue>(item);
				bool structural = false;
				if (ImGui::TreeNodeEx("element", ImGuiTreeNodeFlags_DefaultOpen, "[%zu]  %s", i, StructSummary(property, fields).c_str()))
				{
					if (DrawStructFields(property, fields, context, gesture, assetRoot)) { list[i] = vega::reflect::ToScalar(item); changed = true; }
					structural = ListRowActions(list, i);
					ImGui::TreePop();
				}
				ImGui::PopID();
				if (structural)
				{
					changed = true;
					if (gesture) *gesture = {};
					break;
				}
			}
			if (ImGui::SmallButton("Add") && list.size() < 4096 && property.structInfo && property.structInfo->makeDefault)
			{
				// Adding is discrete: it never merges with a drag, and it starts from the struct field defaults.
				if (gesture) *gesture = {};
				list.emplace_back(property.structInfo->makeDefault());
				changed = true;
			}
			return changed;
		}

		// A CallInEditor button. It lives outside the panel-wide disabled block so a function that opted
		// into play mode stays clickable while playing; every other function is refused in play mode.
		bool DrawFunctionButton(EditorContext& context, const std::string& label, const std::string& tooltip, bool callableInPlay)
		{
			ImGui::BeginDisabled(context.transitioning || (context.playMode && !callableInPlay));
			const bool clicked = ImGui::Button(label.c_str());
			ImGui::EndDisabled();
			if (!tooltip.empty()) ImGui::SetItemTooltip("%s", tooltip.c_str());
			return clicked;
		}
		// A failed call reports like a refused property write; the name matches the function, not a property.
		void DrawFunctionError(const EditorContext& context, const std::string& functionName)
		{
			if (context.propertyError.empty() || context.propertyErrorName != functionName) return;
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f, 0.3f, 0.3f, 1.f });
			ImGui::TextWrapped("%s", context.propertyError.c_str());
			ImGui::PopStyleColor();
		}
		// Draws one Actions group per reflected type of the actor that declares functions.
		void DrawActorFunctions(EditorContext& context, vega::Actor& actor)
		{
			struct Group
			{
				void* object = nullptr;
				const vega::reflect::TypeInfo* type = nullptr;
				std::optional<vega::ComponentIdentity> identity;
			};
			std::vector<Group> groups;
			if (const auto root = ResolvePropertyTarget(actor, {}); root.type) groups.push_back({ root.object, root.type, {} });
			actor.ForEachComponent([&](vega::ActorComponent& component)
			{
				if (dynamic_cast<vega::SceneEntityComponent*>(&component)) return;
				if (const auto* type = context.Engine().Types().FindMostDerived(component))
					groups.push_back({ &component, type, vega::IdentifyComponent(actor, component) });
			});
			bool any = false;
			for (const auto& group : groups)
			{
				const auto functions = group.type->AllFunctions();
				if (functions.empty()) continue;
				if (!any) { ImGui::SeparatorText("Actions"); any = true; }
				ImGui::PushID(group.object);
				for (const auto* function : functions)
				{
					if (DrawFunctionButton(context, function->label, function->tooltip, function->callableInPlay))
					{
						std::string error;
						// A call is not undoable; a successful call still marks the scene dirty for autosave.
						if (CallReflectedFunction(context, ReflectedTarget{ group.object, group.type, group.identity },
								actor.GetUniqueID(), function->name, error))
						{
							context.SceneChanged();
						}
					}
					DrawFunctionError(context, function->name);
				}
				ImGui::PopID();
			}
		}
		// Functions shared by every selected actor, drawn once and run on all of them.
		void DrawSelectionFunctions(EditorContext& context)
		{
			const auto functions = CommonSelectionFunctions(context);
			if (functions.empty()) return;
			ImGui::SeparatorText("Actions");
			int index = 0;
			for (const auto& row : functions)
			{
				ImGui::PushID(index++);
				if (DrawFunctionButton(context, row.label, row.tooltip, row.callableInPlay))
				{
					std::string error;
					InvokeSelectionFunction(context, row, error);
				}
				DrawFunctionError(context, row.name);
				ImGui::PopID();
			}
		}
	}

	void EditorApplication::BuildInspectorPanel()
	{
		VEGA_PROFILE_SCOPE("Editor Inspector");
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
					const auto* identity = selected->FindComponent<vega::SceneEntityComponent>();
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
				// Actions stay outside the disabled block so play-mode-callable functions remain clickable.
				DrawSelectionFunctions(context);
			}
			else if (const auto actor = selection.Get())
			{
				if (!mPrefabWorld)
					if (const auto* identity = actor->FindComponent<vega::SceneEntityComponent>(); identity && !identity->prefab.empty())
					{
						ImGui::Text("Prefab: %s", std::filesystem::path(identity->prefab).stem().stem().u8string().c_str());
						ImGui::SameLine();
						if (ImGui::SmallButton("open")) RequestPrefab(identity->prefab);
						if (ImGui::SmallButton("Revert all")) ChangeAllPrefab(context, history, false);
						ImGui::SameLine();
						if (ImGui::SmallButton("Apply all")) ChangeAllPrefab(context, history, true);
					}
				ImGui::TextWrapped("%s  #%u", ActorTypeName(*actor).c_str(), actor->GetUniqueID());
				const auto* parentIdentity = actor->GetParent() ? actor->GetParent()->FindComponent<vega::SceneEntityComponent>() : nullptr;
				ImGui::Text("Parent: %s", parentIdentity ? parentIdentity->name.c_str() : "none");
				ImGui::Checkbox("Relative Location / Rotation", &context.relativeTransforms);
				if (ImGui::Button("Focus")) context.FocusSelection();
				if (context.playMode) ImGui::TextWrapped("Play inspection only. Runtime changes are discarded on Stop.");
				ImGui::BeginDisabled(context.playMode || context.transitioning);
				if (const auto* identity = actor->FindComponent<vega::SceneEntityComponent>())
				{
					vega::reflect::PropertyInfo nameProperty;
					nameProperty.name = "Name";
					nameProperty.kind = vega::reflect::PropertyKind::String;
					vega::reflect::PropertyValue value = identity->name;
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
							? dynamic_cast<script::ScriptComponent*>(static_cast<vega::ActorComponent*>(target.object)) : nullptr;
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
								vega::reflect::PropertyInfo property;
								property.name = entry.first;
								property.kind = vega::reflect::KindOf(entry.second);
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
								// Struct and struct-list headers anchor the revert menu, because their last widget is a field, not the property.
								const bool anchorsHeader = property->kind == vega::reflect::PropertyKind::Struct ||
									(property->kind == vega::reflect::PropertyKind::List && property->elementKind == vega::reflect::PropertyKind::Struct);
								const std::function<void()> headerMenu = [&] { PrefabPropertyMenu(*actor, target, property->name); };
								const bool changed = DrawProperty(*property, value, false, &context, [&](int index)
								{
									mReferencePick = ReferencePick{ actor->GetUniqueID(), target.componentIdentity, property->name, index };
								}, &gesture, assetRoot, headerMenu);
								if (overridden) ImGui::PopStyleColor();
								if (gesture.activated) mPropertyMergeGroup = history.BeginMergeGroup();
								if (changed && property->setter)
								{
									history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIdentity,
										property->name, std::move(value), gesture.continuous ? mPropertyMergeGroup : 0, context.relativeTransforms));
								}
								ImGui::EndDisabled();
								if (!anchorsHeader) PrefabPropertyMenu(*actor, target, property->name);
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

				actor->ForEachComponent([&](vega::ActorComponent& component)
				{
					if (!dynamic_cast<vega::SceneEntityComponent*>(&component))
						drawSection({ &component, context.Engine().Types().FindMostDerived(component), vega::IdentifyComponent(*actor, component) });

				});
				if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("Physics enabled: %s", actor->IsPhysicsEnabled() ? "Yes" : "No");
					ImGui::Text("Has body: %s", actor->HasPhysicsBody() ? "Yes" : "No");
					ImGui::Text("Body type: %s", actor->GetPhysicsBodyType() == vega::PhysicsBodyType::Static ? "Static" : "Dynamic");
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
				if (ImGui::Button("Delete entity") && actor->FindComponent<vega::SceneEntityComponent>())
					history.Execute(std::make_unique<DestroyActor>(actor->GetUniqueID()));
				ImGui::EndDisabled();
				// Actions stay outside the disabled block so play-mode-callable functions remain clickable.
				DrawActorFunctions(context, *actor);
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

#include "editor/EditorContentAuthoring.h"
#include "editor/NumberFormatting.h"
#include <imgui_internal.h>
#include <algorithm>
#include <cstdio>

namespace vega
{
	namespace
	{
		using namespace ly::content;
		bool Text(const char* label, std::string& text)
		{
			return ImGui::InputText(label, text.data(), text.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
				[](ImGuiInputTextCallbackData* data)
				{
					auto& buffer = *static_cast<std::string*>(data->UserData);
					buffer.resize(static_cast<std::size_t>(data->BufTextLen));
					data->Buf = buffer.data(); return 0;
				}, &text);
		}
		bool TagTree(const std::vector<std::string>& values, const std::string& prefix, std::string& selected, const ImGuiTextFilter& filter)
		{
			bool changed = false;
			std::map<std::string, std::vector<std::string>> children;
			for (const auto& value : values)
			{
				if (!filter.PassFilter(value.c_str())) continue;
				const auto end = value.find('.', prefix.size());
				children[value.substr(0, end)].push_back(value);
			}
			for (const auto& [key, entries] : children)
			{
				const auto label = key.substr(prefix.size());
				const bool leaf = entries.size() == 1 && entries.front() == key;
				if (leaf)
				{
					if (ImGui::Selectable(label.c_str(), selected == key)) { selected = key; changed = true; }
				}
				else if (ImGui::TreeNode(key.c_str(), "%s", label.c_str()))
				{
					if (std::find(entries.begin(), entries.end(), key) != entries.end() && ImGui::Selectable("Use parent tag", selected == key)) { selected = key; changed = true; }
					std::vector<std::string> descendants;
					for (const auto& entry : entries) if (entry != key) descendants.push_back(entry);
					changed = TagTree(descendants, key + '.', selected, filter) || changed;
					ImGui::TreePop();
				}
			}
			return changed;
		}
	}

	Json EditorContentAuthoring::Default(const std::string& pattern) const
	{
		const auto* field = mWorkspace.Field(mWorkspace.selectedType, pattern);
		Json result = field ? field->defaultValue : Json{};
		if (result.is_object())
		{
			const auto* schema = ContentSchemaRegistry::Get().Find(mWorkspace.selectedType);
			for (const auto& child : schema->fields)
			{
				if (child.path.rfind(pattern + '/', 0) != 0) continue;
				const auto key = child.path.substr(pattern.size() + 1);
				if (key != "*" && key.find('/') == std::string::npos) result[key] = Default(child.path);
			}
		}
		return result;
	}

	void EditorContentAuthoring::RefreshCache()
	{
		const auto selection = mWorkspace.selectedType + ':' + mWorkspace.selectedId;
		if (mCachedRevision == mWorkspace.Revision() && mCachedSelection == selection) return;
		mCachedRevision = mWorkspace.Revision(); mCachedSelection = selection;
		mWorkspace.Entries(); mWorkspace.Validate();
		mOptions.clear(); mOther.clear();
		const auto* schema = ContentSchemaRegistry::Get().Find(mWorkspace.selectedType);
		if (schema) for (const auto& field : schema->fields) mOptions[field.path] = mWorkspace.Options(field);
		if (const auto* item = mWorkspace.Selected())
		{
			std::function<void(const Json&, const std::string&, const std::string&)> visit;
			visit = [&](const Json& value, const std::string& path, const std::string& pattern)
			{
				if (!path.empty() && !mWorkspace.Field(mWorkspace.selectedType, pattern)) { mOther[path] = value.dump(2); return; }
				if (value.is_structured()) for (auto it = value.begin(); it != value.end(); ++it)
				{
					const auto key = value.is_array() ? std::to_string(it - value.begin()) : JsonDocument::EscapePointer(it.key());
					visit(it.value(), path + '/' + key, pattern + '/' + (value.is_array() ? "*" : key));
				}
			};
			visit(*item, "", "");
		}
	}

	void EditorContentAuthoring::DrawObject(const Json& value, const std::string& path, const std::string& pattern, CommandHistory& history)
	{
		const auto* schema = ContentSchemaRegistry::Get().Find(mWorkspace.selectedType);
		if (!schema) return;
		std::map<std::string, std::vector<const ContentField*>> categories;
		std::vector<const ContentField*> missing;
		for (const auto& field : schema->fields)
		{
			if (field.path.rfind(pattern + '/', 0) != 0) continue;
			const auto key = field.path.substr(pattern.size() + 1);
			if (key == "*" || key.find('/') != std::string::npos) continue;
			if (value.contains(key)) categories[path.empty() ? field.category : std::string{}].push_back(&field);
			else missing.push_back(&field);
		}
		for (const auto& [category, fields] : categories)
		{
			if (!category.empty() && !ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) continue;
			for (const auto* field : fields)
			{
				const auto key = field->path.substr(pattern.size() + 1);
				DrawField(value.at(key), path + '/' + key, field->path, key, history);
			}
		}
		ImGui::PushID(path.c_str());
		if (!missing.empty() && ImGui::TreeNode("Optional fields"))
		{
			for (const auto* field : missing)
			{
				const auto key = field->path.substr(pattern.size() + 1);
				if (ImGui::SmallButton(("Add " + key).c_str())) mPendingEdit = PendingEdit{ path + '/' + key, Default(field->path), 0 };
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", field->tooltip.c_str());
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void EditorContentAuthoring::DrawField(const Json& value, const std::string& path, const std::string& pattern, const std::string& label, CommandHistory& history)
	{
		const auto* field = mWorkspace.Field(mWorkspace.selectedType, pattern);
		if (!field) return;
		ImGui::PushID(path.c_str());
		if (value.is_structured() && !mJumpPath.empty() && mJumpPath.rfind(path + '/', 0) == 0) ImGui::SetNextItemOpen(true);
		if (path == mJumpPath) { ImGui::SetScrollHereY(.3f); mJumpPath.clear(); }
		if (value.is_object())
		{
			if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) { DrawObject(value, path, pattern, history); ImGui::TreePop(); }
		}
		else if (value.is_array())
		{
			if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::BeginTable("Values", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable))
				{
					ImGui::TableSetupColumn("Level / row", ImGuiTableColumnFlags_WidthFixed, 70.f);
					ImGui::TableSetupColumn("Value"); ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.f); ImGui::TableHeadersRow();
					for (std::size_t i = 0; i < value.size(); ++i)
					{
						ImGui::PushID(static_cast<int>(i));
						ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::Text("%zu", i + 1); ImGui::TableNextColumn();
						DrawField(value[i], path + '/' + std::to_string(i), pattern + "/*", "##Value", history);
						ImGui::TableNextColumn();
						if (ImGui::SmallButton("-")) { auto after = value; after.erase(after.begin() + static_cast<Json::difference_type>(i)); mPendingEdit = PendingEdit{ path, std::move(after), 0 }; }
						ImGui::PopID();
					}
					ImGui::EndTable();
				}
				ImGui::BeginDisabled(!mWorkspace.Field(mWorkspace.selectedType, pattern + "/*"));
				if (ImGui::SmallButton("Add row"))
				{
					auto after = value;
					after.push_back(value.empty() ? Default(pattern + "/*") : value.back());
					mPendingEdit = PendingEdit{ path, std::move(after), 0 };
				}
				ImGui::EndDisabled();
				if (field->kind == FieldKind::Curve && !value.empty())
				{
					// Curve samples are cached on revision in RefreshCache; JSON is never
					// reparsed or reconstructed by the form's frame loop.
					bool numeric = true;
					for (const auto& item : value) numeric = numeric && item.is_number();
					if (numeric) ImGui::PlotLines("Preview", [](void* data, int i) { return static_cast<const Json*>(data)->at(i).get<float>(); }, const_cast<Json*>(&value), static_cast<int>(value.size()), 0, nullptr, FLT_MAX, FLT_MAX, { 0, 65.f });
				}
				ImGui::TreePop();
			}
		}
		else if (path == "/id") ImGui::TextWrapped("ID: %s", value.get_ref<const std::string&>().c_str());
		else
		{
			bool changed = false;
			Json after;
			ImGui::SetNextItemWidth(std::min(360.f, ImGui::GetContentRegionAvail().x * .6f));
			if (value.is_boolean()) { bool v = value.get<bool>(); changed = ImGui::Checkbox(label.c_str(), &v); if (changed) after = v; }
			else if (value.is_number())
			{
				double v = value.get<double>();
				changed = ImGui::InputDouble(label.c_str(), &v, 0., 0., DoubleInputFormat(v).c_str());
				if (changed) after = v;
			}
			else if (value.is_string())
			{
				auto text = value.get<std::string>();
				const auto& options = mOptions[pattern];
				if (field->kind == FieldKind::Tag || field->kind == FieldKind::Attribute || field->kind == FieldKind::Reference || field->kind == FieldKind::Enum)
				{
					if (ImGui::BeginCombo(label.c_str(), text.c_str()))
					{
						mPickerFilter.Draw("Search", 220.f);
						if (field->kind == FieldKind::Tag) changed = TagTree(options, "", text, mPickerFilter);
						else for (const auto& option : options) if (mPickerFilter.PassFilter(option.c_str()) && ImGui::Selectable(option.c_str(), option == text)) { text = option; changed = true; }
						ImGui::EndCombo();
					}
					if (field->kind != FieldKind::Enum)
					{
						ImGui::SameLine();
						if (ImGui::SmallButton("Edit ID")) ImGui::OpenPopup("Custom ID");
						if (ImGui::BeginPopup("Custom ID")) { changed = Text("ID", text) || changed; ImGui::EndPopup(); }
					}
					if (field->kind == FieldKind::Reference)
					{
						ImGui::SameLine();
						if (ImGui::SmallButton("Jump")) { mJumpType = field->referenceType; mJumpId = text; }
					}
				}
				else changed = Text(label.c_str(), text);
				if (changed) after = text;
			}
			if (ImGui::IsItemActivated()) mMergeGroup = history.BeginMergeGroup();
			if (changed) mPendingEdit = PendingEdit{ path, std::move(after), mMergeGroup };
			if (ImGui::IsItemDeactivated()) mMergeGroup = 0;
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s\nCategory: %s\nRange: %g .. %g %s", field->tooltip.c_str(), field->category.c_str(), field->minimum, field->maximum, field->unit.c_str());
		}
		ImGui::PopID();
	}

	void EditorContentAuthoring::Build(EditorContext& context, CommandHistory& history, const std::filesystem::path& root)
	{
		if (!visible) return;
		if (!mWorkspace.Loaded()) mWorkspace.Load(root);
		if (mConfirmExit) { ImGui::OpenPopup("Unsaved content changes"); mConfirmExit = false; }
		if (ImGui::BeginPopupModal("Unsaved content changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("Save content changes before exiting?");
			if (!mWorkspace.error.empty()) ImGui::TextWrapped("%s", mWorkspace.error.c_str());
			bool resume = false;
			if (ImGui::Button("Save")) resume = mWorkspace.Save();
			ImGui::SameLine(); if (ImGui::Button("Discard")) resume = mWorkspace.Revert();
			ImGui::SameLine(); if (ImGui::Button("Cancel")) { mExit = {}; ImGui::CloseCurrentPopup(); }
			if (resume) { ImGui::CloseCurrentPopup(); auto exit = std::move(mExit); if (exit) exit(); }
			ImGui::EndPopup();
		}
		RefreshCache();
		ImGui::SetNextWindowSize({ 1100.f, 680.f }, ImGuiCond_FirstUseEver);
		if (!ImGui::FindWindowSettingsByID(ImHashStr("Content Authoring")))
			if (const auto* viewport = ImGui::FindWindowByName("Viewport"); viewport && viewport->DockId)
				ImGui::SetNextWindowDockID(viewport->DockId, ImGuiCond_FirstUseEver);
		if (mFocus) { ImGui::SetNextWindowFocus(); mFocus = false; }
		if (ImGui::Begin("Content Authoring", &visible))
		{
			ImGui::TextUnformatted("Applies on next Play (restart editor for compiled content catalogs)");
			ImGui::BeginDisabled(context.playMode);
			if (ImGui::Button(mWorkspace.Dirty() ? "Save *" : "Save")) mWorkspace.Save();
			ImGui::SameLine(); if (ImGui::Button("Reload files")) mWorkspace.Load(root);
			ImGui::SameLine(); if (ImGui::Button("Undo")) history.Undo();
			ImGui::SameLine(); if (ImGui::Button("Redo")) history.Redo();
			ImGui::EndDisabled();
			ImGui::SameLine(); if (ImGui::Button("Validate all")) { mWorkspace.Validate(); mShowReport = true; }
			if (!mWorkspace.error.empty()) ImGui::TextColored({1.f,.45f,.35f,1.f}, "%s", mWorkspace.error.c_str());
			if (ImGui::BeginChild("Definitions", { ImGui::GetContentRegionAvail().x * .3f, 0 }, ImGuiChildFlags_Borders))
			{
				if (ImGui::BeginCombo("Type", mWorkspace.selectedType.c_str()))
				{
					for (const auto& [type, schema] : ContentSchemaRegistry::Get().Types()) if (ImGui::Selectable(type.c_str(), type == mWorkspace.selectedType)) { mWorkspace.selectedType = type; mWorkspace.selectedId.clear(); }
					ImGui::EndCombo();
				}
				const float filterWidth = std::max(80.f, ImGui::GetContentRegionAvail().x - 105.f);
				mFilter.Draw("Search", filterWidth); mTagFilter.Draw("Tag/category", filterWidth);
				for (const auto& entry : mWorkspace.Entries())
				{
					if (entry.type != mWorkspace.selectedType || !mFilter.PassFilter(entry.searchText.c_str()) || !mTagFilter.PassFilter(entry.searchText.c_str())) continue;
					const auto label = entry.id + " (" + std::to_string(entry.usedBy) + " used by)";
					if (ImGui::Selectable(label.c_str(), entry.id == mWorkspace.selectedId)) mWorkspace.Open(entry.type, entry.id);
				}
			}
			ImGui::EndChild(); ImGui::SameLine();
			if (ImGui::BeginChild("Form", {}, ImGuiChildFlags_Borders))
			{
				ImGui::TextWrapped("%s", mWorkspace.selectedId.c_str());
				ImGui::BeginDisabled(context.playMode);
				for (const auto* operation : { "New", "Duplicate", "Rename", "Delete" })
				{
					if (ImGui::Button(operation))
					{
						mOperation = operation;
						std::snprintf(mSource, sizeof(mSource), "%s", mWorkspace.selectedId.c_str());
						std::snprintf(mNewId, sizeof(mNewId), "%s", mWorkspace.selectedId.c_str());
						ImGui::OpenPopup("Content operation");
					}
					ImGui::SameLine();
				}
				ImGui::EndDisabled();
				if (ImGui::Button("Find references")) mShowReferences = !mShowReferences;
				if (mShowReferences)
					for (const auto& ref : mWorkspace.References()) if (ref.targetType == mWorkspace.selectedType && ref.targetId == mWorkspace.selectedId)
						if (ImGui::Selectable((ref.type + " " + ref.id + " " + ref.path).c_str())) { mJumpType = ref.type; mJumpId = ref.id; mJumpPath = ref.path; }
				if (ImGui::BeginPopupModal("Content operation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("%s %s", mOperation.c_str(), mWorkspace.selectedId.c_str());
					if (mOperation == "New")
					{
						if (ImGui::BeginCombo("Template", mSource))
						{
							const auto* schema = ContentSchemaRegistry::Get().Find(mWorkspace.selectedType);
							for (const auto& [name, value] : schema->templates) if (ImGui::Selectable(name.c_str())) std::snprintf(mSource, sizeof(mSource), "%s", name.c_str());
							for (const auto& entry : mWorkspace.Entries()) if (entry.type == mWorkspace.selectedType && ImGui::Selectable(entry.id.c_str())) std::snprintf(mSource, sizeof(mSource), "%s", entry.id.c_str());
							ImGui::EndCombo();
						}
					}
					if (mOperation != "Delete") ImGui::InputText("New ID", mNewId, sizeof(mNewId));
					if (mOperation == "Rename" || mOperation == "Delete")
					{
						ImGui::TextUnformatted(mOperation == "Rename" ? "Preview: ID and these references change in one undo step:" : "Still referenced by (remove references before deletion):");
						for (const auto& ref : mWorkspace.References()) if (ref.targetType == mWorkspace.selectedType && ref.targetId == mWorkspace.selectedId)
							ImGui::TextWrapped("%s %s %s", ref.type.c_str(), ref.id.c_str(), ref.path.c_str());
					}
					if (!mWorkspace.error.empty()) ImGui::TextWrapped("%s", mWorkspace.error.c_str());
					if (ImGui::Button("Apply"))
					{
						const bool ok = mOperation == "Delete" ? mWorkspace.Delete(history, mWorkspace.selectedType, mWorkspace.selectedId) :
							mOperation == "Rename" ? mWorkspace.Rename(history, mWorkspace.selectedType, mWorkspace.selectedId, mNewId) :
							mWorkspace.New(history, mWorkspace.selectedType, mSource, mNewId);
						if (ok) ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
				}
				RefreshCache();
				for (const auto& issue : mWorkspace.Validate()) if (issue.type == mWorkspace.selectedType && issue.id == mWorkspace.selectedId)
					ImGui::TextColored({1.f,.45f,.35f,1.f}, "%s %s", issue.path.c_str(), issue.message.c_str());
				ImGui::BeginDisabled(context.playMode);
				if (const auto* selected = mWorkspace.Selected()) DrawObject(*selected, "", "", history);
				ImGui::EndDisabled();
				if (!mOther.empty() && ImGui::CollapsingHeader("Other (preserved, read-only)"))
					for (const auto& [path, text] : mOther) { ImGui::TextDisabled("%s", path.c_str()); ImGui::TextWrapped("%s", text.c_str()); }
			}
			ImGui::EndChild();
		}
		ImGui::End();
		// Apply after the tree traversal: widget references belong to this revision.
		if (mPendingEdit) { mWorkspace.Set(history, mPendingEdit->path, mPendingEdit->value, mPendingEdit->group); mPendingEdit.reset(); }
		if (!mJumpId.empty()) { mWorkspace.Open(mJumpType, mJumpId); mJumpId.clear(); }
		if (mShowReport)
		{
			if (ImGui::Begin("Content validation", &mShowReport))
			{
				const auto& issues = mWorkspace.Validate();
				ImGui::Text("%zu issue(s)", issues.size());
				for (const auto& issue : issues)
				{
					const auto label = issue.type + " " + issue.id + issue.path + ": " + issue.message;
					if (ImGui::Selectable(label.c_str())) { mWorkspace.Open(issue.type, issue.id); mJumpPath = issue.path; }
				}
			}
			ImGui::End();
		}
	}

	bool EditorContentAuthoring::Execute(EditorContext& context, CommandHistory& history, const std::filesystem::path& root, const EditorScriptCommand& command, std::string& detail)
	{
		visible = true;
		mFocus = true;
		if (!mWorkspace.Loaded() && !mWorkspace.Load(root)) { detail = mWorkspace.error; return false; }
		using Op = ScriptOperation;
		bool result = false;
		if (context.playMode && command.operation != Op::ContentOpen && command.operation != Op::ExpectContent && command.operation != Op::ContentValidate) { detail = "Content edits require Edit mode"; return false; }
		switch (command.operation)
		{
		case Op::ContentOpen: result = mWorkspace.Open(command.text, command.property); break;
		case Op::ContentSet: result = mWorkspace.Set(history, command.text, Json::parse(command.values.at(0))); break;
		case Op::ContentNew: result = mWorkspace.New(history, command.text, command.property, command.values.at(0)); break;
		case Op::ContentRename: result = mWorkspace.Rename(history, command.text, command.property, command.values.at(0)); break;
		case Op::ContentSave: result = mWorkspace.Save(); break;
		case Op::ContentReload: result = mWorkspace.Load(root); break;
		case Op::ContentValidate:
			result = true;
			for (const auto& issue : mWorkspace.Validate())
			{
				detail += issue.type + " " + issue.id + issue.path + ": " + issue.message + '\n';
				if (!issue.warning) result = false;
			}
			if (detail.empty()) detail = "All content definitions and references validated";
			return result;
		case Op::ExpectContent:
			try
			{
				if (const auto* item = mWorkspace.Definition(command.text, command.property))
				{
					const auto& actual = item->at(Json::json_pointer{ ContentWorkspace::Pointer(command.values.at(0)) });
					const auto expected = Json::parse(command.values.at(1));
					result = actual == expected;
					detail = "actual " + actual.dump() + " expected " + expected.dump();
				}
			}
			catch (const std::exception& exception) { detail = exception.what(); }
			return result;
		default: detail = "Unknown content command"; return false;
		}
		detail = mWorkspace.error;
		return result;
	}
}

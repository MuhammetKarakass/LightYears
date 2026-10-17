#include "editor/EditorAssetField.h"
#include "editor/EditorCommands.h"
#include <framework/Actor.h>
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/Texture.hpp>
#include <algorithm>
#include <map>
#include <memory>
#include <system_error>

namespace vega
{
	bool ResolveAssetFile(const std::filesystem::path& root, const std::filesystem::path& relative, std::filesystem::path& file)
	{
		if (root.empty() || relative.empty() || relative.is_absolute() || relative.has_root_name()) return false;
		std::error_code error;
		const auto base = std::filesystem::weakly_canonical(root, error);
		if (error) return false;
		const auto candidate = std::filesystem::weakly_canonical(base / relative, error);
		if (error) return false;
		if (std::mismatch(base.begin(), base.end(), candidate.begin(), candidate.end()).first != base.end()) return false;
		file = candidate;
		return std::filesystem::is_regular_file(candidate, error);
	}

	std::vector<std::string> ListAssetChoices(const std::filesystem::path& root, ly::reflect::AssetKind kind)
	{
		std::vector<std::string> choices;
		if (root.empty() || kind == ly::reflect::AssetKind::None) return choices;
		std::error_code error;
		for (std::filesystem::recursive_directory_iterator it{ root, error }, end; it != end && !error; it.increment(error))
		{
			const auto name = it->path().filename().u8string();
			// Hidden folders (.vega/Trash, autosaves) are editor storage, not content.
			if (!name.empty() && name.front() == '.')
			{
				if (it->is_directory(error)) it.disable_recursion_pending();
				continue;
			}
			if (it->is_regular_file(error) && ly::reflect::AcceptsAsset(kind, it->path()))
				choices.push_back(it->path().lexically_relative(root).generic_u8string());
		}
		std::sort(choices.begin(), choices.end());
		return choices;
	}

	bool CheckAssetAssignment(const std::filesystem::path& root, const ly::reflect::PropertyInfo& property,
		const std::filesystem::path& relative, std::string& error)
	{
		if (property.assetKind == ly::reflect::AssetKind::None) { error = property.name + " is not an asset property"; return false; }
		if (!ly::reflect::AcceptsAsset(property.assetKind, relative))
		{
			error = relative.filename().u8string() + " is not a " + ly::reflect::AssetKindName(property.assetKind) +
				" asset (" + ly::reflect::AssetKindFilter(property.assetKind) + ")";
			return false;
		}
		std::filesystem::path file;
		if (!ResolveAssetFile(root, relative, file))
		{
			error = "Not a file under the asset root: " + relative.generic_u8string();
			return false;
		}
		return true;
	}

	bool AssignAssetProperty(EditorContext& context, CommandHistory& history, ly::Actor& actor, const ReflectedTarget& target,
		const ly::reflect::PropertyInfo& property, const std::filesystem::path& root, const std::filesystem::path& relative, std::string& error)
	{
		if (!property.setter) { error = property.name + " is read-only"; return false; }
		if (!CheckAssetAssignment(root, property, relative, error)) return false;
		if (!history.Execute(std::make_unique<SetPropertyCommand>(actor.GetUniqueID(), target.componentIdentity, property.name,
			ly::reflect::PropertyValue{ relative.generic_u8string() })))
		{
			error = context.propertyError;
			return false;
		}
		return true;
	}

	namespace
	{
		// Thumbnails are keyed by path and write time, so a replaced image shows up without a restart.
		// The cache is bounded; clearing it only costs a reload.
		const sf::Texture* AssetThumbnail(const std::filesystem::path& file)
		{
			static std::map<std::string, std::unique_ptr<sf::Texture>> cache;
			std::error_code error;
			const auto stamp = std::filesystem::last_write_time(file, error).time_since_epoch().count();
			const auto key = file.u8string() + "|" + std::to_string(stamp);
			if (const auto found = cache.find(key); found != cache.end()) return found->second.get();
			if (cache.size() >= 32) cache.clear();
			auto texture = std::make_unique<sf::Texture>();
			// A failed load is cached as null, so an unreadable image is not reloaded every frame.
			if (!texture->loadFromFile(file)) texture.reset();
			return cache.emplace(key, std::move(texture)).first->second.get();
		}
	}

	bool DrawAssetField(const ly::reflect::PropertyInfo& property, std::string& value, bool mixed, const std::filesystem::path& root)
	{
		static ImGuiTextFilter filter;
		static std::vector<std::string> choices;
		static std::map<ImGuiID, std::string> refusals;
		const char* label = property.name.c_str();
		const bool editable = static_cast<bool>(property.setter);
		const auto id = ImGui::GetID(label);
		std::filesystem::path file;
		const bool resolved = !mixed && !value.empty() && ResolveAssetFile(root, value, file);
		const bool missing = !mixed && !value.empty() && !resolved;
		bool changed = false;
		ImGui::PushID(label);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SameLine();
		const std::string preview = mixed ? "\xe2\x80\x94" : value.empty() ? "None" : value;
		if (!editable)
		{
			ImGui::TextDisabled("%s", preview.c_str());
		}
		else
		{
			// Button, not BeginCombo: the drop target then attaches to the item on screen and the picker stays a plain popup.
			if (ImGui::Button(preview.c_str()))
			{
				filter.Clear();
				choices = ListAssetChoices(root, property.assetKind);
				ImGui::OpenPopup("Asset picker");
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (const auto* payload = ImGui::AcceptDragDropPayload("VEGA_ASSET"))
				{
					const auto relative = std::filesystem::u8path(static_cast<const char*>(payload->Data));
					std::string error;
					if (CheckAssetAssignment(root, property, relative, error))
					{
						value = relative.generic_u8string();
						refusals.erase(id);
						changed = true;
					}
					else refusals[id] = error;
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(value.empty() && !mixed);
			if (ImGui::SmallButton("Clear"))
			{
				value.clear();
				refusals.erase(id);
				changed = true;
			}
			ImGui::EndDisabled();
			if (ImGui::BeginPopup("Asset picker"))
			{
				filter.Draw("Search");
				if (ImGui::Selectable("None", value.empty())) { value.clear(); refusals.erase(id); changed = true; ImGui::CloseCurrentPopup(); }
				for (const auto& choice : choices)
					if (filter.PassFilter(choice.c_str()) && ImGui::Selectable(choice.c_str(), choice == value))
					{
						value = choice;
						refusals.erase(id);
						changed = true;
						ImGui::CloseCurrentPopup();
					}
				if (choices.empty()) ImGui::TextDisabled("No %s files under the asset root", ly::reflect::AssetKindFilter(property.assetKind));
				ImGui::EndPopup();
			}
		}
		if (missing) ImGui::TextColored(ImVec4{ 1.f, 0.7f, 0.2f, 1.f }, "Missing asset");
		if (const auto refusal = refusals.find(id); editable && refusal != refusals.end())
			ImGui::TextColored(ImVec4{ 1.f, 0.3f, 0.3f, 1.f }, "Refused: %s", refusal->second.c_str());
		if (resolved && property.assetKind == ly::reflect::AssetKind::Texture)
		{
			if (const auto* texture = AssetThumbnail(file))
			{
				const auto size = texture->getSize();
				const float scale = std::min({ 1.f, 64.f / static_cast<float>(size.x), 64.f / static_cast<float>(size.y) });
				ImGui::Image(*texture, sf::Vector2f{ size.x * scale, size.y * scale });
			}
		}
		ImGui::PopID();
		return changed;
	}
}

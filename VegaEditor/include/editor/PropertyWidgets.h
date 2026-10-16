#pragma once

#include <imgui.h>
#include <map>
#include <string>
#include <utility>

namespace vega
{
	// Report one structural edit; the caller submits the complete list as one command.
	template<typename List>
	bool ListRowActions(List& list, std::size_t index)
	{
		if (ImGui::SmallButton("Remove")) { list.erase(list.begin() + index); return true; }
		ImGui::SameLine();
		if (ImGui::SmallButton("Up") && index > 0) { std::swap(list[index], list[index - 1]); return true; }
		ImGui::SameLine();
		if (ImGui::SmallButton("Down") && index + 1 < list.size()) { std::swap(list[index], list[index + 1]); return true; }
		return false;
	}

	inline bool InputStringProperty(const char* label, std::string& value, bool mixed = false)
	{
		// Retain the draft across frames without writing a partially typed asset path.
		static std::map<ImGuiID, std::string> drafts;
		const auto id = ImGui::GetID(label);
		auto& draft = drafts.try_emplace(id, mixed ? std::string{} : value).first->second;
		const bool enter = ImGui::InputTextWithHint(label, mixed ? "\xe2\x80\x94" : "", draft.data(), draft.capacity() + 1,
			ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_EnterReturnsTrue,
			[](ImGuiInputTextCallbackData* data)
			{
				auto& buffer = *static_cast<std::string*>(data->UserData);
				buffer.resize(static_cast<std::size_t>(data->BufTextLen));
				data->Buf = buffer.data();
				return 0;
			}, &draft);
		const bool commit = enter || ImGui::IsItemDeactivatedAfterEdit();
		if (commit) value = draft;
		if (!ImGui::IsItemActive()) drafts.erase(id);
		return commit;
	}
}

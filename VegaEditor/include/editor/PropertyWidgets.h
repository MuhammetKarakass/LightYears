#pragma once

#include <imgui.h>
#include <map>
#include <string>

namespace vega
{
	inline bool InputStringProperty(const char* label, std::string& value)
	{
		// Retain the draft across frames without writing a partially typed asset path.
		static std::map<ImGuiID, std::string> drafts;
		const auto id = ImGui::GetID(label);
		auto& draft = drafts.try_emplace(id, value).first->second;
		const bool enter = ImGui::InputText(label, draft.data(), draft.capacity() + 1,
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

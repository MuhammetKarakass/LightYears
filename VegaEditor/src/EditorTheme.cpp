#include "editor/EditorTheme.h"
#include <imgui.h>
#include <imgui-SFML.h>
#include <filesystem>

namespace vega
{
	void ApplyEditorTheme(const std::filesystem::path& fontPath)
	{
		auto& style = ImGui::GetStyle();
		style.WindowPadding = { 12.f, 10.f };
		style.FramePadding = { 8.f, 5.f };
		style.ItemSpacing = { 8.f, 6.f };
		style.ItemInnerSpacing = { 6.f, 4.f };
		style.WindowRounding = style.ChildRounding = style.FrameRounding = style.TabRounding = 4.f;
		style.PopupRounding = style.GrabRounding = 4.f;
		style.ScrollbarRounding = 6.f;
		style.ScrollbarSize = 12.f;
		style.WindowBorderSize = style.ChildBorderSize = style.PopupBorderSize = 1.f;
		style.FrameBorderSize = 0.f;
		style.DockingSeparatorSize = 4.f;
		const ImVec4 accent{ 0.20f, 0.78f, 0.79f, 1.f };
		auto* c = style.Colors;
		// Start with a complete neutral palette, then give interactive states a single teal accent.
		for (int i = 0; i < ImGuiCol_COUNT; ++i) c[i] = { 0.22f, 0.28f, 0.34f, 1.f };
		c[ImGuiCol_Text] = { 0.85f, 0.89f, 0.93f, 1.f };
		c[ImGuiCol_TextDisabled] = { 0.46f, 0.54f, 0.62f, 1.f };
		c[ImGuiCol_WindowBg] = { 0.075f, 0.095f, 0.125f, 1.f };
		c[ImGuiCol_ChildBg] = { 0.065f, 0.085f, 0.11f, 1.f };
		c[ImGuiCol_PopupBg] = { 0.10f, 0.13f, 0.17f, 0.99f };
		c[ImGuiCol_Border] = { 0.20f, 0.26f, 0.32f, 0.65f };
		c[ImGuiCol_BorderShadow] = { 0.f, 0.f, 0.f, 0.f };
		c[ImGuiCol_FrameBg] = { 0.12f, 0.16f, 0.20f, 1.f };
		c[ImGuiCol_FrameBgHovered] = { 0.17f, 0.26f, 0.31f, 1.f };
		c[ImGuiCol_FrameBgActive] = { 0.15f, 0.34f, 0.38f, 1.f };
		c[ImGuiCol_TitleBg] = c[ImGuiCol_TitleBgCollapsed] = { 0.08f, 0.11f, 0.14f, 1.f };
		c[ImGuiCol_TitleBgActive] = { 0.11f, 0.17f, 0.21f, 1.f };
		c[ImGuiCol_MenuBarBg] = { 0.065f, 0.085f, 0.11f, 1.f };
		c[ImGuiCol_ScrollbarBg] = { 0.05f, 0.07f, 0.09f, 0.5f };
		c[ImGuiCol_ScrollbarGrab] = { 0.23f, 0.30f, 0.36f, 1.f };
		c[ImGuiCol_ScrollbarGrabHovered] = { 0.29f, 0.40f, 0.46f, 1.f };
		c[ImGuiCol_ScrollbarGrabActive] = accent;
		c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = c[ImGuiCol_SliderGrabActive] = accent;
		c[ImGuiCol_Button] = { 0.14f, 0.20f, 0.25f, 1.f };
		c[ImGuiCol_ButtonHovered] = { 0.18f, 0.34f, 0.39f, 1.f };
		c[ImGuiCol_ButtonActive] = { 0.15f, 0.46f, 0.49f, 1.f };
		c[ImGuiCol_Header] = { 0.12f, 0.25f, 0.29f, 1.f };
		c[ImGuiCol_HeaderHovered] = { 0.16f, 0.35f, 0.39f, 1.f };
		c[ImGuiCol_HeaderActive] = { 0.16f, 0.43f, 0.46f, 1.f };
		c[ImGuiCol_Separator] = { 0.22f, 0.30f, 0.36f, 0.65f };
		c[ImGuiCol_SeparatorHovered] = c[ImGuiCol_SeparatorActive] = accent;
		c[ImGuiCol_ResizeGrip] = { 0.20f, 0.78f, 0.79f, 0.15f };
		c[ImGuiCol_ResizeGripHovered] = c[ImGuiCol_ResizeGripActive] = accent;
		c[ImGuiCol_Tab] = c[ImGuiCol_TabDimmed] = { 0.09f, 0.13f, 0.17f, 1.f };
		c[ImGuiCol_TabSelected] = c[ImGuiCol_TabDimmedSelected] = { 0.15f, 0.23f, 0.28f, 1.f };
		c[ImGuiCol_TabHovered] = { 0.18f, 0.34f, 0.39f, 1.f };
		c[ImGuiCol_TabSelectedOverline] = c[ImGuiCol_TabDimmedSelectedOverline] = accent;
		c[ImGuiCol_DockingPreview] = { 0.20f, 0.78f, 0.79f, 0.35f };
		c[ImGuiCol_DockingEmptyBg] = c[ImGuiCol_WindowBg];
		c[ImGuiCol_TableHeaderBg] = c[ImGuiCol_FrameBg];
		c[ImGuiCol_TableBorderStrong] = c[ImGuiCol_TableBorderLight] = c[ImGuiCol_Separator];
		c[ImGuiCol_TableRowBg] = { 0.f, 0.f, 0.f, 0.f };
		c[ImGuiCol_TableRowBgAlt] = { 0.4f, 0.6f, 0.8f, 0.035f };
		c[ImGuiCol_TextSelectedBg] = { 0.20f, 0.78f, 0.79f, 0.30f };
		c[ImGuiCol_NavHighlight] = accent;
		c[ImGuiCol_ModalWindowDimBg] = { 0.02f, 0.03f, 0.05f, 0.7f };

		std::error_code error;
		if (std::filesystem::is_regular_file(fontPath, error))
		{
			// Mixed Inspector values use an em dash outside the default Latin glyph range.
			static const ImWchar glyphs[]{ 0x0020, 0x00ff, 0x2014, 0x2014, 0 };
			if (auto* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 16.f, nullptr, glyphs))
			{
				ImGui::GetIO().FontDefault = font;
				if (!ImGui::SFML::UpdateFontTexture()) ImGui::GetIO().FontDefault = nullptr;
			}
		}
	}
}

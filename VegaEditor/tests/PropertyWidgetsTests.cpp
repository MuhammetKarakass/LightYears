#include "editor/PropertyWidgets.h"
#include <iostream>

int main()
{
	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = { 800.f, 600.f };
	io.DeltaTime = 1.f / 60.f;
	io.ConfigInputTrickleEventQueue = false;
	unsigned char* pixels = nullptr;
	int width = 0, height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	int failures = 0, commits = 0;
	std::string value;
	ImVec2 field{}, other{};
	const auto check = [&](bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	};
	const auto frame = [&]()
	{
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({ 0.f, 0.f });
		ImGui::SetNextWindowSize({ 700.f, 500.f });
		ImGui::Begin("Properties");
		if (vega::InputStringProperty("Texture", value)) ++commits;
		field = ImGui::GetItemRectMin();
		field.x += 20.f;
		field.y += 8.f;
		ImGui::Button("Other");
		other = ImGui::GetItemRectMin();
		other.x += 10.f;
		other.y += 8.f;
		ImGui::End();
		ImGui::Render();
	};
	const auto click = [&](ImVec2 point)
	{
		io.AddMousePosEvent(point.x, point.y);
		io.AddMouseButtonEvent(0, true);
		frame();
		io.AddMouseButtonEvent(0, false);
		frame();
	};
	frame();
	click(field);
	io.AddInputCharactersUTF8("bogus/");
	frame();
	check(value.empty() && commits == 0, "Typing a partial texture path must not commit");
	io.AddInputCharactersUTF8("notfound.png");
	frame();
	check(value.empty() && commits == 0, "Typing a complete path must still wait for commit");
	io.AddKeyEvent(ImGuiKey_Enter, true);
	frame();
	io.AddKeyEvent(ImGuiKey_Enter, false);
	frame();
	check(value == "bogus/notfound.png" && commits == 1, "Enter must commit the entire draft once");
	// A refused command leaves the real property unchanged; the next idle frame must show it.
	value.clear();
	frame();
	click(field);
	const std::string longPath(200, 'a');
	io.AddInputCharactersUTF8(longPath.c_str());
	frame();
	check(value.empty() && commits == 1, "Long drafts must resize without committing");
	click(other);
	check(value == longPath && commits == 2, "Focus loss must commit the full resized draft once");
	frame();
	check(commits == 2, "Idle frames must not repeat a commit");
	ImGui::DestroyContext();
	return failures ? 1 : 0;
}

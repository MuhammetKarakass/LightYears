#include "editor/PropertyWidgets.h"
#include <iostream>
#include <vector>

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
	std::vector<int> list{10,20,30};
	int listCommits = 0;
	const auto listFrame = [&]()
	{
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0,0});
		ImGui::SetNextWindowSize({700,500});
		ImGui::Begin("List operations");
		if (list.size() > 1 && vega::ListRowActions(list, 1)) ++listCommits;
		ImGui::End();
		ImGui::Render();
	};
	listFrame();
	const auto listClick = [&](float x)
	{
		io.AddMousePosEvent(x, 35.f); io.AddMouseButtonEvent(0,true); listFrame();
		io.AddMouseButtonEvent(0,false); listFrame();
	};
	// The fixed test window uses the default font/style: buttons begin at x=8,y=27.
	listClick(100.f);
	check(list == std::vector<int>({10,30,20}) && listCommits == 1, "Down must reorder one row and commit once");
	listClick(75.f);
	check(list == std::vector<int>({30,10,20}) && listCommits == 2, "Up must reorder one row and commit once");
	listClick(25.f);
	check(list == std::vector<int>({30,20}) && listCommits == 3, "Remove must erase one row and commit once");
	listFrame();
	check(listCommits == 3, "Idle list widgets must not repeat operations");
	ImGui::DestroyContext();
	return failures ? 1 : 0;
}

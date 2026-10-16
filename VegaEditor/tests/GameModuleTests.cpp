#include "editor/EditorExtensions.h"
#include "editor/EditorCommands.h"
#include "editor/EditorScript.h"
#include <framework/GameModule.h>
#include <iostream>

namespace
{
	// This test deliberately links only engine/editor services, with no game code.
	class ExampleModule final : public vega::GameModule
	{
	public:
		const char* Name() const override { return "Example"; }
		void RegisterTypes() override {}
		void Initialize(ly::Application&, bool) override {}
		std::shared_ptr<ly::World> CreateLevel(ly::Application&, const nlohmann::json&, bool) override { return {}; }
		vega::ActorDisplayInfo DisplayInfo(const ly::Actor&) const override { return { true, "Example spawn" }; }
		void RegisterEditorExtensions(vega::EditorExtensions& extensions) override
		{
			extensions.AddPanel([this](vega::EditorContext&, vega::CommandHistory&) { ++panels; });
			extensions.AddHierarchyMenu([this](vega::EditorContext&, vega::CommandHistory&) { ++menus; });
			extensions.AddViewport([this](vega::EditorViewportExtensionContext& viewport) { if (viewport.hovered) ++handles; });
			extensions.AddScriptCommand("example-check", [](vega::EditorContext& context, vega::CommandHistory&,
				const std::vector<std::string>& arguments, std::string& detail)
			{
				detail = "Example command";
				return context.playMode && arguments == std::vector<std::string>{ "two words", "42" };
			});
		}
		int panels = 0, menus = 0, handles = 0;
	};
}

int main()
{
	ExampleModule module;
	vega::EditorExtensions extensions;
	module.RegisterEditorExtensions(extensions);
	vega::EditorSelection selection;
	vega::EditorContext context{ nullptr, selection };
	vega::CommandHistory history{ context };
	for (const auto& panel : extensions.panels) panel(context, history);
	for (const auto& menu : extensions.hierarchyMenus) menu(context, history);
	vega::EditorViewportExtensionContext viewport{ context, history, {}, { 800, 600 }, true };
	for (const auto& handle : extensions.viewports) handle(viewport);
	const auto parsed = vega::ParseEditorScriptLine("example-check \"two words\" 42",
		[&](const std::string& name) { return extensions.HasScriptCommand(name); });
	std::string detail;
	if (!parsed.command || !parsed.error.empty()) return 1;
	const auto& command = *parsed.command;
	if (extensions.Execute(command.extensionName, context, history, command.values, detail)) return 1;
	context.playMode = true;
	if (!extensions.Execute(command.extensionName, context, history, command.values, detail) || detail != "Example command") return 1;
	if (extensions.Execute("missing", context, history, {}, detail)) return 1;
	ly::Actor actor{ nullptr };
	if (!module.DisplayInfo(actor).spawnMarker || module.DisplayInfo(actor).label != "Example spawn") return 1;
	ly::World world{ nullptr };
	if (module.PlayerActor(world)) return 1;
	if (module.panels != 1 || module.menus != 1 || module.handles != 1) return 1;
	std::cout << "Game-independent editor extension contract passed\n";
	return 0;
}

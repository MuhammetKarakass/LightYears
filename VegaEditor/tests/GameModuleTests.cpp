#include "framework/EngineContext.h"
#include "editor/ScriptExecution.h"
#include "editor/EditorExtensions.h"
#include "editor/EditorCommands.h"
#include "editor/EditorScript.h"
#include <framework/GameModule.h>
#include <iostream>
#include <SFML/Graphics/RenderWindow.hpp>

namespace
{
	class MinimalModule final : public vega::GameModule
	{
	public:
		const char* Name() const override { return "Minimal"; }
		void RegisterTypes() override {}
		std::shared_ptr<ly::World> CreateLevel(ly::Application&, const nlohmann::json&, bool) override { return {}; }
	};
	class HookModule final : public vega::GameModule, public vega::IPlayHooks, public vega::IAppLoopHooks
	{
	public:
		const char* Name() const override { return "Hooks"; }
		void RegisterTypes() override {}
		std::shared_ptr<ly::World> CreateLevel(ly::Application&, const nlohmann::json&, bool) override { return {}; }
		void BeginPlay(ly::World&) override { ++begins; }
		void EndPlay(ly::World&) override { ++ends; }
		int ExitCode() const override { return 7; }
		int begins = 0, ends = 0;
	};
	// This test deliberately links only engine/editor services, with no game code.
	class ExampleModule final : public vega::GameModule, public vega::IActorDisplayInfo, public vega::IEditorProvider
	{
	public:
		const char* Name() const override { return "Example"; }
		void RegisterTypes() override {}
		std::shared_ptr<ly::World> CreateLevel(ly::Application&, const nlohmann::json&, bool) override { return {}; }
		vega::ActorDisplayInfo DisplayInfo(const ly::Actor&) const override { return { true, "Example spawn" }; }
		void RegisterEditorExtensions(vega::EditorExtensions& extensions) override
		{
			extensions.AddPanel([this](vega::EditorContext&, vega::CommandHistory&) { ++panels; });
			extensions.AddHierarchyMenu([this](vega::EditorContext&, vega::CommandHistory&) { ++menus; });
			extensions.AddViewport([this](vega::EditorViewportExtensionContext& viewport) { if (viewport.hovered) ++handles; });
			extensions.AddScriptCommand(vega::TokenScriptCommand("example-check", vega::Schema("label:text count:uint"), "Check the example module.", vega::ScriptMode::Play, [](vega::EditorContext& context, vega::CommandHistory&,
				const std::vector<std::string>& arguments, std::string& detail)
			{
				detail = "Example command";
				return context.playMode && arguments == std::vector<std::string>{ "two words", "42" };
			}));
		}
		int panels = 0, menus = 0, handles = 0;
	};
}

int main()
{
	ly::EngineContext engineContext;
	ly::EngineContext::Scope contextScope{ engineContext };
	MinimalModule minimal;
	vega::GameModule* core = &minimal;
	if (vega::QueryCapability<vega::IModuleSetup>(core) || vega::QueryCapability<vega::IPlayHooks>(core) ||
		vega::QueryCapability<vega::IPlayerProvider>(core) || vega::QueryCapability<vega::IActorDisplayInfo>(core) ||
		vega::QueryCapability<vega::IEditorProvider>(core) || vega::QueryCapability<vega::ISceneSettingsHandler>(core) ||
		vega::QueryCapability<vega::IAppLoopHooks>(core) || vega::QueryCapability<vega::IContentReload>(core)) return 1;
	if (vega::QueryCapability<vega::IPlayHooks>(static_cast<vega::GameModule*>(nullptr))) return 1;
	HookModule hooks;
	core = &hooks;
	ly::World hookWorld{nullptr};
	auto* play = vega::QueryCapability<vega::IPlayHooks>(core);
	if (!play) return 1;
	play->BeginPlay(hookWorld);
	play->EndPlay(hookWorld);
	const vega::GameModule* constCore = core;
	const auto* loop = vega::QueryCapability<vega::IAppLoopHooks>(constCore);
	static_assert(std::is_same_v<decltype(loop), const vega::IAppLoopHooks*>);
	if (!loop || loop->ExitCode() != 7 || hooks.begins != 1 || hooks.ends != 1) return 1;
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
	const auto parsed = vega::ParseEditorScriptLine("example-check \"two words\" 42", extensions.commands);
	std::string detail;
	const auto execute = [&](const std::string& name)
	{
		const auto* definition = extensions.commands.Find(name);
		if (!definition) return false;
		sf::RenderWindow window;
		vega::EditorScriptState state;
		const std::string line;
		vega::ScriptExecution execution{context, history, window, state, extensions.commands, line};
		definition->execute(execution, *parsed.command);
		detail = execution.detail;
		return execution.success;
	};
	if (!parsed.command || !parsed.error.empty()) return 1;
	const auto& command = *parsed.command;
	if (execute(command.operation)) return 1;
	context.playMode = true;
	if (!execute(command.operation) || detail != "Example command") return 1;
	if (execute("missing")) return 1;
	ly::Actor actor{ nullptr };
	if (!module.DisplayInfo(actor).spawnMarker || module.DisplayInfo(actor).label != "Example spawn") return 1;
	ly::World world{ nullptr };
	if (vega::QueryCapability<vega::IPlayerProvider>(&module)) return 1;
	if (module.panels != 1 || module.menus != 1 || module.handles != 1) return 1;
	std::cout << "Game-independent editor extension contract passed\n";
	return 0;
}

#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "editor/EditorTool.h"
#include "editor/ScriptCommandRegistry.h"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

namespace vega { class EngineContext; }

namespace vega
{
	struct EditorContext;
	class CommandHistory;
	struct EditorScriptCommand;

	struct EditorViewportExtensionContext
	{
		EditorContext& editor;
		CommandHistory& history;
		sf::FloatRect image;
		sf::Vector2u textureSize;
		bool hovered = false;
	};

	// Callbacks run on the editor thread, with the active scene or prefab context.
	// Panels own their ImGui windows; hierarchy callbacks emit menu items, and
	// viewport callbacks can draw overlays or handles within the supplied image.
	class EditorExtensions
	{
	public:
		using UI = std::function<void(EditorContext&, CommandHistory&)>;
		explicit EditorExtensions(vega::EngineContext& context) : mEngine{ context }
		{
			RegisterCoreScriptCommands(commands);
		}
		vega::EngineContext& Engine() const { return mEngine; }
		void AddTool(std::shared_ptr<EditorModuleTool> tool) { tools.push_back(std::move(tool)); }
		void AddScriptCommand(ScriptCommandDefinition definition) { commands.Add(std::move(definition)); }
		bool Active() const { for (const auto& tool : tools) if (tool->Active()) return true; return false; }
		const char* ActiveToolName() const { for (const auto& tool : tools) if (tool->Active()) return tool->Name(); return ""; }
		bool Dirty() const { for (const auto& tool : tools) if (tool->Dirty()) return true; return false; }
		CommandHistory* History() const { for (const auto& tool : tools) if (auto* history = tool->History()) return history; return nullptr; }
		void RequestExit(std::function<void()> continuation) const
		{
			for (const auto& tool : tools) if (tool->Dirty()) { tool->RequestExit(std::move(continuation)); return; }
			continuation();
		}
		void AddPanel(UI draw) { panels.push_back(std::move(draw)); }
		void AddHierarchyMenu(UI draw) { hierarchyMenus.push_back(std::move(draw)); }
		void AddViewport(std::function<void(EditorViewportExtensionContext&)> draw) { viewports.push_back(std::move(draw)); }
		ScriptCommandRegistry commands;
		std::vector<UI> panels, hierarchyMenus;
		std::vector<std::string> inspectorWindows;
		std::vector<std::function<void(EditorViewportExtensionContext&)>> viewports;
		std::vector<std::shared_ptr<EditorModuleTool>> tools;
	private:
		vega::EngineContext& mEngine;
	};
}

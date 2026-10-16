#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include "editor/EditorTool.h"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

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
		using Script = std::function<bool(EditorContext&, CommandHistory&, const std::vector<std::string>&, std::string&)>;
		using ParsedScript = std::function<bool(EditorContext&, const EditorScriptCommand&, std::string&)>;
		void AddTool(std::shared_ptr<EditorModuleTool> tool) { tools.push_back(std::move(tool)); }
		void AddScriptCommand(std::string name, ParsedScript execute) { parsedScripts.emplace(std::move(name), std::move(execute)); }
		bool ExecuteParsed(const std::string& name, EditorContext& context, const EditorScriptCommand& command, std::string& detail) const
		{
			const auto found = parsedScripts.find(name);
			if (found == parsedScripts.end()) { detail = "Module command unavailable: " + name; return false; }
			return found->second(context, command, detail);
		}
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
		void AddScriptCommand(std::string name, Script execute) { scripts.emplace(std::move(name), std::move(execute)); }
		bool HasScriptCommand(const std::string& name) const { return scripts.find(name) != scripts.end() || parsedScripts.find(name) != parsedScripts.end(); }
		bool Execute(const std::string& name, EditorContext& context, CommandHistory& history,
			const std::vector<std::string>& arguments, std::string& detail) const
		{
			const auto found = scripts.find(name);
			if (found == scripts.end()) { detail = "Unknown extension command: " + name; return false; }
			return found->second(context, history, arguments, detail);
		}
		std::vector<UI> panels, hierarchyMenus;
		std::vector<std::string> inspectorWindows;
		std::vector<std::function<void(EditorViewportExtensionContext&)>> viewports;
		std::vector<std::shared_ptr<EditorModuleTool>> tools;
	private:
		std::map<std::string, Script> scripts;
		std::map<std::string, ParsedScript> parsedScripts;
	};
}

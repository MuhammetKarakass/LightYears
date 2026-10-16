#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

namespace vega
{
	struct EditorContext;
	class CommandHistory;

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
		void AddPanel(UI draw) { panels.push_back(std::move(draw)); }
		void AddHierarchyMenu(UI draw) { hierarchyMenus.push_back(std::move(draw)); }
		void AddViewport(std::function<void(EditorViewportExtensionContext&)> draw) { viewports.push_back(std::move(draw)); }
		void AddScriptCommand(std::string name, Script execute) { scripts.emplace(std::move(name), std::move(execute)); }
		bool HasScriptCommand(const std::string& name) const { return scripts.find(name) != scripts.end(); }
		bool Execute(const std::string& name, EditorContext& context, CommandHistory& history,
			const std::vector<std::string>& arguments, std::string& detail) const
		{
			const auto found = scripts.find(name);
			if (found == scripts.end()) { detail = "Unknown extension command: " + name; return false; }
			return found->second(context, history, arguments, detail);
		}
		std::vector<UI> panels, hierarchyMenus;
		std::vector<std::function<void(EditorViewportExtensionContext&)>> viewports;
	private:
		std::map<std::string, Script> scripts;
	};
}

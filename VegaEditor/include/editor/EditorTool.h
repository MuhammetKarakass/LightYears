#pragma once

#include <functional>
#include <SFML/System/Vector2.hpp>

namespace vega
{
	struct EditorContext;
	class CommandHistory;

	// Optional modules own their tool state. The host supplies lifecycle and input
	// arbitration without importing any module implementation or runtime service.
	class EditorModuleTool
	{
	public:
		virtual ~EditorModuleTool() = default;
		virtual void Initialize(EditorContext&) {}
		virtual void Shutdown() {}
		virtual void Update(EditorContext&, sf::Vector2u, bool) {}
		virtual void PlayChanged(bool) {}
		virtual void ViewMenu() {}
		virtual const char* Name() const { return "Tool"; }
		virtual bool Active() const { return false; }
		virtual bool Dirty() const { return false; }
		virtual CommandHistory* History() { return nullptr; }
		virtual void RequestExit(std::function<void()> continuation) { continuation(); }
	};
}

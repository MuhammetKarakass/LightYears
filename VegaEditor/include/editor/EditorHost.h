#pragma once
#include <framework/GameModule.h>
#include <memory>

namespace vega
{
	// Entry points depend on the host contract, not editor implementation types.
	ly::Application* CreateEditorApplication(std::unique_ptr<GameModule> module);
}

#include "editor/EditorApplication.h"

vega::Application* vega::CreateEditorApplication(std::unique_ptr<GameModule> module)
{
	return new EditorApplication(std::move(module));
}

#include "BlankGameModule.h"
#include <editor/EditorHost.h>

ly::Application* GetApplication()
{
	return vega::CreateEditorApplication(std::make_unique<blank::BlankGameModule>());
}

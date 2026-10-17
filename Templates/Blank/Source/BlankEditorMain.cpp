#include "BlankGameModule.h"
#include <editor/EditorHost.h>

vega::Application* GetApplication()
{
	return vega::CreateEditorApplication(std::make_unique<blank::BlankGameModule>());
}

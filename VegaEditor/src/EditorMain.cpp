#include "editor/EditorApplication.h"

ly::Application* GetApplication()
{
	return new vega::EditorApplication();
}

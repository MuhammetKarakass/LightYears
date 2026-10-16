// The LightYears host of the shared editor core.
#include <editor/LightYearsEditorModule.h>
#include "editor/EditorHost.h"

ly::Application* GetApplication()
{
	return vega::CreateEditorApplication(ly::CreateLightYearsEditorModule());
}

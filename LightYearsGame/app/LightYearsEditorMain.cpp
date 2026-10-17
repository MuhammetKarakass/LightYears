// The LightYears host of the shared editor core.
#include <editor/LightYearsEditorModule.h>
#include "editor/EditorHost.h"

vega::Application* GetApplication()
{
	return vega::CreateEditorApplication(vega::CreateLightYearsEditorModule());
}

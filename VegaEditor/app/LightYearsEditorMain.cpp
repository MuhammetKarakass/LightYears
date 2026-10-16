// The single game-specific entry point. B3 can replace it with a per-game executable.
#include <editor/LightYearsEditorModule.h>
#include "editor/EditorApplication.h"

ly::Application* GetApplication()
{
	return vega::CreateEditorApplication(ly::CreateLightYearsEditorModule());
}

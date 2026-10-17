#include "gameFramework/GameApplication.h"

vega::Application* GetApplication()
{
	return new vega::GameApplication();
}

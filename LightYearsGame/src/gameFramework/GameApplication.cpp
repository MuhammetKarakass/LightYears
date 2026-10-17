#include "gameFramework/GameApplication.h"
#include "gameFramework/LightYearsGameModule.h"

namespace vega
{
	GameApplication::GameApplication(bool editor)
		: Application{ GameProjectWindowSize(editor), 64, "LightYears", sf::Style::Close | sf::Style::Titlebar },
		mModule{ std::make_unique<LightYearsGameModule>() }
	{
		RegisterGameModule(*mModule, editor);
	}

	GameApplication::~GameApplication() = default;
}

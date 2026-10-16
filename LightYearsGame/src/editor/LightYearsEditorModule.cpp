#include "editor/LightYearsEditorModule.h"
#include "gameFramework/LightYearsGameModule.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include <editor/EditorExtensions.h>
#include <editor/EditorCommands.h>

namespace ly
{
	namespace
	{
		class LightYearsEditorModule final : public LightYearsGameModule
		{
		public:
			void RegisterEditorExtensions(vega::EditorExtensions& extensions) override
			{
				extensions.AddScriptCommand("expect-player-ship", [](vega::EditorContext& context, vega::CommandHistory&,
					const std::vector<std::string>& arguments, std::string& detail)
				{
					if (arguments.size() != 1 || arguments[0].empty()) { detail = "Invalid arguments"; return false; }
					if (const auto* player = PlayerManager::GetPlayerManager().GetPlayer())
						if (const auto ship = player->GetCurrentSpaceShip().lock())
						{
							detail = "Spawned player ship = " + player->GetShipId();
							return context.playMode && ship->GetWorld() == context.world && player->GetShipId() == arguments[0];
						}
					return false;
				});
			}
		};
	}

	std::unique_ptr<vega::GameModule> CreateLightYearsEditorModule()
	{
		return std::make_unique<LightYearsEditorModule>();
	}
}

#pragma once

#include "gameplay/GameplayWarning.h"

namespace ly
{
	class HUDController
	{
	public:
		virtual ~HUDController() = default;
		virtual void ShowGameplayWarning(const GameplayWarning& warning) {}
		virtual void HideGameplayWarning(GameplayWarningType warningType) {}
	};
}

#pragma once

#include "framework/Core.h"

#include <functional>

namespace ly
{
	class Actor;
	class GameplayEffectVisual;

	using GameplayEffectVisualFactory = std::function<weak_ptr<GameplayEffectVisual>(Actor& owner)>;

	class GameplayEffectVisualRegistry
	{
	public:
		static bool RegisterFactory(const std::string& visualId, GameplayEffectVisualFactory factory);
		static weak_ptr<GameplayEffectVisual> Spawn(const std::string& visualId, Actor& owner);
	};
}

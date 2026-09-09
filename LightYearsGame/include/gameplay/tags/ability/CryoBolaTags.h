#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	// Stable family identity shared by JSON content, the behavior registry and
	// runtime validation. Cryo Bola has no lifecycle state tags because its
	// activation is instant and its projectile owns the remaining lifetime.
	inline const GameplayTag CryoBola{ "Ability.Control.CryoBola" };
}

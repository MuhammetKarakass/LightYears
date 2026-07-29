#pragma once

namespace ly
{
	// Registers shipped effect behaviors and visuals before validating the
	// immutable effect catalog. Kept separate from abilities because weapons,
	// enemies, rewards, and world areas use the same content.
	bool RegisterGameGameplayEffectContent();
}

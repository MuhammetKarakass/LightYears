#pragma once

namespace ly
{
	// Process output policy, independent of context-owned playback and volume settings.
	namespace AudioOutput
	{
		void Initialize(int argc = 0, char** argv = nullptr);
		void SetMuted(bool muted);
		bool IsMuted();
		bool IsForcedMuted();
	}
}

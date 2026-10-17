#include "framework/AudioOutput.h"
#include <SFML/Audio/Listener.hpp>
#include <cstdlib>
#include <string>

namespace ly::AudioOutput
{
	namespace
	{
		bool forced = false;
		bool requested = false;
	}

	void Initialize(int argc, char** argv)
	{
		const char* environment = std::getenv("VEGA_MUTE_AUDIO");
		forced = forced || (environment && std::string{ environment } == "1");
		for (int i = 1; i < argc; ++i)
		{
			const std::string argument{ argv[i] };
			if (argument == "--mute" || argument == "--editor-script" || argument == "--visual-capture" ||
				argument == "--visual-script" || argument == "--perf-autoplay" || argument == "--headless")
				forced = true;
		}
		SetMuted(requested);
	}

	void SetMuted(bool muted)
	{
		requested = muted;
		// Listener gain is applied after mixing: streams, fades and source volumes keep advancing unchanged.
		sf::Listener::setGlobalVolume(IsMuted() ? 0.f : 100.f);
	}

	bool IsMuted() { return forced || requested; }
	bool IsForcedMuted() { return forced; }
}

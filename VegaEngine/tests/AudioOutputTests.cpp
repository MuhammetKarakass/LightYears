#include "framework/AudioOutput.h"
#include "framework/EngineContext.h"
#include "framework/AssetManager.h"
#include "framework/AudioManager.h"
#include <SFML/Audio.hpp>
#include <SFML/System/Sleep.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdlib>

namespace
{
	void Check(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}
}

int main(int argc, char** argv)
{
	try
	{
		const std::string mode = argc > 1 ? argv[1] : "environment";
		if (mode != "environment")
		{
#ifdef _WIN32
			_putenv_s("VEGA_MUTE_AUDIO", "");
#else
			unsetenv("VEGA_MUTE_AUDIO");
#endif
		}
		if (mode == "interactive")
		{
			vega::AudioOutput::Initialize();
			Check(!vega::AudioOutput::IsMuted() && sf::Listener::getGlobalVolume() == 100.f, "Interactive default changed");
			vega::AudioOutput::SetMuted(true);
		}
		else vega::AudioOutput::Initialize(argc, argv);
		Check(vega::AudioOutput::IsMuted() && sf::Listener::getGlobalVolume() == 0.f, "Output is not silent");
		if (mode != "interactive")
		{
			Check(vega::AudioOutput::IsForcedMuted(), "Switch did not force mute");
			vega::AudioOutput::SetMuted(false);
			Check(sf::Listener::getGlobalVolume() == 0.f, "Preference overrode automation mute");
		}

		const auto root = std::filesystem::path{ AUDIO_OUTPUT_TEST_ROOT } / mode;
		std::filesystem::create_directories(root);
		// Generate a long clip under build/, never write to game authoring assets.
		std::vector<std::int16_t> samples(44100 * 4, 1000);
		sf::SoundBuffer buffer;
		Check(buffer.loadFromSamples(samples.data(), samples.size(), 1, 44100, { sf::SoundChannel::Mono }), "Cannot create clip");
		Check(buffer.saveToFile(root / "clip.wav"), "Cannot save clip");
		Check(buffer.saveToFile(root / "next.wav"), "Cannot save fade clip");
		vega::EngineContext context;
		context.Assets().SetAssetRootDirectory(root.generic_string() + "/");
		auto& audio = context.Audio();
		const auto id = audio.PlaySoundTracked("clip.wav", AudioType::SFX_World);
		audio.PlayMusic("clip.wav", AudioType::Music);
		const auto revision = audio.MusicRevision();
		sf::sleep(sf::milliseconds(80));
		Check(id && audio.IsSoundPlaying(id) && audio.IsMusicPlaying(), "Mute stopped playback");
		Check(audio.MusicPlayingOffset() > sf::Time::Zero, "Muted stream did not advance");
		audio.SetMasterVolume(70.f);
		audio.SetMusicVolume(60.f);
		audio.SetSFXVolume(50.f);
		audio.FadeToMusic("next.wav", AudioType::Music, 80.f, 1.f, true, 0.2f);
		audio.Update(0.1f);
		Check(audio.GetMasterVolume() == 70.f && audio.GetMusicVolume() == 60.f && audio.GetSFXVolume() == 50.f,
			"Mute changed logical volume settings");
		Check(audio.IsMusicPlaying() && audio.MusicRevision() > revision && audio.IsSoundPlaying(id), "Mute changed fade or ownership state");
		audio.Update(0.2f);
		audio.Update(0.3f);
		audio.PauseMusic();
		Check(!audio.IsMusicPlaying(), "Pause state changed");
		audio.ResumeMusic();
		Check(audio.IsMusicPlaying() && sf::Listener::getGlobalVolume() == 0.f, "Resume unmuted output");
		audio.StopSound(id);
		audio.StopMusic();
		Check(!audio.IsSoundPlaying(id) && !audio.IsMusicPlaying(), "Stop state changed");
		if (mode == "interactive")
		{
			vega::AudioOutput::SetMuted(false);
			Check(!vega::AudioOutput::IsMuted() && sf::Listener::getGlobalVolume() == 100.f, "Interactive toggle cannot unmute");
		}
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}

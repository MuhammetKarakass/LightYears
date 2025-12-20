#pragma once

#include "framework/Core.h"
#include <SFML/Audio.hpp>
#include "framework/Delegate.h"

namespace ly
{
	class AudioManager
	{
	public:
		static AudioManager& GetAudioManager();

		void PlaySound(const std::string& path, AudioType type, float volume = 100.0f, float pitch = 1.0f);
		
		void PlayMusic(const std::string& path, AudioType type, float volume = 100.0f, float pitch = 1.0f, bool loop = true);
		void StopMusic();
		void PauseMusic();
		void ResumeMusic();

		void SetWorldTimeScale(float timeScale);
		float GetWorldTimeScale() const { return mCurrentTimeScale; }

		void SetMasterVolume(float volume);
		void SetMusicVolume(float volume);
		void SetSFXVolume(float volume);
		float GetMasterVolume() const { return mMasterVolume; }
		float GetMusicVolume() const { return mMusicVolume; }
		float GetSFXVolume() const { return mSFXVolume; }

		// Menü modu (ses kýsma/yavaþlatma)
		void SetMenuMode(bool isMenuOpen);

		// Temizlik
		void CleanCycle();

		Delegate<float> onWorldTimeScaleChanged;

	private:
		AudioManager();

		sf::Sound* AcquireSoundFromPool();
		void ReturnSoundToPool(sf::Sound* sound);

		// Aktif sesler (çalan sesler)
		struct ActiveSound
		{
			sf::Sound* sound;
			shared_ptr<sf::SoundBuffer> buffer;  // Buffer'ý tutuyoruz ki silinmesin
			AudioType type;
			float basePitch;
			float baseVolume;
		};
		
		List<ActiveSound> mActiveSounds;
		
		List<unique_ptr<sf::Sound>> mSoundPool;
		static constexpr size_t MAX_POOL_SIZE = 32;
		static constexpr size_t INITIAL_POOL_SIZE = 8;

		// Müzik
		shared_ptr<sf::Music> mMusic;
		AudioType mMusicType;
		float mMusicBasePitch;
		float mMusicBaseVolume;

		// Global ayarlar
		float mMasterVolume;
		float mMusicVolume;
		float mSFXVolume;
		float mCurrentTimeScale;

		static unique_ptr<AudioManager> audioManager;

		// Volume hesaplama yardýmcýlarý
		float CalculateEffectiveVolume(float baseVolume, AudioType type) const;
		void UpdateAllVolumes();
	};
}
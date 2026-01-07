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

		void PlayMusicWithIntro(const std::string& introPath, const std::string& loopPath, AudioType type, float volume = 100.0f, float pitch = 1.0f);
		void FadeToMusicWithIntro(const std::string& introPath, const std::string& loopPath, AudioType type, float volume = 100.0f, float pitch = 1.0f, float fadeDuration = 3.0f);
		void FadeToMusic(const std::string& path, AudioType type, float volume = 100.0f, float pitch = 1.0f, bool loop = true, float fadeDuration = 3.0f);
		void Update(float deltaTime);

		void SetWorldTimeScale(float timeScale);
		float GetWorldTimeScale() const { return mCurrentTimeScale; }

		void SetMasterVolume(float volume);
		void SetMusicVolume(float volume);
		void SetSFXVolume(float volume);
		float GetMasterVolume() const { return mMasterVolume; }
		float GetMusicVolume() const { return mMusicVolume; }
		float GetSFXVolume() const { return mSFXVolume; }

		void SetMenuMode(bool isMenuOpen);

		void CleanCycle();

		Delegate<float> onWorldTimeScaleChanged;

	private:
		AudioManager();

		sf::Sound* AcquireSoundFromPool();
		void ReturnSoundToPool(sf::Sound* sound);

		struct ActiveSound
		{
			sf::Sound* sound;
			shared_ptr<sf::SoundBuffer> buffer;
			AudioType type;
			float basePitch;
			float baseVolume;
		};
		
		List<ActiveSound> mActiveSounds;
		
		List<unique_ptr<sf::Sound>> mSoundPool;
		static constexpr size_t MAX_POOL_SIZE = 32;
		static constexpr size_t INITIAL_POOL_SIZE = 8;

		shared_ptr<sf::Music> mMusic;
		AudioType mMusicType;
		float mMusicBasePitch;
		float mMusicBaseVolume;

		float mMasterVolume;
		float mMusicVolume;
		float mSFXVolume;
		float mCurrentTimeScale;

		enum class FadeState
		{
			None,
			FadingOut,
			FadingIn
		};

		FadeState mFadeState;
		float mFadeTimer;
		float mFadeDuration;
		float mTargetVolume;

		std::string mNextMusicPath;
		AudioType mNextMusicType;
		float mNextMusicPitch;
		float mNextMusicVolume;
		bool mNextMusicLoop;

		std::string mCurrentMusicPath;
		bool mLoopCrossfadeEnabled;
		float mLoopCrossfadeDuration;
		bool mIsLoopCrossfading;
		shared_ptr<sf::Music> mCrossfadeMusic;

		std::string mPendingLoopPath;
		std::string mNextPendingLoopPath;
		bool mIsPlayingIntro;

		static unique_ptr<AudioManager> audioManager;

		float CalculateEffectiveVolume(float baseVolume, AudioType type) const;
		void UpdateAllVolumes();
	};
}
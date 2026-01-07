#include "framework/AssetManager.h"
#include "framework/AudioManager.h"
#include <memory>

namespace ly
{
	unique_ptr<AudioManager> AudioManager::audioManager = nullptr;

	AudioManager& AudioManager::GetAudioManager()
	{
		if (!audioManager)
		{
			audioManager = unique_ptr<AudioManager>{ new AudioManager };
		}
		return *audioManager;
	}

	AudioManager::AudioManager() :
		mCurrentTimeScale{ 1.0f },
		mMasterVolume{ 100.0f },
		mMusicVolume{ 100.0f },
		mSFXVolume{ 100.0f },
		mMusic{ nullptr },
		mMusicType{ AudioType::Music },
		mMusicBasePitch{ 1.0f },
		mMusicBaseVolume{ 100.0f },
		mIsPlayingIntro{ false }
	{
		mSoundPool.reserve(MAX_POOL_SIZE);
	}

	sf::Sound* AudioManager::AcquireSoundFromPool()
	{
		if (!mSoundPool.empty())
		{
			sf::Sound* sound = mSoundPool.back().release();
			mSoundPool.pop_back();
			return sound;
		}

		return nullptr;
	}

	void AudioManager::ReturnSoundToPool(sf::Sound* sound)
	{
		if (!sound) return;

		sound->stop();

		if (mSoundPool.size() < MAX_POOL_SIZE)
		{
			mSoundPool.push_back(unique_ptr<sf::Sound>(sound));
		}
		else
		{
			delete sound;
		}
	}

	float AudioManager::CalculateEffectiveVolume(float baseVolume, AudioType type) const
	{
		float typeVolume = (type == AudioType::Music) ? mMusicVolume : mSFXVolume;
		return baseVolume * (mMasterVolume / 100.0f) * (typeVolume / 100.0f);
	}

	void AudioManager::UpdateAllVolumes()
	{
		for (auto& activeSound : mActiveSounds)
		{
			float effectiveVolume = CalculateEffectiveVolume(activeSound.baseVolume, activeSound.type);
			activeSound.sound->setVolume(effectiveVolume);
		}

		if (mMusic && mMusic->getStatus() != sf::Music::Status::Stopped)
		{
			float effectiveVolume = CalculateEffectiveVolume(mMusicBaseVolume, mMusicType);
			mMusic->setVolume(effectiveVolume);
		}
	}

	void AudioManager::PlaySound(const std::string& path, AudioType type, float volume, float pitch)
	{
		shared_ptr<sf::SoundBuffer> buffer = AssetManager::GetAssetManager().LoadSoundBuffer(path);
		if (!buffer) return;

		sf::Sound* sound = AcquireSoundFromPool();
		if (sound)
		{
			sound->setBuffer(*buffer);
		}
		else
		{
			sound = new sf::Sound(*buffer);
		}

		float finalPitch = pitch;
		if (type == AudioType::SFX_World)
		{
			finalPitch *= mCurrentTimeScale;
		}

		float effectiveVolume = CalculateEffectiveVolume(volume, type);

		sound->setPitch(finalPitch);
		sound->setVolume(effectiveVolume);
		sound->play();

		mActiveSounds.push_back({ sound, buffer, type, pitch, volume });
	}

	void AudioManager::PlayMusic(const std::string& path, AudioType type, float volume, float pitch, bool loop)
	{
		std::string copyOfPath = path;
		mPendingLoopPath = "";
		mIsPlayingIntro = false;
		if (mMusic)
		{
			mMusic->stop();
		}

		mMusic = std::make_shared<sf::Music>();
		std::string fullPath = AssetManager::GetAssetManager().GetAssetRootDirectory() + copyOfPath;
		
		if (mMusic->openFromFile(fullPath))
		{
			mMusicType = type;
			mMusicBasePitch = pitch;
			mMusicBaseVolume = volume;

			float finalPitch = pitch;
			if (type == AudioType::SFX_World)
			{
				finalPitch *= mCurrentTimeScale;
			}

			float effectiveVolume = CalculateEffectiveVolume(volume, type);

			mMusic->setVolume(effectiveVolume);
			mMusic->setPitch(finalPitch);
			mMusic->setLooping(loop);
			mMusic->play();
		}
	}

	void AudioManager::StopMusic()
	{
		if (mMusic)
		{
			mMusic->stop();
		}
	}

	void AudioManager::PauseMusic()
	{
		if (mMusic)
		{
			mMusic->pause();
		}
	}

	void AudioManager::ResumeMusic()
	{
		if (mMusic && mMusic->getStatus() == sf::Music::Status::Paused)
		{
			mMusic->play();
		}
	}

	void AudioManager::PlayMusicWithIntro(const std::string& introPath, const std::string& loopPath, AudioType type, float volume, float pitch)
	{
		PlayMusic(introPath, type, volume, pitch, false);
		mPendingLoopPath = loopPath;
		mIsPlayingIntro = true;
	}

	void AudioManager::FadeToMusicWithIntro(const std::string& introPath, const std::string& loopPath, AudioType type, float volume, float pitch, float fadeDuration)
	{
		mNextPendingLoopPath = loopPath;
		FadeToMusic(introPath, type, volume, pitch, false, fadeDuration);
	}

	void AudioManager::FadeToMusic(const std::string& path, AudioType type, float volume, float pitch, bool loop, float fadeDuration)
	{
		if (mMusic && mMusic->getStatus() != sf::Music::Status::Stopped)
		{
			std::string currentPath = mNextMusicPath.empty() ? "" : mNextMusicPath;
			if(path == currentPath)
			{
				return;
			}
		}

		mNextMusicPath = path;
		mNextMusicType = type;
		mNextMusicPitch = pitch;
		mNextMusicVolume = volume;
		mNextMusicLoop = loop;

		mFadeDuration = fadeDuration;
		mTargetVolume = volume;

		if(!mMusic || mMusic->getStatus() == sf::Music::Status::Stopped)
		{
			PlayMusic(path, type, 0.0f, pitch, loop);
			if(!mNextPendingLoopPath.empty())
			{
				mPendingLoopPath = mNextPendingLoopPath;
				mIsPlayingIntro = true;
				mNextPendingLoopPath = "";
			}
			mFadeState = FadeState::FadingIn;
			mFadeTimer = 0.0f;
		}
		else
		{
			mFadeState = FadeState::FadingOut;
			mFadeTimer = 0.0f;
		}
	}

	void AudioManager::Update(float deltaTime)
	{
		mFadeTimer += deltaTime;

		float progress = std::min(mFadeTimer / mFadeDuration, 1.0f);
		if(mFadeState == FadeState::FadingOut)
		{
			float newVolume = mMusicBaseVolume * (1.0f - progress);
			float effectiveVolume = CalculateEffectiveVolume(newVolume, mMusicType);
			mMusic->setVolume(effectiveVolume);
			if(progress >= 1.0f)
			{
				mMusic->stop();
				if(!mNextMusicPath.empty())
				{
					PlayMusic(mNextMusicPath, mNextMusicType, 0.0f, mNextMusicPitch, mNextMusicLoop);
					if(!mNextPendingLoopPath.empty())
					{
						mPendingLoopPath = mNextPendingLoopPath;
						mIsPlayingIntro = true;
						mNextPendingLoopPath = "";
					}
					mFadeState = FadeState::FadingIn;
					mFadeTimer = 0.0f;
					mNextMusicPath = "";
				}
				else
				{
					mFadeState = FadeState::None;
				}
			}
		}
		else if(mFadeState == FadeState::FadingIn)
		{
			float newVolume = mTargetVolume * progress;
			mMusicVolume = mTargetVolume;
			float effectiveVolume = CalculateEffectiveVolume(newVolume, mMusicType);
			mMusic->setVolume(effectiveVolume);
			if(progress >= 1.0f)
			{
				mMusicBaseVolume = mTargetVolume;
				mFadeState = FadeState::None;
			}
		}
		if(mMusic && mIsPlayingIntro && mMusic->getStatus() == sf::Music::Status::Stopped)
		{
			PlayMusic(mPendingLoopPath, mMusicType, mMusicBaseVolume, mMusicBasePitch, true);
			mIsPlayingIntro = false;
			mPendingLoopPath = "";
		}
	}

	void AudioManager::SetWorldTimeScale(float timeScale)
	{
		mCurrentTimeScale = timeScale;

		if (mMusic && mMusic->getStatus() == sf::Music::Status::Playing)
		{
			if (mMusicType == AudioType::SFX_World)
			{
				mMusic->setPitch(mMusicBasePitch * mCurrentTimeScale);
			}
		}

		for (auto& activeSound : mActiveSounds)
		{
			if (activeSound.type == AudioType::SFX_World)
			{
				activeSound.sound->setPitch(activeSound.basePitch * mCurrentTimeScale);
			}
		}

		onWorldTimeScaleChanged.Broadcast(mCurrentTimeScale);
	}

	void AudioManager::SetMasterVolume(float volume)
	{
		mMasterVolume = std::clamp(volume, 0.0f, 100.0f);
		UpdateAllVolumes();
	}

	void AudioManager::SetMusicVolume(float volume)
	{
		mMusicVolume = std::clamp(volume, 0.0f, 100.0f);
		UpdateAllVolumes();
	}

	void AudioManager::SetSFXVolume(float volume)
	{
		mSFXVolume = std::clamp(volume, 0.0f, 100.0f);
		UpdateAllVolumes();
	}

	void AudioManager::SetMenuMode(bool isMenuOpen)
	{
		if (isMenuOpen)
		{
			SetWorldTimeScale(0.7f);
			if (mMusic)
			{
				mMusic->setVolume(CalculateEffectiveVolume(mMusicBaseVolume, mMusicType) * 0.3f);
			}
		}
		else
		{
			SetWorldTimeScale(1.0f);
			if (mMusic)
			{
				mMusic->setVolume(CalculateEffectiveVolume(mMusicBaseVolume, mMusicType));
			}
		}
	}

	void AudioManager::CleanCycle()
	{
		for (auto iter = mActiveSounds.begin(); iter != mActiveSounds.end();)
		{
			if (iter->sound->getStatus() == sf::Sound::Status::Stopped)
			{
				ReturnSoundToPool(iter->sound);
				iter = mActiveSounds.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
}
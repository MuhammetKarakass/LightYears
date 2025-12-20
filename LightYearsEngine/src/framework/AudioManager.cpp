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
		mMusicBaseVolume{ 100.0f }
	{
		// SFML 3.x'te sf::Sound default constructor yok, pool'u lazy olarak dolduracaðýz
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

		// Pool boþsa nullptr dön - PlaySound'da buffer ile oluþturulacak
		return nullptr;
	}

	void AudioManager::ReturnSoundToPool(sf::Sound* sound)
	{
		if (!sound) return;

		// Sesi durdur
		sound->stop();

		// Pool doluysa sil, deðilse pool'a ekle
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

		// Müziðin volume'ýný güncelle
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

		// Pool'dan ses al veya yeni oluþtur
		sf::Sound* sound = AcquireSoundFromPool();
		if (sound)
		{
			// Pool'dan gelen sese yeni buffer ata
			sound->setBuffer(*buffer);
		}
		else
		{
			// SFML 3.x: sf::Sound(const sf::SoundBuffer&) constructor kullan
			sound = new sf::Sound(*buffer);
		}

		// Pitch hesapla (SFX_World ise TimeScale'den etkilenir)
		float finalPitch = pitch;
		if (type == AudioType::SFX_World)
		{
			finalPitch *= mCurrentTimeScale;
		}

		// Volume hesapla (Master ve tip bazlý volume)
		float effectiveVolume = CalculateEffectiveVolume(volume, type);

		sound->setPitch(finalPitch);
		sound->setVolume(effectiveVolume);
		sound->play();

		// Buffer'ý da saklýyoruz ki AssetManager temizlese bile silinmesin
		mActiveSounds.push_back({ sound, buffer, type, pitch, volume });
	}

	void AudioManager::PlayMusic(const std::string& path, AudioType type, float volume, float pitch, bool loop)
	{
		if (mMusic)
		{
			mMusic->stop();
		}

		mMusic = std::make_shared<sf::Music>();
		std::string fullPath = AssetManager::GetAssetManager().GetAssetRootDirectory() + path;
		
		if (mMusic->openFromFile(fullPath))
		{
			mMusicType = type;
			mMusicBasePitch = pitch;
			mMusicBaseVolume = volume;

			// Pitch hesapla
			float finalPitch = pitch;
			if (type == AudioType::SFX_World)
			{
				finalPitch *= mCurrentTimeScale;
			}

			// Volume hesapla
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

	void AudioManager::SetWorldTimeScale(float timeScale)
	{
		mCurrentTimeScale = timeScale;

		// Müziði güncelle (eðer SFX_World tipindeyse)
		if (mMusic && mMusic->getStatus() == sf::Music::Status::Playing)
		{
			if (mMusicType == AudioType::SFX_World)
			{
				mMusic->setPitch(mMusicBasePitch * mCurrentTimeScale);
			}
		}

		// Aktif sesleri güncelle
		for (auto& activeSound : mActiveSounds)
		{
			if (activeSound.type == AudioType::SFX_World)
			{
				activeSound.sound->setPitch(activeSound.basePitch * mCurrentTimeScale);
			}
		}

		// SoundComponent'lere haber ver
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
			// Müzik sesini geçici olarak kýs
			if (mMusic)
			{
				mMusic->setVolume(CalculateEffectiveVolume(mMusicBaseVolume, mMusicType) * 0.3f);
			}
		}
		else
		{
			SetWorldTimeScale(1.0f);
			// Müzik sesini normale döndür
			if (mMusic)
			{
				mMusic->setVolume(CalculateEffectiveVolume(mMusicBaseVolume, mMusicType));
			}
		}
	}

	void AudioManager::CleanCycle()
	{
		// Bitmiþ sesleri temizle ve pool'a geri ver
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
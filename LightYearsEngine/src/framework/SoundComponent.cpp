#include "framework/SoundComponent.h"
#include "framework/AudioManager.h"
#include "framework/AssetManager.h"

namespace ly
{
    SoundComponent::SoundComponent(const std::string& path, AudioType type, bool loop) :
  mAudioType{ type },
      mBasePitch{ 1.0f },
        mBoundToAudioManager{ false }
    {
    mSoundBuffer = AssetManager::GetAssetManager().LoadSoundBuffer(path);

        if (mSoundBuffer)
    {
            mSound.emplace(*mSoundBuffer);
    mSound->setLooping(loop);

            if (mAudioType == AudioType::SFX_World)
   {
              mTimeScaleChangedHandle = AudioManager::GetAudioManager()
                  .onWorldTimeScaleChanged.BindAction(this, &SoundComponent::OnTimeScaleChanged);
              mBoundToAudioManager = mTimeScaleChangedHandle.IsValid();
        }
        }
    }

    SoundComponent::~SoundComponent()
    {
        // Önce çalan sesi durdur
        if (mSound.has_value() && mSound->getStatus() != sf::Sound::Status::Stopped)
        {
            mSound->stop();
        }

        if (mBoundToAudioManager && AudioManager::IsInitialized())
        {
            AudioManager::GetAudioManager()
                .onWorldTimeScaleChanged.UnbindAction(mTimeScaleChangedHandle);
        }
        mTimeScaleChangedHandle.Reset();
        mBoundToAudioManager = false;
    }

 void SoundComponent::OnTimeScaleChanged(float newTimeScale)
    {
        if (IsPlaying())
        {
            mSound->setPitch(mBasePitch * newTimeScale);
        }
    }

    void SoundComponent::Play()
    {
        if (!mSound.has_value()) return;

        if (mAudioType == AudioType::SFX_World)
        {
            float timeScale = AudioManager::GetAudioManager().GetWorldTimeScale();
             mSound->setPitch(mBasePitch * timeScale);
        }
        else
        {
            mSound->setPitch(mBasePitch);
         }
        mSound->play();
    }

    void SoundComponent::Stop()
    {
        if (!mSound.has_value()) return;

        mSound->stop();
        onSoundFinished.Broadcast(this);
    }

    void SoundComponent::Pause()
    {
        if (mSound.has_value()) mSound->pause();
    }

    void SoundComponent::SetVolume(float volume)
    {
        if (mSound.has_value()) mSound->setVolume(volume);
    }

    void SoundComponent::SetPitch(float pitch)
    {
        mBasePitch = pitch;
        if (!mSound.has_value()) return;

         if (mAudioType == AudioType::SFX_World)
        {
               float timeScale = AudioManager::GetAudioManager().GetWorldTimeScale();
                mSound->setPitch(mBasePitch * timeScale);
        }
        else
        {
            mSound->setPitch(mBasePitch);
        }
    }

    void SoundComponent::SetLoop(bool loop)
  {
        if (mSound.has_value()) mSound->setLooping(loop);
    }

    bool SoundComponent::IsPlaying() const
    {
        if (!mSound.has_value()) return false;
        return mSound->getStatus() == sf::Sound::Status::Playing;
    }

    bool SoundComponent::IsStopped() const
    {
        if (!mSound.has_value()) return true;
        return mSound->getStatus() == sf::Sound::Status::Stopped;
    }

    weak_ptr<SoundComponent> SoundComponent::GetWeakPtr()
    {
        return weak_from_this();
    }

    weak_ptr<const SoundComponent> SoundComponent::GetWeakPtr() const
    {
         return weak_from_this();
    }
}

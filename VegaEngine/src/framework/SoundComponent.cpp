#include "framework/reflection/Reflection.h"
#include "framework/SoundComponent.h"
#include "framework/AudioManager.h"
#include "framework/AssetManager.h"

namespace vega
{
    SoundComponent::SoundComponent(const std::string& path, AudioType type, bool loop, EngineContext& context) :
		mContext{ context },
  mAudioType{ type },
      mBasePitch{ 1.0f },
        mBoundToAudioManager{ false }
    {
    mSoundBuffer = mContext.Assets().LoadSoundBuffer(path);

        if (mSoundBuffer)
    {
            mSound.emplace(*mSoundBuffer);
    mSound->setLooping(loop);

            if (mAudioType == AudioType::SFX_World)
   {
              mTimeScaleChangedHandle = mContext.Audio()
                  .onWorldTimeScaleChanged.BindAction(this, &SoundComponent::OnTimeScaleChanged);
              mBoundToAudioManager = mTimeScaleChangedHandle.IsValid();
        }
        }
    }

    SoundComponent::~SoundComponent()
    {
        // �nce �alan sesi durdur
        if (mSound.has_value() && mSound->getStatus() != sf::Sound::Status::Stopped)
        {
            mSound->stop();
        }

        if (mBoundToAudioManager && mContext.HasAudio())
        {
            mContext.Audio()
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
            float timeScale = mContext.Audio().GetWorldTimeScale();
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
               float timeScale = mContext.Audio().GetWorldTimeScale();
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

namespace vega
{
	// SoundComponent is independently owned, so it is registered for lookup by
	// name but does not participate in ActorComponent enumeration or discovery.
	VEGA_REFLECT_ROOT(SoundComponent)
	{
		VEGA_READONLY("Playing", IsPlaying);
		VEGA_READONLY("Stopped", IsStopped);
	}
}

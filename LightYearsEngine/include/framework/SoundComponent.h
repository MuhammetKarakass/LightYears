#pragma once

#include "framework/Core.h"
#include "framework/Delegate.h"
#include <SFML/Audio.hpp>

namespace ly{
class SoundComponent : public std::enable_shared_from_this<SoundComponent>
{
public:
	SoundComponent(const std::string& path, AudioType type= AudioType::SFX_World, bool loop=false);
	~SoundComponent();

	void Play();
	void Stop();
	void Pause();

	void SetVolume(float volume);
	void SetPitch(float pitch);
	void SetLoop(bool loop);

	bool IsPlaying() const;
	bool IsStopped() const;

	weak_ptr<SoundComponent> GetWeakPtr();
	weak_ptr<const SoundComponent> GetWeakPtr() const;

	Delegate<SoundComponent*> onSoundFinished;

private:
	void OnTimeScaleChanged(float newTimeScale);

	std::optional<sf::Sound> mSound;
	shared_ptr<sf::SoundBuffer> mSoundBuffer;
	AudioType mAudioType;
	float mBasePitch;
	bool mBoundToAudioManager;
};
}
#pragma once

#include <cstdint>

namespace ly
{
	class AbilityWorldActor;

	// Reusable receiver contract for actors that capture physical projectiles.
	// Relay Prism is the first implementation, but future projectile modifiers
	// can use the same overlap/query boundary without changing projectile code.
	class ProjectileCaptureVolume
	{
	public:
		virtual ~ProjectileCaptureVolume() = default;
		virtual bool TryCaptureProjectile(AbilityWorldActor& projectile) = 0;
		virtual std::uint64_t GetCaptureVolumeId() const = 0;
	};
}


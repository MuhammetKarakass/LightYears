#pragma once

#include "framework/Actor.h"

#include <memory>

namespace ly
{
	class PortalTransferService;
	struct PortalTransferRuntimeState;

	// One coordinator per World keeps portal transit ticking even after the
	// ability that created a portal pair has entered cooldown and destroyed its
	// visual portal actors.
	class PortalTransferRuntimeActor final : public Actor
	{
	public:
		explicit PortalTransferRuntimeActor(World* world);
		~PortalTransferRuntimeActor() override;

		void Tick(float deltaTime) override;
		void Destroy() override;

	private:
		friend class PortalTransferService;
		std::unique_ptr<PortalTransferRuntimeState> mRuntimeState;
	};
}

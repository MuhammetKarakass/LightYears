#pragma once

#include "framework/Core.h"
#include "gameplay/ability/AbilityController.h"

namespace ly
{
	class Actor;

	class AbilitySystem
	{
	public:
		AbilitySystem(Actor* owner = nullptr);

		void SetOwner(Actor* owner) { mOwner = owner; }
		Actor* GetOwner() const { return mOwner; }

		void AddController(AbilitySlot slot, unique_ptr<AbilityController> controller);
		void ClearSlot(AbilitySlot slot);
		void SetSlotInput(AbilitySlot slot, bool inputHeld);
		void Tick(float deltaTime);

		AbilityController* GetController(AbilitySlot slot);
		const AbilityController* GetController(AbilitySlot slot) const;

	private:
		Actor* mOwner;
		Map<AbilitySlot, unique_ptr<AbilityController>> mControllers;
	};
}

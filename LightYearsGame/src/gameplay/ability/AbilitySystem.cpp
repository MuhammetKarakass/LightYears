#include "gameplay/ability/AbilitySystem.h"

namespace ly
{
	AbilitySystem::AbilitySystem(Actor* owner)
		: mOwner{ owner },
		mControllers{}
	{
	}

	void AbilitySystem::AddController(AbilitySlot slot, unique_ptr<AbilityController> controller)
	{
		if (!controller)
		{
			return;
		}

		mControllers[slot] = std::move(controller);
	}

	void AbilitySystem::ClearSlot(AbilitySlot slot)
	{
		mControllers.erase(slot);
	}

	void AbilitySystem::SetSlotInput(AbilitySlot slot, bool inputHeld)
	{
		if (AbilityController* controller = GetController(slot))
		{
			controller->SetInputHeld(inputHeld);
		}
	}

	void AbilitySystem::Tick(float deltaTime)
	{
		for (auto& entry : mControllers)
		{
			if (entry.second)
			{
				entry.second->Tick(deltaTime);
			}
		}
	}

	AbilityController* AbilitySystem::GetController(AbilitySlot slot)
	{
		auto found = mControllers.find(slot);
		return found != mControllers.end() ? found->second.get() : nullptr;
	}

	const AbilityController* AbilitySystem::GetController(AbilitySlot slot) const
	{
		auto found = mControllers.find(slot);
		return found != mControllers.end() ? found->second.get() : nullptr;
	}
}

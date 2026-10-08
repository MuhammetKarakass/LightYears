#pragma once
#include "gameplay/module/ModuleResolver.h"
#include "abilities/AbilityEvent.h"
#include "framework/Delegate.h"

namespace ly
{
	class CombatRuntime;
	class SpaceShip;
	class ModuleShieldReflectionRuntime final
	{
	public:
		explicit ModuleShieldReflectionRuntime(CombatRuntime& combat);
		~ModuleShieldReflectionRuntime();
		ModuleShieldReflectionRuntime(const ModuleShieldReflectionRuntime&) = delete;
		ModuleShieldReflectionRuntime& operator=(const ModuleShieldReflectionRuntime&) = delete;
		void Replace(const List<ResolvedModuleEffects::ShieldReflection>& rules);
		void Tick(float deltaSeconds);
	private:
		struct State { ResolvedModuleEffects::ShieldReflection resolved; float remaining = 0.f; };
		void OnEvent(const sas::AbilityEvent& event);
		void OnDamage(const DamageContext& context);
		CombatRuntime& mCombat;
		SpaceShip* mShip = nullptr;
		List<State> mRules;
		DelegateHandle mEventHandle;
		DelegateHandle mDamageHandle;
		std::uint64_t mGeneration = 0;
	};
}

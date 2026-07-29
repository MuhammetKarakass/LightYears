#pragma once

#include "gameplay/effects/ActiveGameplayEffect.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/ability/AbilityEvent.h"
#include "framework/Delegate.h"

namespace ly
{
	class AttributeSystem;

	class GameplayEffectSystem
	{
	public:
		GameplayEffectSystem(Actor& owner, AttributeSystem& attributes, GameplayTagContainer& ownedTags);

		GameplayEffectHandle ApplyEffect(const GameplayEffectDefinition& definition, Actor* source = nullptr);
		GameplayEffectHandle ApplyEffect(
			const GameplayEffectDefinition& definition,
			const GameplayEffectApplicationContext& context
		);
		GameplayEffectHandle ApplyEffect(
			const GameplayEffectSpec& spec,
			Actor* source = nullptr
		);
		GameplayEffectHandle ApplyEffect(
			const GameplayEffectSpec& spec,
			const GameplayEffectApplicationContext& context
		);
		// Extends an existing duration effect without rebuilding its modifiers,
		// granted tags, or presentation actor.
		bool RefreshEffectDuration(GameplayEffectHandle handle);
		void RemoveEffect(GameplayEffectHandle handle);
		void Tick(float deltaTime);
		void Clear();
		void ProcessIncomingDamage(DamageContext& context);
		List<AbilityEvent> DrainPendingEvents();
		const ActiveGameplayEffect* FindEffect(GameplayEffectHandle handle) const;
		const ActiveGameplayEffect* FindEffectById(const std::string& effectId) const;
		List<GameplayEffectSnapshot> BuildSnapshots() const;
		bool CanApplyEffect(const GameplayEffectDefinition& definition) const;

		Delegate<GameplayEffectHandle> onEffectApplied;
		Delegate<GameplayEffectHandle> onEffectRemoved;
		Delegate<GameplayEffectHandle> onEffectChanged;
		Delegate<> onEffectsChanged;

	private:
		void QueueOwnerEvent(const GameplayTag& eventTag, const DamageContext* context = nullptr, float magnitude = 0.f);
		void ApplyModifiers(ActiveGameplayEffect& effect);
		void RemoveModifiers(ActiveGameplayEffect& effect);
		void GrantTags(const ActiveGameplayEffect& effect);
		void RemoveGrantedTags(const ActiveGameplayEffect& effect);
		void RemoveEffectAt(size_t index);
		void SpawnVisual(ActiveGameplayEffect& effect);
		void SynchronizeVisual(ActiveGameplayEffect& effect);
		void DestroyVisual(ActiveGameplayEffect& effect);

		Actor* mOwner = nullptr;
		AttributeSystem* mAttributes = nullptr;
		GameplayTagContainer* mOwnedTags = nullptr;
		List<ActiveGameplayEffect> mActiveEffects;
		List<AbilityEvent> mPendingEvents;
		unsigned int mNextHandleId = 1;
	};
}

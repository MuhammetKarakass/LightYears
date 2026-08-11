#pragma once

#include "abilities/AbilitySystemRuntime.h"
#include "abilities/AbilityCooldownTags.h"
#include "abilities/GameplayAbilityInstance.h"
#include "attributes/AttributeSystem.h"
#include "effects/ActiveGameplayEffect.h"
#include "effects/GameplayEffectSystem.h"

#include <memory>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace sas
{
	class AbilitySystemComponent
	{
	public:
		using EffectCallbacks = GameplayEffectSystem::Callbacks;

		AbilitySystemComponent();
		virtual ~AbilitySystemComponent() = default;
		AbilitySystemComponent(const AbilitySystemComponent&) = delete;
		AbilitySystemComponent& operator=(const AbilitySystemComponent&) = delete;
		AbilitySystemComponent(AbilitySystemComponent&&) = delete;
		AbilitySystemComponent& operator=(AbilitySystemComponent&&) = delete;

		template <typename Definition, typename Instance>
		AbilitySystemRuntime<Definition, Instance>& InitializeAbilitySystem(
			std::size_t maxPassiveAbilities
		)
		{
			auto runtime =
				std::make_unique<AbilitySystemRuntime<Definition, Instance>>(
					maxPassiveAbilities
				);
			auto* installed = runtime.get();
			mAbilityRuntime = std::move(runtime);
			return *installed;
		}

		template <typename Definition>
		AbilityHandle GrantAbility(
			const Definition& definition,
			std::string* failureReason = nullptr
		)
		{
			if (!mAbilityRuntime ||
				mAbilityRuntime->GetDefinitionType() != typeid(Definition))
			{
				if (failureReason)
				{
					*failureReason =
						"Ability system component has no compatible ability runtime.";
				}
				return {};
			}
			return mAbilityRuntime->GrantAbilityUntyped(
				&definition,
				failureReason
			);
		}

		bool RemoveAbility(
			AbilityHandle handle,
			AbilityEndReason reason = AbilityEndReason::Cancelled
		);
		void ClearAbilitySlot(AbilitySlot slot);
		void SetAbilitySlotInput(AbilitySlot slot, bool inputHeld);
		bool SetAbilityLevel(AbilityHandle handle, int level);
		bool SetAbilityLevel(AbilitySlot slot, int level);
		bool LevelUpAbility(AbilityHandle handle);
		bool LevelUpAbility(AbilitySlot slot);
		void ReduceAbilityCooldowns(
			float amount,
			bool includePrimaryFire = true
		);

		template <typename Instance>
		Instance* FindAbility(AbilityHandle handle)
		{
			return mAbilityRuntime &&
				mAbilityRuntime->GetInstanceType() == typeid(Instance)
					? static_cast<Instance*>(mAbilityRuntime->FindAbility(handle))
					: nullptr;
		}

		template <typename Instance>
		const Instance* FindAbility(AbilityHandle handle) const
		{
			return mAbilityRuntime &&
				mAbilityRuntime->GetInstanceType() == typeid(Instance)
					? static_cast<const Instance*>(
						mAbilityRuntime->FindAbility(handle)
					)
					: nullptr;
		}

		template <typename Instance>
		Instance* FindAbility(AbilitySlot slot)
		{
			return mAbilityRuntime &&
				mAbilityRuntime->GetInstanceType() == typeid(Instance)
					? static_cast<Instance*>(mAbilityRuntime->FindAbility(slot))
					: nullptr;
		}

		template <typename Instance>
		const Instance* FindAbility(AbilitySlot slot) const
		{
			return mAbilityRuntime &&
				mAbilityRuntime->GetInstanceType() == typeid(Instance)
					? static_cast<const Instance*>(
						mAbilityRuntime->FindAbility(slot)
					)
					: nullptr;
		}

		template <typename Instance>
		Instance* FindAbilityById(const std::string& abilityId)
		{
			return mAbilityRuntime &&
				mAbilityRuntime->GetInstanceType() == typeid(Instance)
					? static_cast<Instance*>(
						mAbilityRuntime->FindAbilityById(abilityId)
					)
					: nullptr;
		}

		template <typename Instance>
		const Instance* FindAbilityById(const std::string& abilityId) const
		{
			return mAbilityRuntime &&
				mAbilityRuntime->GetInstanceType() == typeid(Instance)
					? static_cast<const Instance*>(
						mAbilityRuntime->FindAbilityById(abilityId)
					)
					: nullptr;
		}

		ly::List<AbilityRuntimeSnapshot> BuildAbilitySnapshots() const;
		const ly::List<AbilityHandle>& GetPassiveAbilities() const;
		std::size_t GetPassiveAbilityCount() const
		{
			return GetPassiveAbilities().size();
		}

		AbilityInstanceNotifications CreateAbilityInstanceNotifications();
		void NotifyAbilityChanged(AbilityHandle handle);

		template <typename Event, typename Handler>
		void SetGameplayEventHandler(Handler&& handler)
		{
			static_assert(
				std::is_base_of_v<AbilityEvent, Event>,
				"Gameplay event types must derive from sas::AbilityEvent."
			);
			mGameplayEventType = &typeid(Event);
			mGameplayEventHandler =
				[callback = std::forward<Handler>(handler)](
					const void* event
				) mutable
				{
					std::invoke(
						callback,
						*static_cast<const Event*>(event)
					);
				};
		}

		template <typename Event>
		bool HandleGameplayEvent(const Event& event)
		{
			static_assert(
				std::is_base_of_v<AbilityEvent, Event>,
				"Gameplay event types must derive from sas::AbilityEvent."
			);
			onGameplayEvent.Broadcast(event);
			if (!mGameplayEventHandler ||
				!mGameplayEventType ||
				*mGameplayEventType != typeid(Event))
			{
				return false;
			}
			mGameplayEventHandler(&event);
			return true;
		}

		ly::Delegate<AbilityHandle> onAbilityGranted;
		ly::Delegate<AbilityHandle> onAbilityRemoved;
		ly::Delegate<AbilityHandle> onAbilityChanged;
		ly::Delegate<AbilityHandle> onAbilityActivated;
		ly::Delegate<AbilityHandle, AbilityEndReason> onAbilityEnded;
		ly::Delegate<AbilityHandle, int> onAbilityLevelChanged;
		ly::Delegate<> onAbilitiesCleared;
		ly::Delegate<const AbilityEvent&> onGameplayEvent;

		AttributeSystem& GetAttributes() { return mAttributes; }
		const AttributeSystem& GetAttributes() const { return mAttributes; }

		ly::GameplayTagContainer& GetOwnedTags() { return mOwnedTags; }
		const ly::GameplayTagContainer& GetOwnedTags() const
		{
			return mOwnedTags;
		}

		void AddOwnedTag(const ly::GameplayTag& tag);
		void RemoveOwnedTag(const ly::GameplayTag& tag);
		bool HasOwnedTag(
			const ly::GameplayTag& tag,
			bool exactMatch = false
		) const;
		bool HasAllOwnedTags(const ly::List<ly::GameplayTag>& tags) const;
		bool HasAnyOwnedTags(const ly::List<ly::GameplayTag>& tags) const;

		void SetEffectRuntimeCallbacks(EffectCallbacks callbacks);
		GameplayEffectHandle ApplyGameplayEffect(
			const GameplayEffectDefinition& definition,
			const GameplayEffectSourceContext& context = {}
		);
		GameplayEffectHandle ApplyGameplayEffect(
			const GameplayEffectSpec& spec,
			const GameplayEffectSourceContext& context = {}
		);
		bool RefreshGameplayEffectDuration(GameplayEffectHandle handle);
		void RemoveGameplayEffect(GameplayEffectHandle handle);
		std::size_t RemoveGameplayEffectsIf(
			const std::function<bool(const ActiveGameplayEffect&)>& predicate
		);
		ActiveGameplayEffect* FindGameplayEffect(GameplayEffectHandle handle);
		const ActiveGameplayEffect* FindGameplayEffect(
			GameplayEffectHandle handle
		) const;
		ActiveGameplayEffect* FindGameplayEffectById(const std::string& effectId);
		const ActiveGameplayEffect* FindGameplayEffectById(
			const std::string& effectId
		) const;
		ly::List<GameplayEffectRuntimeSnapshot> BuildGameplayEffectSnapshots()
			const;
		bool CanApplyGameplayEffect(
			const GameplayEffectDefinition& definition
		) const;

		template <
			typename EventContext,
			typename EventPhase,
			typename PhaseResolver,
			typename EventProcessor,
			typename ContinuePredicate
		>
		void ProcessGameplayEffectEvent(
			EventContext& context,
			const std::vector<EventPhase>& phases,
			PhaseResolver&& resolvePhase,
			EventProcessor&& process,
			ContinuePredicate&& shouldContinue
		)
		{
			mEffects.ProcessEvent(
				context,
				phases,
				std::forward<PhaseResolver>(resolvePhase),
				std::forward<EventProcessor>(process),
				std::forward<ContinuePredicate>(shouldContinue)
			);
		}

		ly::Delegate<GameplayEffectHandle> onGameplayEffectApplied;
		ly::Delegate<GameplayEffectHandle> onGameplayEffectRemoved;
		ly::Delegate<GameplayEffectHandle> onGameplayEffectChanged;
		ly::Delegate<> onGameplayEffectsChanged;

		void Tick(float deltaTime);
		void Clear();

	protected:
		template <typename Definition, typename Instance>
		void ConfigureAbilityRuntime(
			AbilitySystemRuntime<Definition, Instance>& runtime,
			typename AbilitySystemRuntime<
				Definition,
				Instance
			>::Callbacks callbacks
		)
		{
			auto granted = std::move(callbacks.granted);
			callbacks.granted =
				[this, callback = std::move(granted)](AbilityHandle handle)
				{
					if (callback)
					{
						callback(handle);
					}
					onAbilityGranted.Broadcast(handle);
				};

			auto removed = std::move(callbacks.removed);
			callbacks.removed =
				[this, callback = std::move(removed)](AbilityHandle handle)
				{
					if (callback)
					{
						callback(handle);
					}
					onAbilityRemoved.Broadcast(handle);
				};

			auto changed = std::move(callbacks.changed);
			callbacks.changed =
				[this, callback = std::move(changed)](AbilityHandle handle)
				{
					if (callback)
					{
						callback(handle);
					}
					NotifyAbilityChanged(handle);
				};

			auto cleared = std::move(callbacks.cleared);
			callbacks.cleared =
				[this, callback = std::move(cleared)]()
				{
					if (callback)
					{
						callback();
					}
					onAbilitiesCleared.Broadcast();
				};

			runtime.SetCallbacks(std::move(callbacks));
		}

	private:
		void RebuildEffectRuntimeCallbacks();
		void RefreshCooldownTags();

		std::unique_ptr<AbilitySystemRuntimeBase> mAbilityRuntime;
		const std::type_info* mGameplayEventType = nullptr;
		std::function<void(const void*)> mGameplayEventHandler;
		AttributeSystem mAttributes;
		ly::GameplayTagContainer mOwnedTags;
		GameplayEffectSystem mEffects;
		EffectCallbacks mEffectBindings;
		bool mAbilityCooldownTagActive = false;
		bool mPrimaryWeaponCooldownTagActive = false;
	};
}

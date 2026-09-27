#include "framework/Application.h"
#include "framework/PhysicsSystem.h"
#include "framework/World.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/content/GameAbilityActions.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/energySpear/EnergySpearContracts.h"
#include "gameplay/ability/loadout/AbilityLoadoutManager.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerActor.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolContracts.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolVisualActor.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/cryostasis/CryostasisContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageContext.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileReflectionRegistryActor.h"
#include "gameplay/movement/MovementCollisionService.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/weapon/visuals/ContinuousBeamVisualActor.h"
#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace ly
{
	struct AuditFixesE2EAccess
	{
		static DelegateHandle Subscription(const NanoPlagueControllerActor& actor, const Actor& target)
		{
			for (const auto& [id, infection] : actor.mInfections)
				if (infection.target.lock().get() == &target) return infection.damageSubscription;
			return {};
		}
		static sf::Vector2f LastOrigin(const NanoPlagueControllerActor& actor) { return actor.mPulses.back().origin; }
		static int Ticks(const NanoPlagueControllerActor& actor, const Actor& target)
		{
			for (const auto& [id, infection] : actor.mInfections)
				if (infection.target.lock().get() == &target) return infection.ticksApplied;
			return -1;
		}
		static std::size_t Count(const NanoPlagueControllerActor& actor) { return actor.mInfections.size(); }
		static const void* FirstRecord(const NanoPlagueControllerActor& actor) { return &actor.mInfections.begin()->second; }
	};

	namespace
	{
		std::string& AuditTitle() { static std::string title = "Audit fixes E2E"; return title; }
		class AuditApplication final : public Application
		{
		public:
			AuditApplication() : Application({ 320, 240 }, 32, AuditTitle(), sf::Style::None)
			{ GetRenderWindow().setVisible(false); }
		};
		struct LoadoutObserver
		{
			LightYearsAbilitySystemComponent& system;
			bool clear = false;
			void Granted(sas::AbilityHandle)
			{
				if (clear) system.Clear();
				else throw std::runtime_error("Injected post-commit observer failure");
			}
		};
		class EndpointWall final : public Actor
		{
		public:
			explicit EndpointWall(World* world) : Actor(world)
			{ SetPhysicsBodyType(PhysicsBodyType::Static); SetCollisionLayer(CollisionLayer::Environment); }
			sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override { return { 10.f, 20.f }; }
		};
		class RotatingWall final : public Actor
		{
		public:
			explicit RotatingWall(World* world) : Actor(world)
			{ SetEnablePhysics(false); SetPhysicsBodyType(PhysicsBodyType::Static); SetCollisionLayer(CollisionLayer::Environment); }
			sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override { return { 400.f, 10.f }; }
		};
		struct LifecycleFaultObserver
		{
			LightYearsAbilitySystemComponent& system;
			sas::GameplayEffectDefinition effect;
			bool visited = false, liveDuringClear = false, mutationRejected = false, additionsRejected = false;
			bool throwOnEnd = false;
			bool throwAfterClear = false;
			void Activated(sas::AbilityHandle handle)
			{
				visited = true;
				mutationRejected = !system.RemoveAbility(handle) &&
					!system.RebindAbility(handle, sas::AbilitySlot::Ability2) &&
					!system.GrantAbility(*AbilityData::FindShippedAbilityDefinition(AbilityData::EnergySpear::AbilityId::Basic), sas::AbilitySlot::Ability1).IsValid();
				system.Clear();
				liveDuringClear = system.GetAbility(handle) != nullptr && system.GetAttributes().HasAttribute(OwnerAttributeIds::MaxHealth);
				if (throwAfterClear) throw std::runtime_error("Injected activation failure after Clear");
			}
			void Ended(sas::AbilityHandle, sas::AbilityEndReason)
			{
				system.Clear();
				system.AddOwnedTag(AbilityData::NanoPlague::State::Infected);
				additionsRejected = !system.ApplyGameplayEffect(effect).IsValid() &&
					!system.HasOwnedTag(AbilityData::NanoPlague::State::Infected);
				if (throwOnEnd) throw std::runtime_error("Injected ended failure");
			}
			void EffectRemoved(sas::GameplayEffectHandle) { throw std::runtime_error("Injected effect removal failure"); }
			void GameplayEvent(const sas::AbilityEvent& event)
			{
				if (event.eventTag == GameplayTags::Event::Ability::Activated) { visited = true; system.Clear(); }
			}
		};
		struct AttributeCleanupFault
		{
			sas::AttributeId target;
			std::string message;
			bool armed = false;
			bool triggered = false;
			std::vector<std::string>* order = nullptr;
			void Changed(sas::AttributeId id, float, float)
			{
				if (armed && !triggered && id == target)
				{
					triggered = true;
					if (order) order->push_back(std::string{ "modifier-cleanup:" } + std::string{ id.GetName() });
					throw std::runtime_error(message);
				}
			}
		};
		struct AttributeModifierReentry
		{
			sas::AttributeSystem& attributes;
			sas::AttributeModifierHandle handle;
			bool armed = false;
			bool visited = false;
			std::size_t callbackCount = 0;
			std::vector<std::string> order;
			void Changed(sas::AttributeId, float, float)
			{
				if (!armed) return;
				++callbackCount;
				if (visited) return;
				visited = true;
				order.push_back("attribute-change-callback");
				order.push_back("reentrant-remove-begin");
				attributes.RemoveModifier(handle);
				order.push_back("reentrant-remove-return");
			}
		};
		struct AttributeRegisteredClearFault
		{
			sas::AttributeSystem& attributes;
			bool visited = false;
			void Registered(sas::AttributeId)
			{
				visited = true;
				attributes.Clear();
			}
		};
		struct AttributeRegisteredThrowFault
		{
			std::string message;
			bool visited = false;
			sas::AttributeId registeredId;
			std::vector<std::string> order;
			void Registered(sas::AttributeId id)
			{
				visited = true;
				registeredId = id;
				order.push_back("attribute-registered-callback");
				throw std::runtime_error(message);
			}
		};
		struct AttributeChangedClearFault
		{
			sas::AttributeSystem& attributes;
			sas::AttributeId target;
			bool clear = false;
			bool visited = false;
			std::string message;
			void Changed(sas::AttributeId id, float, float)
			{
				if (visited || id != target) return;
				visited = true;
				if (clear) attributes.Clear();
				else throw std::runtime_error(message);
			}
		};
		struct ActivationBoundaryObserver
		{
			LightYearsAbilitySystemComponent& system;
			GameplayTag phase;
			bool clear = false, cancel = false, visited = false, reentryRejected = false;
			bool cleanupFault = false, committedStateVisible = false;
			void Event(const sas::AbilityEvent& event)
			{
				if (cleanupFault && event.eventTag == AbilityData::ReturnProtocol::Event::Ended)
					throw std::runtime_error("Injected cleanup failure after activation failure");
				if (event.eventTag != phase || visited) return;
				visited = true;
				if (auto* ability = system.GetAbility(sas::AbilitySlot::Ability1))
				{
					committedStateVisible = ability->IsActive();
					reentryRejected = !ability->TryActivate() && !ability->SetLevel(2);
					if (cancel) ability->Cancel(sas::AbilityEndReason::Cancelled);
				}
				if (clear) system.Clear();
				if (!cancel) throw std::runtime_error("Injected precommit activation fault");
			}
		};
		struct NestedLoadoutObserver
		{
			AbilityLoadoutManager& loadout;
			bool equipped = true;
			void Event(const sas::AbilityEvent& event)
			{
				if (event.eventTag == GameplayTags::Event::Combat::DamageReceived)
					equipped = loadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability2);
			}
		};
		struct NanoReentryObserver
		{
			shared_ptr<NanoPlagueControllerActor> controller;
			shared_ptr<SpaceShip> original;
			std::vector<shared_ptr<SpaceShip>> additions;
			NanoPlagueControllerActor::Settings settings;
			bool visited = false;
			void Damaged(const DamageContext&)
			{
				if (visited) return;
				visited = true;
				for (const auto& target : additions) controller->ApplyOrRefreshInfection(*target, 1, settings);
				controller->ApplyOrRefreshInfection(*original, 1, settings);
				controller->Tick(10.f); // Nested Tick must not advance refreshed state.
			}
		};
		struct RuntimeActionAudit
		{
			LightYearsAbilitySystemComponent& system;
			Actor& owner;
			sas::AbilityHandle handle;
			GameplayTag eventTag;
			int cancelAtMagnitude = 0;
			bool nestedReentry = false;
			bool eventVisited = false;
			bool directSetLevelResult = true;
			bool componentSetLevelResult = true;
			bool nestedTryActivateResult = true;
			int levelBeforeNestedCalls = 0;
			int levelAfterNestedCalls = 0;
			int chargesBeforeNestedCalls = 0;
			int chargesAfterNestedCalls = 0;
			std::size_t endCount = 0;
			std::vector<int> magnitudes;
			std::vector<std::string> order;

			void Event(const sas::AbilityEvent& event)
			{
				if (event.eventTag != eventTag || event.GetSource<Actor>() != &owner) return;
				const int magnitude = static_cast<int>(std::round(event.magnitude));
				magnitudes.push_back(magnitude);
				order.push_back("event:" + std::to_string(magnitude));
				if (magnitude == cancelAtMagnitude && !nestedReentry)
				{
					order.push_back("cancel-request");
					if (GameAbility* ability = system.GetAbility(handle))
						ability->Cancel(sas::AbilityEndReason::Cancelled);
					order.push_back("cancel-return");
				}
				if (!nestedReentry || eventVisited || magnitude != cancelAtMagnitude) return;
				eventVisited = true;
				GameAbility* ability = system.GetAbility(handle);
				if (!ability) return;
				levelBeforeNestedCalls = ability->GetLevel();
				chargesBeforeNestedCalls = ability->GetCharges();
				order.push_back("nested-set-level");
				directSetLevelResult = ability->SetLevel(levelBeforeNestedCalls + 1);
				componentSetLevelResult = system.SetAbilityLevel(handle, levelBeforeNestedCalls + 1);
				ability->Tick(1.f);
				system.Tick(1.f);
				nestedTryActivateResult = ability->TryActivate();
				levelAfterNestedCalls = ability->GetLevel();
				chargesAfterNestedCalls = ability->GetCharges();
				order.push_back("cancel-request");
				ability->Cancel(sas::AbilityEndReason::Cancelled);
				order.push_back("cancel-return");
			}

			void Ended(sas::AbilityHandle ended, sas::AbilityEndReason)
			{
				if (!(ended == handle)) return;
				++endCount;
				order.push_back("ability-ended");
			}
		};
		struct ScopedRuleActivationAudit
		{
			LightYearsAbilitySystemComponent& system;
			sas::AbilityHandle handle;
			ScopedAbilityRule rule;
			bool visited = false;
			std::size_t ruleHandle = 0;
			std::vector<std::string>* order = nullptr;

			void Activated(sas::AbilityHandle activated)
			{
				if (!(activated == handle) || visited) return;
				visited = true;
				if (order) order->push_back("scoped-rule-add");
				ruleHandle = system.AddScopedAbilityRule(rule);
			}
		};
		struct BeamDestroyAudit
		{
			uint64_t failingActorId = 0;
			uint64_t siblingActorId = 0;
			uint64_t independentActorId = 0;
			bool throwOnce = true;
			std::vector<std::string>& order;

			void Destroyed(Actor* actor)
			{
				if (!actor) return;
				if (actor->GetUniqueID() == failingActorId)
				{
					order.push_back(throwOnce ? "first-beam-destroy-throws" : "first-beam-destroy-retry");
					if (throwOnce)
					{
						throwOnce = false;
						throw std::runtime_error("Injected E11 first beam EndFire failure");
					}
				}
				else if (actor->GetUniqueID() == siblingActorId)
				{
					order.push_back("first-ability-sibling-beam-destroy");
				}
				else if (actor->GetUniqueID() == independentActorId)
				{
					order.push_back("second-ability-beam-destroy");
				}
			}
		};
		struct InvocationDestroyAudit
		{
			uint64_t actorId = 0;
			bool throwOnce = true;
			std::vector<std::string>& order;

			void Destroyed(Actor* actor)
			{
				if (!actor || actor->GetUniqueID() != actorId) return;
				order.push_back(throwOnce ? "invocation-visual-destroy-throws" : "invocation-visual-destroy-retry");
				if (!throwOnce) return;
				throwOnce = false;
				throw std::runtime_error("Injected E11 ReturnProtocol visual cleanup failure");
			}
		};
		struct ReturnProtocolEventAudit
		{
			Actor& owner;
			std::vector<std::string>& order;
			std::size_t startedCount = 0;
			std::size_t endedCount = 0;

			void Event(const sas::AbilityEvent& event)
			{
				if (event.GetSource<Actor>() != &owner) return;
				if (event.eventTag == AbilityData::ReturnProtocol::Event::Started)
				{
					++startedCount;
					order.push_back("return-protocol-started");
				}
				else if (event.eventTag == AbilityData::ReturnProtocol::Event::Ended)
				{
					++endedCount;
					order.push_back("return-protocol-ended");
				}
			}
		};

		sas::GameplayEffectHandle ApplyAuditIceShell(
			SpaceShip& ship,
			const sas::GameplayEffectDefinition& definition,
			float iceHealth,
			float duration
		)
		{
			auto spec = sas::MakeGameplayEffectSpec(definition);
			spec.duration = duration;
			spec.maxStacks = 1;
			spec.attributes = { sas::GameplayAttribute{
				AbilityData::Cryostasis::Effect::IceHealth, iceHealth, 0.f, iceHealth
			} };
			return ship.GetAbilitySystemComponent().ApplyGameplayEffect(
				spec, sas::GameplayEffectSourceContext{ &ship, nullptr }
			);
		}

		sas::GameplayEffectHandle ApplyAuditBarrier(
			SpaceShip& ship,
			const sas::GameplayEffectDefinition& definition,
			float capacity,
			float duration
		)
		{
			auto spec = sas::MakeGameplayEffectSpec(definition);
			spec.duration = duration;
			spec.attributes = {
				sas::GameplayAttribute{ BarrierEffectSchema::Capacity, capacity, 0.f, capacity },
				sas::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f },
				sas::GameplayAttribute{ BarrierEffectSchema::RegenerationDelay, 0.f, 0.f, 0.f },
				sas::GameplayAttribute{ BarrierEffectSchema::RegenerationDelayRemaining, 0.f, 0.f, 0.f }
			};
			return ship.GetAbilitySystemComponent().ApplyGameplayEffect(
				spec, sas::GameplayEffectSourceContext{ &ship, nullptr }
			);
		}

		struct NestedDamageFrameAudit
		{
			SpaceShip& first;
			SpaceShip& middle;
			const sas::GameplayEffectDefinition& iceShell;
			std::vector<std::string> sequence;
			std::vector<std::uint64_t> sourceIds;
			std::vector<std::uint64_t> targetIds;
			std::vector<float> originalDamages;
			const DamageContext* outerContext = nullptr;
			bool nestedShellApplied = false;
			bool middleContextValid = false;
			bool innerContextValid = false;
			bool outerFrameRestoredAfterNested = false;
			bool outerDamageReceivedMatches = false;
			std::size_t iceBrokenCount = 0;
			std::size_t middleDamageReceivedCount = 0;

			void Record(const char* role, const DamageContext& context)
			{
				sequence.emplace_back(role);
				sourceIds.push_back(context.source ? context.source->GetUniqueID() : 0);
				targetIds.push_back(context.target ? context.target->GetUniqueID() : 0);
				originalDamages.push_back(context.originalDamage);
			}

			void FirstEvent(const sas::AbilityEvent& event)
			{
				if (event.eventTag == AbilityData::Cryostasis::Event::IceBroken)
				{
					const DamageContext* context = event.GetContext<DamageContext>();
					if (!context) return;
					++iceBrokenCount;
					if (!outerContext)
					{
						outerContext = context;
						Record("A", *context);
						DamageContext middleDamage;
						middleDamage.source = &first;
						middleDamage.target = &middle;
						middleDamage.originalDamage = 20.f;
						middleDamage.remainingDamage = 20.f;
						middle.ReceiveDamage(middleDamage);
						nestedShellApplied = ApplyAuditIceShell(first, iceShell, 5.f, 5.f).IsValid();
						DamageContext innerDamage;
						innerDamage.source = &middle;
						innerDamage.target = &first;
						innerDamage.originalDamage = 30.f;
						innerDamage.remainingDamage = 30.f;
						first.ReceiveDamage(innerDamage);
						outerFrameRestoredAfterNested = context == outerContext &&
							context->source == &middle && context->target == &first &&
							context->originalDamage == 10.f && context->remainingDamage == 0.f &&
							context->absorbedDamage == 10.f;
					}
					else if (context->source == &middle && context->target == &first &&
						context->originalDamage == 30.f && context->remainingDamage == 0.f)
					{
						innerContextValid = true;
						Record("A", *context);
					}
					return;
				}
				if (event.eventTag != GameplayTags::Event::Combat::DamageReceived) return;
				const DamageContext* context = event.GetContext<DamageContext>();
				if (context && context->target == &first && context->originalDamage == 10.f)
				{
					outerDamageReceivedMatches = context == outerContext &&
						context->source == &middle && context->remainingDamage == 0.f &&
						context->absorbedDamage == 10.f;
				}
			}

			void MiddleEvent(const sas::AbilityEvent& event)
			{
				if (event.eventTag != GameplayTags::Event::Combat::DamageReceived) return;
				const DamageContext* context = event.GetContext<DamageContext>();
				if (!context || context->target != &middle || context->source != &first ||
					context->originalDamage != 20.f || context->remainingDamage != 20.f) return;
				++middleDamageReceivedCount;
				middleContextValid = true;
				Record("B", *context);
			}
		};

		struct ClearDamageEventAudit
		{
			SpaceShip& target;
			std::vector<std::string>& order;
			std::size_t breakEvents = 0;
			std::size_t damageReceivedEvents = 0;
			std::size_t sourceStatusEvents = 0;
			std::size_t damageProcessedEvents = 0;
			bool clearCompleted = false;
			bool breakContextValid = false;
			float remainingDamageAtBreak = 0.f;

			void TargetEvent(const sas::AbilityEvent& event)
			{
				if (event.eventTag == BarrierEffectSchema::BrokenEventTag)
				{
					++breakEvents;
					const DamageContext* context = event.GetContext<DamageContext>();
					breakContextValid = context && context->target == &target && context->source &&
						context->originalDamage == 20.f;
					remainingDamageAtBreak = context ? context->remainingDamage : -1.f;
					order.push_back("barrier-broken-clear-throw");
					target.GetCombatRuntime().Clear();
					clearCompleted = true;
					throw std::runtime_error("Injected E15 Clear callback failure");
				}
				if (event.eventTag == GameplayTags::Event::Combat::DamageReceived)
				{
					++damageReceivedEvents;
					order.push_back("stale-damage-received");
				}
			}

			void SourceEvent(const sas::AbilityEvent& event)
			{
				if (event.eventTag == AttachmentSchema::Event::SourceStatusIgniteApplied)
				{
					++sourceStatusEvents;
					order.push_back("stale-source-ignite");
				}
			}

			void DamageProcessed(const DamageContext&)
			{
				++damageProcessedEvents;
				order.push_back("stale-damage-processed");
			}
		};

		struct DamageFlowProbe
		{
			SpaceShip& target;
			SpaceShip& source;
			std::size_t breakEvents = 0;
			std::size_t damageReceivedEvents = 0;
			std::size_t sourceStatusEvents = 0;
			std::size_t damageProcessedEvents = 0;
			bool damageReceivedContextValid = false;
			float remainingDamageAtReceived = 0.f;

			void TargetEvent(const sas::AbilityEvent& event)
			{
				if (event.eventTag == BarrierEffectSchema::BrokenEventTag) ++breakEvents;
				if (event.eventTag != GameplayTags::Event::Combat::DamageReceived) return;
				++damageReceivedEvents;
				const DamageContext* context = event.GetContext<DamageContext>();
				damageReceivedContextValid = context && context->source == &source && context->target == &target &&
					context->originalDamage == 20.f;
				remainingDamageAtReceived = context ? context->remainingDamage : -1.f;
			}

			void SourceEvent(const sas::AbilityEvent& event)
			{
				if (event.eventTag == AttachmentSchema::Event::SourceStatusIgniteApplied) ++sourceStatusEvents;
			}

			void DamageProcessed(const DamageContext&)
			{
				++damageProcessedEvents;
			}
		};

		struct CrossFlowBeamDestroyAudit
		{
			std::uint64_t actorId = 0;
			LightYearsAbilitySystemComponent& system;
			std::vector<std::string>& order;
			bool throwOnce = true;
			bool clearCalledBeforeThrow = false;
			std::string clearError;

			void Destroyed(Actor* actor)
			{
				if (!actor || actor->GetUniqueID() != actorId) return;
				if (!throwOnce)
				{
					order.push_back("beam-destroy-retry");
					return;
				}
				throwOnce = false;
				order.push_back("component-clear-from-visual-destroy");
				clearCalledBeforeThrow = true;
				try { system.Clear(); }
				catch (const std::runtime_error& error) { clearError = error.what(); }
				order.push_back("beam-destroy-throws");
				throw std::runtime_error("Injected E18 beam cleanup failure");
			}
		};

		struct CrossFlowEffectAudit
		{
			LightYearsAbilitySystemComponent& system;
			sas::AbilityHandle abilityHandle;
			const GameplayTag& grantedTag;
			const sas::AttributeId& modifiedAttribute;
			float attributeBefore = 0.f;
			std::vector<std::string>& order;
			bool callbackVisited = false;
			bool effectResourcesPresent = false;
			bool cancelAttempted = false;
			bool abilityCancelled = false;
			bool abilityInactiveAfterCancelFault = false;
			bool cancelThrew = false;
			bool clearCalled = false;
			bool clearThrew = false;
			std::string cancelError;
			std::string clearError;

			void Activated(sas::ActiveGameplayEffect&)
			{
				callbackVisited = true;
				order.push_back("effect-activated");
				effectResourcesPresent = system.HasOwnedTag(grantedTag, true) &&
					system.GetAttributes().GetCurrentValue(modifiedAttribute) == attributeBefore + 0.25f;
				order.push_back("ability-cancel");
				if (GameAbility* ability = system.GetAbility(abilityHandle))
				{
					cancelAttempted = true;
					try
					{
						ability->Cancel(sas::AbilityEndReason::Cancelled);
						abilityCancelled = !ability->IsActive();
					}
					catch (const std::runtime_error& error)
					{
						cancelThrew = true;
						cancelError = error.what();
						order.push_back("ability-cleanup-threw");
					}
					abilityInactiveAfterCancelFault = !ability->IsActive();
				}
				order.push_back("component-clear");
				clearCalled = true;
				try { system.Clear(); }
				catch (const std::runtime_error& error)
				{
					clearThrew = true;
					clearError = error.what();
					order.push_back("clear-threw");
				}
			}
		};

		struct PlayerPurchaseLevelAudit
		{
			Player& player;
			LightYearsAbilitySystemComponent& system;
			sas::AbilitySlot slot;
			int targetLevel = 0;
			std::vector<std::string>& order;
			bool attemptNestedPurchase = false;
			bool clearSystem = true;
			bool throwAfterClear = true;
			bool visited = false;
			bool nestedPurchaseResult = true;
			bool clearThrew = false;
			std::uint64_t changedHandle = 0;
			int changedLevel = 0;
			unsigned int scrapAtCallback = 0;
			unsigned int scrapAfterNestedPurchase = 0;
			std::string nestedFailureReason;
			std::string clearError;
			std::string throwMessage;

			void Changed(sas::AbilityHandle handle, int level)
			{
				if (visited || level != targetLevel) return;
				visited = true;
				changedHandle = handle.id;
				changedLevel = level;
				scrapAtCallback = player.GetScrap();
				order.push_back("ability-level-observer");
				if (attemptNestedPurchase)
				{
					order.push_back("nested-purchase-rejected");
					nestedPurchaseResult = player.TryPurchaseAbilityLevel(slot, &nestedFailureReason);
					scrapAfterNestedPurchase = player.GetScrap();
				}
				if (clearSystem)
				{
					order.push_back("component-clear");
					try { system.Clear(); }
					catch (const std::runtime_error& error)
					{
						clearThrew = true;
						clearError = error.what();
					}
				}
				if (throwAfterClear)
				{
					order.push_back("ability-level-observer-throws");
					throw std::runtime_error(throwMessage);
				}
			}
		};

		struct PlayerPurchaseEndAudit
		{
			LightYearsAbilitySystemComponent& system;
			GameplayTag endedTag;
			std::vector<std::string>& order;
			bool clearSystem = true;
			bool throwOnce = true;
			bool handling = false;
			std::size_t endedCount = 0;
			bool clearThrew = false;
			std::string clearError;
			std::string throwMessage;

			void Event(const sas::AbilityEvent& event)
			{
				if (event.eventTag != endedTag || handling) return;
				handling = true;
				++endedCount;
				order.push_back("active-ability-ended");
				if (clearSystem)
				{
					order.push_back("component-clear-during-end");
					try { system.Clear(); }
					catch (const std::runtime_error& error)
					{
						clearThrew = true;
						clearError = error.what();
					}
				}
				if (throwOnce)
				{
					throwOnce = false;
					order.push_back("precommit-end-observer-throws");
					handling = false;
					throw std::runtime_error(throwMessage);
				}
				handling = false;
			}
		};

		AbilityActionSpec MakeEventAction(
			sas::AbilityActionPhase phase,
			const GameplayTag& eventTag,
			float magnitude
		)
		{
			AbilityActionSpec action;
			action.phase = phase;
			action.action = EmitGameplayEventAction{ eventTag, magnitude, {} };
			return action;
		}

		GameAbilityDefinition MakeRuntimeActionAbility(
			const std::string& abilityId,
			sas::AbilitySlot slot,
			const List<AbilityActionSpec>& actions
		)
		{
			GameAbilityDefinition definition;
			definition.abilityId = abilityId;
			definition.slot = slot;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::UntilCancelled;
			definition.maxCharges = 3;
			definition.abilityTags.push_back(GameplayTags::Ability::Utility);
			definition.behaviorType = AbilityBehaviorType::Configured;
			definition.actions = actions;
			return definition;
		}

		PrimaryWeaponDefinition MakeContinuousBeamDefinition(int muzzleCount = 1)
		{
			PrimaryWeaponDefinition definition;
			definition.weaponType = PrimaryWeaponType::BeamContinuous;
			definition.attributes = {
				sas::GameplayAttribute{ CommonAttributeIds::Damage, 1.f },
				sas::GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Range, 100.f },
				sas::GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Width, 4.f }
			};
			definition.muzzleDefinitions.clear();
			for (int index = 0; index < muzzleCount; ++index)
			{
				const float horizontalOffset = index == 0 ? -8.f : 8.f;
				definition.muzzleDefinitions.push_back(WeaponMuzzleDefinition{ { horizontalOffset, 50.f }, 0.f });
			}
			return definition;
		}
	}

	int RunAuditFixesE2E(const char* artifactPath, const std::string& setupError)
	{
		nlohmann::json result{ { "scenario", "audit-fixes.runtime-lifecycle" }, { "passed", false },
			{ "input", { { "maxima", { 100, 120, 150 } }, { "temporaryExcess", 40 },
				{ "nanoSpreadTargets", 4 }, { "callbackFaultInjection", true }, { "nanoReentryTargets", 32 },
				{ "nanoTickDelta", 0.25f }, { "wallRotations", { 0, 90, 0 } } } } };
		auto check = [&](const char* name, bool passed)
		{
			result["assertions"][name] = passed;
			if (!passed) throw std::runtime_error(name);
		};
		auto recordCase = [&](const char* group, nlohmann::json item)
		{
			if (!item.contains("order")) item["order"] = nlohmann::json::array();
			if (!item.contains("identities")) item["identities"] = nlohmann::json::object();
			if (!item.contains("before")) item["before"] = nlohmann::json::object();
			if (!item.contains("after")) item["after"] = nlohmann::json::object();
			if (!item.contains("errors")) item["errors"] = nlohmann::json::array();
			result[group].push_back(std::move(item));
		};
		try
		{
			if (!setupError.empty()) throw std::runtime_error(setupError);
			AuditApplication application;
			PhysicsSystem::Get().InitializeWorld({ 0.f, 0.f });
			World world{ &application };
			World otherWorld{ &application };
			const ShipDefinition definition{ "SpaceShooterRedux/PNG/Enemies/enemyRed5.png", 100.f, {}, 0.f, 0, 0, {}, {} };
			auto spawn = [&](float x, CollisionLayer layer)
			{
				auto ship = world.SpawnActor<SpaceShip>(definition).lock();
				ship->SetActorLocation({ x, 0.f });
				ship->SetCollisionLayer(layer);
				ship->SetCollisionMask(layer == CollisionLayer::Player ? CollisionLayer::Enemy : CollisionLayer::Player);
				return ship;
			};
			auto owner = spawn(0.f, CollisionLayer::Player);
			auto infected = spawn(100.f, CollisionLayer::Enemy);
			std::vector<shared_ptr<SpaceShip>> targets;
			for (int i = 0; i < 4; ++i) targets.push_back(spawn(130.f + 30.f * i, CollisionLayer::Enemy));
			world.TickInternal(0.f);
			auto& system = owner->GetAbilitySystemComponent();

			// Attribute event -> actual ship resources -> decay ledger.
			system.GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, 100.f);
			auto& health = owner->GetHealthComponent();
			auto& shield = owner->GetShieldComponent();
			owner->GetShipRuntime().GetAttributes().SetBaseValue(ShipAttributeIds::MaxShield, 100.f);
			health.GrantTemporaryOverhealth("audit", 40.f, 0.f, 10.f);
			shield.GrantTemporaryOvershield("audit", 40.f, 0.f, 10.f);
			for (float maximum : { 120.f, 150.f })
			{
				system.GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, maximum);
				owner->GetShipRuntime().GetAttributes().SetBaseValue(ShipAttributeIds::MaxShield, maximum);
				check("absoluteExcessPreserved", std::abs(health.GetHealth() - maximum - 40.f) < 0.001f &&
					std::abs(shield.GetShield() - maximum - 40.f) < 0.001f);
			}
			health.TickTemporaryOverhealths(1.f);
			shield.TickTemporaryOvershields(1.f);
			check("decayLedgerPreserved", health.GetHealth() == 180.f && shield.GetShield() == 180.f);
			result["outcome"]["healthAfterDecay"] = health.GetHealth();

			// Real damage resolution kills the direct infection; controller spreads.
			auto controller = NanoPlagueControllerActor::FindOrCreate(world, *owner, NanoPlaguePresentationProfile{});
			NanoPlagueControllerActor::Settings settings;
			settings.baseSpreadTargetCount = 4;
			settings.maximumSpreadTargetCount = 4;
			settings.baseTickDamage = 0.f;
			settings.energyPowerTickScale = 0.f;
			check("nanoApplied", controller->ApplyOrRefreshInfection(*infected, 0, settings));
			const auto oldSubscription = AuditFixesE2EAccess::Subscription(*controller, *infected);
			DamageContext damage;
			damage.target = infected.get();
			damage.source = owner.get();
			damage.originalDamage = 100000.f;
			damage.remainingDamage = 100000.f;
			infected->ReceiveDamage(damage);
			controller->Tick(0.01f);
			int spreadCount = 0;
			for (const auto& target : targets)
				if (target->GetAbilitySystemComponent().HasOwnedTag(AbilityData::NanoPlague::State::Infected)) ++spreadCount;
			check("nanoSpreadAfterRealKill", spreadCount == 4);
			check("spreadOriginStable", AuditFixesE2EAccess::LastOrigin(*controller) == sf::Vector2f{ 100.f, 0.f });
			check("deadInfectionUnsubscribed", !infected->GetCombatRuntime().onDamageResolved.UnbindAction(oldSubscription));
			const auto expiredSubscription = AuditFixesE2EAccess::Subscription(*controller, *targets.front());
			auto shortSettings = settings;
			shortSettings.duration = 0.01f;
			check("nanoRefresh", controller->ApplyOrRefreshInfection(*targets.front(), 1, shortSettings));
			controller->Tick(0.02f);
			check("expiryUnsubscribes", !targets.front()->GetCombatRuntime().onDamageResolved.UnbindAction(expiredSubscription));
			check("nanoReinfect", controller->ApplyOrRefreshInfection(*targets.front(), 1, settings));
			const auto liveSubscription = AuditFixesE2EAccess::Subscription(*controller, *targets.front());
			controller->Destroy();
			check("destroyUnsubscribes", !targets.front()->GetCombatRuntime().onDamageResolved.UnbindAction(liveSubscription));
			check("destroyClearsTag", !targets.front()->GetAbilitySystemComponent().HasOwnedTag(AbilityData::NanoPlague::State::Infected));

			// Shipped grants through public loadout, with real observer faults.
			AbilityLoadoutManager loadout{ system };
			LoadoutObserver observer{ system };
			auto subscription = system.onAbilityGranted.BindAction(&observer, &LoadoutObserver::Granted);
			bool threw = false;
			try { loadout.EquipAbility(AbilityData::EnergySpear::AbilityId::Basic, sas::AbilitySlot::Ability1); }
			catch (const std::runtime_error&) { threw = true; }
			system.onAbilityGranted.UnbindAction(subscription);
			const auto* bound = loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability1);
			check("exceptionCommitReconciled", threw && bound && *bound == AbilityData::EnergySpear::AbilityId::Basic &&
				system.GetAbility(sas::AbilitySlot::Ability1));
			check("reservationsReleased", loadout.EquipAbility(AbilityData::EnergySpear::AbilityId::Basic, sas::AbilitySlot::Ability2));
			subscription = system.onAbilityChanged.BindAction(&observer, &LoadoutObserver::Granted);
			threw = false;
			try { loadout.EquipAbility(AbilityData::EnergySpear::AbilityId::Basic, sas::AbilitySlot::Ability3); }
			catch (const std::runtime_error&) { threw = true; }
			system.onAbilityChanged.UnbindAction(subscription);
			check("rebindExceptionReconciled", threw && !loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability2) &&
				loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability3) && system.GetAbility(sas::AbilitySlot::Ability3));
			observer.clear = true;
			subscription = system.onAbilityGranted.BindAction(&observer, &LoadoutObserver::Granted);
			const bool granted = loadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1);
			system.onAbilityGranted.UnbindAction(subscription);
			check("callbackClearNoGhostSlot", !granted && !system.GetAbility(sas::AbilitySlot::Ability1) &&
				!loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability1) &&
				!loadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability3));

			// Real Return Protocol receiver and Rocket reflection ownership.
			check("returnProtocolEquipped", loadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			system.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			system.Tick(0.01f);
			auto registry = world.FindServiceActor<ProjectileReflectionRegistryActor>();
			check("reflectionRegistryRegistered", registry != nullptr);
			auto foreign = otherWorld.SpawnActor<RocketProjectileActor>(targets.front().get(), RocketPresentationProfile{}, std::nullopt).lock();
			check("crossWorldRejected", !ProjectileReflectionService::TryReflectProjectile(*foreign, *owner));
			auto rocket = world.SpawnActor<RocketProjectileActor>(targets.front().get(), RocketPresentationProfile{}, std::nullopt).lock();
			check("sameWorldReflects", ProjectileReflectionService::TryReflectProjectile(*rocket, *owner) && rocket->GetOwnerActor() == owner.get());
			check("sameRegistryReused", world.FindServiceActor<ProjectileReflectionRegistryActor>() == registry);
			check("emptyWorldLookup", !otherWorld.FindServiceActor<ProjectileReflectionRegistryActor>());

			// Exact endpoint geometry, using the beam's static sweep policy.
			auto wall = world.SpawnActor<EndpointWall>().lock();
			wall->SetActorLocation({ 115.f, 500.f });
			world.TickInternal(0.f);
			movement::StaticGeometrySweepHit hit;
			check("endpointWallHit", movement::FindFirstStaticGeometryHit(*owner, { 0.f, 500.f }, { 100.f, 500.f }, 5.f, hit, false) && hit.fraction == 1.f);

			// Rotate an already indexed body-less box across manual-grid cells.
			auto rotating = world.SpawnActor<RotatingWall>().lock();
			rotating->SetActorLocation({ 128.f, 4128.f });
			world.TickInternal(0.f);
			check("rotationControlOpen", !movement::FindFirstStaticGeometryHit(*owner, { 100.f, 4450.f }, { 155.f, 4450.f }, 0.f, hit, false));
			rotating->SetActorRotation(90.f);
			check("rotationRefreshesCells", movement::FindFirstStaticGeometryHit(*owner, { 100.f, 4450.f }, { 155.f, 4450.f }, 0.f, hit, false));
			rotating->SetActorRotation(0.f);
			check("rotationRemovesOldCells", !movement::FindFirstStaticGeometryHit(*owner, { 100.f, 4450.f }, { 155.f, 4450.f }, 0.f, hit, false));

			// Real shipped activation; Clear runs from the activation delegate.
			auto lifecycleOwner = spawn(3000.f, CollisionLayer::Player);
			world.TickInternal(0.f);
			auto& lifecycle = lifecycleOwner->GetAbilitySystemComponent();
			AbilityLoadoutManager lifecycleLoadout{ lifecycle };
			LifecycleFaultObserver faults{ lifecycle };
			faults.effect.effectId = "E2E.Lifecycle.Persistent";
			faults.effect.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			check("lifecycleEffectApplied", lifecycle.ApplyGameplayEffect(faults.effect).IsValid());
			check("lifecycleAbilityEquipped", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			auto activatedToken = lifecycle.onAbilityActivated.BindAction(&faults, &LifecycleFaultObserver::Activated);
			auto endedToken = lifecycle.onAbilityEnded.BindAction(&faults, &LifecycleFaultObserver::Ended);
			lifecycle.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			lifecycle.Tick(0.01f);
			check("tickClearDeferredUntilReturn", faults.visited && faults.liveDuringClear);
			check("liveInstanceMutationRejected", faults.mutationRejected);
			check("teardownRejectsReapplication", faults.additionsRejected);
			check("tickClearCleansWholeComponent", lifecycle.BuildAbilitySnapshots().empty() && lifecycle.BuildGameplayEffectSnapshots().empty() &&
				!lifecycle.GetAttributes().HasAttribute(OwnerAttributeIds::MaxHealth));
			lifecycle.onAbilityActivated.UnbindAction(activatedToken);
			lifecycle.onAbilityEnded.UnbindAction(endedToken);

			// Independent ended/effect-removal faults must not strand other state.
			check("reuseAfterClear", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			lifecycle.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			lifecycle.Tick(0.01f);
			check("activeBeforeFaultedClear", lifecycle.GetAbility(sas::AbilitySlot::Ability1)->IsActive());
			check("firstCleanupEffect", lifecycle.ApplyGameplayEffect(faults.effect).IsValid());
			auto secondEffect = faults.effect;
			secondEffect.effectId = "E2E.Lifecycle.Second";
			check("secondCleanupEffect", lifecycle.ApplyGameplayEffect(secondEffect).IsValid());
			lifecycle.GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, 100.f);
			lifecycle.PushPrimaryWeaponOverride(sas::ContentId{ "E2E.Override" }, PrimaryWeaponDefinition{});
			lifecycleOwner->GetCombatRuntime().SetDamageProtection("E2E.Protection", true, true);
			faults.throwOnEnd = true;
			endedToken = lifecycle.onAbilityEnded.BindAction(&faults, &LifecycleFaultObserver::Ended);
			auto removedToken = lifecycle.onGameplayEffectRemoved.BindAction(&faults, &LifecycleFaultObserver::EffectRemoved);
			std::string cleanupError;
			try { lifecycleOwner->GetCombatRuntime().Clear(); }
			catch (const std::runtime_error& error) { cleanupError = error.what(); }
			lifecycle.onAbilityEnded.UnbindAction(endedToken);
			lifecycle.onGameplayEffectRemoved.UnbindAction(removedToken);
			check("cleanupPreservesFirstException", cleanupError == "Injected ended failure");
			const auto faultedAbilitySnapshots = lifecycle.BuildAbilitySnapshots();
			const auto faultedEffectSnapshots = lifecycle.BuildGameplayEffectSnapshots();
			nlohmann::json faultedAbilityStates = nlohmann::json::array();
			for (const sas::AbilityRuntimeSnapshot& snapshot : faultedAbilitySnapshots)
			{
				const GameAbility* ability = lifecycle.GetAbility(snapshot.handle);
				faultedAbilityStates.push_back({ { "abilityId", snapshot.abilityId }, { "handle", snapshot.handle.id },
					{ "active", snapshot.active }, { "hasPendingCleanup", ability && ability->HasPendingCleanup() } });
			}
			result["diagnostics"]["faultedClearAfterFirstFailure"] = {
				{ "firstError", cleanupError },
				{ "activeAbilityCount", faultedAbilitySnapshots.size() },
				{ "abilities", faultedAbilityStates },
				{ "activeEffectCount", faultedEffectSnapshots.size() },
				{ "maxHealthAttributeRetained", lifecycle.GetAttributes().HasAttribute(OwnerAttributeIds::MaxHealth) },
				{ "maxHealthValue", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::MaxHealth) },
				{ "primaryWeaponOverrideRetained", lifecycle.GetActivePrimaryWeaponOverride() != nullptr },
				{ "incomingDamageBlocked", lifecycleOwner->GetCombatRuntime().BlocksIncomingDamage() },
				{ "outgoingDamageBlocked", lifecycleOwner->GetCombatRuntime().BlocksOutgoingDamage() }
			};
			check("faultedClearCleansWholeComponent", lifecycle.BuildAbilitySnapshots().empty() && lifecycle.BuildGameplayEffectSnapshots().empty() &&
				!lifecycle.GetAttributes().HasAttribute(OwnerAttributeIds::MaxHealth) && !lifecycle.GetActivePrimaryWeaponOverride());
			check("faultedClearCleansCombatOwner", !lifecycleOwner->GetCombatRuntime().BlocksIncomingDamage() && !lifecycleOwner->GetCombatRuntime().BlocksOutgoingDamage());
			check("reuseAfterFaultedClear", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			LoadoutObserver rebindClear{ lifecycle, true };
			auto rebindClearToken = lifecycle.onAbilityChanged.BindAction(&rebindClear, &LoadoutObserver::Granted);
			const bool rebindCleared = lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability2);
			lifecycle.onAbilityChanged.UnbindAction(rebindClearToken);
			check("rebindClearNoGhostSlot", !rebindCleared && lifecycle.BuildAbilitySnapshots().empty() &&
				!lifecycleLoadout.GetLoadout().FindAbility(sas::AbilitySlot::Ability2));
			check("reuseAfterRebindClear", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			lifecycle.GetAttributes().SetBaseValue(OwnerAttributeIds::MaxHealth, 100.f);
			faults.visited = false;
			activatedToken = lifecycle.onAbilityActivated.BindAction(&faults, &LifecycleFaultObserver::Activated);
			check("directActivationExecutes", lifecycle.GetAbility(sas::AbilitySlot::Ability1)->TryActivate());
			lifecycle.onAbilityActivated.UnbindAction(activatedToken);
			check("directActivationClearSafe", faults.visited && faults.liveDuringClear && lifecycle.BuildAbilitySnapshots().empty());
			check("prepareActivationFault", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			faults.throwAfterClear = true;
			activatedToken = lifecycle.onAbilityActivated.BindAction(&faults, &LifecycleFaultObserver::Activated);
			std::string activationError;
			try { lifecycle.GetAbility(sas::AbilitySlot::Ability1)->TryActivate(); }
			catch (const std::runtime_error& error) { activationError = error.what(); }
			lifecycle.onAbilityActivated.UnbindAction(activatedToken);
			check("clearCompletesDuringExceptionUnwind", activationError == "Injected activation failure after Clear" &&
				lifecycle.BuildAbilitySnapshots().empty() && lifecycle.BuildGameplayEffectSnapshots().empty());

			// Echo suppresses normal instance notifications, but must retain its
			// lifetime boundary when gameplay lifecycle observers request Clear.
			AbilityUseRecord echoRecord;
			echoRecord.abilityId = sas::ContentId{ AbilityData::ReturnProtocol::AbilityId::Basic };
			faults.visited = false;
			auto eventToken = lifecycle.onGameplayEvent.BindAction(&faults, &LifecycleFaultObserver::GameplayEvent);
			lifecycle.InvokeRecordedAbility(echoRecord, {}, 1.f, sas::AbilitySlot::Ability1, true);
			lifecycle.onGameplayEvent.UnbindAction(eventToken);
			check("echoClearCleansInvocation", faults.visited && lifecycle.GetActiveAbilityInvocationCount() == 0 && lifecycle.BuildAbilitySnapshots().empty());

			// Test preparation and committed publication, with/without Clear, plus Echo.
			auto countVisuals = [&]()
			{
				std::size_t count = 0;
				for (const auto& weakVisual : world.GetActorsByTypeIncludingPending<ReturnProtocolVisualActor>())
					if (const auto visual = weakVisual.lock(); visual && !visual->GetIsPendingDestroy()) ++count;
				return count;
			};
			for (const auto phase : { AbilityData::ReturnProtocol::Event::Started, GameplayTags::Event::Ability::Activated })
			{
				for (const bool clear : { false, true })
				{
					const auto baseline = countVisuals();
					if (!lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1))
						throw std::runtime_error("precommit setup grant");
					ActivationBoundaryObserver boundary{ lifecycle, phase, clear };
					boundary.cleanupFault = !clear;
					auto token = lifecycle.onGameplayEvent.BindAction(&boundary, &ActivationBoundaryObserver::Event);
					std::string error;
					try { lifecycle.GetAbility(sas::AbilitySlot::Ability1)->TryActivate(); }
					catch (const std::runtime_error& fault) { error = fault.what(); }
					lifecycle.onGameplayEvent.UnbindAction(token);
					auto probe = world.SpawnActor<RocketProjectileActor>(targets.front().get(), RocketPresentationProfile{}, std::nullopt).lock();
					const bool clean = boundary.visited && boundary.reentryRejected && error == "Injected precommit activation fault" &&
						countVisuals() == baseline && !lifecycle.HasOwnedTag(AbilityData::ReturnProtocol::State::Active) &&
						!ProjectileReflectionService::TryReflectProjectile(*probe, *lifecycleOwner);
					const bool committedPhase = phase == GameplayTags::Event::Ability::Activated;
					result["precommitCases"].push_back({ { "phase", committedPhase ? "committed" : "precommit" },
						{ "clear", clear }, { "cleanupFault", boundary.cleanupFault }, { "clean", clean },
						{ "stateMatchesPhase", boundary.committedStateVisible == committedPhase } });
					if (!clean) throw std::runtime_error("precommit resources leaked");
					if (boundary.committedStateVisible != committedPhase) throw std::runtime_error("activation published before commit");
					if (!clear)
					{
						auto* ability = lifecycle.GetAbility(sas::AbilitySlot::Ability1);
						if (phase == GameplayTags::Event::Ability::Activated)
						{
							if (!ability || !ability->IsOnCooldown()) throw std::runtime_error("committed failure must end with cooldown");
							ability->Tick(ability->GetCooldownRemaining() + 1.f);
						}
						if (!ability || ability->IsActive() || ability->GetCharges() != 1 || ability->IsOnCooldown() || !ability->TryActivate())
							throw std::runtime_error("precommit retry failed");
					}
					lifecycle.Clear();
				}
			}
			check("precommitFailureMatrix", result["precommitCases"].size() == 4);
			const auto visualBaseline = countVisuals();
			ActivationBoundaryObserver echoFault{ lifecycle, AbilityData::ReturnProtocol::Event::Started };
			auto echoFaultToken = lifecycle.onGameplayEvent.BindAction(&echoFault, &ActivationBoundaryObserver::Event);
			threw = false;
			try { lifecycle.InvokeRecordedAbility(echoRecord, {}, 1.f, sas::AbilitySlot::Ability1, true); }
			catch (const std::runtime_error&) { threw = true; }
			lifecycle.onGameplayEvent.UnbindAction(echoFaultToken);
			check("echoPrecommitFailureCleansResources", threw && echoFault.visited && countVisuals() == visualBaseline &&
				!lifecycle.HasOwnedTag(AbilityData::ReturnProtocol::State::Active) && lifecycle.GetActiveAbilityInvocationCount() == 0);
			check("preparePrecommitCancel", lifecycleLoadout.EquipAbility(AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability1));
			ActivationBoundaryObserver cancelStart{ lifecycle, AbilityData::ReturnProtocol::Event::Started, false, true };
			auto cancelToken = lifecycle.onGameplayEvent.BindAction(&cancelStart, &ActivationBoundaryObserver::Event);
			const bool cancelledStart = lifecycle.GetAbility(sas::AbilitySlot::Ability1)->TryActivate();
			lifecycle.onGameplayEvent.UnbindAction(cancelToken);
			check("precommitCancelNoChargeOrResources", !cancelledStart && cancelStart.visited && cancelStart.reentryRejected &&
				countVisuals() == visualBaseline && lifecycle.GetAbility(sas::AbilitySlot::Ability1)->GetCharges() == 1 &&
				!lifecycle.HasOwnedTag(AbilityData::ReturnProtocol::State::Active));
			lifecycle.Clear();

			// A retained view always reads the canonical runtime, even across nested Clear.
			const auto& liveView = lifecycleLoadout.GetLoadout();
			NestedLoadoutObserver nested{ lifecycleLoadout };
			LoadoutObserver nestedClear{ lifecycle, true };
			auto nestedEventToken = lifecycle.onGameplayEvent.BindAction(&nested, &NestedLoadoutObserver::Event);
			auto nestedClearToken = lifecycle.onAbilityGranted.BindAction(&nestedClear, &LoadoutObserver::Granted);
			sas::AbilityEvent nestedEvent;
			nestedEvent.eventTag = GameplayTags::Event::Combat::DamageReceived;
			lifecycle.HandleGameplayEvent(nestedEvent);
			lifecycle.onAbilityGranted.UnbindAction(nestedClearToken);
			check("nestedGrantClearNoGhost", !nested.equipped && lifecycle.BuildAbilitySnapshots().empty() &&
				!liveView.FindAbility(sas::AbilitySlot::Ability2));
			check("liveViewSeesDirectGrant", lifecycle.GrantAbility(*AbilityData::FindShippedAbilityDefinition(AbilityData::ReturnProtocol::AbilityId::Basic),
				sas::AbilitySlot::Ability1).IsValid() && liveView.FindAbility(sas::AbilitySlot::Ability1));
			nestedClearToken = lifecycle.onAbilityChanged.BindAction(&nestedClear, &LoadoutObserver::Granted);
			lifecycle.HandleGameplayEvent(nestedEvent);
			lifecycle.onAbilityChanged.UnbindAction(nestedClearToken);
			lifecycle.onGameplayEvent.UnbindAction(nestedEventToken);
			check("nestedRebindClearNoGhost", !nested.equipped && lifecycle.BuildAbilitySnapshots().empty() &&
				liveView.FindSlot(AbilityData::ReturnProtocol::AbilityId::Basic) == sas::AbilitySlot::None);

			result["effectCases"] = nlohmann::json::array();
			// Attribute callbacks reenter the production component and attribute
			// owner; handles are committed before notification and erased before it.
			auto& effectAttributes = lifecycle.GetAttributes();
			effectAttributes.RegisterAttribute(OwnerAttributeIds::CriticalChance, 1.f);
			const auto reentrantModifier = effectAttributes.AddModifier(
				sas::AttributeModifier{ OwnerAttributeIds::CriticalChance, 1.f }
			);
			const float reentrantBefore = effectAttributes.GetCurrentValue(OwnerAttributeIds::CriticalChance);
			AttributeModifierReentry reentryRemoval{ effectAttributes, reentrantModifier };
			const auto reentryToken = effectAttributes.onAttributeChanged.BindAction(
				&reentryRemoval, &AttributeModifierReentry::Changed
			);
			reentryRemoval.armed = true;
			reentryRemoval.order.push_back("outer-remove-begin");
			effectAttributes.RemoveModifier(reentrantModifier);
			reentryRemoval.order.push_back("outer-remove-return");
			effectAttributes.onAttributeChanged.UnbindAction(reentryToken);
			const bool reentrantRemoveClean = reentrantModifier.IsValid() && reentryRemoval.visited &&
				reentryRemoval.callbackCount == 1 && effectAttributes.GetCurrentValue(OwnerAttributeIds::CriticalChance) == 1.f;
			recordCase("effectCases", { { "case_id", "E01" },
				{ "input", { { "callback", "attribute change observer removes the same handle" }, { "handle", reentrantModifier.id } } },
				{ "expected", "one removal and one change notification with a consistent current value and owner index" },
				{ "actual", { { "callbackVisited", reentryRemoval.visited }, { "callbackCount", reentryRemoval.callbackCount },
					{ "currentValue", effectAttributes.GetCurrentValue(OwnerAttributeIds::CriticalChance) } } },
				{ "order", reentryRemoval.order },
				{ "identities", { { "attributeId", std::string{ OwnerAttributeIds::CriticalChance.GetName() } }, { "modifierHandle", reentrantModifier.id } } },
				{ "before", { { "currentValue", reentrantBefore }, { "modifierHandleValid", reentrantModifier.IsValid() } } },
				{ "after", { { "currentValue", effectAttributes.GetCurrentValue(OwnerAttributeIds::CriticalChance) },
					{ "modifierHandleValid", reentrantModifier.IsValid() } } },
				{ "errors", nlohmann::json::array() }, { "passed", reentrantRemoveClean } });
			check("attributeRemovalReentryIsIdempotent", reentrantRemoveClean);

			const sas::AttributeId clearRegistrationId{ "E2E.Attribute.RegistrationClear" };
			sas::GameplayEffectDefinition clearDuringRegistration;
			clearDuringRegistration.effectId = "E2E.Effect.AttributeRegistrationClear";
			clearDuringRegistration.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			clearDuringRegistration.modifiers.emplace_back(clearRegistrationId, 2.f);
			const bool registrationAttributePresentBefore = effectAttributes.HasAttribute(clearRegistrationId);
			const float registrationAttributeValueBefore = effectAttributes.GetCurrentValue(clearRegistrationId);
			const std::size_t activeEffectsBeforeRegistrationClear = lifecycle.BuildGameplayEffectSnapshots().size();
			AttributeRegisteredClearFault registeredClear{ effectAttributes };
			const auto clearRegisteredToken = effectAttributes.onAttributeRegistered.BindAction(
				&registeredClear, &AttributeRegisteredClearFault::Registered
			);
			const auto clearedRegistrationHandle = lifecycle.ApplyGameplayEffect(clearDuringRegistration);
			effectAttributes.onAttributeRegistered.UnbindAction(clearRegisteredToken);
			const bool registrationClearSafe = registeredClear.visited && clearedRegistrationHandle.IsValid() &&
				!effectAttributes.HasAttribute(clearRegistrationId) &&
				effectAttributes.GetCurrentValue(clearRegistrationId) == 0.f;
			lifecycle.RemoveGameplayEffect(clearedRegistrationHandle);
			recordCase("effectCases", { { "case_id", "E02-registration-clear" },
				{ "input", "implicit registration observer clears AttributeSystem" },
				{ "expected", "in-flight modifier invalidated without stale access" },
				{ "actual", { { "callbackVisited", registeredClear.visited },
					{ "attributePresent", effectAttributes.HasAttribute(clearRegistrationId) },
					{ "effectRemoved", lifecycle.BuildGameplayEffectSnapshots().empty() } } },
				{ "identities", { { "attributeId", std::string{ clearRegistrationId.GetName() } }, { "effectId", clearDuringRegistration.effectId } } },
				{ "before", { { "attributePresent", registrationAttributePresentBefore },
					{ "currentValue", registrationAttributeValueBefore }, { "activeEffects", activeEffectsBeforeRegistrationClear } } },
				{ "after", { { "attributePresent", effectAttributes.HasAttribute(clearRegistrationId) },
					{ "currentValue", effectAttributes.GetCurrentValue(clearRegistrationId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "errors", nlohmann::json::array() }, { "passed", registrationClearSafe && lifecycle.BuildGameplayEffectSnapshots().empty() } });
			check("implicitRegistrationClearInvalidatesAddSafely", registrationClearSafe && lifecycle.BuildGameplayEffectSnapshots().empty());

			const sas::AttributeId throwRegistrationId{ "E2E.Attribute.RegistrationThrow" };
			sas::GameplayEffectDefinition throwDuringRegistration = clearDuringRegistration;
			throwDuringRegistration.effectId = "E2E.Effect.AttributeRegistrationThrow";
			throwDuringRegistration.modifiers.clear();
			throwDuringRegistration.modifiers.emplace_back(throwRegistrationId, 3.f);
			const bool throwRegistrationAttributePresentBefore = effectAttributes.HasAttribute(throwRegistrationId);
			const float throwRegistrationAttributeValueBefore = effectAttributes.GetCurrentValue(throwRegistrationId);
			const std::size_t activeEffectsBeforeRegistrationThrow = lifecycle.BuildGameplayEffectSnapshots().size();
			AttributeRegisteredThrowFault registeredThrow{ "Injected E02 registration failure" };
			const auto throwRegisteredToken = effectAttributes.onAttributeRegistered.BindAction(
				&registeredThrow, &AttributeRegisteredThrowFault::Registered
			);
			std::string registrationError;
			try { lifecycle.ApplyGameplayEffect(throwDuringRegistration); }
			catch (const std::runtime_error& error) { registrationError = error.what(); }
			effectAttributes.onAttributeRegistered.UnbindAction(throwRegisteredToken);
			const bool registrationPersistedAtBase = effectAttributes.HasAttribute(throwRegistrationId) &&
				effectAttributes.GetCurrentValue(throwRegistrationId) == 0.f;
			const bool registrationThrowClean = registeredThrow.visited && registeredThrow.registeredId == throwRegistrationId &&
				registrationError == registeredThrow.message &&
				registrationPersistedAtBase && lifecycle.BuildGameplayEffectSnapshots().empty();
			recordCase("effectCases", { { "case_id", "E02-registration-throw" },
				{ "input", { { "callback", "implicit registration observer throws" }, { "modifierMagnitude", 3.f } } },
				{ "expected", "first error preserved; registered attribute remains at base 0; failed effect and modifier ownership are removed" },
				{ "actual", { { "error", registrationError },
					{ "attributeRegistered", effectAttributes.HasAttribute(throwRegistrationId) },
					{ "currentValue", effectAttributes.GetCurrentValue(throwRegistrationId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "order", registeredThrow.order },
				{ "identities", { { "attributeId", std::string{ throwRegistrationId.GetName() } }, { "effectId", throwDuringRegistration.effectId } } },
				{ "before", { { "attributePresent", throwRegistrationAttributePresentBefore },
					{ "currentValue", throwRegistrationAttributeValueBefore }, { "activeEffects", activeEffectsBeforeRegistrationThrow } } },
				{ "after", { { "attributePresent", effectAttributes.HasAttribute(throwRegistrationId) },
					{ "currentValue", effectAttributes.GetCurrentValue(throwRegistrationId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "errors", { registrationError } }, { "passed", registrationThrowClean } });
			check("implicitRegistrationThrowCleansCommittedHandle", registrationThrowClean);

			const sas::AttributeId clearChangedId{ "E2E.Attribute.ChangeClear" };
			effectAttributes.RegisterAttribute(clearChangedId, 0.f);
			sas::GameplayEffectDefinition clearDuringChange;
			clearDuringChange.effectId = "E2E.Effect.AttributeChangeClear";
			clearDuringChange.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			clearDuringChange.modifiers.emplace_back(clearChangedId, 4.f);
			const bool changedClearAttributePresentBefore = effectAttributes.HasAttribute(clearChangedId);
			const float changedClearAttributeValueBefore = effectAttributes.GetCurrentValue(clearChangedId);
			const std::size_t activeEffectsBeforeChangeClear = lifecycle.BuildGameplayEffectSnapshots().size();
			AttributeChangedClearFault changedClear{ effectAttributes, clearChangedId, true };
			const auto changedClearToken = effectAttributes.onAttributeChanged.BindAction(
				&changedClear, &AttributeChangedClearFault::Changed
			);
			const auto clearedChangeHandle = lifecycle.ApplyGameplayEffect(clearDuringChange);
			effectAttributes.onAttributeChanged.UnbindAction(changedClearToken);
			const bool changedClearSafe = changedClear.visited && clearedChangeHandle.IsValid() &&
				!effectAttributes.HasAttribute(clearChangedId) &&
				effectAttributes.GetCurrentValue(clearChangedId) == 0.f;
			lifecycle.RemoveGameplayEffect(clearedChangeHandle);
			recordCase("effectCases", { { "case_id", "E02-change-clear" },
				{ "input", "attribute changed callback clears AttributeSystem after modifier commit" },
				{ "expected", "no stale modifier access and effect cleanup succeeds" },
				{ "actual", { { "callbackVisited", changedClear.visited },
					{ "attributePresent", effectAttributes.HasAttribute(clearChangedId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "identities", { { "attributeId", std::string{ clearChangedId.GetName() } }, { "effectId", clearDuringChange.effectId } } },
				{ "before", { { "attributePresent", changedClearAttributePresentBefore },
					{ "currentValue", changedClearAttributeValueBefore }, { "activeEffects", activeEffectsBeforeChangeClear } } },
				{ "after", { { "attributePresent", effectAttributes.HasAttribute(clearChangedId) },
					{ "currentValue", effectAttributes.GetCurrentValue(clearChangedId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "errors", nlohmann::json::array() },
				{ "passed", changedClearSafe && lifecycle.BuildGameplayEffectSnapshots().empty() } });
			check("attributeChangeClearLeavesNoStaleModifier", changedClearSafe && lifecycle.BuildGameplayEffectSnapshots().empty());

			const sas::AttributeId throwChangedId{ "E2E.Attribute.ChangeThrow" };
			effectAttributes.RegisterAttribute(throwChangedId, 0.f);
			sas::GameplayEffectDefinition throwDuringChange;
			throwDuringChange.effectId = "E2E.Effect.AttributeChangeThrow";
			throwDuringChange.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			throwDuringChange.modifiers.emplace_back(throwChangedId, 5.f);
			const bool changedThrowAttributePresentBefore = effectAttributes.HasAttribute(throwChangedId);
			const float changedThrowAttributeValueBefore = effectAttributes.GetCurrentValue(throwChangedId);
			const std::size_t activeEffectsBeforeChangeThrow = lifecycle.BuildGameplayEffectSnapshots().size();
			AttributeChangedClearFault changedThrow{ effectAttributes, throwChangedId, false, false,
				"Injected E02 attribute change failure" };
			const auto changedThrowToken = effectAttributes.onAttributeChanged.BindAction(
				&changedThrow, &AttributeChangedClearFault::Changed
			);
			std::string attributeChangeError;
			try { lifecycle.ApplyGameplayEffect(throwDuringChange); }
			catch (const std::runtime_error& error) { attributeChangeError = error.what(); }
			effectAttributes.onAttributeChanged.UnbindAction(changedThrowToken);
			const bool changedThrowClean = changedThrow.visited &&
				attributeChangeError == changedThrow.message &&
				lifecycle.BuildGameplayEffectSnapshots().empty() &&
				effectAttributes.GetCurrentValue(throwChangedId) == 0.f;
			recordCase("effectCases", { { "case_id", "E02-change-throw" },
				{ "input", "attribute changed callback throws after modifier commit" },
				{ "expected", "effect cleanup knows committed handle and rethrows original failure" },
				{ "actual", { { "error", attributeChangeError }, { "currentValue", effectAttributes.GetCurrentValue(throwChangedId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "identities", { { "attributeId", std::string{ throwChangedId.GetName() } }, { "effectId", throwDuringChange.effectId } } },
				{ "before", { { "attributePresent", changedThrowAttributePresentBefore },
					{ "currentValue", changedThrowAttributeValueBefore }, { "activeEffects", activeEffectsBeforeChangeThrow } } },
				{ "after", { { "attributePresent", effectAttributes.HasAttribute(throwChangedId) },
					{ "currentValue", effectAttributes.GetCurrentValue(throwChangedId) },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "errors", { attributeChangeError } }, { "passed", changedThrowClean } });
			check("attributeChangeThrowCleansCommittedHandle", changedThrowClean);

			// Effect callbacks exercise the production runtime against this real
			// SpaceShip component. Each case records its observed ownership state.
			const GameplayTag selfRemoveTag{ "E2E.Effect.SelfRemove" };
			sas::GameplayEffectDefinition selfRemoving;
			selfRemoving.effectId = "E2E.Effect.SelfRemoving";
			selfRemoving.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			selfRemoving.grantedTags.push_back(selfRemoveTag);
			selfRemoving.modifiers.emplace_back(OwnerAttributeIds::CriticalDamage, 0.5f);
			const std::size_t activeEffectsBeforeInitializeSelfRemove = lifecycle.BuildGameplayEffectSnapshots().size();
			const bool selfRemoveTagBeforeInitialize = lifecycle.HasOwnedTag(selfRemoveTag, true);
			auto selfRemoveCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			selfRemoveCallbacks.initialize = [&lifecycle](sas::ActiveGameplayEffect& effect)
			{
				lifecycle.RemoveGameplayEffect(effect.handle);
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(selfRemoveCallbacks));
			const float criticalDamageBeforeSelfRemove = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage);
			const auto selfRemovedHandle = lifecycle.ApplyGameplayEffect(selfRemoving);
			const bool selfRemoveClean = !selfRemovedHandle.IsValid() &&
				lifecycle.BuildGameplayEffectSnapshots().empty() &&
				!lifecycle.HasOwnedTag(selfRemoveTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage) == criticalDamageBeforeSelfRemove;
			recordCase("effectCases", { { "case_id", "E03-initialize-remove" },
				{ "input", { { "callback", "initialize removes its own effect before resources are acquired" },
					{ "effectId", selfRemoving.effectId } } },
				{ "expected", "invalid result with no owned resources or active record" }, { "actual", {
					{ "validHandle", selfRemovedHandle.IsValid() },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "tagOwned", lifecycle.HasOwnedTag(selfRemoveTag, true) },
					{ "criticalDamage", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage) } } },
				{ "identities", { { "effectId", selfRemoving.effectId }, { "ownerActorId", lifecycleOwner->GetUniqueID() } } },
				{ "before", { { "activeEffects", activeEffectsBeforeInitializeSelfRemove },
					{ "criticalDamage", criticalDamageBeforeSelfRemove }, { "tagOwned", selfRemoveTagBeforeInitialize } } },
				{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "criticalDamage", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage) },
					{ "tagOwned", lifecycle.HasOwnedTag(selfRemoveTag, true) } } },
				{ "errors", nlohmann::json::array() }, { "passed", selfRemoveClean } });
			check("effectInitializeSelfRemoveCleansAcquiredResources", selfRemoveClean);

			const GameplayTag activatedSelfRemoveTag{ "E2E.Effect.ActivatedSelfRemove" };
			sas::GameplayEffectDefinition activatedSelfRemoving = selfRemoving;
			activatedSelfRemoving.effectId = "E2E.Effect.ActivatedSelfRemoving";
			activatedSelfRemoving.grantedTags = { activatedSelfRemoveTag };
			std::vector<std::string> activatedRemoveOrder;
			sas::GameplayEffectHandle activatedSelfRemoveIdentity;
			bool resourcesPresentAtActivation = false;
			auto activatedSelfRemoveCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			activatedSelfRemoveCallbacks.initialize = [&activatedRemoveOrder](sas::ActiveGameplayEffect&)
			{
				activatedRemoveOrder.push_back("initialize");
			};
			activatedSelfRemoveCallbacks.activated = [&](sas::ActiveGameplayEffect& effect)
			{
				activatedRemoveOrder.push_back("activated");
				activatedSelfRemoveIdentity = effect.handle;
				resourcesPresentAtActivation = lifecycle.HasOwnedTag(activatedSelfRemoveTag, true) &&
					lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage) > criticalDamageBeforeSelfRemove + 0.49f;
				activatedRemoveOrder.push_back("remove-request");
				lifecycle.RemoveGameplayEffect(effect.handle);
				activatedRemoveOrder.push_back("remove-return");
			};
			activatedSelfRemoveCallbacks.removing = [&activatedRemoveOrder](sas::ActiveGameplayEffect&)
			{
				activatedRemoveOrder.push_back("removing");
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(activatedSelfRemoveCallbacks));
			const std::size_t activeEffectsBeforeActivatedSelfRemove = lifecycle.BuildGameplayEffectSnapshots().size();
			const bool activatedSelfRemoveTagBefore = lifecycle.HasOwnedTag(activatedSelfRemoveTag, true);
			const auto activatedSelfRemovedHandle = lifecycle.ApplyGameplayEffect(activatedSelfRemoving);
			const float criticalDamageAfterActivatedSelfRemove =
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage);
			const bool activatedSelfRemoveClean = !activatedSelfRemovedHandle.IsValid() &&
				activatedSelfRemoveIdentity.IsValid() && resourcesPresentAtActivation &&
				lifecycle.BuildGameplayEffectSnapshots().empty() &&
				!lifecycle.HasOwnedTag(activatedSelfRemoveTag, true) &&
				criticalDamageAfterActivatedSelfRemove == criticalDamageBeforeSelfRemove &&
				activatedRemoveOrder.size() >= 5 && activatedRemoveOrder[0] == "initialize" &&
				activatedRemoveOrder[1] == "activated" && activatedRemoveOrder[2] == "remove-request" &&
				activatedRemoveOrder[3] == "remove-return" && activatedRemoveOrder[4] == "removing";
			recordCase("effectCases", { { "case_id", "E03" },
				{ "input", { { "callback", "activated removes its effect after modifiers and tags are acquired" },
					{ "effectId", activatedSelfRemoving.effectId } } },
				{ "expected", "activation sees acquired resources; apply returns invalid; cleanup removes tag, modifier, and active record" },
				{ "actual", { { "validHandle", activatedSelfRemovedHandle.IsValid() },
					{ "resourcesPresentAtActivation", resourcesPresentAtActivation },
					{ "tagOwnedAfter", lifecycle.HasOwnedTag(activatedSelfRemoveTag, true) },
					{ "criticalDamageAfter", criticalDamageAfterActivatedSelfRemove },
					{ "activeEffectsAfter", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "order", activatedRemoveOrder },
				{ "identities", { { "effectId", activatedSelfRemoving.effectId },
					{ "effectHandle", activatedSelfRemoveIdentity.id }, { "ownerActorId", lifecycleOwner->GetUniqueID() } } },
				{ "before", { { "activeEffects", activeEffectsBeforeActivatedSelfRemove },
					{ "criticalDamage", criticalDamageBeforeSelfRemove }, { "tagOwned", activatedSelfRemoveTagBefore } } },
				{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "criticalDamage", criticalDamageAfterActivatedSelfRemove },
					{ "tagOwned", lifecycle.HasOwnedTag(activatedSelfRemoveTag, true) } } },
				{ "errors", nlohmann::json::array() }, { "passed", activatedSelfRemoveClean } });
			check("effectActivatedSelfRemoveCleansAcquiredResources", activatedSelfRemoveClean);
			lifecycle.SetEffectRuntimeCallbacks({});

			const GameplayTag refreshTag{ "E2E.Effect.RefreshRemove" };
			sas::GameplayEffectDefinition refreshRemoving;
			refreshRemoving.effectId = "E2E.Effect.RefreshRemoving";
			refreshRemoving.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
			refreshRemoving.duration = 10.f;
			refreshRemoving.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
			refreshRemoving.grantedTags.push_back(refreshTag);
			refreshRemoving.modifiers.emplace_back(OwnerAttributeIds::CriticalChance, 0.2f);
			std::vector<std::string> refreshOrder;
			auto refreshCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			refreshCallbacks.changed = [&](sas::ActiveGameplayEffect& effect)
			{
				refreshOrder.push_back("changed");
				refreshOrder.push_back("remove-request");
				lifecycle.RemoveGameplayEffect(effect.handle);
				refreshOrder.push_back("remove-return");
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(refreshCallbacks));
			const float criticalChanceBeforeRefresh = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalChance);
			const auto refreshHandle = lifecycle.ApplyGameplayEffect(refreshRemoving);
			const float criticalChanceBeforeRefreshCall = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalChance);
			const std::size_t activeEffectsBeforeRefreshCall = lifecycle.BuildGameplayEffectSnapshots().size();
			const bool refreshTagOwnedBeforeCall = lifecycle.HasOwnedTag(refreshTag, true);
			const bool refreshResult = refreshHandle.IsValid() && lifecycle.RefreshGameplayEffectDuration(refreshHandle);
			const bool refreshRemoved = refreshHandle.IsValid() && !refreshResult &&
				lifecycle.BuildGameplayEffectSnapshots().empty() && !lifecycle.HasOwnedTag(refreshTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalChance) == criticalChanceBeforeRefresh;
			recordCase("effectCases", { { "case_id", "E04-refresh" },
				{ "input", { { "operation", "duration refresh" }, { "callback", "changed removes active effect" } } },
				{ "expected", "false refresh result and no active resources" },
				{ "actual", { { "refreshResult", refreshResult },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() }, { "tagOwned", lifecycle.HasOwnedTag(refreshTag, true) },
					{ "criticalChance", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalChance) } } },
				{ "order", refreshOrder },
				{ "identities", { { "effectId", refreshRemoving.effectId }, { "effectHandle", refreshHandle.id } } },
				{ "before", { { "activeEffects", activeEffectsBeforeRefreshCall },
					{ "criticalChance", criticalChanceBeforeRefreshCall }, { "tagOwned", refreshTagOwnedBeforeCall } } },
				{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "criticalChance", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalChance) },
					{ "tagOwned", lifecycle.HasOwnedTag(refreshTag, true) } } },
				{ "errors", nlohmann::json::array() }, { "passed", refreshRemoved } });
			check("effectRefreshSelfRemoveCleansAcquiredResources", refreshRemoved);

			const GameplayTag stackTag{ "E2E.Effect.StackClear" };
			sas::GameplayEffectDefinition stackClearing;
			stackClearing.effectId = "E2E.Effect.StackClearing";
			stackClearing.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			stackClearing.stackingPolicy = sas::GameplayEffectStackingPolicy::Stack;
			stackClearing.maxStacks = 2;
			stackClearing.grantedTags.push_back(stackTag);
			stackClearing.modifiers.emplace_back(OwnerAttributeIds::Luck, 1.f);
			std::vector<std::string> stackClearOrder;
			auto stackCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			stackCallbacks.addStack = [&](sas::ActiveGameplayEffect&) -> bool
			{
				stackClearOrder.push_back("add-stack");
				stackClearOrder.push_back("clear-request");
				lifecycle.Clear();
				stackClearOrder.push_back("clear-return");
				return true;
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(stackCallbacks));
			const auto stackHandle = lifecycle.ApplyGameplayEffect(stackClearing);
			const std::size_t activeEffectsBeforeStackReapply = lifecycle.BuildGameplayEffectSnapshots().size();
			const float luckBeforeStackReapply = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::Luck);
			const bool tagOwnedBeforeStackReapply = lifecycle.HasOwnedTag(stackTag, true);
			const auto stackReapply = lifecycle.ApplyGameplayEffect(stackClearing);
			const bool stackClearClean = stackHandle.IsValid() && !stackReapply.IsValid() &&
				lifecycle.BuildGameplayEffectSnapshots().empty() && !lifecycle.HasOwnedTag(stackTag, true) &&
				!lifecycle.GetAttributes().HasAttribute(OwnerAttributeIds::Luck);
			recordCase("effectCases", { { "case_id", "E04-stack-clear" },
				{ "input", { { "operation", "second stack application" }, { "maxStacks", 2 }, { "callback", "addStack requests component Clear" } } },
				{ "expected", "invalid reapply and fully cleared component" },
				{ "actual", { { "reapplyValid", stackReapply.IsValid() },
					{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() }, { "tagOwned", lifecycle.HasOwnedTag(stackTag, true) } } },
				{ "order", stackClearOrder },
				{ "identities", { { "effectId", stackClearing.effectId }, { "effectHandle", stackHandle.id } } },
				{ "before", { { "activeEffects", activeEffectsBeforeStackReapply },
					{ "luck", luckBeforeStackReapply }, { "tagOwned", tagOwnedBeforeStackReapply } } },
				{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "tagOwned", lifecycle.HasOwnedTag(stackTag, true) } } },
				{ "errors", nlohmann::json::array() }, { "passed", stackClearClean } });
			check("effectStackClearStopsFurtherCallbacks", stackClearClean);

			const GameplayTag cappedTag{ "E2E.Effect.CappedReapplyRemove" };
			sas::GameplayEffectDefinition cappedReapplyRemoving;
			cappedReapplyRemoving.effectId = "E2E.Effect.CappedReapplyRemoving";
			cappedReapplyRemoving.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
			cappedReapplyRemoving.duration = 6.f;
			cappedReapplyRemoving.stackingPolicy = sas::GameplayEffectStackingPolicy::Stack;
			cappedReapplyRemoving.maxStacks = 1;
			cappedReapplyRemoving.grantedTags.push_back(cappedTag);
			cappedReapplyRemoving.modifiers.emplace_back(OwnerAttributeIds::Luck, 1.f);
			std::vector<std::string> cappedReapplyOrder;
			sas::GameplayEffectHandle cappedReapplyIdentity;
			bool cappedResourcesPresent = false;
			const std::size_t activeEffectsBeforeCappedApply = lifecycle.BuildGameplayEffectSnapshots().size();
			const float luckBeforeCappedReapply = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::Luck);
			const bool cappedTagOwnedBeforeApply = lifecycle.HasOwnedTag(cappedTag, true);
			auto cappedReapplyCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			cappedReapplyCallbacks.cappedReapply = [&](sas::ActiveGameplayEffect& effect, const sas::GameplayEffectSpec&) -> bool
			{
				cappedReapplyOrder.push_back("capped-reapply");
				cappedReapplyIdentity = effect.handle;
				cappedResourcesPresent = lifecycle.HasOwnedTag(cappedTag, true) &&
					lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::Luck) == luckBeforeCappedReapply + 1.f;
				cappedReapplyOrder.push_back("remove-request");
				lifecycle.RemoveGameplayEffect(effect.handle);
				cappedReapplyOrder.push_back("remove-return");
				return true;
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(cappedReapplyCallbacks));
			const auto cappedInitialHandle = lifecycle.ApplyGameplayEffect(cappedReapplyRemoving);
			const std::size_t activeEffectsBeforeCappedReapply = lifecycle.BuildGameplayEffectSnapshots().size();
			const auto cappedReapplyHandle = lifecycle.ApplyGameplayEffect(cappedReapplyRemoving);
			const float luckAfterCappedReapply = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::Luck);
			const bool cappedReapplySelfRemoveClean = cappedInitialHandle.IsValid() &&
				activeEffectsBeforeCappedReapply == 1 && !cappedReapplyHandle.IsValid() &&
				cappedReapplyIdentity == cappedInitialHandle && cappedResourcesPresent &&
				lifecycle.BuildGameplayEffectSnapshots().empty() && !lifecycle.HasOwnedTag(cappedTag, true) &&
				luckAfterCappedReapply == luckBeforeCappedReapply &&
				cappedReapplyOrder == std::vector<std::string>{ "capped-reapply", "remove-request", "remove-return" };
			recordCase("effectCases", { { "case_id", "E04-capped-reapply-remove" },
				{ "input", { { "stackingPolicy", "Stack" }, { "maxStacks", 1 }, { "reapplyCount", 2 },
					{ "callback", "cappedReapply removes the active effect" } } },
				{ "expected", "capped callback sees existing tag/modifier; reapply is invalid and all resources are removed" },
				{ "actual", { { "initialHandleValid", cappedInitialHandle.IsValid() },
					{ "reapplyHandleValid", cappedReapplyHandle.IsValid() }, { "resourcesPresentAtCappedCallback", cappedResourcesPresent },
					{ "activeEffectsAfter", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "tagOwnedAfter", lifecycle.HasOwnedTag(cappedTag, true) }, { "luckAfter", luckAfterCappedReapply } } },
				{ "order", cappedReapplyOrder },
				{ "identities", { { "effectId", cappedReapplyRemoving.effectId }, { "effectHandle", cappedInitialHandle.id } } },
				{ "before", { { "activeEffects", activeEffectsBeforeCappedApply },
					{ "luck", luckBeforeCappedReapply }, { "tagOwned", cappedTagOwnedBeforeApply } } },
				{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "luck", luckAfterCappedReapply }, { "tagOwned", lifecycle.HasOwnedTag(cappedTag, true) } } },
				{ "errors", nlohmann::json::array() }, { "passed", cappedReapplySelfRemoveClean } });
			check("effectCappedReapplySelfRemoveCleansAcquiredResources", cappedReapplySelfRemoveClean);
			lifecycle.SetEffectRuntimeCallbacks({});

			// A callback failure on each cleanup position must not strand later
			// modifiers, tag ownership, or the effect record. Handles unwind LIFO.
			lifecycle.SetEffectRuntimeCallbacks({});
			const std::vector<sas::AttributeId> cleanupAttributes{
				OwnerAttributeIds::CriticalChance, OwnerAttributeIds::CriticalDamage, OwnerAttributeIds::Luck
			};
			for (std::size_t stage = 0; stage < cleanupAttributes.size(); ++stage)
			{
				sas::GameplayEffectDefinition cleanupEffect;
				cleanupEffect.effectId = "E2E.Effect.Cleanup." + std::to_string(stage);
				cleanupEffect.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
				const GameplayTag cleanupTag{ "E2E.Effect.Cleanup." + std::to_string(stage) };
				cleanupEffect.grantedTags.push_back(cleanupTag);
				for (const sas::AttributeId& id : cleanupAttributes) cleanupEffect.modifiers.emplace_back(id, 1.f);
				std::vector<float> before;
				for (const sas::AttributeId& id : cleanupAttributes) before.push_back(lifecycle.GetAttributes().GetCurrentValue(id));
				const auto cleanupHandle = lifecycle.ApplyGameplayEffect(cleanupEffect);
				std::vector<float> valuesBeforeCleanup;
				for (const sas::AttributeId& id : cleanupAttributes) valuesBeforeCleanup.push_back(lifecycle.GetAttributes().GetCurrentValue(id));
				const std::size_t activeEffectsBeforeCleanup = lifecycle.BuildGameplayEffectSnapshots().size();
				const bool cleanupTagOwnedBefore = lifecycle.HasOwnedTag(cleanupTag, true);
				std::vector<std::string> cleanupOrder;
				AttributeCleanupFault fault{
					cleanupAttributes[cleanupAttributes.size() - 1 - stage],
					"Injected E05 cleanup stage " + std::to_string(stage), true, false
				};
				fault.order = &cleanupOrder;
				const auto token = lifecycle.GetAttributes().onAttributeChanged.BindAction(&fault, &AttributeCleanupFault::Changed);
				std::string cleanupFault;
				try { lifecycle.RemoveGameplayEffect(cleanupHandle); }
				catch (const std::runtime_error& error) { cleanupFault = error.what(); }
				lifecycle.GetAttributes().onAttributeChanged.UnbindAction(token);
				bool valuesRestored = true;
				for (std::size_t i = 0; i < cleanupAttributes.size(); ++i)
				{
					valuesRestored = valuesRestored &&
						std::abs(lifecycle.GetAttributes().GetCurrentValue(cleanupAttributes[i]) - before[i]) < 0.0001f;
				}
				const bool cleanupComplete = cleanupHandle.IsValid() && fault.triggered &&
					cleanupFault == fault.message && valuesRestored && !lifecycle.HasOwnedTag(cleanupTag, true) &&
					lifecycle.BuildGameplayEffectSnapshots().empty();
				recordCase("effectCases", { { "case_id", "E05-" + std::to_string(stage) },
					{ "input", { { "throwPosition", stage == 0 ? "first" : stage == 1 ? "middle" : "last" },
						{ "modifierCount", cleanupAttributes.size() }, { "operation", "single effect Remove" } } },
					{ "expected", "all owned resources cleared; original callback error rethrown" },
					{ "actual", { { "error", cleanupFault }, { "valuesRestored", valuesRestored },
						{ "tagOwned", lifecycle.HasOwnedTag(cleanupTag, true) },
						{ "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() } } },
					{ "order", cleanupOrder },
					{ "identities", { { "effectId", cleanupEffect.effectId }, { "effectHandle", cleanupHandle.id },
						{ "throwAttributeId", std::string{ fault.target.GetName() } } } },
					{ "before", { { "activeEffects", activeEffectsBeforeCleanup },
						{ "attributes", valuesBeforeCleanup }, { "tagOwned", cleanupTagOwnedBefore } } },
					{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
						{ "attributes", { lifecycle.GetAttributes().GetCurrentValue(cleanupAttributes[0]),
							lifecycle.GetAttributes().GetCurrentValue(cleanupAttributes[1]),
							lifecycle.GetAttributes().GetCurrentValue(cleanupAttributes[2]) } },
						{ "tagOwned", lifecycle.HasOwnedTag(cleanupTag, true) } } },
					{ "errors", { cleanupFault } }, { "passed", cleanupComplete } });
				check(stage == 0 ? "effectCleanupThrowFirstContinues" : stage == 1 ? "effectCleanupThrowMiddleContinues" : "effectCleanupThrowLastContinues", cleanupComplete);
				const auto reuseHandle = lifecycle.ApplyGameplayEffect(cleanupEffect);
				lifecycle.RemoveGameplayEffect(reuseHandle);
				const std::string reuseAssertion = "effectRuntimeReusableAfterCleanupThrow" + std::to_string(stage);
				check(reuseAssertion.c_str(), reuseHandle.IsValid() && lifecycle.BuildGameplayEffectSnapshots().empty());
			}

			const GameplayTag retryCleanupTag{ "E2E.Effect.RetryCleanup" };
			sas::GameplayEffectDefinition retryCleanupEffect;
			retryCleanupEffect.effectId = "E2E.Effect.RetryCleanup";
			retryCleanupEffect.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			retryCleanupEffect.grantedTags.push_back(retryCleanupTag);
			retryCleanupEffect.modifiers.emplace_back(OwnerAttributeIds::EnergyPower, 1.f);
			int removalAttempts = 0;
			std::vector<std::string> retryOrder;
			auto retryCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			retryCallbacks.removing = [&](sas::ActiveGameplayEffect&)
			{
				retryOrder.push_back("removing-attempt-" + std::to_string(removalAttempts + 1));
				if (removalAttempts++ == 0) throw std::runtime_error("Injected E05 incomplete presentation cleanup");
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(retryCallbacks));
			const float energyBeforeClearRetry = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
			const auto retryCleanupHandle = lifecycle.ApplyGameplayEffect(retryCleanupEffect);
			const std::size_t activeEffectsBeforeClearCall = lifecycle.BuildGameplayEffectSnapshots().size();
			const float energyBeforeClearCall = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
			const bool tagOwnedBeforeClearCall = lifecycle.HasOwnedTag(retryCleanupTag, true);
			std::string retryCleanupError;
			try { lifecycle.Clear(); }
			catch (const std::runtime_error& error) { retryCleanupError = error.what(); }
			const bool cleanupDebtRetained = retryCleanupError == "Injected E05 incomplete presentation cleanup" &&
				removalAttempts == 1 && lifecycle.BuildGameplayEffectSnapshots().size() == 1 &&
				!lifecycle.HasOwnedTag(retryCleanupTag, true) &&
				!lifecycle.ApplyGameplayEffect(retryCleanupEffect).IsValid();
			lifecycle.Clear();
			const auto retryAfterCleanupDebt = lifecycle.ApplyGameplayEffect(retryCleanupEffect);
			lifecycle.RemoveGameplayEffect(retryAfterCleanupDebt);
			const bool cleanupDebtRetried = retryCleanupHandle.IsValid() && cleanupDebtRetained &&
				removalAttempts == 3 && retryAfterCleanupDebt.IsValid() &&
				lifecycle.BuildGameplayEffectSnapshots().empty();
			recordCase("effectCases", { { "case_id", "E05-clear-retry" },
				{ "input", "removing cleanup callback fails once during full Clear" },
				{ "expected", "record retained as cleanup debt; apply blocked until retry succeeds" },
				{ "actual", { { "firstError", retryCleanupError }, { "attempts", removalAttempts },
					{ "recordRetainedBeforeRetry", cleanupDebtRetained },
					{ "reusableAfterRetry", retryAfterCleanupDebt.IsValid() },
					{ "activeEffectsAfterReuse", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "order", retryOrder },
				{ "identities", { { "effectId", retryCleanupEffect.effectId }, { "effectHandle", retryCleanupHandle.id } } },
				{ "before", { { "activeEffects", activeEffectsBeforeClearCall },
					{ "energyPower", energyBeforeClearCall }, { "tagOwned", tagOwnedBeforeClearCall } } },
				{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "energyPower", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) },
					{ "tagOwned", lifecycle.HasOwnedTag(retryCleanupTag, true) } } },
				{ "errors", { retryCleanupError } }, { "passed", cleanupDebtRetried } });
			check("effectClearRetainsAndRetriesIncompleteCleanup", cleanupDebtRetried);

			const GameplayTag singleRemoveRetryTag{ "E2E.Effect.SingleRemoveRetry" };
			sas::GameplayEffectDefinition singleRemoveRetryEffect;
			singleRemoveRetryEffect.effectId = "E2E.Effect.SingleRemoveRetry";
			singleRemoveRetryEffect.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			singleRemoveRetryEffect.stackingPolicy = sas::GameplayEffectStackingPolicy::Stack;
			singleRemoveRetryEffect.maxStacks = 1;
			singleRemoveRetryEffect.grantedTags.push_back(singleRemoveRetryTag);
			singleRemoveRetryEffect.modifiers.emplace_back(OwnerAttributeIds::EnergyPower, 1.f);
			std::vector<std::string> singleRemoveRetryOrder;
			int singleRemoveAttempts = 0;
			auto singleRemoveCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			singleRemoveCallbacks.removing = [&](sas::ActiveGameplayEffect&)
			{
				++singleRemoveAttempts;
				if (singleRemoveAttempts == 1)
				{
					singleRemoveRetryOrder.push_back("removing-throw-once");
					throw std::runtime_error("Injected E05 single Remove presentation cleanup failure");
				}
				singleRemoveRetryOrder.push_back(singleRemoveAttempts == 2 ? "removing-retry" : "removing-reuse");
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(singleRemoveCallbacks));
			const float energyBeforeSingleRemoveRetry = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
			const auto singleRemoveRetryHandle = lifecycle.ApplyGameplayEffect(singleRemoveRetryEffect);
			const std::size_t activeEffectsBeforeSingleRemove = lifecycle.BuildGameplayEffectSnapshots().size();
			const float energyBeforeSingleRemove = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
			const bool tagOwnedBeforeSingleRemove = lifecycle.HasOwnedTag(singleRemoveRetryTag, true);
			std::string singleRemoveFirstError;
			try { lifecycle.RemoveGameplayEffect(singleRemoveRetryHandle); }
			catch (const std::runtime_error& error) { singleRemoveFirstError = error.what(); }
			const std::size_t activeEffectsAfterFirstFailure = lifecycle.BuildGameplayEffectSnapshots().size();
			const auto singleRemoveApplyWhileDebt = lifecycle.ApplyGameplayEffect(singleRemoveRetryEffect);
			const bool singleRemoveDebtRetained = singleRemoveRetryHandle.IsValid() && activeEffectsBeforeSingleRemove == 1 &&
				singleRemoveFirstError == "Injected E05 single Remove presentation cleanup failure" &&
				activeEffectsAfterFirstFailure == 1 && !singleRemoveApplyWhileDebt.IsValid() &&
				!lifecycle.HasOwnedTag(singleRemoveRetryTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) == energyBeforeSingleRemoveRetry;
			lifecycle.RemoveGameplayEffect(singleRemoveRetryHandle);
			const bool singleRemoveDebtCleared = lifecycle.BuildGameplayEffectSnapshots().empty() &&
				!lifecycle.HasOwnedTag(singleRemoveRetryTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) == energyBeforeSingleRemoveRetry;
			const auto singleRemoveReusedHandle = lifecycle.ApplyGameplayEffect(singleRemoveRetryEffect);
			lifecycle.RemoveGameplayEffect(singleRemoveReusedHandle);
			const bool singleRemoveRetryReusable = singleRemoveRetryHandle.IsValid() && singleRemoveDebtRetained &&
				singleRemoveDebtCleared && singleRemoveReusedHandle.IsValid() && singleRemoveAttempts == 3 &&
				lifecycle.BuildGameplayEffectSnapshots().empty() &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) == energyBeforeSingleRemoveRetry;
			recordCase("effectCases", { { "case_id", "E05-single-remove-retry" },
				{ "input", { { "operation", "RemoveGameplayEffect on one effect" },
					{ "removingCallback", "throw once, then succeed" }, { "stackingPolicy", "Stack" }, { "maxStacks", 1 } } },
				{ "expected", "first cleanup error is preserved; record retains cleanup debt; retry clears it and the component can reuse the same effect" },
				{ "actual", { { "firstError", singleRemoveFirstError }, { "removeAttempts", singleRemoveAttempts },
					{ "activeEffectsBeforeRemove", activeEffectsBeforeSingleRemove },
					{ "activeEffectsAfterFirstFailure", activeEffectsAfterFirstFailure },
					{ "recordRetainedBeforeRetry", singleRemoveDebtRetained },
					{ "sameKeyApplyRejectedDuringDebt", !singleRemoveApplyWhileDebt.IsValid() },
					{ "debtClearedAfterRetry", singleRemoveDebtCleared },
					{ "reusedHandleValid", singleRemoveReusedHandle.IsValid() },
					{ "activeEffectsAfterReuse", lifecycle.BuildGameplayEffectSnapshots().size() } } },
				{ "order", singleRemoveRetryOrder },
				{ "identities", { { "effectId", singleRemoveRetryEffect.effectId },
					{ "effectHandle", singleRemoveRetryHandle.id }, { "ownerActorId", lifecycleOwner->GetUniqueID() } } },
				{ "before", { { "activeEffects", activeEffectsBeforeSingleRemove },
					{ "energyPower", energyBeforeSingleRemove }, { "tagOwned", tagOwnedBeforeSingleRemove } } },
				{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "energyPower", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) },
					{ "tagOwned", lifecycle.HasOwnedTag(singleRemoveRetryTag, true) } } },
				{ "errors", { singleRemoveFirstError } }, { "passed", singleRemoveRetryReusable } });
			check("effectSingleRemoveRetainsDebtAndRetries", singleRemoveRetryReusable);

			// Shared tag counts and attribute modifiers remain owned independently.
			const GameplayTag sharedEffectTag{ "E2E.Effect.Shared" };
			sas::GameplayEffectDefinition sharedFirst;
			sharedFirst.effectId = "E2E.Effect.Shared.First";
			sharedFirst.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			sharedFirst.grantedTags.push_back(sharedEffectTag);
			sharedFirst.modifiers.emplace_back(OwnerAttributeIds::EnergyPower, 1.f);
			auto sharedSecond = sharedFirst;
			sharedSecond.effectId = "E2E.Effect.Shared.Second";
			const float energyBeforeShared = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
			std::vector<std::string> sharedOrder;
			sharedOrder.push_back("apply-first");
			const auto sharedFirstHandle = lifecycle.ApplyGameplayEffect(sharedFirst);
			sharedOrder.push_back("apply-second");
			const auto sharedSecondHandle = lifecycle.ApplyGameplayEffect(sharedSecond);
			sharedOrder.push_back("remove-first");
			lifecycle.RemoveGameplayEffect(sharedFirstHandle);
			const float energyAfterFirstRemoval = lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
			sharedOrder.push_back("remove-second");
			const bool sharedOwnershipIsolated = sharedFirstHandle.IsValid() && sharedSecondHandle.IsValid() &&
				lifecycle.HasOwnedTag(sharedEffectTag, true) &&
			energyAfterFirstRemoval == energyBeforeShared + 1.f;
			lifecycle.RemoveGameplayEffect(sharedSecondHandle);
			const bool sharedOwnershipReleased = sharedOwnershipIsolated && !lifecycle.HasOwnedTag(sharedEffectTag, true) &&
				lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) == energyBeforeShared;
			recordCase("effectCases", { { "case_id", "E06" },
				{ "input", { { "effects", { sharedFirst.effectId, sharedSecond.effectId } },
					{ "sharedTag", "E2E.Effect.Shared" }, { "sharedAttribute", std::string{ OwnerAttributeIds::EnergyPower.GetName() } } } },
				{ "expected", "removing one effect retains the other tag contribution and modifier; removing both releases them" },
				{ "actual", { { "retainedAfterFirstRemoval", sharedOwnershipIsolated },
					{ "energyAfterFirstRemoval", energyAfterFirstRemoval },
					{ "releasedAfterSecondRemoval", sharedOwnershipReleased },
					{ "energyAfterSecondRemoval", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) },
					{ "tagOwnedAfterSecondRemoval", lifecycle.HasOwnedTag(sharedEffectTag, true) } } },
				{ "order", sharedOrder },
				{ "identities", { { "firstEffectId", sharedFirst.effectId }, { "firstHandle", sharedFirstHandle.id },
					{ "secondEffectId", sharedSecond.effectId }, { "secondHandle", sharedSecondHandle.id } } },
				{ "before", { { "activeEffects", 0 }, { "energyPower", energyBeforeShared }, { "tagOwned", false } } },
				{ "after", { { "activeEffects", lifecycle.BuildGameplayEffectSnapshots().size() },
					{ "energyPower", lifecycle.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) },
					{ "tagOwned", lifecycle.HasOwnedTag(sharedEffectTag, true) } } },
				{ "errors", nlohmann::json::array() }, { "passed", sharedOwnershipReleased } });
			check("effectSharedTagAndModifierOwnership", sharedOwnershipReleased);

			// Same stacking-key reentry is rejected, while an independent nested
			// effect is admitted through the same live component.
			sas::GameplayEffectDefinition nestedOuter;
			nestedOuter.effectId = "E2E.Effect.Nested.Outer";
			nestedOuter.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
			nestedOuter.duration = 10.f;
			nestedOuter.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
			sas::GameplayEffectDefinition nestedOther = nestedOuter;
			nestedOther.effectId = "E2E.Effect.Nested.Other";
			bool nestedApplied = false;
			std::vector<std::string> nestedOrder;
			sas::GameplayEffectHandle sameKeyResult, otherEffectResult;
			auto nestedCallbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			nestedCallbacks.activated = [&](sas::ActiveGameplayEffect& effect)
			{
				if (effect.spec.definition.effectId != nestedOuter.effectId || nestedApplied) return;
				nestedApplied = true;
				nestedOrder.push_back("outer-activated");
				nestedOrder.push_back("same-key-apply");
				sameKeyResult = lifecycle.ApplyGameplayEffect(nestedOuter);
				nestedOrder.push_back("other-effect-apply");
				otherEffectResult = lifecycle.ApplyGameplayEffect(nestedOther);
			};
			lifecycle.SetEffectRuntimeCallbacks(std::move(nestedCallbacks));
			const auto nestedOuterResult = lifecycle.ApplyGameplayEffect(nestedOuter);
			const std::size_t activeEffectsAfterNestedApply = lifecycle.BuildGameplayEffectSnapshots().size();
			const bool nestedKeyPolicy = nestedOuterResult.IsValid() && nestedApplied &&
				!sameKeyResult.IsValid() && otherEffectResult.IsValid() &&
				activeEffectsAfterNestedApply == 2;
			recordCase("effectCases", { { "case_id", "E07" },
				{ "input", { { "callback", "activated applies same stacking key and a distinct effect" },
					{ "outerEffectId", nestedOuter.effectId }, { "otherEffectId", nestedOther.effectId } } },
				{ "expected", "same-key apply is invalid; independent nested effect is accepted and both active records remain" },
				{ "actual", { { "sameKeyValid", sameKeyResult.IsValid() },
					{ "otherEffectValid", otherEffectResult.IsValid() }, { "activeEffects", activeEffectsAfterNestedApply } } },
				{ "order", nestedOrder },
				{ "identities", { { "outerHandle", nestedOuterResult.id }, { "sameKeyHandle", sameKeyResult.id },
					{ "otherEffectHandle", otherEffectResult.id } } },
				{ "before", { { "activeEffects", 0 } } },
				{ "after", { { "activeEffects", activeEffectsAfterNestedApply } } },
				{ "errors", nlohmann::json::array() }, { "passed", nestedKeyPolicy } });
			check("effectSameKeyNestedApplyRejectedOtherEffectAccepted", nestedKeyPolicy);
			lifecycle.SetEffectRuntimeCallbacks({});
			lifecycle.RemoveGameplayEffect(nestedOuterResult);
			lifecycle.RemoveGameplayEffect(otherEffectResult);
			check("effectNestedApplyCleanup", lifecycle.BuildGameplayEffectSnapshots().empty());

			// Production GameAbility action phases stop after a callback cancels the
			// executing instance, and nested instance operations are rejected.
			auto actionOwner = spawn(7000.f, CollisionLayer::Player);
			world.TickInternal(0.f);
			auto& actionSystem = actionOwner->GetAbilitySystemComponent();
			const GameplayTag actionEventTag = GameplayTags::Event::Owner::DamageTaken;
			const std::size_t abilityCountBeforeE08 = actionSystem.BuildAbilitySnapshots().size();
			const float energyBeforeE08 = actionSystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower);
			auto firstCancelDefinition = MakeRuntimeActionAbility(
				"Ability.Utility.AuditFixes.CancelFirst",
				sas::AbilitySlot::Ability1,
				{
					MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 101.f),
					MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 102.f),
					MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 103.f),
					MakeEventAction(sas::AbilityActionPhase::OnEnd, actionEventTag, 190.f)
				}
			);
			const auto firstCancelHandle = actionSystem.GrantAbility(firstCancelDefinition, sas::AbilitySlot::Ability1);
			check("E08FirstAbilityGranted", firstCancelHandle.IsValid());
			RuntimeActionAudit firstCancelAudit{ actionSystem, *actionOwner, firstCancelHandle, actionEventTag, 101 };
			const auto firstActionEventToken = actionSystem.onGameplayEvent.BindAction(
				&firstCancelAudit, &RuntimeActionAudit::Event
			);
			const auto firstActionEndedToken = actionSystem.onAbilityEnded.BindAction(
				&firstCancelAudit, &RuntimeActionAudit::Ended
			);
			const int firstChargesBeforeInput = actionSystem.GetAbility(firstCancelHandle)->GetCharges();
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			actionSystem.Tick(0.01f);
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability1, false);
			const std::vector<int> firstCancelMagnitudes = firstCancelAudit.magnitudes;
			const std::vector<std::string> firstCancelOrder = firstCancelAudit.order;
			const bool firstCancelStopped = firstCancelMagnitudes == std::vector<int>{ 101, 190 } &&
				firstCancelAudit.endCount == 1 && !actionSystem.GetAbility(firstCancelHandle)->IsActive();
			const std::size_t abilityCountAfterFirstCancel = actionSystem.BuildAbilitySnapshots().size();
			actionSystem.onGameplayEvent.UnbindAction(firstActionEventToken);
			actionSystem.onAbilityEnded.UnbindAction(firstActionEndedToken);

			auto middleCancelDefinition = MakeRuntimeActionAbility(
				"Ability.Utility.AuditFixes.CancelMiddle",
				sas::AbilitySlot::Ability2,
				{
					MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 201.f),
					MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 202.f),
					MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 203.f),
					MakeEventAction(sas::AbilityActionPhase::OnEnd, actionEventTag, 290.f)
				}
			);
			const auto middleCancelHandle = actionSystem.GrantAbility(middleCancelDefinition, sas::AbilitySlot::Ability2);
			check("E08MiddleAbilityGranted", middleCancelHandle.IsValid());
			RuntimeActionAudit middleCancelAudit{ actionSystem, *actionOwner, middleCancelHandle, actionEventTag, 202 };
			const auto middleActionEventToken = actionSystem.onGameplayEvent.BindAction(
				&middleCancelAudit, &RuntimeActionAudit::Event
			);
			const auto middleActionEndedToken = actionSystem.onAbilityEnded.BindAction(
				&middleCancelAudit, &RuntimeActionAudit::Ended
			);
			const int middleChargesBeforeInput = actionSystem.GetAbility(middleCancelHandle)->GetCharges();
			const std::size_t abilityCountBeforeMiddleCancel = actionSystem.BuildAbilitySnapshots().size();
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability2, true);
			actionSystem.Tick(0.01f);
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability2, false);
			const std::vector<int> middleCancelMagnitudes = middleCancelAudit.magnitudes;
			const std::vector<std::string> middleCancelOrder = middleCancelAudit.order;
			const bool middleCancelStopped = middleCancelMagnitudes == std::vector<int>{ 201, 202, 290 } &&
				middleCancelAudit.endCount == 1 && !actionSystem.GetAbility(middleCancelHandle)->IsActive();
			const std::size_t abilityCountAfterMiddleCancel = actionSystem.BuildAbilitySnapshots().size();
			actionSystem.onGameplayEvent.UnbindAction(middleActionEventToken);
			actionSystem.onAbilityEnded.UnbindAction(middleActionEndedToken);
			std::vector<std::string> e08Order{ "first-callback" };
			e08Order.insert(e08Order.end(), firstCancelOrder.begin(), firstCancelOrder.end());
			e08Order.push_back("middle-callback");
			e08Order.insert(e08Order.end(), middleCancelOrder.begin(), middleCancelOrder.end());
			const bool e08Passed = firstCancelStopped && middleCancelStopped;
			recordCase("abilityCases", { { "case_id", "E08" },
				{ "input", { { "firstCancelAtMagnitude", 101 }, { "middleCancelAtMagnitude", 202 },
					{ "whileActiveActionCount", 3 }, { "lifetime", "UntilCancelled" } } },
				{ "expected", "first and middle WhileActive event callbacks cancel the live ability; later actions do not run and each execution ends once" },
				{ "actual", { { "first", { { "magnitudes", firstCancelMagnitudes }, { "endCount", firstCancelAudit.endCount },
					{ "active", actionSystem.GetAbility(firstCancelHandle)->IsActive() }, { "chargesBeforeInput", firstChargesBeforeInput } } },
					{ "middle", { { "magnitudes", middleCancelMagnitudes }, { "endCount", middleCancelAudit.endCount },
					{ "active", actionSystem.GetAbility(middleCancelHandle)->IsActive() }, { "chargesBeforeInput", middleChargesBeforeInput } } } } },
				{ "order", e08Order },
				{ "identities", { { "ownerActorId", actionOwner->GetUniqueID() },
					{ "firstAbilityId", firstCancelDefinition.abilityId }, { "firstHandle", firstCancelHandle.id },
					{ "middleAbilityId", middleCancelDefinition.abilityId }, { "middleHandle", middleCancelHandle.id } } },
				{ "before", { { "abilities", abilityCountBeforeE08 }, { "energyPower", energyBeforeE08 },
					{ "firstCharges", firstChargesBeforeInput }, { "middleCharges", middleChargesBeforeInput } } },
				{ "after", { { "abilitiesAfterFirst", abilityCountAfterFirstCancel },
					{ "abilitiesAfterMiddle", abilityCountAfterMiddleCancel },
					{ "energyPower", actionSystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower) },
					{ "firstActive", actionSystem.GetAbility(firstCancelHandle)->IsActive() },
					{ "middleActive", actionSystem.GetAbility(middleCancelHandle)->IsActive() } } },
				{ "errors", nlohmann::json::array() }, { "passed", e08Passed } });
			check("E08WhileActiveCancelStopsActionsAndEndsOnce", e08Passed);

			auto nestedActionDefinition = MakeRuntimeActionAbility(
				"Ability.Utility.AuditFixes.NestedInstanceOperations",
				sas::AbilitySlot::Ability3,
				{
					MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 301.f),
					MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 302.f),
					MakeEventAction(sas::AbilityActionPhase::OnEnd, actionEventTag, 390.f)
				}
			);
			nestedActionDefinition.levelProgression.push_back(AbilityLevelStep{});
			const std::size_t abilitiesBeforeE09 = actionSystem.BuildAbilitySnapshots().size();
			const auto nestedActionHandle = actionSystem.GrantAbility(nestedActionDefinition, sas::AbilitySlot::Ability3);
			check("E09NestedAbilityGranted", nestedActionHandle.IsValid());
			RuntimeActionAudit nestedActionAudit{ actionSystem, *actionOwner, nestedActionHandle, actionEventTag, 301 };
			nestedActionAudit.nestedReentry = true;
			const auto nestedActionEventToken = actionSystem.onGameplayEvent.BindAction(
				&nestedActionAudit, &RuntimeActionAudit::Event
			);
			const auto nestedActionEndedToken = actionSystem.onAbilityEnded.BindAction(
				&nestedActionAudit, &RuntimeActionAudit::Ended
			);
			const int nestedLevelBefore = actionSystem.GetAbility(nestedActionHandle)->GetLevel();
			const int nestedChargesBefore = actionSystem.GetAbility(nestedActionHandle)->GetCharges();
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
			actionSystem.Tick(0.01f);
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability3, false);
			const bool e09Passed = nestedActionAudit.eventVisited &&
				!nestedActionAudit.directSetLevelResult && !nestedActionAudit.componentSetLevelResult &&
				!nestedActionAudit.nestedTryActivateResult &&
				nestedActionAudit.levelBeforeNestedCalls == 1 && nestedActionAudit.levelAfterNestedCalls == 1 &&
				nestedActionAudit.chargesBeforeNestedCalls == nestedActionAudit.chargesAfterNestedCalls &&
				nestedActionAudit.endCount == 1 && nestedActionAudit.magnitudes == std::vector<int>{ 301, 390 } &&
				!actionSystem.GetAbility(nestedActionHandle)->IsActive();
			recordCase("abilityCases", { { "case_id", "E09" },
				{ "input", { { "eventMagnitude", 301 }, { "directSetLevel", 2 }, { "componentSetLevel", 2 },
					{ "nestedTickDelta", 1.f }, { "nestedActivate", true } } },
				{ "expected", "direct and component level changes plus nested Tick and activation are rejected inside the live callback; level and charges do not change" },
				{ "actual", { { "eventVisited", nestedActionAudit.eventVisited },
					{ "directSetLevelResult", nestedActionAudit.directSetLevelResult },
					{ "componentSetLevelResult", nestedActionAudit.componentSetLevelResult },
					{ "nestedTryActivateResult", nestedActionAudit.nestedTryActivateResult },
					{ "levelBeforeNestedCalls", nestedActionAudit.levelBeforeNestedCalls },
					{ "levelAfterNestedCalls", nestedActionAudit.levelAfterNestedCalls },
					{ "chargesBeforeNestedCalls", nestedActionAudit.chargesBeforeNestedCalls },
					{ "chargesAfterNestedCalls", nestedActionAudit.chargesAfterNestedCalls },
					{ "finalLevel", actionSystem.GetAbility(nestedActionHandle)->GetLevel() },
					{ "finalCharges", actionSystem.GetAbility(nestedActionHandle)->GetCharges() },
					{ "endCount", nestedActionAudit.endCount }, { "magnitudes", nestedActionAudit.magnitudes } } },
				{ "order", nestedActionAudit.order },
				{ "identities", { { "ownerActorId", actionOwner->GetUniqueID() },
					{ "abilityId", nestedActionDefinition.abilityId }, { "abilityHandle", nestedActionHandle.id } } },
				{ "before", { { "abilities", abilitiesBeforeE09 },
					{ "level", nestedLevelBefore }, { "charges", nestedChargesBefore } } },
				{ "after", { { "abilities", actionSystem.BuildAbilitySnapshots().size() },
					{ "level", actionSystem.GetAbility(nestedActionHandle)->GetLevel() },
					{ "charges", actionSystem.GetAbility(nestedActionHandle)->GetCharges() },
					{ "active", actionSystem.GetAbility(nestedActionHandle)->IsActive() } } },
				{ "errors", nlohmann::json::array() }, { "passed", e09Passed } });
			actionSystem.onGameplayEvent.UnbindAction(nestedActionEventToken);
			actionSystem.onAbilityEnded.UnbindAction(nestedActionEndedToken);
			check("E09NestedInstanceMutationIsRejected", e09Passed);

			const GameplayTag snapshotEventTag = GameplayTags::Event::Source::DamageDealt;
			auto snapshotDefinition = MakeRuntimeActionAbility(
				"Ability.Utility.AuditFixes.ScopedActionSnapshot",
				sas::AbilitySlot::Ability4,
				{ MakeEventAction(sas::AbilityActionPhase::OnEnd, snapshotEventTag, 701.f) }
			);
			AbilityLevelStep scopedStep;
			scopedStep.addedActions.push_back(
				MakeEventAction(sas::AbilityActionPhase::OnEnd, snapshotEventTag, 702.f)
			);
			snapshotDefinition.levelProgression.push_back(scopedStep);
			const auto snapshotHandle = actionSystem.GrantAbility(snapshotDefinition, sas::AbilitySlot::Ability4);
			check("E10SnapshotAbilityGranted", snapshotHandle.IsValid());
			std::vector<std::string> scopedRuleOrder;
			ScopedRuleActivationAudit scopedRuleAudit{ actionSystem, snapshotHandle, {}, false, 0, &scopedRuleOrder };
			scopedRuleAudit.rule.requiredAbilityTags.push_back(GameplayTags::Ability::Utility);
			scopedRuleAudit.rule.levelBonus = 1;
			const auto scopedRuleToken = actionSystem.onAbilityActivated.BindAction(
				&scopedRuleAudit, &ScopedRuleActivationAudit::Activated
			);
			RuntimeActionAudit snapshotAudit{ actionSystem, *actionOwner, snapshotHandle, snapshotEventTag };
			const auto snapshotEventToken = actionSystem.onGameplayEvent.BindAction(
				&snapshotAudit, &RuntimeActionAudit::Event
			);
			const auto snapshotEndedToken = actionSystem.onAbilityEnded.BindAction(
				&snapshotAudit, &RuntimeActionAudit::Ended
			);
			const std::size_t baseActionCount = actionSystem.GetAbility(snapshotHandle)->GetDefinition().actions.size();
			const int snapshotLevelBefore = actionSystem.GetAbility(snapshotHandle)->GetLevel();
			const std::size_t abilityCountBeforeE10 = actionSystem.BuildAbilitySnapshots().size();
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
			actionSystem.Tick(0.01f);
			const std::size_t actionCountAfterScopeRefresh = actionSystem.GetAbility(snapshotHandle)->GetDefinition().actions.size();
			const int snapshotLevelAfterScopeRefresh = actionSystem.GetAbility(snapshotHandle)->GetLevel();
			actionSystem.GetAbility(snapshotHandle)->Cancel(sas::AbilityEndReason::Cancelled);
			const std::vector<int> firstSnapshotEndMagnitudes = snapshotAudit.magnitudes;
			const bool firstActivationUsedOldSnapshot = firstSnapshotEndMagnitudes == std::vector<int>{ 701 };
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability4, false);
			actionSystem.Tick(0.01f);
			snapshotAudit.magnitudes.clear();
			snapshotAudit.order.clear();
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
			actionSystem.Tick(0.01f);
			actionSystem.GetAbility(snapshotHandle)->Cancel(sas::AbilityEndReason::Cancelled);
			const std::vector<int> secondActivationEndMagnitudes = snapshotAudit.magnitudes;
			const bool secondActivationUsedCurrentDefinition = secondActivationEndMagnitudes == std::vector<int>{ 701, 702 };
			actionSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability4, false);
			actionSystem.Tick(0.01f);
			const bool scopedRuleRemoved = actionSystem.RemoveScopedAbilityRule(scopedRuleAudit.ruleHandle);
			const std::size_t actionCountAfterScopeRemoval = actionSystem.GetAbility(snapshotHandle)->GetDefinition().actions.size();
			const bool e10Passed = scopedRuleAudit.visited && scopedRuleAudit.ruleHandle != 0 &&
				baseActionCount == 1 && actionCountAfterScopeRefresh == 2 &&
				snapshotLevelBefore == 1 && snapshotLevelAfterScopeRefresh == 1 &&
				firstActivationUsedOldSnapshot && secondActivationUsedCurrentDefinition &&
				snapshotAudit.endCount == 2 && scopedRuleRemoved && actionCountAfterScopeRemoval == 1 &&
				!actionSystem.GetAbility(snapshotHandle)->IsActive();
			recordCase("abilityCases", { { "case_id", "E10" },
				{ "input", { { "activation", "OnPressed" }, { "scopeChange", "levelBonus +1 with one added OnEnd action" },
					{ "baseOnEndMagnitude", 701 }, { "scopedOnEndMagnitude", 702 } } },
				{ "expected", "the active execution ends with its captured base OnEnd spec; the next activation captures the scoped action list" },
				{ "actual", { { "scopedRuleVisited", scopedRuleAudit.visited }, { "scopedRuleHandle", scopedRuleAudit.ruleHandle },
					{ "baseActionCount", baseActionCount }, { "actionCountAfterScopeRefresh", actionCountAfterScopeRefresh },
					{ "actionCountAfterScopeRemoval", actionCountAfterScopeRemoval },
					{ "snapshotLevelBefore", snapshotLevelBefore }, { "snapshotLevelAfterScopeRefresh", snapshotLevelAfterScopeRefresh },
					{ "firstActivationEndMagnitudes", firstSnapshotEndMagnitudes },
					{ "secondActivationEndMagnitudes", secondActivationEndMagnitudes },
					{ "endCount", snapshotAudit.endCount }, { "scopeRemoved", scopedRuleRemoved } } },
				{ "order", scopedRuleOrder },
				{ "identities", { { "ownerActorId", actionOwner->GetUniqueID() },
					{ "abilityId", snapshotDefinition.abilityId }, { "abilityHandle", snapshotHandle.id },
					{ "scopeRuleHandle", scopedRuleAudit.ruleHandle } } },
				{ "before", { { "abilities", abilityCountBeforeE10 }, { "level", snapshotLevelBefore },
					{ "actionCount", baseActionCount } } },
				{ "after", { { "abilities", actionSystem.BuildAbilitySnapshots().size() },
					{ "level", actionSystem.GetAbility(snapshotHandle)->GetLevel() },
					{ "actionCount", actionCountAfterScopeRemoval }, { "active", actionSystem.GetAbility(snapshotHandle)->IsActive() } } },
				{ "errors", nlohmann::json::array() }, { "passed", e10Passed } });
			actionSystem.onGameplayEvent.UnbindAction(snapshotEventToken);
			actionSystem.onAbilityEnded.UnbindAction(snapshotEndedToken);
			actionSystem.onAbilityActivated.UnbindAction(scopedRuleToken);
			check("E10ActiveActionSnapshotSurvivesScopedRefresh", e10Passed);

			// Real OnPressed abilities create several shipped beam visuals. Clear
			// must attempt every visual and every ability after the first EndFire
			// callback throws, then retry only the retained cleanup debt.
			auto beamOwner = spawn(8000.f, CollisionLayer::Player);
			world.TickInternal(0.f);
			auto& beamSystem = beamOwner->GetAbilitySystemComponent();
			const auto existingBeamActors = world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>();
			Set<uint64_t> existingBeamIds;
			for (const auto& weakBeam : existingBeamActors)
				if (const auto beam = weakBeam.lock()) existingBeamIds.insert(beam->GetUniqueID());
			auto firstBeamAction = MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 0.f);
			firstBeamAction.action = FireWeaponAction{ MakeContinuousBeamDefinition(2) };
			auto secondBeamAction = MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 0.f);
			secondBeamAction.action = FireWeaponAction{ MakeContinuousBeamDefinition() };
			auto firstBeamDefinition = MakeRuntimeActionAbility(
				"Ability.Utility.AuditFixes.MultiBeamCleanup",
				sas::AbilitySlot::Ability1,
				{ firstBeamAction, MakeEventAction(sas::AbilityActionPhase::OnEnd, actionEventTag, 811.f) }
			);
			auto secondBeamDefinition = MakeRuntimeActionAbility(
				"Ability.Utility.AuditFixes.IndependentBeamCleanup",
				sas::AbilitySlot::Ability2,
				{ secondBeamAction, MakeEventAction(sas::AbilityActionPhase::OnEnd, actionEventTag, 821.f) }
			);
			const std::size_t abilitiesBeforeE11 = beamSystem.BuildAbilitySnapshots().size();
			const std::size_t activeEffectsBeforeE11 = beamSystem.BuildGameplayEffectSnapshots().size();
			const auto firstBeamHandle = beamSystem.GrantAbility(firstBeamDefinition, sas::AbilitySlot::Ability1);
			const auto secondBeamHandle = beamSystem.GrantAbility(secondBeamDefinition, sas::AbilitySlot::Ability2);
			check("E11BeamCleanupAbilitiesGranted", firstBeamHandle.IsValid() && secondBeamHandle.IsValid());
			RuntimeActionAudit firstBeamActionAudit{ beamSystem, *beamOwner, firstBeamHandle, actionEventTag };
			RuntimeActionAudit secondBeamActionAudit{ beamSystem, *beamOwner, secondBeamHandle, actionEventTag };
			const auto firstBeamEventToken = beamSystem.onGameplayEvent.BindAction(
				&firstBeamActionAudit, &RuntimeActionAudit::Event
			);
			const auto secondBeamEventToken = beamSystem.onGameplayEvent.BindAction(
				&secondBeamActionAudit, &RuntimeActionAudit::Event
			);
			const auto firstBeamEndedToken = beamSystem.onAbilityEnded.BindAction(
				&firstBeamActionAudit, &RuntimeActionAudit::Ended
			);
			const auto secondBeamEndedToken = beamSystem.onAbilityEnded.BindAction(
				&secondBeamActionAudit, &RuntimeActionAudit::Ended
			);
			const std::size_t liveBeamsBeforeActivation = [&]
			{
				std::size_t count = 0;
				for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
					if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy()) ++count;
				return count;
			}();
			beamSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			beamSystem.Tick(0.01f);
			beamSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability1, false);
			beamSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability2, true);
			beamSystem.Tick(0.01f);
			beamSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability2, false);
			world.TickInternal(0.f);
			std::vector<shared_ptr<ContinuousBeamVisualActor>> ownedBeamActors;
			for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
			{
				if (const auto beam = weakBeam.lock(); beam && existingBeamIds.find(beam->GetUniqueID()) == existingBeamIds.end())
					ownedBeamActors.push_back(beam);
			}
			const auto beamActorsAfterActivation = world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>();
			std::size_t liveBeamActorsAfterActivation = 0;
			for (const auto& weakBeam : beamActorsAfterActivation)
				if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy()) ++liveBeamActorsAfterActivation;
			const GameAbility* firstBeamAbilityAfterActivation = beamSystem.GetAbility(firstBeamHandle);
			const GameAbility* secondBeamAbilityAfterActivation = beamSystem.GetAbility(secondBeamHandle);
			result["diagnostics"]["e11BeamSetup"] = {
				{ "expectedNewVisuals", 3 }, { "newVisuals", ownedBeamActors.size() },
				{ "allVisualsIncludingPending", beamActorsAfterActivation.size() },
				{ "liveVisuals", liveBeamActorsAfterActivation }, { "preexistingVisuals", existingBeamIds.size() },
				{ "firstAbilityActive", firstBeamAbilityAfterActivation && firstBeamAbilityAfterActivation->IsActive() },
				{ "secondAbilityActive", secondBeamAbilityAfterActivation && secondBeamAbilityAfterActivation->IsActive() },
				{ "firstActionPhase", static_cast<int>(firstBeamAction.phase) },
				{ "secondActionPhase", static_cast<int>(secondBeamAction.phase) }
			};
			check("E11ThreeRealBeamVisualsStarted", ownedBeamActors.size() == 3 &&
				firstBeamAbilityAfterActivation && firstBeamAbilityAfterActivation->IsActive() &&
				secondBeamAbilityAfterActivation && secondBeamAbilityAfterActivation->IsActive());
			std::vector<std::string> e11Order;
			BeamDestroyAudit beamDestroyAudit{
				ownedBeamActors[0]->GetUniqueID(), ownedBeamActors[1]->GetUniqueID(),
				ownedBeamActors[2]->GetUniqueID(), true, e11Order
			};
			std::vector<DelegateHandle> beamDestroyTokens;
			for (const auto& beam : ownedBeamActors)
			{
				beamDestroyTokens.push_back(beam->onActorDestroyed.BindAction(
					&beamDestroyAudit, &BeamDestroyAudit::Destroyed
				));
			}
			std::string firstEndFireError;
			try { beamSystem.Clear(); }
			catch (const std::runtime_error& error) { firstEndFireError = error.what(); }
			const bool failedBeamRetainedAfterFirstClear = !ownedBeamActors[0]->GetIsPendingDestroy();
			const bool siblingBeamCleanedAfterFirstError = ownedBeamActors[1]->GetIsPendingDestroy();
			const bool independentAbilityBeamCleanedAfterFirstError = ownedBeamActors[2]->GetIsPendingDestroy();
			const bool firstClearEndedBothAbilitiesOnce = firstBeamActionAudit.endCount == 1 &&
				secondBeamActionAudit.endCount == 1;
			std::string retryEndFireError;
			try { beamSystem.Clear(); }
			catch (const std::runtime_error& error) { retryEndFireError = error.what(); }
			const bool allFirstExecutionVisualsCleaned = std::all_of(
				ownedBeamActors.begin(), ownedBeamActors.end(),
				[](const shared_ptr<ContinuousBeamVisualActor>& beam) { return beam->GetIsPendingDestroy(); }
			);
			const bool bothAbilitiesEndedOnceAfterRetry = firstBeamActionAudit.endCount == 1 &&
				secondBeamActionAudit.endCount == 1;
			for (std::size_t index = 0; index < ownedBeamActors.size(); ++index)
				ownedBeamActors[index]->onActorDestroyed.UnbindAction(beamDestroyTokens[index]);
			beamSystem.onGameplayEvent.UnbindAction(firstBeamEventToken);
			beamSystem.onGameplayEvent.UnbindAction(secondBeamEventToken);
			beamSystem.onAbilityEnded.UnbindAction(firstBeamEndedToken);
			beamSystem.onAbilityEnded.UnbindAction(secondBeamEndedToken);

			auto reuseAction = MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 0.f);
			reuseAction.action = FireWeaponAction{ MakeContinuousBeamDefinition() };
			auto reuseDefinition = MakeRuntimeActionAbility(
				"Ability.Utility.AuditFixes.ReuseAfterCleanupRetry",
				sas::AbilitySlot::Ability1,
				{ reuseAction }
			);
			const auto reuseHandle = beamSystem.GrantAbility(reuseDefinition, sas::AbilitySlot::Ability1);
			const std::size_t liveBeamsBeforeReuse = [&]
			{
				std::size_t count = 0;
				for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
					if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy()) ++count;
				return count;
			}();
			beamSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			beamSystem.Tick(0.01f);
			beamSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability1, false);
			world.TickInternal(0.f);
			const bool reuseAbilityActive = reuseHandle.IsValid() && beamSystem.GetAbility(reuseHandle) &&
				beamSystem.GetAbility(reuseHandle)->IsActive();
			const std::size_t liveBeamsAfterReuseActivation = [&]
			{
				std::size_t count = 0;
				for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
					if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy()) ++count;
				return count;
			}();
			std::string reuseClearError;
			try { beamSystem.Clear(); }
			catch (const std::runtime_error& error) { reuseClearError = error.what(); }
			const std::size_t liveBeamsAfterE11 = [&]
			{
				std::size_t count = 0;
				for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
					if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy()) ++count;
				return count;
			}();

			// Echo Protocol drives a real temporary Return Protocol invocation. A
			// throwing visual-destroy callback must leave that invocation owned for
			// the next component Clear, then permit the same production input path.
			const GameAbilityDefinition* returnDefinition =
				AbilityData::FindShippedAbilityDefinition(AbilityData::ReturnProtocol::AbilityId::Basic);
			const GameAbilityDefinition* echoDefinition =
				AbilityData::FindShippedAbilityDefinition(AbilityData::EchoProtocol::AbilityId::Basic);
			if (!returnDefinition || !echoDefinition)
				throw std::runtime_error("E11 shipped Return Protocol or Echo Protocol definition is missing");
			auto countLiveReturnProtocolVisuals = [&]()
			{
				std::size_t count = 0;
				for (const auto& weakVisual : world.GetActorsByTypeIncludingPending<ReturnProtocolVisualActor>())
					if (const auto visual = weakVisual.lock(); visual && !visual->GetIsPendingDestroy()) ++count;
				return count;
			};
			Set<uint64_t> existingReturnVisualIds;
			for (const auto& weakVisual : world.GetActorsByTypeIncludingPending<ReturnProtocolVisualActor>())
				if (const auto visual = weakVisual.lock()) existingReturnVisualIds.insert(visual->GetUniqueID());
			const std::size_t abilitiesBeforeInvocation = beamSystem.BuildAbilitySnapshots().size();
			const std::size_t activeEffectsBeforeInvocation = beamSystem.BuildGameplayEffectSnapshots().size();
			const std::size_t activeInvocationsBefore = beamSystem.GetActiveAbilityInvocationCount();
			const std::size_t liveReturnVisualsBefore = countLiveReturnProtocolVisuals();
			const auto returnHandle = beamSystem.GrantAbility(*returnDefinition, sas::AbilitySlot::Ability1);
			const auto echoHandle = beamSystem.GrantAbility(*echoDefinition, sas::AbilitySlot::Ability3);
			check("E11ReturnProtocolAndEchoGranted", returnHandle.IsValid() && echoHandle.IsValid());
			auto activateSlotInput = [&beamSystem](sas::AbilitySlot slot)
			{
				beamSystem.SetAbilitySlotInput(slot, true);
				beamSystem.Tick(0.01f);
				beamSystem.SetAbilitySlotInput(slot, false);
			};
			activateSlotInput(sas::AbilitySlot::Ability1);
			GameAbility* returnSource = beamSystem.GetAbility(returnHandle);
			const bool returnSourceActivated = returnSource && returnSource->IsActive();
			AbilityUseRecord firstReturnRecord;
			bool firstReturnRecordCaptured = false;
			if (const AbilityUseRecord* recorded = beamSystem.GetAbilityUseHistory().FindLatestUnconsumed(
				[&](const AbilityUseRecord& candidate) { return candidate.abilityId == returnDefinition->abilityId; }
			))
			{
				firstReturnRecord = *recorded;
				firstReturnRecordCaptured = true;
			}
			if (returnSource) returnSource->Cancel(sas::AbilityEndReason::Cancelled);
			const bool returnSourceEndedBeforeEcho = returnSource && !returnSource->IsActive();
			std::vector<std::string> returnInvocationOrder;
			ReturnProtocolEventAudit returnEventAudit{ *beamOwner, returnInvocationOrder };
			const auto returnEventToken = beamSystem.onGameplayEvent.BindAction(
				&returnEventAudit, &ReturnProtocolEventAudit::Event
			);
			activateSlotInput(sas::AbilitySlot::Ability3);
			const bool firstReturnRecordConsumed = beamSystem.GetAbilityUseHistory().FindLatestUnconsumed(
				[&](const AbilityUseRecord& candidate) { return candidate.abilityId == returnDefinition->abilityId; }
			) == nullptr;
			std::vector<shared_ptr<ReturnProtocolVisualActor>> firstInvocationVisuals;
			for (const auto& weakVisual : world.GetActorsByTypeIncludingPending<ReturnProtocolVisualActor>())
			{
				if (const auto visual = weakVisual.lock(); visual && !visual->GetIsPendingDestroy() &&
					existingReturnVisualIds.find(visual->GetUniqueID()) == existingReturnVisualIds.end())
				{
					firstInvocationVisuals.push_back(visual);
				}
			}
			const std::size_t firstInvocationCount = beamSystem.GetActiveAbilityInvocationCount();
			const std::size_t liveReturnVisualsAfterEcho = countLiveReturnProtocolVisuals();
			const std::size_t startedEventsAfterEcho = returnEventAudit.startedCount;
			std::uint64_t firstInvocationVisualId = 0;
			InvocationDestroyAudit invocationDestroyAudit{ 0, true, e11Order };
			DelegateHandle invocationDestroyToken;
			if (!firstInvocationVisuals.empty())
			{
				firstInvocationVisualId = firstInvocationVisuals.front()->GetUniqueID();
				invocationDestroyAudit.actorId = firstInvocationVisualId;
				invocationDestroyToken = firstInvocationVisuals.front()->onActorDestroyed.BindAction(
					&invocationDestroyAudit, &InvocationDestroyAudit::Destroyed
				);
			}
			std::string firstInvocationClearError;
			try { beamSystem.Clear(); }
			catch (const std::runtime_error& error) { firstInvocationClearError = error.what(); }
			const std::size_t invocationCountAfterFailedClear = beamSystem.GetActiveAbilityInvocationCount();
			const bool invocationVisualRetainedAfterFailure = !firstInvocationVisuals.empty() &&
				!firstInvocationVisuals.front()->GetIsPendingDestroy();
			const std::size_t liveReturnVisualsAfterFailedClear = countLiveReturnProtocolVisuals();
			const std::size_t startedEventsAfterFailedClear = returnEventAudit.startedCount;
			const std::size_t endedEventsAfterFailedClear = returnEventAudit.endedCount;
			std::string invocationRetryClearError;
			try { beamSystem.Clear(); }
			catch (const std::runtime_error& error) { invocationRetryClearError = error.what(); }
			const std::size_t invocationCountAfterRetry = beamSystem.GetActiveAbilityInvocationCount();
			const bool invocationVisualCleanedAfterRetry = !firstInvocationVisuals.empty() &&
				firstInvocationVisuals.front()->GetIsPendingDestroy();
			const std::size_t liveReturnVisualsAfterRetry = countLiveReturnProtocolVisuals();
			const std::size_t startedEventsAfterRetry = returnEventAudit.startedCount;
			const std::size_t endedEventsAfterRetry = returnEventAudit.endedCount;
			if (!firstInvocationVisuals.empty())
				firstInvocationVisuals.front()->onActorDestroyed.UnbindAction(invocationDestroyToken);
			beamSystem.onGameplayEvent.UnbindAction(returnEventToken);

			Set<uint64_t> visualIdsBeforeInvocationReuse;
			for (const auto& weakVisual : world.GetActorsByTypeIncludingPending<ReturnProtocolVisualActor>())
				if (const auto visual = weakVisual.lock()) visualIdsBeforeInvocationReuse.insert(visual->GetUniqueID());
			const auto returnReuseHandle = beamSystem.GrantAbility(*returnDefinition, sas::AbilitySlot::Ability1);
			const auto echoReuseHandle = beamSystem.GrantAbility(*echoDefinition, sas::AbilitySlot::Ability3);
			activateSlotInput(sas::AbilitySlot::Ability1);
			GameAbility* returnReuseSource = beamSystem.GetAbility(returnReuseHandle);
			const bool returnReuseSourceActivated = returnReuseSource && returnReuseSource->IsActive();
			AbilityUseRecord reuseReturnRecord;
			bool reuseReturnRecordCaptured = false;
			if (const AbilityUseRecord* recorded = beamSystem.GetAbilityUseHistory().FindLatestUnconsumed(
				[&](const AbilityUseRecord& candidate) { return candidate.abilityId == returnDefinition->abilityId; }
			))
			{
				reuseReturnRecord = *recorded;
				reuseReturnRecordCaptured = true;
			}
			if (returnReuseSource) returnReuseSource->Cancel(sas::AbilityEndReason::Cancelled);
			const bool returnReuseSourceEndedBeforeEcho = returnReuseSource && !returnReuseSource->IsActive();
			activateSlotInput(sas::AbilitySlot::Ability3);
			const bool reuseReturnRecordConsumed = beamSystem.GetAbilityUseHistory().FindLatestUnconsumed(
				[&](const AbilityUseRecord& candidate) { return candidate.abilityId == returnDefinition->abilityId; }
			) == nullptr;
			std::vector<shared_ptr<ReturnProtocolVisualActor>> reusedInvocationVisuals;
			for (const auto& weakVisual : world.GetActorsByTypeIncludingPending<ReturnProtocolVisualActor>())
			{
				if (const auto visual = weakVisual.lock(); visual && !visual->GetIsPendingDestroy() &&
					visualIdsBeforeInvocationReuse.find(visual->GetUniqueID()) == visualIdsBeforeInvocationReuse.end())
				{
					reusedInvocationVisuals.push_back(visual);
				}
			}
			const std::size_t invocationCountAfterReuse = beamSystem.GetActiveAbilityInvocationCount();
			std::string reuseInvocationClearError;
			try { beamSystem.Clear(); }
			catch (const std::runtime_error& error) { reuseInvocationClearError = error.what(); }
			const std::size_t invocationCountAfterReuseClear = beamSystem.GetActiveAbilityInvocationCount();
			const bool reusedInvocationVisualCleaned = !reusedInvocationVisuals.empty() &&
				reusedInvocationVisuals.front()->GetIsPendingDestroy();
			const std::size_t liveReturnVisualsAfterInvocationReuse = countLiveReturnProtocolVisuals();
			const bool e11InvocationPassed = returnSourceActivated && firstReturnRecordCaptured &&
				returnSourceEndedBeforeEcho && firstReturnRecordConsumed && firstInvocationCount == 1 &&
				firstInvocationVisuals.size() == 1 && liveReturnVisualsAfterEcho == liveReturnVisualsBefore + 1 &&
				firstInvocationClearError == "Injected E11 ReturnProtocol visual cleanup failure" &&
				invocationCountAfterFailedClear == 1 && invocationVisualRetainedAfterFailure &&
				liveReturnVisualsAfterFailedClear == liveReturnVisualsBefore + 1 &&
				startedEventsAfterEcho == 1 && startedEventsAfterFailedClear == 1 && endedEventsAfterFailedClear == 0 &&
				invocationRetryClearError.empty() && invocationCountAfterRetry == 0 && invocationVisualCleanedAfterRetry &&
				liveReturnVisualsAfterRetry == liveReturnVisualsBefore && startedEventsAfterRetry == 1 &&
				endedEventsAfterRetry == 0 &&
				returnInvocationOrder == std::vector<std::string>{ "return-protocol-started" } &&
				returnReuseHandle.IsValid() && echoReuseHandle.IsValid() && returnReuseSourceActivated &&
				reuseReturnRecordCaptured && returnReuseSourceEndedBeforeEcho && reuseReturnRecordConsumed &&
				invocationCountAfterReuse == 1 && reusedInvocationVisuals.size() == 1 &&
				reuseInvocationClearError.empty() && invocationCountAfterReuseClear == 0 &&
				reusedInvocationVisualCleaned && liveReturnVisualsAfterInvocationReuse == liveReturnVisualsBefore;
			const bool e11Passed = firstEndFireError == "Injected E11 first beam EndFire failure" &&
				failedBeamRetainedAfterFirstClear && siblingBeamCleanedAfterFirstError &&
				independentAbilityBeamCleanedAfterFirstError && firstClearEndedBothAbilitiesOnce &&
				retryEndFireError.empty() && allFirstExecutionVisualsCleaned && bothAbilitiesEndedOnceAfterRetry &&
				reuseHandle.IsValid() && reuseAbilityActive && liveBeamsAfterReuseActivation == liveBeamsBeforeReuse + 1 &&
				reuseClearError.empty() && liveBeamsAfterE11 == liveBeamsBeforeActivation && e11InvocationPassed;
			nlohmann::json e11Actual = {
				{ "beamCleanup", {
					{ "firstError", firstEndFireError },
					{ "failedBeamRetainedAfterFirstClear", failedBeamRetainedAfterFirstClear },
					{ "siblingBeamCleanedAfterFirstError", siblingBeamCleanedAfterFirstError },
					{ "independentAbilityBeamCleanedAfterFirstError", independentAbilityBeamCleanedAfterFirstError },
					{ "firstClearEndedBothAbilitiesOnce", firstClearEndedBothAbilitiesOnce },
					{ "retryError", retryEndFireError },
					{ "firstExecutionVisualsCleanedAfterRetry", allFirstExecutionVisualsCleaned },
					{ "bothAbilitiesEndedOnceAfterRetry", bothAbilitiesEndedOnceAfterRetry },
					{ "reuseHandleValid", reuseHandle.IsValid() }, { "reuseAbilityActive", reuseAbilityActive },
					{ "liveBeamsBeforeReuse", liveBeamsBeforeReuse },
					{ "liveBeamsAfterReuseActivation", liveBeamsAfterReuseActivation },
					{ "reuseClearError", reuseClearError }, { "liveBeamsAfterCleanup", liveBeamsAfterE11 }
				} },
				{ "invocationCleanup", {
					{ "sourceActivated", returnSourceActivated },
					{ "sourceEndedBeforeEcho", returnSourceEndedBeforeEcho },
					{ "recordCaptured", firstReturnRecordCaptured },
					{ "recordConsumedByEcho", firstReturnRecordConsumed },
					{ "recordSequence", firstReturnRecord.sequence },
					{ "invocationCount", firstInvocationCount },
					{ "invocationVisualCount", firstInvocationVisuals.size() },
					{ "invocationVisualId", firstInvocationVisualId },
					{ "liveVisualsAfterEcho", liveReturnVisualsAfterEcho },
					{ "firstClearError", firstInvocationClearError },
					{ "invocationsAfterFailedClear", invocationCountAfterFailedClear },
					{ "visualRetainedAfterFailure", invocationVisualRetainedAfterFailure },
					{ "liveVisualsAfterFailedClear", liveReturnVisualsAfterFailedClear },
					{ "startedEventsAfterEcho", startedEventsAfterEcho },
					{ "startedEventsAfterFailedClear", startedEventsAfterFailedClear },
					{ "endedEventsAfterFailedClear", endedEventsAfterFailedClear },
					{ "retryClearError", invocationRetryClearError },
					{ "invocationsAfterRetry", invocationCountAfterRetry },
					{ "visualCleanedAfterRetry", invocationVisualCleanedAfterRetry },
					{ "liveVisualsAfterRetry", liveReturnVisualsAfterRetry },
					{ "startedEventsAfterRetry", startedEventsAfterRetry },
					{ "endedEventsAfterRetry", endedEventsAfterRetry },
					{ "returnEvents", returnInvocationOrder },
					{ "reuseSourceActivated", returnReuseSourceActivated },
					{ "reuseRecordCaptured", reuseReturnRecordCaptured },
					{ "reuseRecordSequence", reuseReturnRecord.sequence },
					{ "reuseRecordConsumedByEcho", reuseReturnRecordConsumed },
					{ "reuseInvocationCount", invocationCountAfterReuse },
					{ "reuseInvocationVisualCount", reusedInvocationVisuals.size() },
					{ "reuseInvocationVisualCleaned", reusedInvocationVisualCleaned },
					{ "reuseClearError", reuseInvocationClearError },
					{ "invocationsAfterReuseClear", invocationCountAfterReuseClear },
					{ "liveVisualsBefore", liveReturnVisualsBefore },
					{ "liveVisualsAfterReuseClear", liveReturnVisualsAfterInvocationReuse }
				} }
			};
			recordCase("abilityCases", { { "case_id", "E11" },
				{ "input", { { "abilitySlots", { "Ability1", "Ability2" } }, { "weaponType", "BeamContinuous" },
					{ "muzzleCounts", { 2, 1 } }, { "faults", { "first beam actor destroy throws once", "invoked Return Protocol visual destroy throws once" } },
					{ "invocationPath", "Return Protocol slot input, then Echo Protocol slot input; retry and re-use use the same production path" } } },
				{ "expected", "Clear attempts sibling beam visuals and the second active ability after the first EndFire fault; it retains a failed real Echo invocation for retry, suppresses Return Protocol Ended while Clear is pending without replay on retry, and permits later input-driven reuse" },
				{ "actual", e11Actual },
				{ "order", e11Order },
				{ "identities", { { "ownerActorId", beamOwner->GetUniqueID() },
					{ "firstAbilityId", firstBeamDefinition.abilityId }, { "firstAbilityHandle", firstBeamHandle.id },
					{ "secondAbilityId", secondBeamDefinition.abilityId }, { "secondAbilityHandle", secondBeamHandle.id },
					{ "firstBeamActorId", ownedBeamActors[0]->GetUniqueID() },
					{ "siblingBeamActorId", ownedBeamActors[1]->GetUniqueID() },
					{ "independentAbilityBeamActorId", ownedBeamActors[2]->GetUniqueID() },
					{ "reuseAbilityId", reuseDefinition.abilityId }, { "reuseAbilityHandle", reuseHandle.id },
					{ "returnProtocolAbilityId", returnDefinition->abilityId }, { "returnProtocolHandle", returnHandle.id },
					{ "echoProtocolAbilityId", echoDefinition->abilityId }, { "echoProtocolHandle", echoHandle.id },
					{ "firstReturnRecordAbilityHandle", firstReturnRecord.abilityHandle.id },
					{ "firstReturnRecordSequence", firstReturnRecord.sequence },
					{ "firstInvocationVisualId", firstInvocationVisualId },
					{ "returnReuseHandle", returnReuseHandle.id }, { "echoReuseHandle", echoReuseHandle.id },
					{ "reuseRecordAbilityHandle", reuseReturnRecord.abilityHandle.id },
					{ "reuseRecordSequence", reuseReturnRecord.sequence },
					{ "reuseInvocationVisualId", reusedInvocationVisuals.empty() ? 0 : reusedInvocationVisuals.front()->GetUniqueID() } } },
				{ "before", { { "abilities", abilitiesBeforeE11 }, { "activeEffects", activeEffectsBeforeE11 },
					{ "liveBeams", liveBeamsBeforeActivation }, { "invocationAbilities", abilitiesBeforeInvocation },
					{ "invocationEffects", activeEffectsBeforeInvocation },
					{ "activeInvocations", activeInvocationsBefore }, { "liveReturnProtocolVisuals", liveReturnVisualsBefore } } },
				{ "after", { { "abilities", beamSystem.BuildAbilitySnapshots().size() },
					{ "activeEffects", beamSystem.BuildGameplayEffectSnapshots().size() },
					{ "liveBeams", liveBeamsAfterE11 },
					{ "activeInvocations", beamSystem.GetActiveAbilityInvocationCount() },
					{ "liveReturnProtocolVisuals", liveReturnVisualsAfterInvocationReuse },
					{ "firstExecutionVisualsPendingDestroy", allFirstExecutionVisualsCleaned },
					{ "firstAbilityOnEndCount", firstBeamActionAudit.endCount },
					{ "secondAbilityOnEndCount", secondBeamActionAudit.endCount } } },
				{ "errors", { firstEndFireError, retryEndFireError, reuseClearError, firstInvocationClearError,
					invocationRetryClearError, reuseInvocationClearError } }, { "passed", e11Passed } });
			check("E11ClearContinuesAndRetriesFireWeaponCleanup", e11Passed);

			auto findPlayerAbilitySnapshot = [](PlayerSpaceShip& ship, const std::string& abilityId)
			{
				const auto snapshots = ship.GetAbilitySystemComponent().BuildAbilitySnapshots();
				const auto found = std::find_if(snapshots.begin(), snapshots.end(), [&](const sas::AbilityRuntimeSnapshot& snapshot)
				{
					return snapshot.abilityId == abilityId;
				});
				return found == snapshots.end() ? sas::AbilityRuntimeSnapshot{} : *found;
			};
			auto spawnAuditPlayerShip = [&](Player& player, float x)
			{
				auto ship = player.SpawnSpaceShip(&world).lock();
				if (!ship) throw std::runtime_error("E12/E13 production Player ship spawn failed");
				ship->SetActorLocation({ x, 8000.f });
				world.TickInternal(0.f);
				return ship;
			};
			auto respawnAuditPlayerShip = [&](Player& player, const shared_ptr<PlayerSpaceShip>& oldShip, float x)
			{
				oldShip->Destroy();
				world.TickInternal(0.f);
				return spawnAuditPlayerShip(player, x);
			};
			const auto playerPurchaseSlot = sas::AbilitySlot::Ability4;
			const std::string playerPurchaseAbilityId = AbilityData::Cryostasis::AbilityId::Basic;
			std::vector<std::string> e12Order;

			Player normalPurchasePlayer;
			normalPurchasePlayer.AwardScrap(500);
			auto normalPurchaseShip = spawnAuditPlayerShip(normalPurchasePlayer, 9000.f);
			auto& normalPurchaseSystem = normalPurchaseShip->GetAbilitySystemComponent();
			const std::size_t normalPurchaseAbilitiesBefore = normalPurchaseSystem.BuildAbilitySnapshots().size();
			GameAbility* normalPurchaseAbility = normalPurchaseSystem.FindAbility<GameAbility>(playerPurchaseSlot);
			if (!normalPurchaseAbility || normalPurchaseAbility->GetDefinition().abilityId != playerPurchaseAbilityId)
				throw std::runtime_error("E12 Player default loadout is missing Cryostasis in Ability4");
			const unsigned int normalPurchaseCost = normalPurchaseAbility->GetDefinition().GetScrapCostToReachLevel(2);
			const unsigned int normalPurchaseScrapBefore = normalPurchasePlayer.GetScrap();
			const int normalPurchaseLevelBefore = normalPurchaseAbility->GetLevel();
			const std::uint64_t normalPurchaseHandle = normalPurchaseAbility->GetHandle().id;
			std::string normalPurchaseFailure;
			e12Order.push_back("normal-purchase");
			const bool normalPurchaseResult = normalPurchasePlayer.TryPurchaseAbilityLevel(playerPurchaseSlot, &normalPurchaseFailure);
			const unsigned int normalPurchaseScrapAfter = normalPurchasePlayer.GetScrap();
			const int normalPurchaseLevelAfter = findPlayerAbilitySnapshot(*normalPurchaseShip, playerPurchaseAbilityId).level;
			auto normalRespawnShip = respawnAuditPlayerShip(normalPurchasePlayer, normalPurchaseShip, 9100.f);
			const auto normalRespawnSnapshot = findPlayerAbilitySnapshot(*normalRespawnShip, playerPurchaseAbilityId);
			const bool normalPurchaseRespawned = normalRespawnSnapshot.abilityId == playerPurchaseAbilityId;
			const int normalPurchaseRespawnLevel = normalPurchaseRespawned ? normalRespawnSnapshot.level : -1;
			const std::uint64_t normalRespawnHandle = normalPurchaseRespawned ? normalRespawnSnapshot.handle.id : 0;
			const bool normalPurchasePassed = normalPurchaseResult && normalPurchaseCost > 0 &&
				normalPurchaseLevelBefore == 1 && normalPurchaseLevelAfter == 2 && normalPurchaseRespawned &&
				normalPurchaseRespawnLevel == 2 && normalPurchaseScrapAfter == normalPurchaseScrapBefore - normalPurchaseCost;

			Player postcommitPurchasePlayer;
			postcommitPurchasePlayer.AwardScrap(500);
			auto postcommitPurchaseShip = spawnAuditPlayerShip(postcommitPurchasePlayer, 9200.f);
			auto& postcommitPurchaseSystem = postcommitPurchaseShip->GetAbilitySystemComponent();
			const std::size_t postcommitAbilitiesBefore = postcommitPurchaseSystem.BuildAbilitySnapshots().size();
			GameAbility* postcommitPurchaseAbility = postcommitPurchaseSystem.FindAbility<GameAbility>(playerPurchaseSlot);
			if (!postcommitPurchaseAbility || postcommitPurchaseAbility->GetDefinition().abilityId != playerPurchaseAbilityId)
				throw std::runtime_error("E12 postcommit Player default loadout is missing Cryostasis");
			const unsigned int postcommitPurchaseCost = postcommitPurchaseAbility->GetDefinition().GetScrapCostToReachLevel(2);
			const unsigned int postcommitPurchaseScrapBefore = postcommitPurchasePlayer.GetScrap();
			const std::uint64_t postcommitPurchaseHandle = postcommitPurchaseAbility->GetHandle().id;
			std::vector<std::string> postcommitOrder;
			PlayerPurchaseLevelAudit postcommitPurchaseAudit{
				postcommitPurchasePlayer, postcommitPurchaseSystem, playerPurchaseSlot, 2, postcommitOrder
			};
			postcommitPurchaseAudit.throwMessage = "Injected E12 postcommit level observer failure";
			const auto postcommitLevelToken = postcommitPurchaseSystem.onAbilityLevelChanged.BindAction(
				&postcommitPurchaseAudit, &PlayerPurchaseLevelAudit::Changed
			);
			std::string postcommitPurchaseError;
			e12Order.push_back("postcommit-purchase");
			try { (void)postcommitPurchasePlayer.TryPurchaseAbilityLevel(playerPurchaseSlot); }
			catch (const std::runtime_error& error) { postcommitPurchaseError = error.what(); }
			postcommitPurchaseSystem.onAbilityLevelChanged.UnbindAction(postcommitLevelToken);
			const unsigned int postcommitPurchaseScrapAfter = postcommitPurchasePlayer.GetScrap();
			const std::size_t postcommitAbilitiesAfterClear = postcommitPurchaseSystem.BuildAbilitySnapshots().size();
			auto postcommitRespawnShip = respawnAuditPlayerShip(postcommitPurchasePlayer, postcommitPurchaseShip, 9300.f);
			const auto postcommitRespawnSnapshot = findPlayerAbilitySnapshot(*postcommitRespawnShip, playerPurchaseAbilityId);
			const bool postcommitRespawned = postcommitRespawnSnapshot.abilityId == playerPurchaseAbilityId;
			const int postcommitRespawnLevel = postcommitRespawned ? postcommitRespawnSnapshot.level : -1;
			const std::uint64_t postcommitRespawnHandle = postcommitRespawned ? postcommitRespawnSnapshot.handle.id : 0;
			const bool postcommitPurchasePassed = postcommitPurchaseAudit.visited && postcommitPurchaseAudit.changedLevel == 2 &&
				postcommitPurchaseAudit.changedHandle == postcommitPurchaseHandle &&
				postcommitPurchaseAudit.scrapAtCallback == postcommitPurchaseScrapBefore - postcommitPurchaseCost &&
				!postcommitPurchaseAudit.clearThrew && postcommitPurchaseError == postcommitPurchaseAudit.throwMessage &&
				postcommitPurchaseCost > 0 && postcommitPurchaseScrapAfter == postcommitPurchaseScrapBefore - postcommitPurchaseCost &&
				postcommitAbilitiesAfterClear == 0 && postcommitRespawned && postcommitRespawnLevel == 2;

			Player precommitPurchasePlayer;
			precommitPurchasePlayer.AwardScrap(500);
			auto precommitPurchaseShip = spawnAuditPlayerShip(precommitPurchasePlayer, 9400.f);
			auto& precommitPurchaseSystem = precommitPurchaseShip->GetAbilitySystemComponent();
			const std::size_t precommitAbilitiesBefore = precommitPurchaseSystem.BuildAbilitySnapshots().size();
			GameAbility* precommitPurchaseAbility = precommitPurchaseSystem.FindAbility<GameAbility>(playerPurchaseSlot);
			if (!precommitPurchaseAbility || precommitPurchaseAbility->GetDefinition().abilityId != playerPurchaseAbilityId)
				throw std::runtime_error("E12 precommit Player default loadout is missing Cryostasis");
			const unsigned int precommitPurchaseCost = precommitPurchaseAbility->GetDefinition().GetScrapCostToReachLevel(2);
			const unsigned int precommitPurchaseScrapBefore = precommitPurchasePlayer.GetScrap();
			const std::uint64_t precommitPurchaseHandle = precommitPurchaseAbility->GetHandle().id;
			precommitPurchaseSystem.SetAbilitySlotInput(playerPurchaseSlot, true);
			precommitPurchaseSystem.Tick(0.01f);
			precommitPurchaseSystem.SetAbilitySlotInput(playerPurchaseSlot, false);
			precommitPurchaseSystem.Tick(0.f);
			const bool precommitAbilityWasActive = precommitPurchaseAbility->IsActive();
			std::vector<std::string> precommitOrder;
			PlayerPurchaseEndAudit precommitPurchaseAudit{
				precommitPurchaseSystem, AbilityData::Cryostasis::Event::Ended, precommitOrder
			};
			precommitPurchaseAudit.throwMessage = "Injected E12 precommit end observer failure";
			const auto precommitEventToken = precommitPurchaseSystem.onGameplayEvent.BindAction(
				&precommitPurchaseAudit, &PlayerPurchaseEndAudit::Event
			);
			std::string precommitPurchaseError;
			e12Order.push_back("active-precommit-purchase");
			try { (void)precommitPurchasePlayer.TryPurchaseAbilityLevel(playerPurchaseSlot); }
			catch (const std::runtime_error& error) { precommitPurchaseError = error.what(); }
			precommitPurchaseSystem.onGameplayEvent.UnbindAction(precommitEventToken);
			std::string precommitRetryClearError;
			try { precommitPurchaseSystem.Clear(); }
			catch (const std::runtime_error& error) { precommitRetryClearError = error.what(); }
			const unsigned int precommitPurchaseScrapAfter = precommitPurchasePlayer.GetScrap();
			const std::size_t precommitAbilitiesAfterClear = precommitPurchaseSystem.BuildAbilitySnapshots().size();
			auto precommitRespawnShip = respawnAuditPlayerShip(precommitPurchasePlayer, precommitPurchaseShip, 9500.f);
			const auto precommitRespawnSnapshot = findPlayerAbilitySnapshot(*precommitRespawnShip, playerPurchaseAbilityId);
			const bool precommitRespawned = precommitRespawnSnapshot.abilityId == playerPurchaseAbilityId;
			const int precommitRespawnLevel = precommitRespawned ? precommitRespawnSnapshot.level : -1;
			const std::uint64_t precommitRespawnHandle = precommitRespawned ? precommitRespawnSnapshot.handle.id : 0;
			const bool precommitPurchasePassed = precommitAbilityWasActive && precommitPurchaseAudit.endedCount > 0 &&
				!precommitPurchaseAudit.clearThrew && precommitPurchaseError == precommitPurchaseAudit.throwMessage && precommitPurchaseCost > 0 &&
				precommitPurchaseScrapAfter == precommitPurchaseScrapBefore && precommitRetryClearError.empty() &&
				precommitAbilitiesAfterClear == 0 && precommitRespawned && precommitRespawnLevel == 1;
			const bool e12Passed = normalPurchasePassed && postcommitPurchasePassed && precommitPurchasePassed;
			nlohmann::json e12Actual = {
				{ "normalPurchase", { { "result", normalPurchaseResult }, { "failureReason", normalPurchaseFailure },
					{ "levelBefore", normalPurchaseLevelBefore }, { "levelAfter", normalPurchaseLevelAfter },
					{ "cost", normalPurchaseCost }, { "scrapBefore", normalPurchaseScrapBefore },
					{ "scrapAfter", normalPurchaseScrapAfter }, { "respawnLevel", normalPurchaseRespawnLevel },
					{ "respawned", normalPurchaseRespawned }, { "respawnActorId", normalRespawnShip->GetUniqueID() } } },
				{ "postcommitClearThrow", { { "observerVisited", postcommitPurchaseAudit.visited },
					{ "levelSeen", postcommitPurchaseAudit.changedLevel }, { "levelHandle", postcommitPurchaseAudit.changedHandle },
					{ "scrapAtObserver", postcommitPurchaseAudit.scrapAtCallback }, { "cost", postcommitPurchaseCost },
					{ "scrapBefore", postcommitPurchaseScrapBefore }, { "scrapAfter", postcommitPurchaseScrapAfter },
					{ "clearThrew", postcommitPurchaseAudit.clearThrew }, { "clearError", postcommitPurchaseAudit.clearError },
					{ "observerError", postcommitPurchaseError }, { "abilitiesBefore", postcommitAbilitiesBefore },
					{ "abilitiesAfterClear", postcommitAbilitiesAfterClear }, { "order", postcommitOrder },
					{ "respawned", postcommitRespawned }, { "respawnLevel", postcommitRespawnLevel },
					{ "respawnActorId", postcommitRespawnShip->GetUniqueID() } } },
				{ "precommitClearThrow", { { "activeBeforePurchase", precommitAbilityWasActive },
					{ "endedCallbacks", precommitPurchaseAudit.endedCount }, { "cost", precommitPurchaseCost },
					{ "scrapBefore", precommitPurchaseScrapBefore }, { "scrapAfter", precommitPurchaseScrapAfter },
					{ "clearThrew", precommitPurchaseAudit.clearThrew }, { "clearError", precommitPurchaseAudit.clearError },
					{ "purchaseError", precommitPurchaseError }, { "retryClearError", precommitRetryClearError },
					{ "abilitiesBefore", precommitAbilitiesBefore }, { "abilitiesAfterClear", precommitAbilitiesAfterClear },
					{ "order", precommitOrder }, { "respawned", precommitRespawned },
					{ "respawnLevel", precommitRespawnLevel }, { "respawnActorId", precommitRespawnShip->GetUniqueID() } } },
				{ "subcasePasses", { { "normalPurchaseAndRespawn", normalPurchasePassed },
					{ "postcommitObserverClearThrow", postcommitPurchasePassed },
					{ "precommitActiveEndClearThrow", precommitPurchasePassed } } }
			};
			recordCase("abilityCases", { { "case_id", "E12" },
				{ "input", { { "purchaseSlot", "Ability4" }, { "abilityId", playerPurchaseAbilityId },
					{ "paths", { "Player::TryPurchaseAbilityLevel", "Player::SpawnSpaceShip respawn" } },
					{ "faults", { "active End event clears before commit and throws once", "level observer clears after commit and throws" } } } },
				{ "expected", "successful purchase charges once and survives Player ship respawn; precommit End failure does not charge or persist; postcommit Clear/throw preserves the charge and purchased level" },
				{ "actual", e12Actual }, { "order", e12Order },
				{ "identities", { { "abilityId", playerPurchaseAbilityId }, { "slot", "Ability4" },
					{ "normalShipActorId", normalPurchaseShip->GetUniqueID() }, { "normalAbilityHandle", normalPurchaseHandle },
					{ "normalRespawnAbilityHandle", normalRespawnHandle },
					{ "postcommitShipActorId", postcommitPurchaseShip->GetUniqueID() },
					{ "postcommitAbilityHandle", postcommitPurchaseHandle }, { "postcommitObserverHandle", postcommitPurchaseAudit.changedHandle },
					{ "postcommitRespawnAbilityHandle", postcommitRespawnHandle },
					{ "precommitShipActorId", precommitPurchaseShip->GetUniqueID() },
					{ "precommitAbilityHandle", precommitPurchaseHandle }, { "precommitRespawnAbilityHandle", precommitRespawnHandle } } },
				{ "before", { { "normalPlayerScrap", normalPurchaseScrapBefore }, { "normalAbilityLevel", normalPurchaseLevelBefore },
					{ "normalAbilityCount", normalPurchaseAbilitiesBefore },
					{ "postcommitPlayerScrap", postcommitPurchaseScrapBefore }, { "precommitPlayerScrap", precommitPurchaseScrapBefore },
					{ "precommitAbilityActive", precommitAbilityWasActive } } },
				{ "after", { { "normalPlayerScrap", normalPurchaseScrapAfter }, { "normalRespawnLevel", normalPurchaseRespawnLevel },
					{ "postcommitPlayerScrap", postcommitPurchaseScrapAfter }, { "postcommitRespawnLevel", postcommitRespawnLevel },
					{ "precommitPlayerScrap", precommitPurchaseScrapAfter }, { "precommitRespawnLevel", precommitRespawnLevel } } },
				{ "errors", { postcommitPurchaseError, precommitPurchaseError, precommitRetryClearError } },
				{ "passed", e12Passed } });
			check("E12PlayerPurchaseCommitAndRespawn", e12Passed);

			Player nestedPurchasePlayer;
			nestedPurchasePlayer.AwardScrap(500);
			auto nestedPurchaseShip = spawnAuditPlayerShip(nestedPurchasePlayer, 9600.f);
			auto& nestedPurchaseSystem = nestedPurchaseShip->GetAbilitySystemComponent();
			const std::size_t nestedPurchaseAbilitiesBefore = nestedPurchaseSystem.BuildAbilitySnapshots().size();
			GameAbility* nestedPurchaseAbility = nestedPurchaseSystem.FindAbility<GameAbility>(playerPurchaseSlot);
			if (!nestedPurchaseAbility || nestedPurchaseAbility->GetDefinition().abilityId != playerPurchaseAbilityId)
				throw std::runtime_error("E13 Player default loadout is missing Cryostasis");
			const unsigned int nestedPurchaseCost = nestedPurchaseAbility->GetDefinition().GetScrapCostToReachLevel(2);
			const unsigned int nestedPurchaseScrapBefore = nestedPurchasePlayer.GetScrap();
			const int nestedPurchaseLevelBefore = nestedPurchaseAbility->GetLevel();
			const std::uint64_t nestedPurchaseHandle = nestedPurchaseAbility->GetHandle().id;
			std::vector<std::string> e13Order;
			PlayerPurchaseLevelAudit nestedPurchaseAudit{
				nestedPurchasePlayer, nestedPurchaseSystem, playerPurchaseSlot, 2, e13Order
			};
			nestedPurchaseAudit.attemptNestedPurchase = true;
			nestedPurchaseAudit.throwMessage = "Injected E13 nested-purchase observer failure";
			const auto nestedPurchaseToken = nestedPurchaseSystem.onAbilityLevelChanged.BindAction(
				&nestedPurchaseAudit, &PlayerPurchaseLevelAudit::Changed
			);
			std::string nestedPurchaseError;
			e13Order.push_back("outer-purchase");
			try { (void)nestedPurchasePlayer.TryPurchaseAbilityLevel(playerPurchaseSlot); }
			catch (const std::runtime_error& error) { nestedPurchaseError = error.what(); }
			nestedPurchaseSystem.onAbilityLevelChanged.UnbindAction(nestedPurchaseToken);
			const unsigned int nestedPurchaseScrapAfter = nestedPurchasePlayer.GetScrap();
			const std::size_t nestedAbilitiesAfterClear = nestedPurchaseSystem.BuildAbilitySnapshots().size();
			auto nestedRespawnShip = respawnAuditPlayerShip(nestedPurchasePlayer, nestedPurchaseShip, 9700.f);
			const auto nestedRespawnSnapshots = nestedRespawnShip->GetAbilitySystemComponent().BuildAbilitySnapshots();
			const auto nestedRespawnSnapshot = std::find_if(nestedRespawnSnapshots.begin(), nestedRespawnSnapshots.end(),
				[&](const sas::AbilityRuntimeSnapshot& snapshot) { return snapshot.abilityId == playerPurchaseAbilityId; });
			const bool nestedRespawned = nestedRespawnSnapshot != nestedRespawnSnapshots.end();
			const int nestedRespawnLevel = nestedRespawned ? nestedRespawnSnapshot->level : -1;
			const std::uint64_t nestedRespawnHandle = nestedRespawned ? nestedRespawnSnapshot->handle.id : 0;
			const bool e13Passed = nestedPurchaseCost > 0 && nestedPurchaseAudit.visited &&
				nestedPurchaseAudit.changedHandle == nestedPurchaseHandle && nestedPurchaseAudit.changedLevel == 2 &&
				nestedPurchaseAudit.scrapAtCallback == nestedPurchaseScrapBefore - nestedPurchaseCost &&
				!nestedPurchaseAudit.nestedPurchaseResult &&
				nestedPurchaseAudit.nestedFailureReason == "Another ability purchase is already in progress." &&
				nestedPurchaseAudit.scrapAfterNestedPurchase == nestedPurchaseAudit.scrapAtCallback &&
				!nestedPurchaseAudit.clearThrew && nestedPurchaseError == nestedPurchaseAudit.throwMessage &&
				nestedPurchaseScrapAfter == nestedPurchaseScrapBefore - nestedPurchaseCost &&
				nestedAbilitiesAfterClear == 0 && nestedRespawned && nestedRespawnLevel == 2;
			nlohmann::json e13Actual = {
				{ "observerVisited", nestedPurchaseAudit.visited }, { "levelSeen", nestedPurchaseAudit.changedLevel },
				{ "levelHandle", nestedPurchaseAudit.changedHandle }, { "cost", nestedPurchaseCost },
				{ "scrapBefore", nestedPurchaseScrapBefore }, { "scrapAtObserver", nestedPurchaseAudit.scrapAtCallback },
				{ "nestedPurchaseResult", nestedPurchaseAudit.nestedPurchaseResult },
				{ "nestedFailureReason", nestedPurchaseAudit.nestedFailureReason },
				{ "scrapAfterNestedPurchase", nestedPurchaseAudit.scrapAfterNestedPurchase },
				{ "scrapAfterOuterPurchase", nestedPurchaseScrapAfter }, { "clearThrew", nestedPurchaseAudit.clearThrew },
				{ "clearError", nestedPurchaseAudit.clearError }, { "observerError", nestedPurchaseError },
				{ "abilitiesAfterClear", nestedAbilitiesAfterClear }, { "respawned", nestedRespawned },
				{ "respawnLevel", nestedRespawnLevel }, { "respawnActorId", nestedRespawnShip->GetUniqueID() }
			};
			recordCase("abilityCases", { { "case_id", "E13" },
				{ "input", { { "purchaseSlot", "Ability4" }, { "abilityId", playerPurchaseAbilityId },
					{ "outerPurchase", "Player::TryPurchaseAbilityLevel" }, { "observer", "nested same-slot purchase, Clear, then throw" } } },
				{ "expected", "nested purchase is refused without a second charge; the committed outer purchase remains stored across Player ship respawn despite Clear and observer throw" },
				{ "actual", e13Actual }, { "order", e13Order },
				{ "identities", { { "abilityId", playerPurchaseAbilityId }, { "slot", "Ability4" },
					{ "shipActorId", nestedPurchaseShip->GetUniqueID() }, { "abilityHandle", nestedPurchaseHandle },
					{ "observerHandle", nestedPurchaseAudit.changedHandle }, { "respawnActorId", nestedRespawnShip->GetUniqueID() },
					{ "respawnAbilityHandle", nestedRespawnHandle } } },
				{ "before", { { "playerScrap", nestedPurchaseScrapBefore }, { "abilityLevel", nestedPurchaseLevelBefore },
					{ "abilityCount", nestedPurchaseAbilitiesBefore } } },
				{ "after", { { "playerScrap", nestedPurchaseScrapAfter }, { "nestedPurchaseScrap", nestedPurchaseAudit.scrapAfterNestedPurchase },
					{ "abilitiesAfterClear", nestedAbilitiesAfterClear }, { "respawnLevel", nestedRespawnLevel } } },
				{ "errors", { nestedPurchaseError } }, { "passed", e13Passed } });
			check("E13NestedPlayerPurchaseCannotDoubleCharge", e13Passed);

			// Shipped Cryostasis Ice Shell effect events carry the live damage
			// context. Re-entering A's own CombatRuntime through B must restore A's
			// suspended frame before its normal DamageReceived event.
			const auto* iceShellDefinition = EffectData::FindGameplayEffectDefinition(
				AbilityData::Cryostasis::Effect::IceShellId
			);
			if (!iceShellDefinition) throw std::runtime_error("E14 shipped Cryostasis Ice Shell definition is missing");
			auto damageFirst = spawn(8200.f, CollisionLayer::Player);
			auto damageMiddle = spawn(8260.f, CollisionLayer::Enemy);
			world.TickInternal(0.f);
			const std::size_t e14EffectsBeforeA = damageFirst->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			const std::size_t e14EffectsBeforeB = damageMiddle->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			const float e14IceShellDuration = 5.f;
			const auto outerIceShell = ApplyAuditIceShell(*damageFirst, *iceShellDefinition, 5.f, e14IceShellDuration);
			const std::size_t e14EffectsAfterSetup = damageFirst->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			result["diagnostics"]["e14IceShellSetup"] = {
				{ "handle", outerIceShell.id }, { "valid", outerIceShell.IsValid() },
				{ "duration", e14IceShellDuration }, { "effectsBefore", e14EffectsBeforeA },
				{ "effectsAfterSetup", e14EffectsAfterSetup }
			};
			check("E14OuterIceShellApplied", outerIceShell.IsValid() && e14EffectsAfterSetup == e14EffectsBeforeA + 1);
			NestedDamageFrameAudit nestedDamageAudit{ *damageFirst, *damageMiddle, *iceShellDefinition };
			const auto firstDamageEventToken = damageFirst->GetAbilitySystemComponent().onGameplayEvent.BindAction(
				&nestedDamageAudit, &NestedDamageFrameAudit::FirstEvent
			);
			const auto middleDamageEventToken = damageMiddle->GetAbilitySystemComponent().onGameplayEvent.BindAction(
				&nestedDamageAudit, &NestedDamageFrameAudit::MiddleEvent
			);
			DamageContext outerDamage;
			outerDamage.source = damageMiddle.get();
			outerDamage.target = damageFirst.get();
			outerDamage.originalDamage = 10.f;
			outerDamage.remainingDamage = 10.f;
			damageFirst->ReceiveDamage(outerDamage);
			const std::size_t e14EffectsAfterA = damageFirst->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			const std::size_t e14EffectsAfterB = damageMiddle->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			damageFirst->GetAbilitySystemComponent().onGameplayEvent.UnbindAction(firstDamageEventToken);
			damageMiddle->GetAbilitySystemComponent().onGameplayEvent.UnbindAction(middleDamageEventToken);
			const bool e14Passed = outerIceShell.IsValid() && nestedDamageAudit.nestedShellApplied &&
				nestedDamageAudit.iceBrokenCount == 2 && nestedDamageAudit.middleDamageReceivedCount == 1 &&
				nestedDamageAudit.middleContextValid && nestedDamageAudit.innerContextValid &&
				nestedDamageAudit.outerFrameRestoredAfterNested && nestedDamageAudit.outerDamageReceivedMatches &&
				nestedDamageAudit.sequence == std::vector<std::string>{ "A", "B", "A" } &&
				nestedDamageAudit.sourceIds == std::vector<std::uint64_t>{ damageMiddle->GetUniqueID(),
					damageFirst->GetUniqueID(), damageMiddle->GetUniqueID() } &&
				nestedDamageAudit.targetIds == std::vector<std::uint64_t>{ damageFirst->GetUniqueID(),
					damageMiddle->GetUniqueID(), damageFirst->GetUniqueID() } &&
				nestedDamageAudit.originalDamages == std::vector<float>{ 10.f, 20.f, 30.f } &&
				e14EffectsAfterA == 0 && e14EffectsAfterB == e14EffectsBeforeB;
			nlohmann::json e14Actual = {
				{ "iceShellHandle", outerIceShell.id },
				{ "iceBrokenCount", nestedDamageAudit.iceBrokenCount },
				{ "middleDamageReceivedCount", nestedDamageAudit.middleDamageReceivedCount },
				{ "typedMiddleContextValid", nestedDamageAudit.middleContextValid },
				{ "typedInnerContextValid", nestedDamageAudit.innerContextValid },
				{ "nestedIceShellApplied", nestedDamageAudit.nestedShellApplied },
				{ "outerFrameRestoredAfterNested", nestedDamageAudit.outerFrameRestoredAfterNested },
				{ "outerDamageReceivedMatches", nestedDamageAudit.outerDamageReceivedMatches },
				{ "sequence", nestedDamageAudit.sequence },
				{ "sourceActorIds", nestedDamageAudit.sourceIds },
				{ "targetActorIds", nestedDamageAudit.targetIds },
				{ "originalDamages", nestedDamageAudit.originalDamages }
			};
			recordCase("damageCases", { { "case_id", "E14" },
				{ "input", { { "outerTarget", "A" }, { "middleTarget", "B" }, { "outerDamage", 10.f },
					{ "middleDamage", 20.f }, { "innerDamage", 30.f }, { "shippedEffect", "Cryostasis Ice Shell" },
					{ "iceShellDuration", e14IceShellDuration } } },
				{ "expected", "Typed context delivery through the shipped IceBroken event nests damage on the same A runtime and restores outer context in A→B→A order" },
				{ "actual", e14Actual }, { "order", nestedDamageAudit.sequence },
				{ "identities", { { "A", damageFirst->GetUniqueID() }, { "B", damageMiddle->GetUniqueID() },
					{ "iceShellEffectId", iceShellDefinition->effectId } } },
				{ "before", { { "AActiveEffects", e14EffectsBeforeA }, { "BActiveEffects", e14EffectsBeforeB } } },
				{ "after", { { "AActiveEffects", e14EffectsAfterA }, { "BActiveEffects", e14EffectsAfterB } } },
				{ "errors", nlohmann::json::array() }, { "passed", e14Passed } });
			check("E14NestedDamageRestoresAContext", e14Passed);

			// The shipped barrier-break event leaves residual damage. Clearing from
			// its synchronous callback must invalidate the old frame before its
			// DamageReceived, Thermal/Ignite, source-status, or processed callbacks.
			const auto* barrierDefinition = EffectData::FindGameplayEffectDefinition(BarrierEffectSchema::BasicEffectId);
			if (!barrierDefinition) throw std::runtime_error("E15 shipped Basic Barrier definition is missing");
			auto controlDamageTarget = spawn(8300.f, CollisionLayer::Enemy);
			auto controlDamageSource = spawn(8360.f, CollisionLayer::Player);
			world.TickInternal(0.f);
			const float e15BarrierDuration = 5.f;
			const std::size_t controlEffectsBeforeSetup = controlDamageTarget->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			const auto controlBarrier = ApplyAuditBarrier(*controlDamageTarget, *barrierDefinition, 5.f, e15BarrierDuration);
			const std::size_t controlEffectsAfterSetup = controlDamageTarget->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			result["diagnostics"]["e15NormalControlBarrierSetup"] = {
				{ "handle", controlBarrier.id }, { "valid", controlBarrier.IsValid() },
				{ "duration", e15BarrierDuration }, { "effectsBefore", controlEffectsBeforeSetup },
				{ "effectsAfterSetup", controlEffectsAfterSetup }
			};
			check("E15NormalControlBarrierApplied", controlBarrier.IsValid() &&
				controlEffectsAfterSetup == controlEffectsBeforeSetup + 1);
			DamageFlowProbe normalDamageControl{ *controlDamageTarget, *controlDamageSource };
			const auto controlTargetEventToken = controlDamageTarget->GetAbilitySystemComponent().onGameplayEvent.BindAction(
				&normalDamageControl, &DamageFlowProbe::TargetEvent
			);
			const auto controlSourceEventToken = controlDamageSource->GetAbilitySystemComponent().onGameplayEvent.BindAction(
				&normalDamageControl, &DamageFlowProbe::SourceEvent
			);
			const auto controlProcessedToken = controlDamageTarget->GetCombatRuntime().onDamageProcessed.BindAction(
				&normalDamageControl, &DamageFlowProbe::DamageProcessed
			);
			DamageContext controlDamage;
			controlDamage.source = controlDamageSource.get();
			controlDamage.target = controlDamageTarget.get();
			controlDamage.originalDamage = 20.f;
			controlDamage.remainingDamage = 20.f;
			controlDamage.damageTags.push_back(DamageTypeSchema::Thermal);
			controlDamage.payload.igniteStacks = 1;
			controlDamageTarget->ReceiveDamage(controlDamage);
			const std::size_t e15ControlEffectsAfter = controlDamageTarget->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			const bool e15ControlIgniteTag = controlDamageTarget->GetAbilitySystemComponent().HasOwnedTag(DamageStatusSchema::Ignite);
			controlDamageTarget->GetAbilitySystemComponent().onGameplayEvent.UnbindAction(controlTargetEventToken);
			controlDamageSource->GetAbilitySystemComponent().onGameplayEvent.UnbindAction(controlSourceEventToken);
			controlDamageTarget->GetCombatRuntime().onDamageProcessed.UnbindAction(controlProcessedToken);
			const bool e15ControlPassed = controlBarrier.IsValid() && normalDamageControl.breakEvents == 1 &&
				normalDamageControl.damageReceivedEvents == 1 && normalDamageControl.damageReceivedContextValid &&
				normalDamageControl.remainingDamageAtReceived == 15.f && normalDamageControl.sourceStatusEvents == 1 &&
				normalDamageControl.damageProcessedEvents == 1 && e15ControlIgniteTag && e15ControlEffectsAfter > 0;
			check("E15BarrierNormalControlReachesLateCallbacks", e15ControlPassed);
			auto clearDamageTarget = spawn(8400.f, CollisionLayer::Enemy);
			auto clearDamageSource = spawn(8460.f, CollisionLayer::Player);
			world.TickInternal(0.f);
			const std::size_t e15EffectsBeforeSetup = clearDamageTarget->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			const auto e15Barrier = ApplyAuditBarrier(*clearDamageTarget, *barrierDefinition, 5.f, e15BarrierDuration);
			const std::size_t e15EffectsBefore = clearDamageTarget->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			result["diagnostics"]["e15ClearBarrierSetup"] = {
				{ "handle", e15Barrier.id }, { "valid", e15Barrier.IsValid() },
				{ "duration", e15BarrierDuration }, { "effectsBefore", e15EffectsBeforeSetup },
				{ "effectsAfterSetup", e15EffectsBefore }
			};
			check("E15ClearBarrierApplied", e15Barrier.IsValid() && e15EffectsBefore == e15EffectsBeforeSetup + 1);
			const std::size_t e15SourceEffectsBefore = clearDamageSource->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			std::vector<std::string> e15Order;
			ClearDamageEventAudit clearDamageAudit{ *clearDamageTarget, e15Order };
			const auto clearDamageEventToken = clearDamageTarget->GetAbilitySystemComponent().onGameplayEvent.BindAction(
				&clearDamageAudit, &ClearDamageEventAudit::TargetEvent
			);
			const auto sourceStatusEventToken = clearDamageSource->GetAbilitySystemComponent().onGameplayEvent.BindAction(
				&clearDamageAudit, &ClearDamageEventAudit::SourceEvent
			);
			const auto damageProcessedToken = clearDamageTarget->GetCombatRuntime().onDamageProcessed.BindAction(
				&clearDamageAudit, &ClearDamageEventAudit::DamageProcessed
			);
			DamageContext clearDuringDamage;
			clearDuringDamage.source = clearDamageSource.get();
			clearDuringDamage.target = clearDamageTarget.get();
			clearDuringDamage.originalDamage = 20.f;
			clearDuringDamage.remainingDamage = 20.f;
			clearDuringDamage.damageTags.push_back(DamageTypeSchema::Thermal);
			clearDuringDamage.payload.igniteStacks = 1;
			std::string e15Error;
			try { clearDamageTarget->ReceiveDamage(clearDuringDamage); }
			catch (const std::runtime_error& error) { e15Error = error.what(); }
			const std::size_t breakEventsAfterThrow = clearDamageAudit.breakEvents;
			const std::size_t damageReceivedAfterThrow = clearDamageAudit.damageReceivedEvents;
			const std::size_t sourceStatusAfterThrow = clearDamageAudit.sourceStatusEvents;
			const std::size_t damageProcessedAfterThrow = clearDamageAudit.damageProcessedEvents;
			world.TickInternal(0.01f);
			const std::size_t breakEventsAfterTick = clearDamageAudit.breakEvents;
			const std::size_t damageReceivedAfterTick = clearDamageAudit.damageReceivedEvents;
			const std::size_t sourceStatusAfterTick = clearDamageAudit.sourceStatusEvents;
			const std::size_t damageProcessedAfterTick = clearDamageAudit.damageProcessedEvents;
			const std::size_t e15EffectsAfter = clearDamageTarget->GetAbilitySystemComponent().BuildGameplayEffectSnapshots().size();
			const bool igniteTagAfterClear = clearDamageTarget->GetAbilitySystemComponent().HasOwnedTag(DamageStatusSchema::Ignite);
			clearDamageTarget->GetAbilitySystemComponent().onGameplayEvent.UnbindAction(clearDamageEventToken);
			clearDamageSource->GetAbilitySystemComponent().onGameplayEvent.UnbindAction(sourceStatusEventToken);
			clearDamageTarget->GetCombatRuntime().onDamageProcessed.UnbindAction(damageProcessedToken);
			const bool e15Passed = e15ControlPassed && e15Barrier.IsValid() && clearDamageAudit.clearCompleted &&
				clearDamageAudit.breakContextValid && clearDamageAudit.remainingDamageAtBreak == 15.f &&
				e15Error == "Injected E15 Clear callback failure" && breakEventsAfterThrow == 1 &&
				damageReceivedAfterThrow == 0 && sourceStatusAfterThrow == 0 && damageProcessedAfterThrow == 0 &&
				breakEventsAfterTick == breakEventsAfterThrow && damageReceivedAfterTick == 0 &&
				sourceStatusAfterTick == 0 && damageProcessedAfterTick == 0 && e15EffectsAfter == 0 &&
				!igniteTagAfterClear && e15SourceEffectsBefore == 0;
			nlohmann::json e15Actual = {
				{ "barrierHandle", e15Barrier.id }, { "clearCompleted", clearDamageAudit.clearCompleted },
				{ "typedBreakContextValid", clearDamageAudit.breakContextValid },
				{ "remainingDamageAtBreak", clearDamageAudit.remainingDamageAtBreak },
				{ "breakEventsAfterThrow", breakEventsAfterThrow }, { "breakEventsAfterNextTick", breakEventsAfterTick },
				{ "damageReceivedAfterThrow", damageReceivedAfterThrow }, { "damageReceivedAfterNextTick", damageReceivedAfterTick },
				{ "sourceIgniteEventsAfterThrow", sourceStatusAfterThrow }, { "sourceIgniteEventsAfterNextTick", sourceStatusAfterTick },
				{ "damageProcessedAfterThrow", damageProcessedAfterThrow }, { "damageProcessedAfterNextTick", damageProcessedAfterTick },
				{ "igniteTagAfterClear", igniteTagAfterClear },
				{ "normalControl", {
					{ "barrierHandle", controlBarrier.id }, { "breakEvents", normalDamageControl.breakEvents },
					{ "damageReceivedEvents", normalDamageControl.damageReceivedEvents },
					{ "typedDamageContextValid", normalDamageControl.damageReceivedContextValid },
					{ "remainingDamageAtDamageReceived", normalDamageControl.remainingDamageAtReceived },
					{ "sourceIgniteEvents", normalDamageControl.sourceStatusEvents },
					{ "damageProcessedEvents", normalDamageControl.damageProcessedEvents },
					{ "igniteTag", e15ControlIgniteTag }, { "activeEffectsAfter", e15ControlEffectsAfter },
					{ "passed", e15ControlPassed }
				} },
				{ "targetEffectsBefore", e15EffectsBefore },
				{ "targetEffectsAfter", e15EffectsAfter }, { "sourceEffectsBefore", e15SourceEffectsBefore }
			};
			recordCase("damageCases", { { "case_id", "E15" },
				{ "input", { { "target", "A" }, { "source", "B" }, { "barrierCapacity", 5.f },
					{ "damage", 20.f }, { "damageTags", { DamageTypeSchema::Thermal.ToString() } },
					{ "igniteStacks", 1 }, { "barrierDuration", e15BarrierDuration },
					{ "fault", "Clear then throw from BarrierBroken event listener" } } },
				{ "expected", "The generation change stops the old event batch before DamageReceived and status callbacks, and no stale event replays on the next tick" },
				{ "actual", e15Actual }, { "order", e15Order },
				{ "identities", { { "targetActorId", clearDamageTarget->GetUniqueID() },
					{ "sourceActorId", clearDamageSource->GetUniqueID() }, { "barrierEffectId", barrierDefinition->effectId } } },
				{ "before", { { "targetEffects", e15EffectsBefore }, { "sourceEffects", e15SourceEffectsBefore } } },
				{ "after", { { "targetEffects", e15EffectsAfter }, { "breakEvents", breakEventsAfterTick },
					{ "damageReceivedEvents", damageReceivedAfterTick }, { "sourceStatusEvents", sourceStatusAfterTick },
					{ "damageProcessedEvents", damageProcessedAfterTick }, { "igniteTag", igniteTagAfterClear } } },
				{ "errors", { e15Error } }, { "passed", e15Passed } });
			check("E15ClearThrowStopsDamageEventBatch", e15Passed);

			// A production effect activation callback cancels an active beam ability,
			// requests component Clear, and crosses the one-shot visual cleanup fault.
			auto crossFlowOwner = spawn(8600.f, CollisionLayer::Player);
			world.TickInternal(0.f);
			auto& crossFlowSystem = crossFlowOwner->GetAbilitySystemComponent();
			Set<std::uint64_t> e18ExistingBeamIds;
			for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
				if (const auto beam = weakBeam.lock()) e18ExistingBeamIds.insert(beam->GetUniqueID());
			auto crossFlowFire = MakeEventAction(sas::AbilityActionPhase::WhileActive, actionEventTag, 0.f);
			crossFlowFire.action = FireWeaponAction{ MakeContinuousBeamDefinition() };
			auto crossFlowDefinition = MakeRuntimeActionAbility(
				"Ability.Utility.AuditFixes.EffectCancelClearThrow",
				sas::AbilitySlot::Ability4,
				{ crossFlowFire, MakeEventAction(sas::AbilityActionPhase::OnEnd, actionEventTag, 881.f) }
			);
			const auto crossFlowHandle = crossFlowSystem.GrantAbility(crossFlowDefinition, sas::AbilitySlot::Ability4);
			PrimaryWeaponDefinition e18OverrideDefinition = MakeContinuousBeamDefinition();
			e18OverrideDefinition.weaponId = "Weapon.E2E.CrossFlowOverride";
			const auto e18OverrideHandle = crossFlowSystem.PushPrimaryWeaponOverride(
				sas::ContentId{ crossFlowDefinition.abilityId }, e18OverrideDefinition
			);
			const bool e18OverrideInstalled = e18OverrideHandle != 0 &&
				crossFlowSystem.GetActivePrimaryWeaponOverride() != nullptr;
			check("E18ProductionPrimaryWeaponOverrideInstalled", e18OverrideInstalled);
			RuntimeActionAudit crossFlowActionAudit{ crossFlowSystem, *crossFlowOwner, crossFlowHandle, actionEventTag };
			const auto crossFlowEventToken = crossFlowSystem.onGameplayEvent.BindAction(
				&crossFlowActionAudit, &RuntimeActionAudit::Event
			);
			const auto crossFlowEndedToken = crossFlowSystem.onAbilityEnded.BindAction(
				&crossFlowActionAudit, &RuntimeActionAudit::Ended
			);
			crossFlowSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
			crossFlowSystem.Tick(0.01f);
			crossFlowSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability4, false);
			world.TickInternal(0.f);
			std::vector<shared_ptr<ContinuousBeamVisualActor>> crossFlowBeams;
			for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
			{
				if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy() &&
					e18ExistingBeamIds.find(beam->GetUniqueID()) == e18ExistingBeamIds.end())
					crossFlowBeams.push_back(beam);
			}
			if (crossFlowBeams.size() != 1)
				throw std::runtime_error("E18 production FireWeapon activation did not create one beam visual");
			const std::size_t e18AbilitiesBefore = crossFlowSystem.BuildAbilitySnapshots().size();
			const std::size_t e18EffectsBefore = crossFlowSystem.BuildGameplayEffectSnapshots().size();
			const float e18AttributeBefore = crossFlowSystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage);
			std::vector<std::string> e18Order;
			CrossFlowBeamDestroyAudit e18DestroyAudit{
				crossFlowBeams.front()->GetUniqueID(), crossFlowSystem, e18Order
			};
			const auto e18DestroyToken = crossFlowBeams.front()->onActorDestroyed.BindAction(
				&e18DestroyAudit, &CrossFlowBeamDestroyAudit::Destroyed
			);
			GameplayTag e18EffectTag{ "E2E.Effect.CrossFlowCleanup" };
			sas::GameplayEffectDefinition e18Effect;
			e18Effect.effectId = "E2E.Effect.CrossFlowCleanup";
			e18Effect.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
			e18Effect.grantedTags.push_back(e18EffectTag);
			e18Effect.modifiers.emplace_back(OwnerAttributeIds::CriticalDamage, 0.25f);
			CrossFlowEffectAudit e18Callback{
				crossFlowSystem, crossFlowHandle, e18EffectTag, OwnerAttributeIds::CriticalDamage,
				e18AttributeBefore, e18Order
			};
			auto e18Callbacks = sas::AbilitySystemComponent::EffectCallbacks{};
			e18Callbacks.activated = [&e18Callback](sas::ActiveGameplayEffect& effect) { e18Callback.Activated(effect); };
			crossFlowSystem.SetEffectRuntimeCallbacks(std::move(e18Callbacks));
			const auto e18ActiveOverrideBeforeCallback = crossFlowSystem.GetActivePrimaryWeaponOverride() != nullptr;
			const std::size_t e18ActiveEffectsBeforeApply = crossFlowSystem.BuildGameplayEffectSnapshots().size();
			std::string e18ApplyError;
			sas::GameplayEffectHandle e18EffectHandle;
			try { e18EffectHandle = crossFlowSystem.ApplyGameplayEffect(e18Effect); }
			catch (const std::runtime_error& error) { e18ApplyError = error.what(); }
			const bool e18BeamPendingAfterClear = crossFlowBeams.front()->GetIsPendingDestroy();
			const std::size_t e18AbilitiesAfterClear = crossFlowSystem.BuildAbilitySnapshots().size();
			const std::size_t e18EffectsAfterClear = crossFlowSystem.BuildGameplayEffectSnapshots().size();
			const std::size_t e18InvocationsAfterClear = crossFlowSystem.GetActiveAbilityInvocationCount();
			const bool e18TagAfterClear = crossFlowSystem.HasOwnedTag(e18EffectTag, true);
			const bool e18AttributePresentAfterClear = crossFlowSystem.GetAttributes().HasAttribute(OwnerAttributeIds::CriticalDamage);
			const float e18AttributeAfterClear = crossFlowSystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::CriticalDamage);
			const bool e18WeaponOverrideAfterClear = crossFlowSystem.GetActivePrimaryWeaponOverride() != nullptr;
			const bool e18ClearDrainedCommonDebts = e18BeamPendingAfterClear && e18AbilitiesAfterClear == 0 &&
				e18EffectsAfterClear == 0 && e18InvocationsAfterClear == 0 && !e18TagAfterClear &&
				!e18AttributePresentAfterClear && !e18WeaponOverrideAfterClear;
			crossFlowBeams.front()->onActorDestroyed.UnbindAction(e18DestroyToken);
			crossFlowSystem.onGameplayEvent.UnbindAction(crossFlowEventToken);
			crossFlowSystem.onAbilityEnded.UnbindAction(crossFlowEndedToken);
			crossFlowSystem.SetEffectRuntimeCallbacks({});
			const std::size_t e18LiveBeamsBeforeReuse = [&]
			{
				std::size_t count = 0;
				for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
					if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy()) ++count;
				return count;
			}();
			const auto e18ReuseHandle = crossFlowSystem.GrantAbility(crossFlowDefinition, sas::AbilitySlot::Ability4);
			crossFlowSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
			crossFlowSystem.Tick(0.01f);
			crossFlowSystem.SetAbilitySlotInput(sas::AbilitySlot::Ability4, false);
			world.TickInternal(0.f);
			const bool e18ReuseActive = e18ReuseHandle.IsValid() && crossFlowSystem.GetAbility(e18ReuseHandle) &&
				crossFlowSystem.GetAbility(e18ReuseHandle)->IsActive();
			const std::size_t e18LiveBeamsAfterReuse = [&]
			{
				std::size_t count = 0;
				for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
					if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy()) ++count;
				return count;
			}();
			std::string e18ReuseClearError;
			try { crossFlowSystem.Clear(); }
			catch (const std::runtime_error& error) { e18ReuseClearError = error.what(); }
			const std::size_t e18LiveBeamsAfterReuseClear = [&]
			{
				std::size_t count = 0;
				for (const auto& weakBeam : world.GetActorsByTypeIncludingPending<ContinuousBeamVisualActor>())
					if (const auto beam = weakBeam.lock(); beam && !beam->GetIsPendingDestroy()) ++count;
				return count;
			}();
			const bool e18Passed = crossFlowHandle.IsValid() && e18OverrideInstalled && e18Callback.callbackVisited &&
				e18Callback.effectResourcesPresent && e18ActiveOverrideBeforeCallback && e18Callback.cancelAttempted &&
				e18Callback.cancelThrew && e18Callback.cancelError == "Injected E18 beam cleanup failure" &&
				e18Callback.abilityInactiveAfterCancelFault && e18Callback.clearCalled && !e18Callback.clearThrew &&
				e18Callback.clearError.empty() && e18DestroyAudit.throwOnce == false && e18ClearDrainedCommonDebts &&
				e18DestroyAudit.clearCalledBeforeThrow && e18DestroyAudit.clearError.empty() &&
				e18ApplyError.empty() && e18ActiveEffectsBeforeApply == 0 &&
				crossFlowActionAudit.endCount == 1 && e18ReuseHandle.IsValid() && e18ReuseActive &&
				e18LiveBeamsAfterReuse == e18LiveBeamsBeforeReuse + 1 && e18ReuseClearError.empty() &&
				e18LiveBeamsAfterReuseClear == e18LiveBeamsBeforeReuse;
			nlohmann::json e18Actual = {
				{ "callbackVisited", e18Callback.callbackVisited }, { "effectResourcesPresentAtCallback", e18Callback.effectResourcesPresent },
				{ "activeOverrideHandle", e18OverrideHandle }, { "activeOverrideInstalled", e18OverrideInstalled },
				{ "activeOverrideBeforeCallback", e18ActiveOverrideBeforeCallback },
				{ "cancelAttempted", e18Callback.cancelAttempted }, { "cancelThrew", e18Callback.cancelThrew },
				{ "cancelError", e18Callback.cancelError }, { "abilityInactiveAfterCancelFault", e18Callback.abilityInactiveAfterCancelFault },
				{ "clearCalled", e18Callback.clearCalled }, { "clearThrew", e18Callback.clearThrew },
				{ "clearError", e18Callback.clearError }, { "effectApplyError", e18ApplyError },
				{ "clearCalledFromVisualDestroyBeforeThrow", e18DestroyAudit.clearCalledBeforeThrow },
				{ "visualCallbackClearError", e18DestroyAudit.clearError },
				{ "effectHandleValidAfterClear", e18EffectHandle.IsValid() },
				{ "beamPendingAfterClear", e18BeamPendingAfterClear },
				{ "abilitiesAfterClear", e18AbilitiesAfterClear }, { "effectsAfterClear", e18EffectsAfterClear },
				{ "invocationsAfterClear", e18InvocationsAfterClear }, { "tagAfterClear", e18TagAfterClear },
				{ "attributePresentAfterClear", e18AttributePresentAfterClear },
				{ "attributeBefore", e18AttributeBefore }, { "attributeAfterClear", e18AttributeAfterClear },
				{ "weaponOverrideAfterClear", e18WeaponOverrideAfterClear },
				{ "endCountAfterClear", crossFlowActionAudit.endCount },
				{ "reuseHandle", e18ReuseHandle.id }, { "reuseActive", e18ReuseActive },
				{ "liveBeamsBeforeReuse", e18LiveBeamsBeforeReuse },
				{ "liveBeamsAfterReuse", e18LiveBeamsAfterReuse }, { "reuseClearError", e18ReuseClearError },
				{ "liveBeamsAfterReuseClear", e18LiveBeamsAfterReuseClear }
			};
			recordCase("abilityCases", { { "case_id", "E18" },
				{ "input", { { "ability", crossFlowDefinition.abilityId }, { "slot", "Ability4" },
					{ "weaponType", "BeamContinuous" }, { "effectId", e18Effect.effectId },
					{ "effectCallback", "activated cancels active ability then requests component Clear" },
					{ "fault", "active beam actor destroy callback throws once" } } },
				{ "expected", "Effect→Cancel→Clear crosses and retries cleanup debt, clears shared dependencies, then the same component accepts a new input activation" },
				{ "actual", e18Actual }, { "order", e18Order },
				{ "identities", { { "ownerActorId", crossFlowOwner->GetUniqueID() },
					{ "abilityId", crossFlowDefinition.abilityId }, { "abilityHandle", crossFlowHandle.id },
					{ "weaponOverrideSourceId", crossFlowDefinition.abilityId },
					{ "weaponOverrideHandle", e18OverrideHandle }, { "weaponOverrideId", e18OverrideDefinition.weaponId },
					{ "beamActorId", crossFlowBeams.front()->GetUniqueID() },
					{ "effectId", e18Effect.effectId }, { "effectHandle", e18EffectHandle.id },
					{ "reuseAbilityHandle", e18ReuseHandle.id } } },
				{ "before", { { "abilities", e18AbilitiesBefore }, { "effects", e18EffectsBefore },
					{ "weaponOverrideActive", e18ActiveOverrideBeforeCallback },
					{ "activeEffectsBeforeApply", e18ActiveEffectsBeforeApply }, { "criticalDamage", e18AttributeBefore } } },
				{ "after", { { "abilities", e18AbilitiesAfterClear }, { "effects", e18EffectsAfterClear },
					{ "invocations", e18InvocationsAfterClear }, { "tagOwned", e18TagAfterClear },
					{ "attributePresent", e18AttributePresentAfterClear }, { "criticalDamage", e18AttributeAfterClear },
					{ "weaponOverrideActive", e18WeaponOverrideAfterClear }, { "beamPendingDestroy", e18BeamPendingAfterClear },
					{ "liveBeamsBeforeReuse", e18LiveBeamsBeforeReuse }, { "liveBeamsAfterReuse", e18LiveBeamsAfterReuse },
					{ "liveBeamsAfterReuseClear", e18LiveBeamsAfterReuseClear } } },
				{ "errors", { e18Callback.cancelError, e18Callback.clearError, e18ApplyError, e18ReuseClearError } },
				{ "passed", e18Passed } });
			check("E18EffectCancelClearCleanupAndReuse", e18Passed);

			// Damage callback grows indexed storage and refreshes its live entry.
			auto nanoTarget = spawn(3500.f, CollisionLayer::Enemy);
			auto reentryController = NanoPlagueControllerActor::FindOrCreate(world, *owner, NanoPlaguePresentationProfile{});
			NanoReentryObserver reentry{ reentryController, nanoTarget };
			reentry.settings.baseTickDamage = 1.f;
			reentry.settings.energyPowerTickScale = 0.f;
			for (int i = 0; i < 32; ++i) reentry.additions.push_back(spawn(3600.f + i * 20.f, CollisionLayer::Enemy));
			check("nanoReentryApplied", reentryController->ApplyOrRefreshInfection(*nanoTarget, 1, reentry.settings));
			world.TickInternal(0.f);
			const auto countBefore = AuditFixesE2EAccess::Count(*reentryController);
			const auto recordBefore = AuditFixesE2EAccess::FirstRecord(*reentryController);
			auto damageToken = nanoTarget->GetCombatRuntime().onDamageResolved.BindAction(&reentry, &NanoReentryObserver::Damaged);
			reentryController->Tick(0.25f);
			check("nanoCallbackGrowsStableStorage", reentry.visited && AuditFixesE2EAccess::Count(*reentryController) == countBefore + 32 &&
				AuditFixesE2EAccess::FirstRecord(*reentryController) == recordBefore);
			result["outcome"]["nanoRecordsBefore"] = countBefore;
			result["outcome"]["nanoRecordsAfter"] = AuditFixesE2EAccess::Count(*reentryController);
			result["outcome"]["cleanupError"] = cleanupError;
			result["outcome"]["activationError"] = activationError;
			check("nanoRefreshOwnsNewCounters", AuditFixesE2EAccess::Ticks(*reentryController, *nanoTarget) == 0);
			check("nanoNewInfectionsStartNextTick", AuditFixesE2EAccess::Ticks(*reentryController, *reentry.additions.front()) == 0);
			reentryController->Tick(0.25f);
			check("nanoContinuesAfterReentry", AuditFixesE2EAccess::Ticks(*reentryController, *nanoTarget) == 1 &&
				AuditFixesE2EAccess::Ticks(*reentryController, *reentry.additions.front()) == 1);
			nanoTarget->GetCombatRuntime().onDamageResolved.UnbindAction(damageToken);
			reentryController->Tick(10.f);
			check("nanoBatchExpiryClearsIndexedRecords", AuditFixesE2EAccess::Count(*reentryController) == 0 &&
				!nanoTarget->GetAbilitySystemComponent().HasOwnedTag(AbilityData::NanoPlague::State::Infected));
			reentryController->Destroy();
			result["passed"] = true;
		}
		catch (const std::exception& error) { result["error"] = error.what(); }
		const std::filesystem::path path{ artifactPath };
		std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		output << result.dump(2) << '\n';
		return output.good() && result["passed"].get<bool>() ? 0 : 1;
	}
}

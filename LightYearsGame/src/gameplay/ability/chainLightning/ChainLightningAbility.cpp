#include "gameplay/ability/chainLightning/ChainLightningAbility.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/chainLightning/ChainLightningContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetingTypes.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"
#include "framework/TimerManager.h"
#include "framework/World.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/chainLightning/ChainLightningPresentationIds.h"
#include "presentation/ability/chainLightning/ChainLightningPresentationProfile.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_set>
#include <vector>

namespace ly
{
	class ChainLightningAbilityLifetimeState;

	namespace
	{
		constexpr std::size_t RequiredAttributeCount = 6;
		constexpr float BaseDuration = 0.f;
		constexpr float BaseDamage = 40.f;
		constexpr float BaseInitialTargetRange = 700.f;
		constexpr float BaseBounceRange = 350.f;
		constexpr float BaseBounceCount = 5.f;
		constexpr float BaseLinkTravelTime = 0.22f;
		constexpr float BaseElectricStacks = 1.f;
		constexpr float Epsilon = 0.0001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		const sas::GameplayAttribute* FindAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}

		bool HasValidAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId,
			float minimumBaseValue,
			bool requireWholeNumber = false
		)
		{
			const sas::GameplayAttribute* attribute = FindAttribute(
				definition,
				attributeId
			);
			if (!attribute ||
				!std::isfinite(attribute->baseValue) ||
				!std::isfinite(attribute->currentValue) ||
				!std::isfinite(attribute->minValue) ||
				!std::isfinite(attribute->maxValue) ||
				attribute->minValue > attribute->maxValue ||
				attribute->baseValue < minimumBaseValue ||
				attribute->baseValue < attribute->minValue ||
				attribute->baseValue > attribute->maxValue)
			{
				return false;
			}
			return !requireWholeNumber ||
				std::round(attribute->baseValue) == attribute->baseValue;
		}

		bool HasExactAbilityTags(const GameAbilityDefinition& definition)
		{
			if (definition.abilityTags.size() != 2)
			{
				return false;
			}

			bool hasCategory = false;
			bool hasFamily = false;
			for (const GameplayTag& tag : definition.abilityTags)
			{
				hasCategory = hasCategory || tag.MatchesTagExact(
					AbilityData::ChainLightning::CategoryTag
				);
				hasFamily = hasFamily || tag.MatchesTagExact(
					AbilityData::ChainLightning::FamilyTag
				);
			}
			return hasCategory && hasFamily;
		}

		bool HasModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			bool positive
		)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::isfinite(modifier.magnitude) &&
					(positive ? modifier.magnitude > 0.f : modifier.magnitude < 0.f))
				{
					return true;
				}
			}
			return false;
		}

		weak_ptr<Actor> MakeWeakActor(Actor* actor)
		{
			if (!actor)
			{
				return {};
			}
			const shared_ptr<Object> object = actor->GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<Actor>(object)
				: weak_ptr<Actor>{};
		}

		struct ChainLightningTraversalData
		{
			weak_ptr<Object> worldLifetime;
			weak_ptr<ChainLightningAbilityLifetimeState> abilityLifetime;
			int remainingBounces{ 0 };
			std::uint64_t currentTargetId{ 0 };
			float bounceRange{ 0.f };
			float linkTravelTime{ 0.f };
			float damage{ 0.f };
			List<GameplayTag> damageTags;
			DamagePayload payload;
			sas::ContentId sourceAbilityId;
			List<GameplayTag> sourceAbilityTags;
			sf::Color arcColor = sf::Color::White;
		};

		class ChainLightningTimerOwnerActor final : public Actor
		{
		public:
			explicit ChainLightningTimerOwnerActor(World* world) : Actor(world) {}

			~ChainLightningTimerOwnerActor() override
			{
				UnbindOwner();
				ClearTimerHandles();
			}

			bool BindOwner(const weak_ptr<Actor>& owner)
			{
				const shared_ptr<Actor> strongOwner = owner.lock();
				if (!strongOwner || strongOwner->GetIsPendingDestroy()) return false;

				mOwner = owner;
				mOwnerDestroyedHandle = strongOwner->onActorDestroyed.BindAction(
					this,
					&ChainLightningTimerOwnerActor::OnOwnerDestroyed
				);
				return mOwnerDestroyedHandle.IsValid();
			}

			void StartTraversal(
				ChainLightningTraversalData traversal,
				const weak_ptr<Actor>& initialTarget
			)
			{
				mTraversal = std::move(traversal);
				mCurrentTarget = initialTarget;
				mFirstLinkPending = true;
				if (const shared_ptr<Actor> target = initialTarget.lock())
				{
					mTraversal.currentTargetId = target->GetUniqueID();
					mLastTargetLocation = target->GetActorLocation();
					mStruckTargetIds.insert(mTraversal.currentTargetId);
				}
				ScheduleNextLink();
			}

			void Cancel()
			{
				Finish();
			}

		private:
			shared_ptr<Actor> FindNextTarget(
				World& world,
				Actor& owner,
				const sf::Vector2f& origin,
				bool previouslyStruck
			) const
			{
				targeting::TargetingQuery query;
				query.source = &owner;
				query.origin = origin;
				query.range = mTraversal.bounceRange;
				query.requiredTargetLayers = owner.GetCollisionLayer() == CollisionLayer::Player
					? CollisionLayer::Enemy
					: CollisionLayer::Player;
				query.requireCollisionCompatibility = true;
				query.excludeSource = true;
				query.filter = [this, previouslyStruck](
					const Actor*,
					const Actor& candidate,
					const targeting::TargetingCandidate&
				)
				{
					const std::uint64_t candidateId = const_cast<Actor&>(candidate).GetUniqueID();
					if (candidateId == mTraversal.currentTargetId)
					{
						return false;
					}
					const bool wasStruck = mStruckTargetIds.find(candidateId) !=
						mStruckTargetIds.end();
					return dynamic_cast<const Combatant*>(&candidate) != nullptr &&
						wasStruck == previouslyStruck;
				};
				query.comparator = [](
					const targeting::TargetingCandidate& left,
					const targeting::TargetingCandidate& right
				)
				{
					return left.distanceSquared < right.distanceSquared;
				};

				const List<targeting::TargetingCandidate> candidates =
					targeting::AutoTargeting::FindTargets(world, query);
				return candidates.empty() ? shared_ptr<Actor>{} : candidates.front().actor;
			}

			void ScheduleNextLink()
			{
				if (mIsFinished || GetIsPendingDestroy()) return;
				const shared_ptr<Object> selfObject = GetWeakPtr().lock();
				const shared_ptr<ChainLightningTimerOwnerActor> self =
					std::dynamic_pointer_cast<ChainLightningTimerOwnerActor>(selfObject);
				if (!self) return;
				const weak_ptr<ChainLightningTimerOwnerActor> selfWeak = self;
				const TimerHandle handle = TimerManager::GetGameTimerManager().SetTimer(
					mTraversal.worldLifetime,
					[selfWeak]()
					{
						if (const shared_ptr<ChainLightningTimerOwnerActor> timerOwner = selfWeak.lock())
						{
							timerOwner->AdvanceLink();
						}
					},
					mTraversal.linkTravelTime,
					false
				);
				mScheduledTimerHandle = handle;
			}

			void AdvanceLink()
			{
				if (mIsFinished || GetIsPendingDestroy()) return;
				mScheduledTimerHandle.reset();
				if (mTraversal.abilityLifetime.expired())
				{
					Finish();
					return;
				}

				const shared_ptr<World> world = std::dynamic_pointer_cast<World>(
					mTraversal.worldLifetime.lock()
				);
				const shared_ptr<Actor> owner = mOwner.lock();
				if (!world || world->GetIsPendingDestroy() || !owner ||
					owner->GetIsPendingDestroy())
				{
					Finish();
					return;
				}

				shared_ptr<Actor> target = mCurrentTarget.lock();
				sf::Vector2f startLocation{};
				if (mFirstLinkPending)
				{
					if (!target || target->GetIsPendingDestroy())
					{
						Finish();
						return;
					}
					mFirstLinkPending = false;
					startLocation = owner->GetActorLocation();
				}
				else
				{
					if (mTraversal.remainingBounces <= 0)
					{
						Finish();
						return;
					}
					startLocation = target && !target->GetIsPendingDestroy()
						? target->GetActorLocation()
						: mLastTargetLocation;
					target = FindNextTarget(*world, *owner, startLocation, false);
					if (!target)
					{
						target = FindNextTarget(*world, *owner, startLocation, true);
					}
					if (!target)
					{
						Finish();
						return;
					}
					--mTraversal.remainingBounces;
				}

				if (!target || target->GetIsPendingDestroy())
				{
					Finish();
					return;
				}
				const sf::Vector2f targetLocation = target->GetActorLocation();
				mLastTargetLocation = targetLocation;
				mCurrentTarget = target;
				mTraversal.currentTargetId = target->GetUniqueID();
				mStruckTargetIds.insert(mTraversal.currentTargetId);
				world->SpawnActor<ElectricArcVisualActor>(
					startLocation,
					targetLocation,
					mTraversal.arcColor
				);
				ApplyCombatDamage(
					*target,
					mTraversal.damage,
					owner.get(),
					mTraversal.damageTags,
					mTraversal.payload,
					mTraversal.sourceAbilityId,
					mTraversal.sourceAbilityTags,
					DamageDeliveryType::Beam
				);

				if (mTraversal.remainingBounces > 0 && !mIsFinished && !GetIsPendingDestroy())
				{
					ScheduleNextLink();
				}
				else
				{
					Finish();
				}
			}

			void OnOwnerDestroyed(Actor* destroyedOwner)
			{
				const shared_ptr<Actor> owner = mOwner.lock();
				if (owner && owner.get() == destroyedOwner) Finish();
			}

			void Finish()
			{
				if (mIsFinished) return;
				mIsFinished = true;
				UnbindOwner();
				ClearTimerHandles();
				if (!GetIsPendingDestroy()) Destroy();
			}

			void UnbindOwner()
			{
				if (const shared_ptr<Actor> owner = mOwner.lock())
				{
					owner->onActorDestroyed.UnbindAction(mOwnerDestroyedHandle);
				}
				mOwnerDestroyedHandle.Reset();
				mOwner.reset();
			}

			void ClearTimerHandles()
			{
				if (!mScheduledTimerHandle) return;
				TimerManager& timerManager = TimerManager::GetGameTimerManager();
				timerManager.ClearTimer(*mScheduledTimerHandle);
				mScheduledTimerHandle.reset();
			}

			weak_ptr<Actor> mOwner;
			DelegateHandle mOwnerDestroyedHandle;
			ChainLightningTraversalData mTraversal;
			weak_ptr<Actor> mCurrentTarget;
			std::optional<TimerHandle> mScheduledTimerHandle;
			sf::Vector2f mLastTargetLocation{};
			std::unordered_set<std::uint64_t> mStruckTargetIds;
			bool mFirstLinkPending{ false };
			bool mIsFinished{ false };
		};
	}

	class ChainLightningAbilityLifetimeState final
	{
	public:
		~ChainLightningAbilityLifetimeState()
		{
			for (const weak_ptr<Actor>& castOwner : mCastOwners)
			{
				const shared_ptr<Actor> actor = castOwner.lock();
				const shared_ptr<ChainLightningTimerOwnerActor> timerOwner =
					std::dynamic_pointer_cast<ChainLightningTimerOwnerActor>(actor);
				if (timerOwner) timerOwner->Cancel();
			}
		}

		void RegisterCast(const shared_ptr<ChainLightningTimerOwnerActor>& castOwner)
		{
			mCastOwners.erase(
				std::remove_if(
					mCastOwners.begin(),
					mCastOwners.end(),
					[](const weak_ptr<Actor>& candidate)
					{
						const shared_ptr<Actor> actor = candidate.lock();
						return !actor || actor->GetIsPendingDestroy();
					}
				),
				mCastOwners.end()
			);
			mCastOwners.push_back(castOwner);
		}

	private:
		std::vector<weak_ptr<Actor>> mCastOwners;
	};

	ChainLightningAbility::ChainLightningAbility()
		: mLifetimeState{ std::make_shared<ChainLightningAbilityLifetimeState>() }
	{
	}

	ChainLightningAbility::~ChainLightningAbility() = default;

	bool ChainLightningAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::ChainLightning::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::ChainLightning &&
			HasExactAbilityTags(definition);

		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 &&
			std::isfinite(definition.cooldown) && definition.cooldown > 0.f &&
			NearlyEqual(definition.duration, BaseDuration);

		const bool validDamageIdentity =
			definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(
				DamageTypeSchema::Electric
			);

		const bool validAttributes =
			definition.attributes.size() == RequiredAttributeCount &&
			HasValidAttribute(
				definition,
				CommonAttributeIds::Damage,
				0.f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::InitialTargetRange,
				0.01f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::BounceRange,
				0.01f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::BaseBounceCount,
				1.f,
				true
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::LinkTravelTime,
				0.01f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::ChainLightning::Attribute::ElectricStacks,
				1.f,
				true
			);

		// Chain Lightning is a behavior-owned traversal. Generic actions would
		// execute immediately and could accidentally create a second damage path.
		const bool validRuntimeOwnership =
			definition.actions.empty() &&
			definition.triggers.empty() &&
			definition.effectSpecs.empty();
		const bool validScaling = definition.scalingRules.size() == 1 &&
			definition.scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
			definition.scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyPower &&
			definition.scalingRules.front().operation == sas::AttributeModifierOperation::Add &&
			std::isfinite(definition.scalingRules.front().coefficient) &&
			definition.scalingRules.front().coefficient > 0.f;

		bool validProgression = !definition.levelProgression.empty() &&
			definition.levelUpgradeScrapCosts.size() == definition.levelProgression.size();
		float resolvedCooldown = definition.cooldown;
		if (validProgression)
		{
			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				if (step.attributeModifiers.size() != 2 || step.scalingRules.size() != 1 ||
					!step.unlockedUpgradeIds.empty() ||
					!step.addedActions.empty() ||
					!step.addedTriggers.empty() ||
					!HasModifier(
						step,
						CommonAttributeIds::Damage,
						true
					) ||
					!HasModifier(
						step,
						CommonAttributeIds::Cooldown,
						false
					) ||
					step.scalingRules.front().targetAttributeId != CommonAttributeIds::Damage ||
					step.scalingRules.front().sourceAttributeId != OwnerAttributeIds::EnergyPower ||
					step.scalingRules.front().operation != sas::AttributeModifierOperation::Add ||
					!std::isfinite(step.scalingRules.front().coefficient) ||
					step.scalingRules.front().coefficient < 0.f)
				{
					validProgression = false;
					break;
				}

				const auto cooldownModifier = std::find_if(
					step.attributeModifiers.begin(), step.attributeModifiers.end(),
					[](const sas::AttributeModifier& modifier)
					{
						return modifier.attributeId == CommonAttributeIds::Cooldown;
					}
				);
				resolvedCooldown += cooldownModifier->magnitude;
				if (!std::isfinite(resolvedCooldown) || resolvedCooldown <= 0.f)
				{
					validProgression = false;
					break;
				}
			}
		}

		if (!validIdentity || !validLifecycle || !validDamageIdentity ||
			!validAttributes || !validRuntimeOwnership || !validProgression || !validScaling)
		{
			if (failureReason)
			{
				*failureReason =
				"Chain Lightning requires its electric identity, six declared traversal and electric payload attributes, EnergyPower damage scaling, and aligned progression costs.";
			}
			return false;
		}

		return true;
	}

	bool ChainLightningAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world || world->GetIsPendingDestroy())
		{
			return false;
		}
		const weak_ptr<Object> worldLifetime = world->GetWeakPtr();
		if (worldLifetime.expired()) return false;

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const float initialRange = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::InitialTargetRange,
				BaseInitialTargetRange
			)
		);
		const float bounceRange = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::BounceRange,
				BaseBounceRange
			)
		);
		const double maximumBounceCount = static_cast<double>(std::numeric_limits<int>::max());
		const double configuredBounceCount = std::max(
			0.0,
			std::round(static_cast<double>(sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::BaseBounceCount,
				BaseBounceCount
			)))
		);
		const int baseBounceCount = static_cast<int>(std::min(configuredBounceCount, maximumBounceCount));
		const float ownerLuck = context.abilitySystem.GetAttributes().GetCurrentValue(
			OwnerAttributeIds::Luck
		);
		const double bonusBounceCount = std::floor(
			static_cast<double>(std::isfinite(ownerLuck) ? std::max(0.f, ownerLuck) : 0.f) * 0.05
		);
		const int safeBonusBounces = static_cast<int>(std::min(
			bonusBounceCount,
			maximumBounceCount - static_cast<double>(baseBounceCount)
		));
		const float linkTravelTime = std::max(
			0.001f,
			sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::LinkTravelTime,
				BaseLinkTravelTime
			)
		);

		const CollisionLayer opposingLayer =
			context.owner.GetCollisionLayer() == CollisionLayer::Player
			? CollisionLayer::Enemy
			: context.owner.GetCollisionLayer() == CollisionLayer::Enemy
				? CollisionLayer::Player
				: CollisionLayer::None;
		if (opposingLayer == CollisionLayer::None || initialRange <= 0.f || bounceRange <= 0.f)
		{
			return false;
		}

		const sf::Vector2f mouseLocation = world->GetMouseWorldPosition();
		targeting::TargetingQuery initialQuery;
		initialQuery.source = &context.owner;
		initialQuery.origin = context.owner.GetActorLocation();
		initialQuery.range = initialRange;
		initialQuery.requiredTargetLayers = opposingLayer;
		initialQuery.requireCollisionCompatibility = true;
		initialQuery.comparator = [mouseLocation](
			const targeting::TargetingCandidate& left,
			const targeting::TargetingCandidate& right
		)
		{
			const auto distanceToMouse = [&](const shared_ptr<Actor>& actor)
			{
				if (!actor)
				{
					return std::numeric_limits<float>::max();
				}
				const sf::Vector2f delta = actor->GetActorLocation() - mouseLocation;
				return delta.x * delta.x + delta.y * delta.y;
			};
			return distanceToMouse(left.actor) < distanceToMouse(right.actor);
		};
		initialQuery.filter = [](
			const Actor*,
			const Actor& candidate,
			const targeting::TargetingCandidate&
		)
		{
			return dynamic_cast<const Combatant*>(&candidate) != nullptr;
		};

		const List<targeting::TargetingCandidate> initialTargets =
			targeting::AutoTargeting::FindTargets(*world, initialQuery);
		if (initialTargets.empty() || !initialTargets.front().actor)
		{
			LY_GAME_INFO("Chain Lightning: activation rejected because no initial target was found.");
			return false;
		}

		const ChainLightningPresentationProfile* profile =
			PresentationProfileRegistry<ChainLightningPresentationProfile>::Find(
				ChainLightningPresentationIds::ArcBasic
			);
		if (!profile)
		{
			LY_GAME_INFO("Chain Lightning: activation rejected because its arc presentation profile is missing.");
			return false;
		}

		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, values);
		payload.electricStacks = std::max(
			1,
			static_cast<int>(std::lround(sas::FindAttributeValue(
				values,
				AbilityData::ChainLightning::Attribute::ElectricStacks,
				BaseElectricStacks
			)))
		);

		const float damage = std::max(
			0.f,
			sas::FindAttributeValue(values, CommonAttributeIds::Damage, BaseDamage)
		);
		const weak_ptr<Actor> ownerWeak = MakeWeakActor(&context.owner);
		if (ownerWeak.expired()) return false;
		const weak_ptr<ChainLightningTimerOwnerActor> timerOwner =
			world->SpawnActor<ChainLightningTimerOwnerActor>();
		const shared_ptr<ChainLightningTimerOwnerActor> strongTimerOwner = timerOwner.lock();
		if (!strongTimerOwner || !strongTimerOwner->BindOwner(ownerWeak))
		{
			if (strongTimerOwner) strongTimerOwner->Cancel();
			return false;
		}
		mLifetimeState->RegisterCast(strongTimerOwner);
		ChainLightningTraversalData traversal;
		traversal.worldLifetime = worldLifetime;
		traversal.abilityLifetime = mLifetimeState;
		traversal.remainingBounces = baseBounceCount + safeBonusBounces;
		traversal.bounceRange = bounceRange;
		traversal.linkTravelTime = linkTravelTime;
		traversal.damage = damage;
		traversal.damageTags = damageTags;
		traversal.payload = payload;
		traversal.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		traversal.sourceAbilityTags = context.definition.abilityTags;
		traversal.arcColor = profile->outerColor;
		LY_GAME_INFO(
			"Chain Lightning: started live traversal with %d additional bounce(s); link interval=%.3fs, damage=%.2f.",
			traversal.remainingBounces,
			linkTravelTime,
			damage
		);
		strongTimerOwner->StartTraversal(std::move(traversal), initialTargets.front().actor);
		return true;
	}
}

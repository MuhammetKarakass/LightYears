#include "gameplay/module/rules/legendary/LegendaryReactionCoreRuntime.h"
#include "framework/World.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/control/ControlEffects.h"
#include "gameplay/damage/ArmorBreak.h"
#include "gameplay/damage/FragmentBurst.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/movement/TimedPullActor.h"
#include "gameplay/weapon/ShotDeviation.h"
#include "presentation/ability/statusReaction/StatusReactionFlashActor.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>

namespace vega
{
	namespace
	{
		bool IsAlive(const Actor& actor)
		{
			if (actor.GetIsPendingDestroy()) return false;
			const auto* ship = dynamic_cast<const SpaceShip*>(&actor);
			return !ship || ship->GetHealthComponent().GetHealth() > 0.f;
		}

		const sas::ContentId& PresentationOf(StatusReactionKind kind)
		{
			switch (kind)
			{
			case StatusReactionKind::Steam: return StatusReactionPresentationIds::Steam;
			case StatusReactionKind::Magnet: return StatusReactionPresentationIds::Magnet;
			case StatusReactionKind::Plasma: return StatusReactionPresentationIds::Plasma;
			case StatusReactionKind::Fracture: return StatusReactionPresentationIds::Fracture;
			case StatusReactionKind::Superconduct: return StatusReactionPresentationIds::Superconduct;
			case StatusReactionKind::Shrapnel: return StatusReactionPresentationIds::Shrapnel;
			case StatusReactionKind::None: break;
			}
			return StatusReactionPresentationIds::Steam;
		}
	}

	LegendaryReactionCoreRuntime::LegendaryReactionCoreRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime)
		: mAbilities{ abilities }, mCombatRuntime{ combatRuntime }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
	{
		mStatusHandle = mCombatRuntime.onStatusStacksAdded.BindAction(this, &LegendaryReactionCoreRuntime::OnStatusStacksAdded);
	}

	LegendaryReactionCoreRuntime::~LegendaryReactionCoreRuntime()
	{
		mCombatRuntime.onStatusStacksAdded.UnbindAction(mStatusHandle);
		Clear();
	}

	void LegendaryReactionCoreRuntime::Replace(const List<ResolvedModuleLegendaryReactionCoreRule>& rules)
	{
		if (rules.empty()) { Clear(); return; }
		// A rebuild with the module still installed keeps the per-target locks and the hits waiting for pairing.
		mRules = rules;
	}

	void LegendaryReactionCoreRuntime::Tick(float deltaSeconds)
	{
		const float step = std::max(0.f, deltaSeconds);
		for (TargetLock& lock : mLocks) lock.remaining -= step;
		mLocks.erase(std::remove_if(mLocks.begin(), mLocks.end(), [](const TargetLock& lock) { return lock.remaining <= 0.f; }), mLocks.end());
		ProcessPending();
	}

	void LegendaryReactionCoreRuntime::Clear()
	{
		if (mLease) mLease->active = false;
		mLease = std::make_shared<ModuleGeneratedLease>();
		mRules.clear();
		mPending.clear();
		mLocks.clear();
		mReactionCounts.clear();
	}

	int LegendaryReactionCoreRuntime::GetReactionCount(StatusReactionKind kind) const
	{
		const auto found = std::find_if(mReactionCounts.begin(), mReactionCounts.end(), [kind](const auto& entry) { return entry.first == kind; });
		return found != mReactionCounts.end() ? found->second : 0;
	}

	float LegendaryReactionCoreRuntime::GetTargetLockSeconds(std::uint64_t targetId) const
	{
		const auto found = std::find_if(mLocks.begin(), mLocks.end(), [targetId](const TargetLock& lock) { return lock.targetId == targetId; });
		return found != mLocks.end() ? std::max(0.f, found->remaining) : 0.f;
	}

	void LegendaryReactionCoreRuntime::OnStatusStacksAdded(const StatusStacksAddedEvent& event)
	{
		const DamageContext* context = event.context;
		// Only the owner's own real attacks: DoT ticks, generated damage (reactions included) and suppressed spreads never pair.
		if (mRules.empty() || !mShip || !event.target || !context || context->source != mShip || context->deliveryType == DamageDeliveryType::Periodic ||
			context->payload.skipModuleDamageRules || context->payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed) return;
		PendingStack pending;
		pending.target = event.target->GetWeakActor();
		pending.targetId = event.target->GetUniqueID();
		pending.added = event.addedStatuses;
		for (const DamageStatusStackState& state : event.before) pending.carriedBefore.push_back(state.status);
		if (pending.carriedBefore.empty()) return;
		mPending.push_back(std::move(pending));
	}

	void LegendaryReactionCoreRuntime::ProcessPending()
	{
		List<PendingStack> pending = std::move(mPending);
		mPending.clear();
		if (mRules.empty() || !mShip || !IsAlive(*mShip)) return;
		const ResolvedModuleLegendaryReactionCoreRule rule = mRules.front();
		for (const PendingStack& entry : pending)
		{
			const auto target = entry.target.lock();
			auto* combatant = target ? dynamic_cast<Combatant*>(target.get()) : nullptr;
			if (!combatant || !IsAlive(*target) || GetTargetLockSeconds(entry.targetId) > 0.f) continue;
			auto& targetAbilities = combatant->GetAbilitySystemComponent();
			const StatusReactionPair pair = SelectStatusReactionPair(entry.added, entry.carriedBefore, CaptureDamageStatusStacks(targetAbilities));
			if (pair.kind == StatusReactionKind::None) continue;
			ConsumeStatusReactionPair(targetAbilities, pair);
			mLocks.push_back({ entry.targetId, rule.targetCooldownSeconds });
			Fire(rule, pair.kind, *target);
			if (mRules.empty() || !IsAlive(*mShip)) return;
		}
	}

	void LegendaryReactionCoreRuntime::Fire(const ResolvedModuleLegendaryReactionCoreRule& rule, StatusReactionKind kind, Actor& target)
	{
		const auto counter = std::find_if(mReactionCounts.begin(), mReactionCounts.end(), [kind](const auto& entry) { return entry.first == kind; });
		if (counter != mReactionCounts.end()) ++counter->second;
		else mReactionCounts.emplace_back(kind, 1);

		const auto& attributes = mAbilities.GetAttributes();
		const float attackPower = std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::AttackPower));
		const float energyPower = std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::EnergyPower));
		const sf::Vector2f origin = target.GetActorLocation();
		float flashRadius = 0.f;
		switch (kind)
		{
		case StatusReactionKind::Steam:
			ApplyShotDeviation(target, rule.steamDeviationDegrees, rule.steamSeconds, mShip);
			break;
		case StatusReactionKind::Magnet:
			TimedPullActor::Spawn(*mShip, target, rule.magnetRadius, rule.magnetSpeed, rule.magnetSeconds);
			flashRadius = rule.magnetRadius;
			break;
		case StatusReactionKind::Plasma:
			ApplyModuleGeneratedAreaDamage(*mShip, *mShip, mLease, origin, rule.plasmaRadius, rule.plasmaDamage + rule.plasmaEnergyPowerCoefficient * energyPower, { DamageTypeSchema::Thermal });
			flashRadius = rule.plasmaRadius;
			break;
		case StatusReactionKind::Fracture:
			ApplyArmorBreak(target, 1.f, rule.fractureSeconds, rule.bossArmorReductionFraction, mShip);
			break;
		case StatusReactionKind::Superconduct:
			ApplyControlStun(target, rule.stunSeconds, mShip);
			break;
		case StatusReactionKind::Shrapnel:
		{
			FragmentBurstRequest request;
			request.origin = origin;
			request.radius = rule.shrapnelRadius;
			request.fragmentCount = rule.shrapnelCount;
			request.damagePerFragment = rule.shrapnelDamage + rule.shrapnelAttackPowerCoefficient * attackPower + rule.shrapnelEnergyPowerCoefficient * energyPower;
			request.maxHitsPerTarget = rule.shrapnelMaxHitsPerTarget;
			request.damageTags = { DamageTypeSchema::Kinetic };
			ApplyFragmentBurst(*mShip, &target, request);
			flashRadius = rule.shrapnelRadius;
			break;
		}
		case StatusReactionKind::None:
			return;
		}
		if (World* world = mShip->GetWorld()) StatusReactionFlashActor::Spawn(*world, PresentationOf(kind), origin, flashRadius);
	}
}

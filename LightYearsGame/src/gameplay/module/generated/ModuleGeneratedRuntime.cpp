#include "gameplay/module/generated/ModuleGeneratedRuntime.h"
#include "framework/GameplayTag.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/module/generated/ModuleGuidedProjectileActor.h"
#include "gameplay/module/generated/ModuleMineActor.h"
#include "gameplay/module/generated/ModuleSpeedLanceActor.h"
#include "gameplay/module/generated/ModuleAnnularBurstActor.h"
#include "gameplay/module/generated/ModuleDelayedExplosionActor.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/combat/Combatant.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"
#include <algorithm>
#include <cmath>
#include <tuple>
#include <type_traits>
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/combat/EffectConfig.h"

namespace vega
{
	namespace
	{
		void Merge(ModuleRicochetRule& a, const ModuleRicochetRule& b) { a.damageFraction += b.damageFraction; a.range = std::max(a.range, b.range); a.secondaryFraction += b.secondaryFraction; a.transferStatus = a.transferStatus || b.transferStatus; }
		void Merge(ModuleGuidedSalvoRule& a, const ModuleGuidedSalvoRule& b) { if (!a.damageType.IsValid()) a.damageType = b.damageType; a.damageFraction += b.damageFraction; a.projectileCount += b.projectileCount; a.speed = std::max(a.speed, b.speed); a.lifetime = std::max(a.lifetime, b.lifetime); a.targetRange = std::max(a.targetRange, b.targetRange); a.maxActive = std::max(a.maxActive, b.maxActive); }
		void Merge(ModuleTrailMineRule& a, const ModuleTrailMineRule& b)
		{
			if (!a.damageType.IsValid()) a.damageType = b.damageType;
			a.damage += b.damage; a.attackPowerCoefficient += b.attackPowerCoefficient; a.spacing = std::max(a.spacing, b.spacing); a.windowSeconds = std::max(a.windowSeconds, b.windowSeconds);
			if (b.fuseSeconds > 0.f) a.fuseSeconds = a.fuseSeconds > 0.f ? std::min(a.fuseSeconds, b.fuseSeconds) : b.fuseSeconds;
			a.radius = std::max(a.radius, b.radius); a.maxActive = std::max(a.maxActive, b.maxActive); a.additionalLimit += b.additionalLimit; a.cryoStacks += b.cryoStacks;
		}
		// Catalog validation rejects a second delayed or kill explosion on one module, so these never merge two rules.
		void Merge(ModuleDelayedExplosionRule& a, const ModuleDelayedExplosionRule& b) { a = b; }
		void Merge(ModuleKillExplosionRule& a, const ModuleKillExplosionRule& b) { a = b; }
		void Merge(ModuleAnnularBurstRule& a, const ModuleAnnularBurstRule& b) { a = b; }
		void Merge(ModuleSpeedLanceRule& a, const ModuleSpeedLanceRule& b) { a.speedDamageCoefficient += b.speedDamageCoefficient; a.sampleSeconds = std::max(a.sampleSeconds, b.sampleSeconds); a.projectileSpeed = std::max(a.projectileSpeed, b.projectileSpeed); a.range = std::max(a.range, b.range); a.echoDelaySeconds = std::max(a.echoDelaySeconds, b.echoDelaySeconds); a.echoFraction = std::max(a.echoFraction, b.echoFraction); a.firstTargetElectricStacks += b.firstTargetElectricStacks; }
		List<shared_ptr<Actor>> Nearest(World& world, Actor& source, const sf::Vector2f& origin, float range)
		{
			auto targets = targeting::FindOpposingCombatants(world, source, origin, range, false);
			std::sort(targets.begin(), targets.end(), [origin](const auto& a, const auto& b)
			{
				const auto da = a->GetActorLocation() - origin; const auto db = b->GetActorLocation() - origin;
				const float left = da.x * da.x + da.y * da.y; const float right = db.x * db.x + db.y * db.y;
				return left == right ? a->GetUniqueID() < b->GetUniqueID() : left < right;
			});
			return targets;
		}
		bool Has(const List<GameplayTag>& tags, const GameplayTag& wanted) { return std::any_of(tags.begin(), tags.end(), [&wanted](const auto& tag) { return tag.MatchesTag(wanted); }); }
	}
	ModuleGeneratedRuntime::ModuleGeneratedRuntime(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
	{
		mEventHandle = mAbilities.onGameplayEvent.BindAction(this, &ModuleGeneratedRuntime::OnEvent);
	}
	ModuleGeneratedRuntime::~ModuleGeneratedRuntime()
	{
		mShuttingDown = true;
		mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
		for (auto& state : mRicochets) Cancel(state.lease);
		for (auto& state : mSalvos) Cancel(state.lease);
		for (auto& state : mAftershocks) Cancel(state.lease);
		for (auto& state : mMines) Cancel(state.lease);
		for (auto& state : mKillExplosions) Cancel(state.lease);
		for (auto& state : mLances) Cancel(state.lease);
		for (auto& state : mAnnularBursts) Cancel(state.lease);
	}
	void ModuleGeneratedRuntime::Cancel(const std::shared_ptr<ModuleGeneratedLease>& lease) noexcept
	{
		if (!lease) return;
		lease->active = false;
		const auto actors = std::move(lease->actors);
		lease->actors.clear();
		for (const auto& weak : actors) if (const auto actor = weak.lock()) { try { actor->Destroy(); } catch (...) {} }
	}
	void ModuleGeneratedRuntime::Prune(const std::shared_ptr<ModuleGeneratedLease>& lease)
	{
		auto& actors = lease->actors;
		actors.erase(std::remove_if(actors.begin(), actors.end(), [](const auto& weak) { const auto actor = weak.lock(); return !actor || actor->GetIsPendingDestroy(); }), actors.end());
	}
	void ModuleGeneratedRuntime::Replace(const ResolvedModuleGeneratedRules& rules) { if (!mShuttingDown) { mPending = rules; Flush(); } }
	void ModuleGeneratedRuntime::Reconcile(const ResolvedModuleGeneratedRules& rules)
	{
		const auto update = [this](const auto& desired, auto& current)
		{
			using States = std::decay_t<decltype(current)>;
			States next;
			for (const auto& resolved : desired)
			{
				auto found = std::find_if(next.begin(), next.end(), [&resolved](const auto& state) { return state.instanceId == resolved.instanceId; });
				if (found == next.end()) { typename States::value_type state; state.instanceId = resolved.instanceId; state.rule = resolved.rule; next.push_back(std::move(state)); }
				else Merge(found->rule, resolved.rule);
			}
			for (auto& state : next)
			{
				auto previous = std::find_if(current.begin(), current.end(), [&state](const auto& value) { return value.instanceId == state.instanceId && value.rule == state.rule; });
				if (previous != current.end()) { state = *previous; previous->lease.reset(); }
				else { state.lease = std::make_shared<ModuleGeneratedLease>(); state.lease->key = mNextKey++; }
			}
			for (auto& previous : current) Cancel(previous.lease);
			current = std::move(next);
		};
		update(rules.ricochets, mRicochets); update(rules.guidedSalvos, mSalvos); update(rules.delayedExplosions, mAftershocks); update(rules.trailMines, mMines); update(rules.killExplosions, mKillExplosions); update(rules.speedLances, mLances); update(rules.annularBursts, mAnnularBursts);
		mLanceWindows.erase(std::remove_if(mLanceWindows.begin(), mLanceWindows.end(), [](const auto& window) { return !window.lease->active; }), mLanceWindows.end());
		mLanceEchoes.erase(std::remove_if(mLanceEchoes.begin(), mLanceEchoes.end(), [](const auto& echo) { return !echo.lease->active; }), mLanceEchoes.end());
	}
	void ModuleGeneratedRuntime::OnEvent(const sas::AbilityEvent& event)
	{
		if (mShuttingDown || !mShip || GetRuleCount() == 0) return;
		Event snapshot; snapshot.tag = event.eventTag; snapshot.abilityId = event.sourceAbilityId; snapshot.abilityTags = event.sourceAbilityTags;
		if (event.eventTag == GameplayTags::Event::Ability::Activated)
		{
			const auto* ability = mAbilities.GetAbilityById(event.sourceAbilityId.ToString());
			if (!ability || !sas::IsLoadoutAbilitySlot(ability->GetDefinition().slot)) return;
		}
		else
		{
			const auto* context = event.GetContext<DamageContext>();
			if (!context || context->source != mShip || context->appliedDamage <= 0.f || context->payload.skipModuleDamageRules || context->payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed) return;
			if (event.eventTag != GameplayTags::Event::Source::DamageDealt && event.eventTag != GameplayTags::Event::Combat::KillConfirmed) return;
			if (event.eventTag == GameplayTags::Event::Combat::KillConfirmed && (!context->targetWasKilled || !context->targetWasEnemyCombatant)) return;
			snapshot.hitGroup = context->payload.hitGroup;
			snapshot.target = context->target ? context->target->GetWeakActor() : weak_ptr<Actor>{};
			snapshot.origin = { context->targetLocationAtResolution.x, context->targetLocationAtResolution.y };
			snapshot.damage = context->appliedDamage;
			snapshot.damageTags = context->damageTags;
			snapshot.targetMaxHealth = context->targetMaxHealthAtResolution;
			snapshot.targetStatuses = context->targetStatusesAtResolution;
			snapshot.targetStatusEffects = context->targetStatusEffectsAtResolution;
			if (const auto* projectile = dynamic_cast<const PrimaryWeaponProjectileActor*>(context->deliveryActor))
				snapshot.primaryProjectile = ModuleRicochetProjectileSnapshot{ projectile->GetPresentationDefinition(), projectile->GetDamageAttributes(), projectile->GetProjectileSpeed() };
		}
		mEvents.push_back(std::move(snapshot));
		Flush();
	}
	void ModuleGeneratedRuntime::Flush()
	{
		if (mBusy || mShuttingDown) return;
		const auto lifetime = mShip ? mShip->GetWeakActor().lock() : shared_ptr<Actor>{};
		ModuleTriggerService::EventScope moduleEvent{ mAbilities.GetModuleTriggerService() };
		mBusy = true;
		try
		{
			while (mPending || !mEvents.empty())
			{
				if (mPending) { const auto desired = std::move(*mPending); mPending.reset(); Reconcile(desired); }
				if (!mEvents.empty()) { auto event = std::move(mEvents.front()); mEvents.erase(mEvents.begin()); Execute(event); }
			}
		}
		catch (...) { mBusy = false; throw; }
		mBusy = false;
	}
	bool ModuleGeneratedRuntime::Force(std::uint32_t instanceId, float powerMultiplier)
	{
		if (mBusy || mShuttingDown) return false;
		const auto serial = mFiredSerial;
		Event event; event.forcedInstanceId = instanceId; event.power = powerMultiplier;
		mEvents.push_back(std::move(event)); Flush();
		return serial != mFiredSerial;
	}
	void ModuleGeneratedRuntime::Execute(const Event& event)
	{
		if (!mShip || !mShip->GetWorld() || mShip->GetIsPendingDestroy() || mShip->GetHealthComponent().GetHealth() <= 0.f) return;
		if (event.tag == GameplayTags::Event::Ability::Activated || event.forcedInstanceId)
		{
			auto& triggers = mAbilities.GetModuleTriggerService();
			const auto accepts = [&](std::uint32_t id, const GameplayTag& family) { return event.forcedInstanceId ? id == event.forcedInstanceId : triggers.MatchesAbilityEventTags(id, { family }, event.abilityTags); };
			const auto report = [&](std::uint32_t id) { ++mFiredSerial; triggers.Notify({ id, ModuleRuleKind::Generated, 0 }, event.forcedInstanceId == 0); };
			const auto* profile = PresentationProfileRegistry<ModuleAnnularBurstPresentationProfile>::Find(ModuleGeneratedPresentationIds::AnnularBurst.ToString());
			if (profile) for (auto& state : mAnnularBursts)
			{
				if (!accepts(state.instanceId, GameplayTags::Ability::Utility)) continue;
				const auto lease = state.lease; const auto rule = state.rule; Prune(lease);
				const float damage = (rule.baseDamage + std::max(0.f, mAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)) * rule.energyPowerCoefficient) * event.power;
				const auto actor = mShip->GetWorld()->SpawnActor<ModuleAnnularBurstActor>(mShip->GetWeakActor(), *profile, lease, rule.innerRadius, rule.outerRadius).lock();
				if (actor) { actor->SetActorLocation(mShip->GetActorLocation()); lease->actors.push_back(actor); actor->Detonate(damage); report(state.instanceId); }
				if (mPending || !lease->active || mShip->GetIsPendingDestroy()) return;
			}
			for (auto& state : mMines)
				if (accepts(state.instanceId, GameplayTags::Ability::Movement)) { state.window = state.rule.windowSeconds; state.carriedDistance = 0.f; state.power = event.power; report(state.instanceId); }
			for (const auto& state : mLances)
				if (accepts(state.instanceId, GameplayTags::Ability::Movement))
				{
					auto rule = state.rule; rule.speedDamageCoefficient *= event.power;
					mLanceWindows.push_back({ state.lease, rule, rule.sampleSeconds, 0.f }); report(state.instanceId);
				}
		}
		else if (event.tag == GameplayTags::Event::Source::DamageDealt)
		{
			Ricochet(event);
			if (!mPending) SpawnSalvos(event);
			if (!mPending) SpawnExplosions(event);
		}
		else SpawnExplosions(event);
	}
	void ModuleGeneratedRuntime::Ricochet(const Event& event)
	{
		if (!event.hitGroup || event.hitGroup->kind != DamageHitGroup::Kind::PrimaryShot) return;
		const auto source = mShip->GetWeakActor().lock();
		const auto originalTarget = event.target.lock();
		for (auto& state : mRicochets)
		{
			if (state.rule.damageFraction <= 0.f || !event.hitGroup->consumedModuleRules.insert({ mShip->GetUniqueID(), state.lease->key }).second) continue;
			mAbilities.GetModuleTriggerService().Notify({ state.instanceId, ModuleRuleKind::Generated, 0 });
			List<std::uint64_t> visited;
			if (originalTarget) visited.push_back(originalTarget->GetUniqueID());
			if (event.primaryProjectile)
			{
				Prune(state.lease);
				const auto status = state.rule.transferStatus && !event.targetStatusEffects.empty() ? std::optional<sas::GameplayEffectSpec>{ event.targetStatusEffects.front() } : std::nullopt;
				ModuleRicochetProjectileActor::Spawn(*mShip, *event.primaryProjectile, event.origin, state.lease, visited, state.rule.range, event.damage * state.rule.damageFraction, event.damageTags, event.damage * state.rule.secondaryFraction, status);
				continue;
			}
			auto origin = event.origin;
			for (float fraction : { state.rule.damageFraction, state.rule.secondaryFraction })
			{
				if (fraction <= 0.f || !state.lease->active || mPending) break;
				const auto targets = Nearest(*mShip->GetWorld(), *mShip, origin, state.rule.range);
				const auto selected = std::find_if(targets.begin(), targets.end(), [&visited](const auto& target) { return std::find(visited.begin(), visited.end(), target->GetUniqueID()) == visited.end(); });
				if (selected == targets.end())
				{
					const auto* profile = PresentationProfileRegistry<ModuleGuidedPresentationProfile>::Find(ModuleGeneratedPresentationIds::Guided.ToString());
					Prune(state.lease);
					if (profile && state.lease->actors.size() < 64)
					{
						const float angle = std::uniform_real_distribution<float>{ 0.f, 6.283185307f }(mRandom);
						const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
						const GameplayTag damageType = event.damageTags.empty() ? DamageTypeSchema::Photonic : event.damageTags.front();
						const auto actor = mShip->GetWorld()->SpawnActor<ModuleGuidedProjectileActor>(mShip, *profile, state.lease, weak_ptr<Actor>{}, 600.f, state.rule.range / 600.f, event.damage * fraction, damageType).lock();
						if (actor)
						{
							actor->SetActorLocation(origin);
							actor->SetDamageTags(event.damageTags);
							actor->SetUnguidedDirection(direction, visited);
							state.lease->actors.push_back(actor);
						}
					}
					break;
				}
				const auto target = *selected;
				visited.push_back(target->GetUniqueID()); origin = target->GetActorLocation();
				ApplyCombatDamage(*target, event.damage * fraction, source.get(), event.damageTags, MakeModuleGeneratedDamagePayload(), {}, {}, DamageDeliveryType::Direct);
				if (state.rule.transferStatus && state.lease->active && !mPending && !target->GetIsPendingDestroy() && !event.targetStatusEffects.empty())
				{
					auto spec = event.targetStatusEffects.front();
					spec.attributes.emplace_back(DamageAttributeIds::SuppressModuleTriggers, 1.f, 1.f);
					if (auto* combatant = dynamic_cast<Combatant*>(target.get()))
					{
						auto& abilities = combatant->GetAbilitySystemComponent();
						const auto* existing = abilities.FindGameplayEffectById(spec.definition.effectId);
						// A capped refresh retains its old payload; do not create unsuppressed
						// periodic work by extending an unrelated existing status at its cap.
						if (!existing || existing->stackCount < spec.maxStacks) abilities.ApplyGameplayEffect(spec, source.get());
					}
				}
			}
			if (mPending) return;
		}
	}
	void ModuleGeneratedRuntime::SpawnSalvos(const Event& event)
	{
		if (!event.hitGroup || event.hitGroup->kind != DamageHitGroup::Kind::AbilityUse || !Has(event.abilityTags, GameplayTags::Ability::Offense)) return;
		const auto* profile = PresentationProfileRegistry<ModuleGuidedPresentationProfile>::Find(ModuleGeneratedPresentationIds::Guided.ToString());
		if (!profile) return;
		for (auto& state : mSalvos)
		{
			if (state.rule.damageFraction <= 0.f || !state.rule.damageType.IsValid() || !event.hitGroup->consumedModuleRules.insert({ mShip->GetUniqueID(), state.lease->key }).second) continue;
			Prune(state.lease);
			mAbilities.GetModuleTriggerService().Notify({ state.instanceId, ModuleRuleKind::Generated, 1 });
			const auto targets = Nearest(*mShip->GetWorld(), *mShip, mShip->GetActorLocation(), state.rule.targetRange);
			for (std::size_t index = 0; !targets.empty() && index < static_cast<std::size_t>(state.rule.projectileCount) && state.lease->actors.size() < static_cast<std::size_t>(state.rule.maxActive); ++index)
			{
				const auto actor = mShip->GetWorld()->SpawnActor<ModuleGuidedProjectileActor>(mShip, *profile, state.lease, targets[index % targets.size()], state.rule.speed, state.rule.lifetime, event.damage * state.rule.damageFraction, state.rule.damageType).lock();
				if (!actor) continue;
				actor->SetActorLocation(mShip->GetActorLocation()); state.lease->actors.push_back(actor);
			}
		}
	}
	void ModuleGeneratedRuntime::SpawnExplosions(const Event& event)
	{
		const auto spawn = [this, &event](auto& state, const sas::ContentId& profileId, float damage, float radius, float delay, const List<GameplayTag>& tags)
		{
			Prune(state.lease);
			if (state.lease->actors.size() >= static_cast<std::size_t>(state.rule.maxActive)) return;
			const auto* profile = PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Find(profileId.ToString());
			if (!profile) return;
			const auto actor = mShip->GetWorld()->SpawnActor<ModuleDelayedExplosionActor>(mShip->GetWeakActor(), *profile, state.lease, damage, radius, delay, tags).lock();
			if (actor) { actor->SetActorLocation(event.origin); state.lease->actors.push_back(actor); mAbilities.GetModuleTriggerService().Notify({ state.instanceId, ModuleRuleKind::Generated, 2 }); }
		};
		if (event.tag == GameplayTags::Event::Source::DamageDealt && event.hitGroup && event.hitGroup->kind == DamageHitGroup::Kind::AbilityUse && Has(event.abilityTags, GameplayTags::Ability::Offense))
			for (auto& state : mAftershocks)
				if (event.hitGroup->consumedModuleRules.insert({ mShip->GetUniqueID(), state.lease->key }).second) spawn(state, ModuleGeneratedPresentationIds::Explosion, event.damage * state.rule.damageFraction, state.rule.radius, state.rule.delaySeconds, event.damageTags);
		if (event.tag == GameplayTags::Event::Combat::KillConfirmed)
			for (auto& state : mKillExplosions)
				if (std::all_of(state.rule.requiredTargetStatuses.begin(), state.rule.requiredTargetStatuses.end(), [&event](const auto& tag) { return Has(event.targetStatuses, tag); })) spawn(state, state.rule.presentationId, event.targetMaxHealth * state.rule.maxHealthFraction * mRunCounterDamageMultiplier, state.rule.radius, 0.f, { state.rule.damageType });
	}
	void ModuleGeneratedRuntime::DropMines(float deltaSeconds)
	{
		const auto* profile = PresentationProfileRegistry<ModuleMinePresentationProfile>::Find(ModuleGeneratedPresentationIds::Mine.ToString());
		if (!profile || !mShip->GetWorld()) return;
		const auto& movement = mShip->GetMovementComponent();
		const auto origin = movement.GetCurrentMovementStartLocation();
		const auto end = mShip->GetActorLocation();
		for (auto& state : mMines)
		{
			const float activeSeconds = std::min(deltaSeconds, state.window);
			state.window = std::max(0.f, state.window - deltaSeconds);
			if (activeSeconds <= 0.f || state.rule.damage <= 0.f || !state.rule.damageType.IsValid() || state.rule.spacing <= 0.f) continue;
			const float distance = mShip->IsInPortalTransit() ? 0.f : movement.GetCurrentMovementDistance() * activeSeconds / deltaSeconds;
			if (distance <= 0.f) continue;
			Prune(state.lease);
			float next = state.rule.spacing - state.carriedDistance;
			const std::size_t limit = static_cast<std::size_t>(state.rule.maxActive + state.rule.additionalLimit);
			while (next <= distance && state.lease->actors.size() < limit)
			{
				const float damage = (state.rule.damage + state.rule.attackPowerCoefficient * std::max(0.f, mAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower))) * state.power;
				const auto actor = mShip->GetWorld()->SpawnActor<ModuleMineActor>(mShip->GetWeakActor(), *profile, state.lease, damage, state.rule.radius, state.rule.fuseSeconds, state.rule.cryoStacks, state.rule.damageType).lock();
				if (!actor) break;
				actor->SetActorLocation(origin + (end - origin) * (next / movement.GetCurrentMovementDistance()));
				state.lease->actors.push_back(actor); next += state.rule.spacing;
			}
			state.carriedDistance = std::fmod(state.carriedDistance + distance, state.rule.spacing);
		}
	}
	void ModuleGeneratedRuntime::SpawnLance(const std::shared_ptr<ModuleGeneratedLease>& lease, const ModuleSpeedLanceRule& rule, float damage, const sf::Vector2f& direction)
	{
		if (!lease || !lease->active || !mShip || !mShip->GetWorld() || mShip->GetIsPendingDestroy() || mShip->GetHealthComponent().GetHealth() <= 0.f || damage <= 0.f) return;
		const auto* profile = PresentationProfileRegistry<ModuleSpeedLancePresentationProfile>::Find(ModuleGeneratedPresentationIds::SpeedLance.ToString());
		if (!profile) return;
		Prune(lease);
		const auto actor = mShip->GetWorld()->SpawnActor<ModuleSpeedLanceActor>(mShip, *profile, lease, direction, rule.projectileSpeed, rule.range, damage, rule.firstTargetElectricStacks).lock();
		if (actor) { actor->SetActorLocation(mShip->GetActorLocation()); lease->actors.push_back(actor); }
	}
	void ModuleGeneratedRuntime::TickLances(float deltaSeconds)
	{
		List<LanceEcho> due;
		for (auto& echo : mLanceEchoes) { echo.remaining -= deltaSeconds; if (echo.remaining <= 0.f && echo.lease->active) due.push_back(echo); }
		mLanceEchoes.erase(std::remove_if(mLanceEchoes.begin(), mLanceEchoes.end(), [](const auto& echo) { return echo.remaining <= 0.f || !echo.lease->active; }), mLanceEchoes.end());
		List<LanceWindow> ready;
		for (auto& window : mLanceWindows)
		{
			if (!window.lease->active) continue;
			const float speed = mShip->IsInPortalTransit() ? 0.f : mShip->GetMovementComponent().GetCurrentMovementSpeed();
			window.peakSpeed = std::max(window.peakSpeed, speed); window.remaining -= deltaSeconds;
			if (window.remaining <= 0.f) ready.push_back(window);
		}
		mLanceWindows.erase(std::remove_if(mLanceWindows.begin(), mLanceWindows.end(), [](const auto& window) { return window.remaining <= 0.f || !window.lease->active; }), mLanceWindows.end());
		for (const auto& window : ready)
		{
			const float damage = window.peakSpeed * window.rule.speedDamageCoefficient; const auto direction = mShip->GetActorForwardDirection();
			SpawnLance(window.lease, window.rule, damage, direction);
			if (window.rule.echoFraction > 0.f)
			{
				LanceEcho echo{ window.lease, window.rule, window.rule.echoDelaySeconds + window.remaining, damage * window.rule.echoFraction, direction };
				if (echo.remaining <= 0.f) due.push_back(echo); else mLanceEchoes.push_back(echo);
			}
		}
		for (const auto& echo : due) SpawnLance(echo.lease, echo.rule, echo.damage, echo.direction);
	}
	void ModuleGeneratedRuntime::Tick(float deltaSeconds)
	{
		if (mShuttingDown || mBusy || !mShip || deltaSeconds <= 0.f) return;
		const auto lifetime = mShip->GetWeakActor().lock();
		if (!lifetime || mShip->GetIsPendingDestroy() || mShip->GetHealthComponent().GetHealth() <= 0.f) { Replace({}); return; }
		ModuleTriggerService::EventScope moduleEvent{ mAbilities.GetModuleTriggerService() };
		mBusy = true;
		try { DropMines(deltaSeconds); TickLances(deltaSeconds); } catch (...) { mBusy = false; throw; }
		mBusy = false;
		Flush();
	}
	std::size_t ModuleGeneratedRuntime::GetActiveActorCount() const
	{
		std::size_t count = 0;
		const auto add = [&count](const auto& states) { for (const auto& state : states) if (state.lease) for (const auto& weak : state.lease->actors) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++count; };
		add(mRicochets); add(mSalvos); add(mAftershocks); add(mMines); add(mKillExplosions); add(mLances); add(mAnnularBursts);
		return count;
	}
	std::size_t ModuleGeneratedRuntime::GetRuleCount() const { return mRicochets.size() + mSalvos.size() + mAftershocks.size() + mMines.size() + mKillExplosions.size() + mLances.size() + mAnnularBursts.size(); }
}

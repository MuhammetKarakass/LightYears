#include "gameplay/ability/AbilityActivationPolicy.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/CollisionLayers.h"
#include "player/PlayerSpaceShip.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace vega
{
	namespace
	{
		std::size_t SlotIndex(sas::AbilitySlot slot) { return static_cast<std::size_t>(static_cast<int>(slot) - static_cast<int>(sas::AbilitySlot::Ability1)); }
		bool HasFamily(const GameAbility& ability, const GameplayTag& family) { const auto& tags = ability.GetDefinition().abilityTags; return std::find(tags.begin(), tags.end(), family) != tags.end(); }
	}
	AbilityAimScope::AbilityAimScope(LightYearsAbilitySystemComponent& owner, const std::optional<AbilityAimOverride>& aim) : mOwner{ owner }, mPrevious{ owner.GetExecutionAim() } { owner.SetExecutionAim(aim); }
	AbilityAimScope::~AbilityAimScope() { mOwner.SetExecutionAim(mPrevious); }
	sf::Vector2f ResolveAbilityAimPoint(const Actor& owner)
	{
		if (const auto* combatant = dynamic_cast<const Combatant*>(&owner))
			if (const auto& aim = combatant->GetAbilitySystemComponent().GetExecutionAim()) return aim->targetPoint;
		return owner.GetWorld() ? owner.GetWorld()->GetMouseWorldPosition() : owner.GetActorLocation() + owner.GetActorForwardDirection();
	}
	sf::Vector2f ResolveAbilityAimDirection(const Actor& owner)
	{
		if (const auto* combatant = dynamic_cast<const Combatant*>(&owner))
			if (const auto& aim = combatant->GetAbilitySystemComponent().GetExecutionAim()) return aim->direction;
		return owner.GetActorForwardDirection();
	}
	std::size_t AbilityActivationPolicy::BankIndex(const GameAbility& ability) const { return SlotIndex(ability.GetRuntimeSlot()) + (mOwner.GetSecondAbility(ability.GetRuntimeSlot()) == &ability ? 4 : 0); }
	void AbilityActivationPolicy::ConfigureTiming(sas::AbilitySlot slot, std::uint32_t source, std::optional<AbilityTimingSpec> spec)
	{
		const auto i = SlotIndex(slot);
		if (mTimingSources[i] != source) { mTimingStates[i] = {}; mTimingStates[i + 4] = {}; }
		mTimingSources[i] = source; mTiming[i] = std::move(spec);
	}
	void AbilityActivationPolicy::ConfigureAutocast(sas::AbilitySlot slot, std::uint32_t source, std::optional<AbilityAutocastSpec> spec)
	{
		const auto i = SlotIndex(slot);
		if (mAutocastSources[i] != source)
		{
			for (const auto bank : { i, i + 4 })
			{
				if (mAutocastStates[bank].charging) if (auto* ability = mOwner.GetAbility(mAutocastStates[bank].handle)) ability->SetInputHeld(false);
				mAutocastStates[bank] = {};
			}
		}
		mAutocastSources[i] = source; mAutocast[i] = std::move(spec);
	}
	void AbilityActivationPolicy::ConfigureRandomPower(sas::AbilitySlot slot, std::optional<AbilityRandomPowerSpec> spec) { mPower[SlotIndex(slot)] = std::move(spec); }
	void AbilityActivationPolicy::ConfigureDamageType(sas::AbilitySlot slot, const GameplayTag& type) { mDamageTypes[SlotIndex(slot)] = type; }
	GameplayTag AbilityActivationPolicy::GetDamageType(sas::AbilitySlot slot) const { return sas::IsLoadoutAbilitySlot(slot) ? mDamageTypes[SlotIndex(slot)] : GameplayTag{}; }
	bool AbilityActivationPolicy::CanAutomate(bool checkActionLock) const
	{
		const auto* ship = dynamic_cast<const PlayerSpaceShip*>(&mOwner.GetOwner());
		return ship && !ship->GetIsPendingDestroy() && ship->GetHealthComponent().GetHealth() > 0.f && !ship->IsPossessing() && !ship->IsInPortalTransit() && (!checkActionLock || !mOwner.HasOwnedTag(GameplayTagSchema::BlockAbilityActivation)) && !mOwner.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned);
	}
	bool AbilityActivationPolicy::StartTimed(GameAbility& ability, TimingState& state, const AbilityTimingSpec& spec)
	{
		const float distance = std::abs(state.elapsed - spec.durationSeconds * spec.windowCenterFraction);
		const bool small = state.elapsed < spec.durationSeconds && distance <= spec.smallWindowSeconds * .5f;
		const bool big = state.elapsed < spec.durationSeconds && distance <= spec.bigWindowSeconds * .5f;
		state.pending = small ? spec.small : big ? spec.big : spec.miss;
		state.open = false;
		const bool activated = ability.TryActivateWithInput(state.held);
		if (activated) { state.feedback = small ? "PERFECT" : big ? "GOOD" : "MISS"; state.good = big; state.feedbackRemaining = spec.feedbackSeconds; }
		state.pending.reset();
		return activated;
	}
	bool AbilityActivationPolicy::HandleInput(GameAbility& ability, bool held)
	{
		if (!sas::IsLoadoutAbilitySlot(ability.GetRuntimeSlot())) return false;
		const auto index = BankIndex(ability), slot = index % 4;
		if (mAutocastStates[index].charging) return true;
		if (!mTiming[slot]) return false;
		auto& state = mTimingStates[index];
		if (!(state.handle == ability.GetHandle())) { state = {}; state.handle = ability.GetHandle(); }
		const bool pressed = held && !state.held;
		state.held = held;
		if (state.open)
		{
			if (pressed) StartTimed(ability, state, *mTiming[slot]);
			return true;
		}
		if (pressed && !ability.IsActive() && ability.CanActivateNow())
		{
			state.open = true; state.elapsed = 0.f; state.feedback.clear();
			ability.SetInputHeld(false);
			return true;
		}
		return false;
	}
	AbilityActivationModifiers AbilityActivationPolicy::PrepareActivation(GameAbility& ability)
	{
		if (!sas::IsLoadoutAbilitySlot(ability.GetRuntimeSlot()) || ability.GetActivationOrigin() != sas::AbilityActivationOrigin::NormalInput) return {};
		const auto index = BankIndex(ability);
		if (mTimingStates[index].pending) return *mTimingStates[index].pending;
		if (const auto& spec = mPower[index % 4])
		{
			std::uniform_real_distribution<float> roll{ spec->minimum, spec->maximum }, chance{ 0.f, 1.f };
			float value = roll(mRandom);
			const float probability = std::clamp(spec->doubleRollChancePerLuck * mOwner.GetAttributes().GetCurrentValue(OwnerAttributeIds::Luck), 0.f, 1.f);
			if (chance(mRandom) < probability) value = std::max(value, roll(mRandom));
			mRolledPower[index] = value;
			return { value, 1.f };
		}
		return {};
	}
	void AbilityActivationPolicy::CommitActivation(GameAbility& ability)
	{
		if (!sas::IsLoadoutAbilitySlot(ability.GetRuntimeSlot()) || ability.GetActivationOrigin() != sas::AbilityActivationOrigin::NormalInput) return;
		const auto index = BankIndex(ability);
		if (const auto& spec = mPower[index % 4])
		{
			char text[32]; std::snprintf(text, sizeof(text), "x%.2f", mRolledPower[index]);
			auto& state = mTimingStates[index]; state.feedback = text; state.good = mRolledPower[index] >= 1.f; state.feedbackRemaining = spec->feedbackSeconds;
		}
	}
	float AbilityActivationPolicy::ResolveAutocastRange(GameAbility& ability, float fallback) const
	{
		auto definition = ability.GetDefinition();
		definition.mainEffectMultiplier = mOwner.GetAbilityMainEffectMultiplier(definition, ability.GetHandle());
		AbilityExecutionContext context{ &mOwner, &definition, nullptr, &ability };
		const auto values = AbilityActionAttributeResolver::ResolveAbilityAttributes(context);
		const auto read = [](const sas::GameplayAttributeList& attributes, bool area)
		{
			if (!area) if (const float common = sas::FindAttributeValue(attributes, CommonAttributeIds::Range, 0.f); common > 0.f) return common;
			float range = 0.f;
			for (const auto& value : attributes)
			{
				const std::string name{ value.id.GetName() };
				const auto ends = [&name](const std::string& suffix) { return name.size() >= suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0; };
				if ((!area && (ends("Range") || ends("Length") || ends("MaximumDistance") || ends("MaxTravelDistance"))) || (area && ends("Radius") && value.id != CollisionAttributeIds::Radius))
					range = std::max(range, value.currentValue);
			}
			return range;
		};
		List<sas::GameplayAttributeList> actorValues;
		List<sas::ContentId> actors = definition.actorDefinitionIds;
		for (const auto& action : definition.actions) if (const auto* spawn = std::get_if<SpawnActorAction>(&action.action))
			if (std::find(actors.begin(), actors.end(), spawn->actorDefinitionId) == actors.end()) actors.push_back(spawn->actorDefinitionId);
		for (const auto& actorId : actors) if (const auto* actor = AbilityData::FindAbilityActorDefinition(actorId.ToString())) actorValues.push_back(AbilityActionAttributeResolver::ResolveAttributes(context, nullptr, actor->attributes));
		float range = read(values, false);
		if (range <= 0.f) for (const auto& attributes : actorValues) range = std::max(range, read(attributes, false));
		if (range <= 0.f)
		{
			range = read(values, true);
			for (const auto& attributes : actorValues) range = std::max(range, read(attributes, true));
		}
		return ability.ResolveAutocastRange(range > 0.f ? range : fallback);
	}
	void AbilityActivationPolicy::Tick(float deltaTime)
	{
		const float dt = std::max(0.f, deltaTime);
		for (std::size_t i = 0; i < 8; ++i)
		{
			auto& timing = mTimingStates[i]; timing.feedbackRemaining = std::max(0.f, timing.feedbackRemaining - dt);
			const auto slot = static_cast<sas::AbilitySlot>(static_cast<int>(sas::AbilitySlot::Ability1) + static_cast<int>(i % 4));
			auto* ability = i < 4 ? mOwner.GetAbility(slot) : mOwner.GetSecondAbility(slot);
			if (!ability || (i >= 4 && !mOwner.IsTwinRackEnabled())) { timing.open = false; continue; }
			if (timing.open)
			{
				if (!CanAutomate() || !(timing.handle == ability->GetHandle()) || !ability->CanActivateNow()) timing.open = false;
				else if (mTiming[i % 4]) { timing.elapsed += dt; if (timing.elapsed >= mTiming[i % 4]->durationSeconds) StartTimed(*ability, timing, *mTiming[i % 4]); }
			}
			const auto& spec = mAutocast[i % 4];
			if (!spec) continue;
			auto& automatic = mAutocastStates[i];
			if (automatic.charging)
			{
				if (!(automatic.handle == ability->GetHandle())) { automatic = {}; continue; }
				if (!CanAutomate(false)) { ability->SetInputHeld(false); if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Interrupted); automatic = {}; continue; }
				automatic.elapsed += dt;
				if (!ability->IsActive() || automatic.elapsed >= ability->GetActiveDuration()) { ability->SetInputHeld(false); automatic = {}; }
				continue;
			}
			if (!CanAutomate() || ability->IsActive() || !ability->CanActivateNow()) continue;
			auto* ship = dynamic_cast<PlayerSpaceShip*>(&mOwner.GetOwner());
			std::optional<AbilityAimOverride> aim;
			if (HasFamily(*ability, GameplayTags::Ability::Movement))
			{
				auto direction = ship->GetMovementComponent().GetAbilityWorldMovementInput();
				if (GetVectorLength(direction) <= .001f || mOwner.HasOwnedTag(GameplayTagSchema::BlockMovementInput)) continue;
				NormalizeVector(direction); aim = AbilityAimOverride{ direction, ship->GetActorLocation() + direction * ResolveAutocastRange(*ability, spec->defaultRange) };
			}
			else
			{
				targeting::TargetingQuery query; query.source = ship; query.origin = ship->GetActorLocation(); query.range = ResolveAutocastRange(*ability, spec->defaultRange); query.maxTargets = 1; query.requiredTargetLayers = GameCollision::Enemy;
				query.filter = [](const Actor*, const Actor& actor, const targeting::TargetingCandidate&)
				{
					const auto* combatant = dynamic_cast<const Combatant*>(&actor);
					const auto* targetShip = dynamic_cast<const SpaceShip*>(&actor);
					const auto* portal = dynamic_cast<const PortalTransferParticipant*>(&actor);
					return combatant && !actor.GetIsPendingDestroy() && (!targetShip || targetShip->GetHealthComponent().GetHealth() > 0.f) && (!portal || !portal->IsInPortalTransit());
				};
				const auto target = ship->GetWorld() ? targeting::AutoTargeting::FindTarget(*ship->GetWorld(), query).lock() : nullptr;
				const bool defense = HasFamily(*ability, GameplayTags::Ability::Defense);
				if (!target && !(defense && mDamageAge <= spec->recentDamageSeconds)) continue;
				if (!defense && !HasFamily(*ability, GameplayTags::Ability::Offense) && !HasFamily(*ability, GameplayTags::Ability::Utility)) continue;
				auto direction = target ? target->GetActorLocation() - ship->GetActorLocation() : ship->GetActorForwardDirection();
				if (GetVectorLength(direction) <= .001f) direction = ship->GetActorForwardDirection();
				NormalizeVector(direction); aim = AbilityAimOverride{ direction, target ? target->GetActorLocation() : ship->GetActorLocation() + direction * query.range };
			}
			ability->SetActivationAim(aim);
			if (ability->TryActivateWithInput(true))
			{
				if (ability->IsActive()) { automatic.handle = ability->GetHandle(); automatic.charging = true; automatic.elapsed = 0.f; }
				else ability->SetInputHeld(false);
			}
			else { ability->SetInputHeld(false); ability->SetActivationAim(std::nullopt); }
		}
	}
	AbilityTimingView AbilityActivationPolicy::GetView(sas::AbilitySlot slot, bool second) const
	{
		AbilityTimingView view;
		if (!sas::IsLoadoutAbilitySlot(slot)) return view;
		const auto index = SlotIndex(slot); const auto& state = mTimingStates[index + (second ? 4 : 0)];
		view.open = state.open; view.good = state.good;
		if (state.feedbackRemaining > 0.f) view.feedback = state.feedback;
		if (const auto& spec = mTiming[index])
		{
			view.progress = std::clamp(state.elapsed / spec->durationSeconds, 0.f, 1.f);
			view.bigStart = spec->windowCenterFraction - spec->bigWindowSeconds / (2.f * spec->durationSeconds); view.bigEnd = spec->windowCenterFraction + spec->bigWindowSeconds / (2.f * spec->durationSeconds);
			view.smallStart = spec->windowCenterFraction - spec->smallWindowSeconds / (2.f * spec->durationSeconds); view.smallEnd = spec->windowCenterFraction + spec->smallWindowSeconds / (2.f * spec->durationSeconds);
		}
		return view;
	}
	bool AbilityActivationPolicy::HasConfiguration(sas::AbilitySlot slot) const
	{
		if (!sas::IsLoadoutAbilitySlot(slot)) return false;
		const auto i = SlotIndex(slot); return mTiming[i].has_value() || mAutocast[i].has_value() || mPower[i].has_value() || mDamageTypes[i].IsValid();
	}
	void AbilityActivationPolicy::Clear() { mTimingStates = {}; mAutocastStates = {}; }
}

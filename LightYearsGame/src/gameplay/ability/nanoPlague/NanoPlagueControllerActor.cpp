#include "gameplay/ability/nanoPlague/NanoPlagueControllerActor.h"

#include "framework/World.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <exception>
#include <limits>

namespace ly
{
	namespace
	{
		weak_ptr<Actor> MakeWeakActor(Actor& actor)
		{
			const shared_ptr<Object> object = actor.GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<Actor>(object)
				: weak_ptr<Actor>{};
		}
	}

	NanoPlagueControllerActor::NanoPlagueControllerActor(
		World* world,
		Actor* owner,
		const NanoPlaguePresentationProfile& profile
	)
		: Actor(world)
		, mOwner(owner ? MakeWeakActor(*owner) : weak_ptr<Actor>{})
		, mProfile(profile)
	{
		SetRenderLayer(RenderLayer::World);
		SetEnablePhysics(false);
	}

	NanoPlagueControllerActor::~NanoPlagueControllerActor()
	{
		mDestroying = true;
		mRegistryRegistration.Reset();
		while (!mInfections.empty())
		{
			try { RemoveInfectionById(mInfections.begin()->first); }
			catch (...) {} // Destruction must still release every subscription.
		}
	}

	shared_ptr<NanoPlagueControllerActor> NanoPlagueControllerActor::FindOrCreate(
		World& world,
		Actor& owner,
		const NanoPlaguePresentationProfile& profile
	)
	{
		if (owner.GetIsPendingDestroy() || owner.GetWorld() != &world)
		{
			return {};
		}

		const weak_ptr<Actor> ownerWeak = MakeWeakActor(owner);
		const shared_ptr<Actor> managedOwner = ownerWeak.lock();
		if (!managedOwner || managedOwner.get() != &owner) return {};

		const shared_ptr<NanoPlagueControllerRegistryActor> registry =
			NanoPlagueControllerRegistryActor::GetOrCreate(world);
		if (!registry) return {};
		if (const shared_ptr<NanoPlagueControllerActor> existing =
			registry->FindActiveController(owner))
		{
			return existing;
		}

		const shared_ptr<NanoPlagueControllerActor> controller =
			world.SpawnActor<NanoPlagueControllerActor>(&owner, profile).lock();
		if (!controller) return {};

		controller->mRegistryRegistration = registry->RegisterController(owner, controller);
		if (!controller->mRegistryRegistration.IsValid())
		{
			controller->Destroy();
			return {};
		}
		return controller;
	}

	bool NanoPlagueControllerActor::ApplyOrRefreshInfection(
		Actor& target,
		int generation,
		const Settings& settings,
		const sf::Vector2f* visualOrigin
	)
	{
		const std::uint64_t safeGeneration = generation > 0
			? static_cast<std::uint64_t>(generation)
			: 0;
		return ApplyOrRefreshInfectionInternal(target, safeGeneration, settings, visualOrigin);
	}

	bool NanoPlagueControllerActor::ApplyOrRefreshInfectionInternal(
		Actor& target,
		std::uint64_t generation,
		const Settings& settings,
		const sf::Vector2f* visualOrigin
	)
	{
		if (mDestroying || GetIsPendingDestroy() || target.GetIsPendingDestroy() ||
			dynamic_cast<Combatant*>(&target) == nullptr)
		{
			return false;
		}
		const shared_ptr<Actor> owner = GetOwnerActor();
		if (!owner) return false;
		const sf::Vector2f origin = visualOrigin ? *visualOrigin : owner->GetActorLocation();
		Combatant& targetCombatant = dynamic_cast<Combatant&>(target);
		auto existing = std::find_if(mInfections.begin(), mInfections.end(), [&target](const auto& entry)
		{
			return entry.second.target.lock().get() == &target;
		});
		// The owner-local map handles refreshes; this tag excludes other controllers.
		if (existing == mInfections.end() && targetCombatant.GetAbilitySystemComponent().HasOwnedTag(
			AbilityData::NanoPlague::State::Infected,
			true
		))
		{
			return false;
		}

		mSettings = settings;
		mSettings.duration = std::max(0.01f, mSettings.duration);
		mSettings.tickInterval = std::max(0.01f, mSettings.tickInterval);
		mSettings.baseSpreadTargetCount = std::max(1, mSettings.baseSpreadTargetCount);

		if (existing != mInfections.end())
		{
			Infection& infection = existing->second;
			infection.remainingDuration = mSettings.duration;
			++infection.revision;
			infection.tickAccumulator = 0.f;
			infection.ticksApplied = 0;
			// A direct infection must never lose its one spread generation because
			// a later generation-one application refreshed the same target.
			infection.generation = std::min(infection.generation, generation);
			AddPulse(origin, target, generation > 0);
			return true;
		}

		Infection infection;
		infection.id = mNextInfectionId++;
		infection.target = MakeWeakActor(target);
		infection.generation = generation;
		infection.remainingDuration = mSettings.duration;
		const auto id = infection.id;
		auto& stored = mInfections.emplace(id, std::move(infection)).first->second;
		try
		{
			stored.damageSubscription = targetCombatant.GetCombatRuntime().onDamageResolved.BindAction(
				GetWeakPtr(), &NanoPlagueControllerActor::OnTargetDamageResolved);
			targetCombatant.GetAbilitySystemComponent().AddOwnedTag(AbilityData::NanoPlague::State::Infected);
		}
		catch (...)
		{
			const auto error = std::current_exception();
			try { RemoveInfectionById(id); } catch (...) {}
			std::rethrow_exception(error);
		}
		if (mDestroying) return false;
		AddPulse(origin, target, generation > 0);
		return true;
	}

	void NanoPlagueControllerActor::Tick(float deltaTime)
	{
		if (mDestroying || mTicking || GetIsPendingDestroy()) return;
		mTicking = true;
		struct TickScope { bool& ticking; ~TickScope() { ticking = false; } } scope{ mTicking };
		const float safeDeltaTime = std::max(0.f, deltaTime);
		mVisualAge += safeDeltaTime;
		mPulses.erase(std::remove_if(mPulses.begin(), mPulses.end(), [safeDeltaTime](Pulse& pulse)
		{
			pulse.elapsed += safeDeltaTime;
			return pulse.elapsed >= pulse.duration || pulse.target.expired();
		}), mPulses.end());

		// New infections start aging next Tick. A refresh during damage owns its
		// new duration/counters; the old tick must not overwrite that new state.
		std::vector<std::uint64_t> ids;
		ids.reserve(mInfections.size());
		for (const auto& [id, infection] : mInfections) ids.push_back(id);
		for (const std::uint64_t id : ids)
		{
			Infection* infection = FindInfection(id);
			if (!infection) continue;
			if (infection->pendingSpread)
			{
				ResolvePendingSpread(*infection);
				if (mDestroying) return;
				RemoveInfectionById(id);
				continue;
			}
			const shared_ptr<Actor> target = infection->target.lock();
			if (!target || target->GetIsPendingDestroy())
			{
				RemoveInfectionById(id);
				continue;
			}
			const std::uint64_t revision = infection->revision;
			const float interval = mSettings.tickInterval;
			const int maximumTicks = std::max(1, static_cast<int>(std::round(mSettings.duration / interval)));
			infection->remainingDuration -= safeDeltaTime;
			infection->tickAccumulator += safeDeltaTime;
			while (infection && infection->revision == revision && !infection->pendingSpread &&
				infection->tickAccumulator >= interval && infection->ticksApplied < maximumTicks)
			{
				infection->tickAccumulator -= interval;
				++infection->ticksApplied;
				ApplyTick(*target);
				if (mDestroying) return;
				infection = FindInfection(id);
			}
			if (!infection) continue;
			if (infection->pendingSpread)
			{
				ResolvePendingSpread(*infection);
				if (mDestroying) return;
				RemoveInfectionById(id);
			}
			else if (infection->revision == revision &&
				(infection->remainingDuration <= 0.f || infection->ticksApplied >= maximumTicks))
			{
				RemoveInfectionById(id);
			}
		}
		if (mInfections.empty() && mPulses.empty()) Destroy();
	}

	NanoPlagueControllerActor::Infection* NanoPlagueControllerActor::FindInfection(std::uint64_t id)
	{
		const auto found = mInfections.find(id);
		return found == mInfections.end() ? nullptr : &found->second;
	}

	void NanoPlagueControllerActor::RemoveInfectionById(std::uint64_t id)
	{
		const auto found = mInfections.find(id);
		if (found == mInfections.end()) return;
		const Infection infection = found->second;
		mInfections.erase(found); // Detach before callbacks can re-infect or destroy.
		if (const shared_ptr<Actor> target = infection.target.lock())
		{
			if (Combatant* combatant = dynamic_cast<Combatant*>(target.get()))
			{
				combatant->GetCombatRuntime().onDamageResolved.UnbindAction(infection.damageSubscription);
				combatant->GetAbilitySystemComponent().RemoveOwnedTag(AbilityData::NanoPlague::State::Infected);
			}
		}
	}

	void NanoPlagueControllerActor::Render(sf::RenderWindow& window)
	{
		for (const auto& [id, infection] : mInfections)
		{
			const shared_ptr<Actor> target = infection.target.lock();
			if (!target || target->GetIsPendingDestroy())
			{
				continue;
			}
			const float pulse = 0.5f + 0.5f * std::sin(
				mVisualAge * 7.f
			);
			sf::CircleShape aura(mProfile.auraRadius + pulse * 4.f);
			aura.setOrigin({ aura.getRadius(), aura.getRadius() });
			aura.setPosition(target->GetActorLocation());
			aura.setFillColor(sf::Color{
				mProfile.infectionColor.r,
				mProfile.infectionColor.g,
				mProfile.infectionColor.b,
				static_cast<std::uint8_t>(45.f + pulse * 35.f)
			});
			aura.setOutlineThickness(1.5f);
			aura.setOutlineColor(mProfile.infectionColor);
			window.draw(aura);
		}

		for (const Pulse& pulse : mPulses)
		{
			const shared_ptr<Actor> target = pulse.target.lock();
			if (!target)
			{
				continue;
			}
			const float alpha = std::clamp(
				1.f - pulse.elapsed / std::max(0.01f, pulse.duration), 0.f, 1.f
			);
			const sf::Color color = pulse.isSpread
				? mProfile.spreadColor
				: mProfile.injectionColor;
			sf::VertexArray line(sf::PrimitiveType::Lines, 2);
			line[0].position = pulse.origin;
			line[1].position = target->GetActorLocation();
			line[0].color = sf::Color{ color.r, color.g, color.b,
				static_cast<std::uint8_t>(alpha * color.a) };
			line[1].color = line[0].color;
			window.draw(line);
		}
	}

	shared_ptr<Actor> NanoPlagueControllerActor::GetOwnerActor() const
	{
		const shared_ptr<Actor> owner = mOwner.lock();
		if (!owner || owner->GetIsPendingDestroy() || owner->GetWorld() != GetWorld()) return {};
		return owner;
	}

	void NanoPlagueControllerActor::OnTargetDamageResolved(const DamageContext& context)
	{
		if (!context.targetWasKilled || !context.target)
		{
			return;
		}
		for (auto& [id, infection] : mInfections)
		{
			if (infection.target.lock().get() != context.target)
			{
				continue;
			}
			infection.pendingSpread = true;
			infection.deathLocation = {
				context.targetLocationAtResolution.x,
				context.targetLocationAtResolution.y
			};
		}
	}

	void NanoPlagueControllerActor::ResolvePendingSpread(Infection infection)
	{
		const shared_ptr<Actor> owner = GetOwnerActor();
		World* world = GetWorld();
		if (!owner || !world)
		{
			return;
		}

		std::uint64_t remainingTargets = ResolveSpreadTargetCount();
		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world,
			*owner,
			infection.deathLocation,
			mSettings.spreadRadius
		))
		{
			if (remainingTargets <= 0)
			{
				break;
			}
			if (!target || target->GetIsPendingDestroy() || HasInfection(*target))
			{
				continue;
			}
			if (ApplyOrRefreshInfectionInternal(
				*target,
				infection.generation == std::numeric_limits<std::uint64_t>::max()
					? infection.generation
					: infection.generation + 1,
				mSettings,
				&infection.deathLocation
			))
			{
				--remainingTargets;
			}
		}
	}

	void NanoPlagueControllerActor::ApplyTick(Actor& target)
	{
		const Settings settings = mSettings;
		const shared_ptr<Actor> owner = GetOwnerActor();
		const Combatant* ownerCombatant = owner
			? dynamic_cast<const Combatant*>(owner.get())
			: nullptr;
		if (!owner || !ownerCombatant)
		{
			return;
		}
		const float energyPower = std::max(0.f, ownerCombatant
			->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
				OwnerAttributeIds::EnergyPower
			));
		ApplyCombatDamage(
			target,
			std::max(0.f, settings.baseTickDamage + energyPower * settings.energyPowerTickScale),
			owner.get(),
			{ DamageTypeSchema::Electric },
			{},
			settings.sourceAbilityId,
			settings.sourceAbilityTags,
			DamageDeliveryType::Area,
			this
		);
	}

	void NanoPlagueControllerActor::Destroy()
	{
		if (mDestroying) return;
		mDestroying = true;
		mRegistryRegistration.Reset();
		std::exception_ptr error;
		while (!mInfections.empty())
		{
			try { RemoveInfectionById(mInfections.rbegin()->first); }
			catch (...) { if (!error) error = std::current_exception(); }
		}
		mPulses.clear();
		try { Actor::Destroy(); }
		catch (...) { if (!error) error = std::current_exception(); }
		if (error) std::rethrow_exception(error);
	}

	bool NanoPlagueControllerActor::HasInfection(const Actor& target) const
	{
		return std::any_of(
			mInfections.begin(),
			mInfections.end(),
			[&target](const auto& entry)
			{
				return entry.second.target.lock().get() == &target;
			}
		);
	}

	void NanoPlagueControllerActor::AddPulse(
		const sf::Vector2f& origin,
		Actor& target,
		bool isSpread
	)
	{
		mPulses.push_back(Pulse{
			origin,
			MakeWeakActor(target),
			0.f,
			isSpread ? mProfile.spreadDuration : mProfile.injectionDuration,
			isSpread
		});
	}

	std::uint64_t NanoPlagueControllerActor::ResolveSpreadTargetCount() const
	{
		const shared_ptr<Actor> owner = GetOwnerActor();
		const Combatant* combatant = owner ? dynamic_cast<const Combatant*>(owner.get()) : nullptr;
		const float rawLuck = combatant
			? combatant->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
				OwnerAttributeIds::Luck
			)
			: 0.f;
		const double luck = std::isfinite(rawLuck)
			? static_cast<double>(std::max(0.f, rawLuck))
			: 0.0;
		const std::uint64_t baseCount = static_cast<std::uint64_t>(
			std::max(1, mSettings.baseSpreadTargetCount)
		);
		const std::uint64_t maximumBonus = std::numeric_limits<std::uint64_t>::max() - baseCount;
		const double bonus = std::floor(luck / 100.0);
		return bonus >= static_cast<double>(maximumBonus)
			? std::numeric_limits<std::uint64_t>::max()
			: baseCount + static_cast<std::uint64_t>(bonus);
	}
}

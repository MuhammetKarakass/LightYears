#include "gameplay/ability/nanoPlague/NanoPlagueControllerActor.h"

#include "framework/World.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"

#include <algorithm>
#include <cstdint>
#include <cmath>

namespace ly
{
	namespace
	{
		struct ControllerRegistryEntry
		{
			weak_ptr<Actor> owner;
			weak_ptr<NanoPlagueControllerActor> controller;
		};

		List<ControllerRegistryEntry>& GetControllerRegistry()
		{
			static List<ControllerRegistryEntry> registry;
			return registry;
		}

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
		, mUnmanagedOwner(mOwner.expired() ? owner : nullptr)
		, mProfile(profile)
	{
		SetRenderLayer(RenderLayer::World);
		SetEnablePhysics(false);
	}

	NanoPlagueControllerActor::~NanoPlagueControllerActor()
	{
		for (std::size_t index = mInfections.size(); index > 0; --index)
		{
			RemoveInfection(index - 1);
		}
	}

	shared_ptr<NanoPlagueControllerActor> NanoPlagueControllerActor::FindOrCreate(
		World& world,
		Actor& owner,
		const NanoPlaguePresentationProfile& profile
	)
	{
		List<ControllerRegistryEntry>& registry = GetControllerRegistry();
		for (std::size_t index = 0; index < registry.size();)
		{
			const shared_ptr<Actor> registeredOwner = registry[index].owner.lock();
			const shared_ptr<NanoPlagueControllerActor> controller =
				registry[index].controller.lock();
			if (!registeredOwner || !controller || controller->GetIsPendingDestroy())
			{
				registry.erase(registry.begin() + index);
				continue;
			}
			if (registeredOwner.get() == &owner)
			{
				return controller;
			}
			++index;
		}

		const shared_ptr<NanoPlagueControllerActor> controller =
			world.SpawnActor<NanoPlagueControllerActor>(&owner, profile).lock();
		if (controller)
		{
			registry.push_back({
				MakeWeakActor(owner),
				weak_ptr<NanoPlagueControllerActor>{ controller }
			});
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
		if (target.GetIsPendingDestroy() ||
			dynamic_cast<Combatant*>(&target) == nullptr)
		{
			return false;
		}

		mSettings = settings;
		mSettings.duration = std::max(0.01f, mSettings.duration);
		mSettings.tickInterval = std::max(0.01f, mSettings.tickInterval);
		mSettings.baseSpreadTargetCount = std::max(1, mSettings.baseSpreadTargetCount);
		mSettings.maximumSpreadTargetCount = std::max(
			mSettings.baseSpreadTargetCount,
			mSettings.maximumSpreadTargetCount
		);

		for (Infection& infection : mInfections)
		{
			if (infection.target.lock().get() != &target)
			{
				continue;
			}
			infection.remainingDuration = mSettings.duration;
			infection.tickAccumulator = 0.f;
			infection.ticksApplied = 0;
			// A direct infection must never lose its one spread generation because
			// a later generation-one application refreshed the same target.
			infection.generation = std::min(infection.generation, generation);
			const sf::Vector2f origin = visualOrigin
				? *visualOrigin
				: GetOwnerActor()
					? GetOwnerActor()->GetActorLocation()
					: target.GetActorLocation();
			AddPulse(origin, target, generation > 0);
			return true;
		}

		Infection infection;
		infection.target = MakeWeakActor(target);
		infection.generation = std::max(0, generation);
		infection.remainingDuration = mSettings.duration;
		mInfections.push_back(std::move(infection));
		dynamic_cast<Combatant&>(target).GetAbilitySystemComponent().AddOwnedTag(
			AbilityData::NanoPlague::State::Infected
		);
		dynamic_cast<Combatant&>(target).GetCombatRuntime().onDamageResolved.BindAction(
			GetWeakPtr(),
			&NanoPlagueControllerActor::OnTargetDamageResolved
		);
		const sf::Vector2f origin = visualOrigin
			? *visualOrigin
			: GetOwnerActor()
				? GetOwnerActor()->GetActorLocation()
				: target.GetActorLocation();
		AddPulse(origin, target, generation > 0);
		return true;
	}

	void NanoPlagueControllerActor::Tick(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		mVisualAge += safeDeltaTime;
		for (std::size_t index = 0; index < mPulses.size();)
		{
			mPulses[index].elapsed += safeDeltaTime;
			if (mPulses[index].elapsed >= mPulses[index].duration ||
				mPulses[index].target.expired())
			{
				mPulses.erase(mPulses.begin() + index);
				continue;
			}
			++index;
		}

		for (std::size_t index = 0; index < mInfections.size();)
		{
			Infection& infection = mInfections[index];
			if (infection.pendingSpread)
			{
				ResolvePendingSpread(infection);
				RemoveInfection(index);
				continue;
			}

			const shared_ptr<Actor> target = infection.target.lock();
			if (!target || target->GetIsPendingDestroy())
			{
				RemoveInfection(index);
				continue;
			}

			infection.remainingDuration -= safeDeltaTime;
			infection.tickAccumulator += safeDeltaTime;
			const int maximumTicks = std::max(
				1,
				static_cast<int>(std::round(mSettings.duration / mSettings.tickInterval))
			);
			while (!infection.pendingSpread &&
				infection.tickAccumulator >= mSettings.tickInterval &&
				infection.ticksApplied < maximumTicks)
			{
				infection.tickAccumulator -= mSettings.tickInterval;
				ApplyTick(infection, *target);
				++infection.ticksApplied;
			}

			if (infection.pendingSpread)
			{
				continue;
			}
			if (infection.remainingDuration <= 0.f || infection.ticksApplied >= maximumTicks)
			{
				RemoveInfection(index);
				continue;
			}
			++index;
		}

		if (mInfections.empty() && mPulses.empty())
		{
			Destroy();
		}
	}

	void NanoPlagueControllerActor::Render(sf::RenderWindow& window)
	{
		for (const Infection& infection : mInfections)
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

	Actor* NanoPlagueControllerActor::GetOwnerActor() const
	{
		const shared_ptr<Actor> owner = mOwner.lock();
		return owner ? owner.get() : mUnmanagedOwner;
	}

	void NanoPlagueControllerActor::OnTargetDamageResolved(const DamageContext& context)
	{
		if (!context.targetWasKilled || !context.target)
		{
			return;
		}
		for (Infection& infection : mInfections)
		{
			if (infection.target.lock().get() != context.target)
			{
				continue;
			}
			if (infection.generation == 0)
			{
				infection.pendingSpread = true;
				infection.deathLocation = {
					context.targetLocationAtResolution.x,
					context.targetLocationAtResolution.y
				};
			}
		}
	}

	void NanoPlagueControllerActor::ResolvePendingSpread(const Infection& infection)
	{
		Actor* owner = GetOwnerActor();
		World* world = GetWorld();
		if (infection.generation != 0 || !owner || !world)
		{
			return;
		}

		int remainingTargets = ResolveSpreadTargetCount();
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
			if (ApplyOrRefreshInfection(
				*target,
				1,
				mSettings,
				&infection.deathLocation
			))
			{
				--remainingTargets;
			}
		}
	}

	void NanoPlagueControllerActor::ApplyTick(Infection&, Actor& target)
	{
		Actor* owner = GetOwnerActor();
		const Combatant* ownerCombatant = owner
			? dynamic_cast<const Combatant*>(owner)
			: nullptr;
		if (!owner || !ownerCombatant)
		{
			return;
		}
		const float energyMax = std::max(0.f, ownerCombatant
			->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
				OwnerAttributeIds::EnergyMax
			));
		ApplyCombatDamage(
			target,
			std::max(0.f, mSettings.baseTickDamage + energyMax * mSettings.energyMaxTickScale),
			owner,
			{ DamageTypeSchema::Energy },
			{},
			mSettings.sourceAbilityId,
			mSettings.sourceAbilityTags,
			DamageDeliveryType::Area,
			this
		);
	}

	void NanoPlagueControllerActor::RemoveInfection(std::size_t index)
	{
		if (index >= mInfections.size())
		{
			return;
		}
		if (const shared_ptr<Actor> target = mInfections[index].target.lock())
		{
			if (Combatant* combatant = dynamic_cast<Combatant*>(target.get()))
			{
				combatant->GetAbilitySystemComponent().RemoveOwnedTag(
					AbilityData::NanoPlague::State::Infected
				);
			}
		}
		mInfections.erase(mInfections.begin() + index);
	}

	bool NanoPlagueControllerActor::HasInfection(const Actor& target) const
	{
		return std::any_of(
			mInfections.begin(),
			mInfections.end(),
			[&target](const Infection& infection)
			{
				return infection.target.lock().get() == &target;
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

	int NanoPlagueControllerActor::ResolveSpreadTargetCount() const
	{
		const Actor* owner = GetOwnerActor();
		const Combatant* combatant = owner ? dynamic_cast<const Combatant*>(owner) : nullptr;
		const float luckFactor = combatant
			? std::clamp(combatant->GetCombatRuntime().GetCombatLuckFactor(), 0.f, 1.f)
			: 0.f;
		int result = mSettings.baseSpreadTargetCount;
		while (result < mSettings.maximumSpreadTargetCount &&
			RandRange(0.f, 1.f) < luckFactor)
		{
			++result;
		}
		return result;
	}
}

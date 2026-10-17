#include "gameplay/module/generated/ModuleSmolderingFieldActor.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace vega
{
	namespace
	{
		// One pulse per second of field life; the stack count per pulse is data (smolderStacksPerSecond).
		constexpr float kPulseIntervalSeconds = 1.f;
		constexpr float kTimeEpsilon = 1e-4f;
	}

	ModuleSmolderingFieldActor::ModuleSmolderingFieldActor(World* world, const weak_ptr<Actor>& source, const ModuleExplosionPresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, float seconds, float radius, int stacksPerPulse)
		: Actor{ world }, mSource{ source }, mProfile{ profile }, mLease{ lease }, mRadius{ radius }, mStacksPerPulse{ stacksPerPulse }, mRemaining{ seconds },
		  mMaxPulses{ static_cast<int>(std::floor(seconds / kPulseIntervalSeconds + kTimeEpsilon)) }
	{
		SetEnablePhysics(false);
		SetRenderLayer(RenderLayer::WorldVfx);
	}

	void ModuleSmolderingFieldActor::Tick(float deltaTime)
	{
		const auto source = mSource.lock();
		const auto lease = mLease.lock();
		if (!source || source->GetIsPendingDestroy() || !lease || !lease->active) { Destroy(); return; }
		const float step = std::max(0.f, deltaTime);
		mElapsed += step;
		mRemaining -= step;
		// Pulses fire at 1, 2, ... seconds of field life, so a field of N seconds pulses N times, the last one on its final frame.
		while (mPulses < mMaxPulses && mElapsed + kTimeEpsilon >= static_cast<float>(mPulses + 1) * kPulseIntervalSeconds)
		{
			++mPulses;
			Pulse(*source);
		}
		if (mRemaining <= kTimeEpsilon) Destroy();
	}

	void ModuleSmolderingFieldActor::Pulse(Actor& source)
	{
		World* world = GetWorld();
		if (!world || mStacksPerPulse <= 0) return;
		const auto targets = targeting::FindOpposingCombatants(*world, source, GetActorLocation(), mRadius, false);
		for (const auto& target : targets)
		{
			if (!target || target->GetIsPendingDestroy()) continue;
			auto* combatant = dynamic_cast<Combatant*>(target.get());
			if (!combatant) continue;
			DamageContext context;
			context.source = &source;
			context.target = target.get();
			context.remainingDamage = 1.f; // Status application only: the field resolves no damage and triggers no module rules.
			context.damageTags = { DamageTypeSchema::Thermal };
			context.payload.igniteStacks = mStacksPerPulse;
			context.payload.skipModuleDamageRules = true;
			DamageTypeSystem::ApplyStatusEffects(combatant->GetAbilitySystemComponent(), context);
		}
	}

	void ModuleSmolderingFieldActor::Render(sf::RenderTarget& window)
	{
		sf::CircleShape field{ mRadius };
		field.setOrigin({ mRadius, mRadius });
		field.setPosition(GetActorLocation());
		field.setFillColor(sf::Color{ mProfile.ring.r, mProfile.ring.g, mProfile.ring.b, 40 });
		field.setOutlineColor(mProfile.ring);
		field.setOutlineThickness(mProfile.thickness * 0.5f);
		window.draw(field);
	}
}

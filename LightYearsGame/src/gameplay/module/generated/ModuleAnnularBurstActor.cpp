#include "gameplay/module/generated/ModuleAnnularBurstActor.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/HealthComponent.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include <SFML/Graphics/CircleShape.hpp>
#include <algorithm>

namespace vega
{
	ModuleAnnularBurstActor::ModuleAnnularBurstActor(World* world, const weak_ptr<Actor>& source, const ModuleAnnularBurstPresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, float innerRadius, float outerRadius) : Actor{ world }, mSource{ source }, mProfile{ profile }, mLease{ lease }, mInnerRadius{ innerRadius }, mOuterRadius{ outerRadius }, mRemaining{ profile.duration }
	{
		SetEnablePhysics(false); SetRenderLayer(RenderLayer::WorldVfx);
	}
	void ModuleAnnularBurstActor::Detonate(float damage)
	{
		const auto source = mSource.lock(); const auto lease = mLease.lock();
		if (mDetonated || !source || source->GetIsPendingDestroy() || !source->GetWorld() || !lease || !lease->active || damage <= 0.f) return;
		mDetonated = true; const auto origin = GetActorLocation();
		const auto targets = targeting::FindOpposingCombatants(*source->GetWorld(), *source, origin, mOuterRadius, false);
		for (const auto& target : targets)
		{
			if (!lease->active || source->GetIsPendingDestroy()) break;
			if (!target || target->GetIsPendingDestroy()) continue;
			const auto offset = target->GetActorLocation() - origin; const float distanceSquared = offset.x * offset.x + offset.y * offset.y;
			if (distanceSquared < mInnerRadius * mInnerRadius || distanceSquared > mOuterRadius * mOuterRadius) continue;
			ApplyCombatDamage(*target, damage, source.get(), { DamageTypeSchema::Energy }, MakeModuleGeneratedDamagePayload(0, DamageSourceScalingPolicy::AlreadyResolved), {}, {}, DamageDeliveryType::Area, this);
		}
	}
	void ModuleAnnularBurstActor::Tick(float deltaTime)
	{
		const auto source = mSource.lock(); const auto lease = mLease.lock(); mRemaining -= std::max(0.f, deltaTime);
		if (!source || source->GetIsPendingDestroy() || !lease || !lease->active || mRemaining <= 0.f) Destroy();
	}
	void ModuleAnnularBurstActor::Render(sf::RenderTarget& target)
	{
		const float alpha = std::clamp(mRemaining / mProfile.duration, 0.f, 1.f);
		const auto draw = [&](float radius, sf::Color color) { sf::CircleShape ring{ radius }; ring.setOrigin({ radius, radius }); ring.setPosition(GetActorLocation()); color.a = static_cast<std::uint8_t>(color.a * alpha); ring.setFillColor(sf::Color::Transparent); ring.setOutlineColor(color); ring.setOutlineThickness(mProfile.thickness); target.draw(ring); };
		draw(mInnerRadius, mProfile.innerRing); draw(mOuterRadius, mProfile.outerRing);
	}
}

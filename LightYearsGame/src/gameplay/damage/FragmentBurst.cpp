#include "gameplay/damage/FragmentBurst.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/targeting/CombatantTargetQuery.h"

#include <algorithm>

namespace vega
{
	List<std::pair<std::uint64_t, int>> ApplyFragmentBurst(Actor& source, Actor* center, const FragmentBurstRequest& request)
	{
		List<std::pair<std::uint64_t, int>> hits;
		if (!source.GetWorld() || request.fragmentCount <= 0 || !(request.damagePerFragment > 0.f) || request.maxHitsPerTarget <= 0) return hits;
		auto targets = targeting::FindOpposingCombatants(*source.GetWorld(), source, request.origin, request.radius, false);
		const sf::Vector2f origin = request.origin;
		const auto distanceSquared = [origin](const shared_ptr<Actor>& actor) { const sf::Vector2f offset = actor->GetActorLocation() - origin; return offset.x * offset.x + offset.y * offset.y; };
		std::sort(targets.begin(), targets.end(), [&](const shared_ptr<Actor>& left, const shared_ptr<Actor>& right)
		{
			const bool leftCenter = left.get() == center, rightCenter = right.get() == center;
			if (leftCenter != rightCenter) return leftCenter;
			const float a = distanceSquared(left), b = distanceSquared(right);
			return a == b ? left->GetUniqueID() < right->GetUniqueID() : a < b;
		});
		targets.erase(std::remove_if(targets.begin(), targets.end(), [](const shared_ptr<Actor>& actor) { return !actor || actor->GetIsPendingDestroy() || !dynamic_cast<Combatant*>(actor.get()); }), targets.end());
		for (const auto& target : targets) hits.emplace_back(target->GetUniqueID(), 0);

		const DamagePayload payload = MakeModuleGeneratedDamagePayload(0, DamageSourceScalingPolicy::AlreadyResolved);
		int remaining = request.fragmentCount;
		for (int round = 0; round < request.maxHitsPerTarget && remaining > 0; ++round)
			for (std::size_t index = 0; index < targets.size() && remaining > 0; ++index)
			{
				if (source.GetIsPendingDestroy()) return hits;
				Actor& target = *targets[index];
				if (target.GetIsPendingDestroy()) continue;
				--remaining;
				++hits[index].second;
				ApplyCombatDamage(target, request.damagePerFragment, &source, request.damageTags, payload, {}, {}, DamageDeliveryType::Area, nullptr);
			}
		return hits;
	}
}

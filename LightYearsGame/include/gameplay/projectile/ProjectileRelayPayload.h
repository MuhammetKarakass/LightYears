#pragma once

#include "framework/Core.h"
#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"

#include <SFML/System/Vector2.hpp>
#include <cstdint>

namespace ly
{
	class Actor;

	// A projectile lineage prevents a Relay clone from being consumed by a
	// Relay that already participated in creating that lineage. Keeping this
	// data in the common projectile layer allows future projectile-transforming
	// abilities to share the same loop-safety rule.
	struct ProjectileRelayLineage
	{
		List<std::uint64_t> visitedRelayIds;
		unsigned int generation = 0;

		bool HasVisited(std::uint64_t relayId) const;
		ProjectileRelayLineage Appended(std::uint64_t relayId) const;
	};

	// Data that is common to every physical projectile family. Concrete
	// projectile actors keep their own typed presentation and behavior state;
	// this payload carries only the combat/source state that must survive a
	// Relay conversion.
	struct ProjectileRelaySnapshot
	{
		float damage = 0.f;
		sas::GameplayAttributeList damageAttributes;
		List<GameplayTag> damageTags;
		sas::ContentId sourceAbilityId;
		List<GameplayTag> sourceAbilityTags;
		sf::Vector2f velocity{};
		// Optional live target for projectile families that support homing. The
		// concrete clone decides how this target affects its own movement.
		weak_ptr<Actor> homingTarget;
		float collisionRadius = 0.f;
		float remainingLifetime = 0.f;
		ProjectileRelayLineage lineage;
	};

	struct ProjectileRelayCloneRequest
	{
		sf::Vector2f location{};
		sf::Vector2f direction{ 0.f, -1.f };
		float damage = 0.f;
		// Relay families can need to distribute family-owned runtime payloads
		// (for example a carried resource) without adding those fields to this
		// common combat snapshot. The concrete clone uses these neutral split
		// facts to partition its own typed state.
		int cloneIndex = 0;
		int cloneCount = 1;
		float transferRatio = 1.f;
		bool allowFriendlyFire = false;
		ProjectileRelaySnapshot snapshot;
	};
}

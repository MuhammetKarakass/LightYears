#pragma once

#include <framework/CollisionLayer.h>
#include <cstdint>

namespace vega::GameCollision
{
	inline constexpr CollisionLayer Player = CollisionLayer::Layer0;
	inline constexpr CollisionLayer PlayerBullet = CollisionLayer::Layer1;
	inline constexpr CollisionLayer Enemy = CollisionLayer::Layer2;
	inline constexpr CollisionLayer EnemyBullet = CollisionLayer::Layer3;
	inline constexpr CollisionLayer Powerup = CollisionLayer::Layer4;
	inline constexpr CollisionLayer Environment = CollisionLayer::Layer5;
	inline constexpr CollisionLayer RelayProjectile = CollisionLayer::Layer6;
	inline constexpr CollisionLayer FriendlySummon = CollisionLayer::Layer7;
	inline constexpr CollisionLayer AllPlayers = static_cast<CollisionLayer>(static_cast<std::uint32_t>(Player) | static_cast<std::uint32_t>(PlayerBullet));
	inline constexpr CollisionLayer AllEnemies = static_cast<CollisionLayer>(static_cast<std::uint32_t>(Enemy) | static_cast<std::uint32_t>(EnemyBullet));
	inline constexpr CollisionLayer AllBullets = static_cast<CollisionLayer>(static_cast<std::uint32_t>(PlayerBullet) | static_cast<std::uint32_t>(EnemyBullet));
	inline constexpr CollisionLayer AllShips = static_cast<CollisionLayer>(static_cast<std::uint32_t>(Player) | static_cast<std::uint32_t>(Enemy) | static_cast<std::uint32_t>(FriendlySummon));
	inline constexpr CollisionLayer AllFriendlyCombatants = static_cast<CollisionLayer>(static_cast<std::uint32_t>(Player) | static_cast<std::uint32_t>(FriendlySummon));
	inline constexpr CollisionLayer AllRelayTargets = static_cast<CollisionLayer>(static_cast<std::uint32_t>(Player) | static_cast<std::uint32_t>(Enemy) | static_cast<std::uint32_t>(FriendlySummon));
}

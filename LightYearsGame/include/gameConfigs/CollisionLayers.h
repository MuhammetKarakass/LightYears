#pragma once

#include <framework/CollisionLayer.h>
#include <cstdint>

namespace vega::GameCollision
{
	inline constexpr vega::CollisionLayer Player = vega::CollisionLayer::Layer0;
	inline constexpr vega::CollisionLayer PlayerBullet = vega::CollisionLayer::Layer1;
	inline constexpr vega::CollisionLayer Enemy = vega::CollisionLayer::Layer2;
	inline constexpr vega::CollisionLayer EnemyBullet = vega::CollisionLayer::Layer3;
	inline constexpr vega::CollisionLayer Powerup = vega::CollisionLayer::Layer4;
	inline constexpr vega::CollisionLayer Environment = vega::CollisionLayer::Layer5;
	inline constexpr vega::CollisionLayer RelayProjectile = vega::CollisionLayer::Layer6;
	inline constexpr vega::CollisionLayer FriendlySummon = vega::CollisionLayer::Layer7;
	inline constexpr vega::CollisionLayer AllPlayers = static_cast<vega::CollisionLayer>(static_cast<std::uint32_t>(Player) | static_cast<std::uint32_t>(PlayerBullet));
	inline constexpr vega::CollisionLayer AllEnemies = static_cast<vega::CollisionLayer>(static_cast<std::uint32_t>(Enemy) | static_cast<std::uint32_t>(EnemyBullet));
	inline constexpr vega::CollisionLayer AllBullets = static_cast<vega::CollisionLayer>(static_cast<std::uint32_t>(PlayerBullet) | static_cast<std::uint32_t>(EnemyBullet));
	inline constexpr vega::CollisionLayer AllShips = static_cast<vega::CollisionLayer>(static_cast<std::uint32_t>(Player) | static_cast<std::uint32_t>(Enemy) | static_cast<std::uint32_t>(FriendlySummon));
	inline constexpr vega::CollisionLayer AllFriendlyCombatants = static_cast<vega::CollisionLayer>(static_cast<std::uint32_t>(Player) | static_cast<std::uint32_t>(FriendlySummon));
	inline constexpr vega::CollisionLayer AllRelayTargets = static_cast<vega::CollisionLayer>(static_cast<std::uint32_t>(Player) | static_cast<std::uint32_t>(Enemy) | static_cast<std::uint32_t>(FriendlySummon));
}

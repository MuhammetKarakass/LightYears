#pragma once

#include <framework/Core.h>

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
	inline constexpr CollisionLayer AllPlayers = static_cast<CollisionLayer>(static_cast<uint8_t>(Player) | static_cast<uint8_t>(PlayerBullet));
	inline constexpr CollisionLayer AllEnemies = static_cast<CollisionLayer>(static_cast<uint8_t>(Enemy) | static_cast<uint8_t>(EnemyBullet));
	inline constexpr CollisionLayer AllBullets = static_cast<CollisionLayer>(static_cast<uint8_t>(PlayerBullet) | static_cast<uint8_t>(EnemyBullet));
	inline constexpr CollisionLayer AllShips = static_cast<CollisionLayer>(static_cast<uint8_t>(Player) | static_cast<uint8_t>(Enemy) | static_cast<uint8_t>(FriendlySummon));
	inline constexpr CollisionLayer AllFriendlyCombatants = static_cast<CollisionLayer>(static_cast<uint8_t>(Player) | static_cast<uint8_t>(FriendlySummon));
	inline constexpr CollisionLayer AllRelayTargets = static_cast<CollisionLayer>(static_cast<uint8_t>(Player) | static_cast<uint8_t>(Enemy) | static_cast<uint8_t>(FriendlySummon));
}

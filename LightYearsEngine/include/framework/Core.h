#pragma once
#include <stdio.h>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <cstdint>

// =====================================================
// COLLISION LAYER SYSTEM - Çarpýþma Katman Sistemi (Bit mask)
// =====================================================

enum class CollisionLayer : uint8_t
{
    None = 0,            // 0000 0000

    // PLAYER FACTION
    Player = 1 << 0,     // 0000 0001 - Player ship
    PlayerBullet = 1 << 1,// 0000 0010 - Player bullets

    // ENEMY FACTION
    Enemy = 1 << 2,      // 0000 0100 - Enemy ships
    EnemyBullet = 1 << 3,// 0000 1000 - Enemy bullets

    // NEUTRAL OBJECTS
    Powerup = 1 << 4,    // 0001 0000 - Power-ups
    Environment = 1 << 5,// 0010 0000 - Obstacles

    // COMBINATIONS (Bit masks)
    AllPlayers = static_cast<uint8_t>(Player) | static_cast<uint8_t>(PlayerBullet),
    AllEnemies = static_cast<uint8_t>(Enemy) | static_cast<uint8_t>(EnemyBullet),
    AllBullets = static_cast<uint8_t>(PlayerBullet) | static_cast<uint8_t>(EnemyBullet),
    AllShips   = static_cast<uint8_t>(Player) | static_cast<uint8_t>(Enemy)
};

// Bitwise operators for enum class CollisionLayer
inline CollisionLayer operator|(CollisionLayer a, CollisionLayer b)
{
    return static_cast<CollisionLayer>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline CollisionLayer& operator|=(CollisionLayer& a, CollisionLayer b)
{
    a = a | b; return a;
}
inline CollisionLayer operator&(CollisionLayer a, CollisionLayer b)
{
    return static_cast<CollisionLayer>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
inline CollisionLayer& operator&=(CollisionLayer& a, CollisionLayer b)
{
    a = a & b; return a;
}
inline bool HasCollisionLayer(CollisionLayer mask, CollisionLayer layer)
{
    return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(layer)) != 0;
}

namespace ly
{
    template<typename T>
    using unique_ptr = std::unique_ptr<T>;

    template<typename T>
    using shared_ptr = std::shared_ptr<T>;

    template<typename T>
    using weak_ptr = std::weak_ptr<T>;

    template<typename T>
    using List = std::vector<T>;

    template<typename keyType, typename valueType, typename Pr = std::less<keyType>>
    using Map = std::map<keyType, valueType, Pr>;

    template<typename keyType, typename valueType, typename hasher = std::hash<keyType>>
    using Dictionary = std::unordered_map<keyType, valueType, hasher>;

    template<typename T>
    using Set = std::unordered_set<T>;

	// LOG isim , M mesaj , ... argümanlar; M mesaj, ## virgül silme duruma göre __VA_ARGS__ argümanlar ...  
    #define LOG(M, ...) printf(M "\n", ##__VA_ARGS__)
}
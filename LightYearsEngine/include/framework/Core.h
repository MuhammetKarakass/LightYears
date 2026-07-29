#pragma once
#include <stdio.h>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <string>
#include <cctype>
#include <functional>

#include "framework/debug/Assert.h"
#include "framework/debug/Log.h"
#include "framework/debug/Profiler.h"

// =====================================================
// COLLISION LAYER SYSTEM
// =====================================================

enum class AudioType
{
    Music,
    SFX_World,
    SFX_UI
};

enum class CollisionLayer : uint8_t
{
    None = 0,  // 0000 0000

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

namespace ly
{
    // =====================================================
    // GAMEPLAY TAG SYSTEM - UE5 Style Type-Safe Tags (Auto-Indexed)
    // =====================================================
    struct GameplayTag 
    {
        std::string name;

        // Constructors
        GameplayTag() = default;
        GameplayTag(const std::string& n) : name(n) {}
        GameplayTag(const char* n) : name(n) {}

        // Comparison operators
        bool operator==(const GameplayTag& other) const { return name == other.name; }
        bool operator!=(const GameplayTag& other) const { return name != other.name; }
        bool operator<(const GameplayTag& other) const { return name < other.name; }

        // Utility methods
        const std::string& ToString() const { return name; }
        bool IsValid() const { return !name.empty(); }
        // Append an index to create a unique runtime tag.
        GameplayTag WithIndex(int index) const
        {
            return GameplayTag{ name + "_" + std::to_string(index) };
        }
        // Remove a numeric runtime index from the tag.
        GameplayTag GetBaseTag() const
        {
            size_t pos = name.rfind('_');
            if (pos != std::string::npos)
            {
                std::string suffix = name.substr(pos + 1);
                bool isNumber = !suffix.empty();
                for (char c : suffix)
                {
                    if (!std::isdigit(static_cast<unsigned char>(c)))
                    {
                        isNumber = false;
                        break;
                    }
                }
                if (isNumber)
                {
                    return GameplayTag{ name.substr(0, pos) };
                }
            }
            return *this;
        }

        bool MatchesBase(const GameplayTag& baseTag) const
        {
            return GetBaseTag() == baseTag;
        }

        bool MatchesTagExact(const GameplayTag& other) const
        {
            return GetBaseTag() == other.GetBaseTag();
        }

        bool MatchesTag(const GameplayTag& parentTag) const
        {
            const std::string child = GetBaseTag().name;
            const std::string parent = parentTag.GetBaseTag().name;
            if (child == parent)
            {
                return true;
            }
            return !parent.empty() && child.size() > parent.size() &&
                child.compare(0, parent.size(), parent) == 0 && child[parent.size()] == '.';
        }
    };

    struct GameplayTagHash
    {
        size_t operator()(const GameplayTag& tag) const
        {
            return std::hash<std::string>{}(tag.name);
        }
    };

    class GameplayTagContainer
    {
    public:
        void AddTag(const GameplayTag& tag)
        {
            if (tag.IsValid())
            {
                ++mTagCounts[tag];
            }
        }

        void RemoveTag(const GameplayTag& tag)
        {
            auto found = mTagCounts.find(tag);
            if (found == mTagCounts.end())
            {
                return;
            }
            if (--found->second <= 0)
            {
                mTagCounts.erase(found);
            }
        }

        bool HasTag(const GameplayTag& tag, bool exactMatch = false) const
        {
            for (const auto& pair : mTagCounts)
            {
                if (exactMatch ? pair.first.MatchesTagExact(tag) : pair.first.MatchesTag(tag))
                {
                    return true;
                }
            }
            return false;
        }

        bool HasAll(const std::vector<GameplayTag>& tags) const
        {
            for (const GameplayTag& tag : tags)
            {
                if (!HasTag(tag))
                {
                    return false;
                }
            }
            return true;
        }

        bool HasAny(const std::vector<GameplayTag>& tags) const
        {
            for (const GameplayTag& tag : tags)
            {
                if (HasTag(tag))
                {
                    return true;
                }
            }
            return false;
        }

        void Clear() { mTagCounts.clear(); }
        bool IsEmpty() const { return mTagCounts.empty(); }

    private:
        std::unordered_map<GameplayTag, int, GameplayTagHash> mTagCounts;
    };

    // =====================================================
    // PREDEFINED GAMEPLAY TAGS
    // =====================================================
    namespace GameTags
    {
        namespace Ship
        {
            inline const GameplayTag Engine_Main  = { "Ship.Engine.Main" };
            inline const GameplayTag Engine_Left  = { "Ship.Engine.Left" };
            inline const GameplayTag Engine_Right = { "Ship.Engine.Right" };

            inline const GameplayTag Weapon_Main  = { "Ship.Weapon.Main" };
            inline const GameplayTag Shield_Front = { "Ship.Shield.Front" };
        }

        namespace Bullet
        {
            inline const GameplayTag Laser_Red  = { "Bullet.Laser.Red" };
            inline const GameplayTag Laser_Blue = { "Bullet.Laser.Blue" };
        }

        namespace FX
        {
            inline const GameplayTag Explosion_Small = { "FX.Explosion.Small" };
        }
        namespace Environment
        {
			inline const GameplayTag Planet = { "Environment.Planet" };
            inline const GameplayTag Planet_Large = { "Environment.Planet.Large" };
            inline const GameplayTag Planet_Small = { "Environment.Planet.Small" };
            inline const GameplayTag Asteroid_Large = { "Environment.Asteroid.Large" };
            inline const GameplayTag Asteroid_Small = { "Environment.Asteroid.Small" };
		}
    }
}

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

}

// Legacy compatibility for the remaining engine asset/shader error sites.
// New code should select an explicit channel and level.
#define LOG(...) LY_CORE_ERROR(__VA_ARGS__)

#pragma once
#include <stdio.h>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <string>
#include <string_view>
#include <cctype>
#include <functional>

#include "framework/debug/Assert.h"
#include "framework/reflection/ReflectionDeclare.h"
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

	Layer0 = 1 << 0,
	Layer1 = 1 << 1,
	Layer2 = 1 << 2,
	Layer3 = 1 << 3,
	Layer4 = 1 << 4,
	Layer5 = 1 << 5,
	Layer6 = 1 << 6,
	Layer7 = 1 << 7,
};

namespace vega
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
            const std::string_view base = GetBaseView();
            if (base.size() == name.size())
            {
                return *this;
            }
            return GameplayTag{ std::string{ base } };
        }

        bool MatchesBase(const GameplayTag& baseTag) const
        {
            return GetBaseView() == std::string_view{ baseTag.name };
        }

        bool MatchesTagExact(const GameplayTag& other) const
        {
            return *this == other;
        }

        bool MatchesTag(const GameplayTag& parentTag) const
        {
            const std::string_view child = GetBaseView();
            const std::string_view parent = parentTag.GetBaseView();
            if (child == parent)
            {
                return true;
            }
            return !parent.empty() && child.size() > parent.size() &&
                child.compare(0, parent.size(), parent) == 0 && child[parent.size()] == '.';
        }

    private:
        // Name without a trailing "_<digits>" runtime index; never allocates.
        std::string_view GetBaseView() const
        {
            const size_t pos = name.rfind('_');
            if (pos == std::string::npos || pos + 1 == name.size())
            {
                return name;
            }
            for (size_t i = pos + 1; i < name.size(); ++i)
            {
                if (!std::isdigit(static_cast<unsigned char>(name[i])))
                {
                    return name;
                }
            }
            return std::string_view{ name }.substr(0, pos);
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
		std::vector<std::pair<GameplayTag, int>> BuildCountSnapshot() const
		{
			std::vector<std::pair<GameplayTag, int>> result(mTagCounts.begin(), mTagCounts.end());
			return result;
		}

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

namespace vega
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
#define LOG(...) VEGA_CORE_ERROR(__VA_ARGS__)

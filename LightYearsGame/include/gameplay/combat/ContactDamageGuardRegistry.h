#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace ly
{
class Actor;

struct ContactDamageGuardHandle
{
    uint64_t id{0};

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return id != 0;
    }

    constexpr bool operator==(const ContactDamageGuardHandle& other) const noexcept
    {
        return id == other.id;
    }
};

class ContactDamageGuardRegistry
{
public:
    using Guard = std::function<bool(const Actor&, const Actor&)>;

    ContactDamageGuardRegistry() = default;
    ~ContactDamageGuardRegistry() = default;

    ContactDamageGuardHandle Register(Guard guard);
    bool Unregister(ContactDamageGuardHandle handle);
    [[nodiscard]] bool Allows(const Actor& source, const Actor& target) const;
    void Clear();
    [[nodiscard]] bool IsEmpty() const;

private:
    struct Entry
    {
        uint64_t id{0};
        Guard guard;
    };

    std::vector<Entry> mGuards;
    // Do not reset this counter in Clear(). A handle issued before Clear()
    // must never become capable of unregistering a later guard.
    uint64_t mNextId{1};
};

} // namespace ly

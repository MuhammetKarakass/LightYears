#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <utility>

namespace ly
{
	class Actor;
	class World;
	// Persistent identity with a weak runtime cache; references never own actors.
	class ActorRef
	{
	public:
		ActorRef() = default;
		explicit ActorRef(std::string guid) : mGuid{ std::move(guid) } {}
		explicit ActorRef(Actor& actor);
		const std::string& Guid() const { return mGuid; }
		std::shared_ptr<Actor> Resolve(World& world) const;
		bool operator==(const ActorRef& other) const { return mGuid == other.mGuid; }
		bool operator!=(const ActorRef& other) const { return !(*this == other); }
	private:
		std::string mGuid;
		mutable std::weak_ptr<Actor> mActor;
		mutable unsigned int mWorld = 0;
		mutable std::uint64_t mRevision = static_cast<std::uint64_t>(-1);
	};
}

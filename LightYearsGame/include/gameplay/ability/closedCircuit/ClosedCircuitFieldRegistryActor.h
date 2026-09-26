#pragma once

#include "framework/Actor.h"

#include <cstdint>
#include <unordered_map>

namespace ly
{
	class ClosedCircuitFieldActor;

	class ClosedCircuitFieldRegistryActor final : public Actor
	{
	public:
		class Registration
		{
		public:
			Registration() = default;
			~Registration();
			Registration(Registration&& other) noexcept;
			Registration& operator=(Registration&& other) noexcept;
			Registration(const Registration&) = delete;
			Registration& operator=(const Registration&) = delete;

			bool IsValid() const { return mOwnerId != 0u && mGeneration != 0u && !mRegistry.expired(); }
			void Reset();

		private:
			friend class ClosedCircuitFieldRegistryActor;
			Registration(
				weak_ptr<ClosedCircuitFieldRegistryActor> registry,
				unsigned int ownerId,
				std::uint64_t generation
			);

			weak_ptr<ClosedCircuitFieldRegistryActor> mRegistry;
			unsigned int mOwnerId = 0u;
			std::uint64_t mGeneration = 0u;
		};

		explicit ClosedCircuitFieldRegistryActor(World* world);

		static shared_ptr<ClosedCircuitFieldRegistryActor> GetOrCreate(World& world);
		Registration RegisterField(
			Actor& owner,
			const shared_ptr<ClosedCircuitFieldActor>& field
		);
		shared_ptr<ClosedCircuitFieldActor> FindActiveField(Actor& owner);

	private:
		friend class Registration;
		struct Entry
		{
			std::uint64_t generation = 0u;
			weak_ptr<ClosedCircuitFieldActor> field;
		};

		void UnregisterField(unsigned int ownerId, std::uint64_t generation);

		std::unordered_map<unsigned int, Entry> mFields;
		std::uint64_t mNextGeneration = 0u;
	};
}

#pragma once

#include "framework/Actor.h"

#include <cstdint>
#include <unordered_map>

namespace vega
{
	class ClosedCircuitFieldActor;

	class ClosedCircuitFieldRegistry final
	{
		struct State;
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

			bool IsValid() const { return mOwnerId != 0u && mGeneration != 0u && !mState.expired(); }
			void Reset();

		private:
			friend class ClosedCircuitFieldRegistry;
			Registration(weak_ptr<State> state, unsigned int ownerId, std::uint64_t generation);

			weak_ptr<State> mState;
			unsigned int mOwnerId = 0u;
			std::uint64_t mGeneration = 0u;
		};

		explicit ClosedCircuitFieldRegistry(World* world);
		ClosedCircuitFieldRegistry(const ClosedCircuitFieldRegistry&) = delete;
		ClosedCircuitFieldRegistry& operator=(const ClosedCircuitFieldRegistry&) = delete;

		static ClosedCircuitFieldRegistry* GetOrCreate(World& world);
		Registration RegisterField(Actor& owner, const shared_ptr<ClosedCircuitFieldActor>& field);
		shared_ptr<ClosedCircuitFieldActor> FindActiveField(Actor& owner);

	private:
		friend class Registration;
		struct Entry
		{
			std::uint64_t generation = 0u;
			weak_ptr<ClosedCircuitFieldActor> field;
		};

		// Tokens lock state, never a service pointer. Access stays on the world thread.
		struct State
		{
			void UnregisterField(unsigned int ownerId, std::uint64_t generation);
			std::unordered_map<unsigned int, Entry> entries;
		};

		World* const mWorld;
		shared_ptr<State> mState = std::make_shared<State>();
		std::uint64_t mNextGeneration = 0u;
	};
}

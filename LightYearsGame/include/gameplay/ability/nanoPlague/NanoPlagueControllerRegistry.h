#pragma once

#include "framework/Actor.h"

#include <cstdint>
#include <unordered_map>

namespace vega
{
	class NanoPlagueControllerActor;

	class NanoPlagueControllerRegistry final
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

			bool IsValid() const
			{
				return mOwnerId != 0u && mGeneration != 0u && !mState.expired();
			}
			void Reset();

		private:
			friend class NanoPlagueControllerRegistry;
			Registration(weak_ptr<State> state, unsigned int ownerId, std::uint64_t generation);

			weak_ptr<State> mState;
			unsigned int mOwnerId = 0u;
			std::uint64_t mGeneration = 0u;
		};

		explicit NanoPlagueControllerRegistry(World* world);
		NanoPlagueControllerRegistry(const NanoPlagueControllerRegistry&) = delete;
		NanoPlagueControllerRegistry& operator=(const NanoPlagueControllerRegistry&) = delete;

		static NanoPlagueControllerRegistry* GetOrCreate(World& world);
		Registration RegisterController(Actor& owner, const shared_ptr<NanoPlagueControllerActor>& controller);
		shared_ptr<NanoPlagueControllerActor> FindActiveController(Actor& owner);

	private:
		friend class Registration;
		struct Entry
		{
			std::uint64_t generation = 0u;
			weak_ptr<NanoPlagueControllerActor> controller;
		};

		// Tokens lock state, never a service pointer. Access stays on the world thread.
		struct State
		{
			void UnregisterController(unsigned int ownerId, std::uint64_t generation);
			std::unordered_map<unsigned int, Entry> entries;
		};

		World* const mWorld;
		shared_ptr<State> mState = std::make_shared<State>();
		std::uint64_t mNextGeneration = 0u;
	};
}

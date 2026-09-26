#pragma once

#include "framework/Actor.h"

#include <cstdint>
#include <unordered_map>

namespace ly
{
	class NanoPlagueControllerActor;

	class NanoPlagueControllerRegistryActor final : public Actor
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

			bool IsValid() const
			{
				return mOwnerId != 0u && mGeneration != 0u && !mRegistry.expired();
			}
			void Reset();

		private:
			friend class NanoPlagueControllerRegistryActor;
			Registration(
				weak_ptr<NanoPlagueControllerRegistryActor> registry,
				unsigned int ownerId,
				std::uint64_t generation
			);

			weak_ptr<NanoPlagueControllerRegistryActor> mRegistry;
			unsigned int mOwnerId = 0u;
			std::uint64_t mGeneration = 0u;
		};

		explicit NanoPlagueControllerRegistryActor(World* world);

		static shared_ptr<NanoPlagueControllerRegistryActor> GetOrCreate(World& world);
		Registration RegisterController(
			Actor& owner,
			const shared_ptr<NanoPlagueControllerActor>& controller
		);
		shared_ptr<NanoPlagueControllerActor> FindActiveController(Actor& owner);

	private:
		friend class Registration;
		struct Entry
		{
			std::uint64_t generation = 0u;
			weak_ptr<NanoPlagueControllerActor> controller;
		};

		void UnregisterController(unsigned int ownerId, std::uint64_t generation);

		std::unordered_map<unsigned int, Entry> mControllers;
		std::uint64_t mNextGeneration = 0u;
	};
}

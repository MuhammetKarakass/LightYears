#pragma once
#include "framework/Core.h"  // weak_ptr, shared_ptr, List
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <utility>

namespace vega
{
	class DelegateHandle
	{
	public:
		DelegateHandle() = default;

		bool IsValid() const { return mId != 0; }
		void Reset() { mId = 0; }

	private:
		explicit DelegateHandle(std::uint64_t id) : mId{ id } {}

		std::uint64_t mId{ 0 };

		template<typename...>
		friend class Delegate;
	};

	class Object;

	// Multicast event. Callbacks may bind, unbind or clear during Broadcast: removals are
	// deferred and additions are queued until the outermost Broadcast finishes.
	template<typename... Args>
	class Delegate
	{
	public:
		// Binds a member function of an Object-derived class; the listener is dropped once the object expires.
		template<typename ClassName>
		DelegateHandle BindAction(weak_ptr<Object> obj, void(ClassName::* callback)(Args...))
		{
			std::function<bool(Args...)> callBackFunc = [obj, callback](Args... args)->bool
			{
				if (shared_ptr<Object> strongRef = obj.lock())
				{
					(static_cast<ClassName*>(strongRef.get())->*callback)(args...);
					return true;
				}
				return false;
			};

			return AddCallback(std::move(callBackFunc));
		}

		// Binds a member function on an object that is not shared_ptr managed. Lifetime is NOT checked.
		template<typename ClassName>
		DelegateHandle BindAction(ClassName* obj, void(ClassName::* callback)(Args...))
		{
			std::function<bool(Args...)> callBackFunc = [obj, callback](Args... args)->bool
			{
				if (obj != nullptr)
				{
					(obj->*callback)(args...);
					return true;
				}
				return false;
			};

			return AddCallback(std::move(callBackFunc));
		}

		bool UnbindAction(DelegateHandle handle)
		{
			if (!handle.IsValid())
			{
				return false;
			}

			bool removed = MarkInactive(mCallbacks, handle.mId);
			removed = MarkInactive(mPendingCallbacks, handle.mId) || removed;

			if (mBroadcastDepth == 0)
			{
				CompactCallbacks();
			}

			return removed;
		}

		void Clear()
		{
			for (CallbackEntry& entry : mCallbacks)
			{
				entry.active = false;
			}
			mPendingCallbacks.clear();

			if (mBroadcastDepth == 0)
			{
				mCallbacks.clear();
			}
		}

		void Broadcast(Args... args)
		{
			// Effects can change before any presentation or UI listener subscribes.
			// Avoid constructing a Debug iterator for an empty callback vector.
			if (mCallbacks.empty())
			{
				return;
			}

			++mBroadcastDepth;
			try
			{
				// Index loop: callbacks can append to mPendingCallbacks but never reallocate mCallbacks.
				for (std::size_t index = 0; index < mCallbacks.size(); ++index)
				{
					CallbackEntry& entry = mCallbacks[index];
					if (!entry.active)
					{
						continue;
					}
					// A callback that returns false has an expired listener; deactivate it.
					if (!entry.callback(args...))
					{
						entry.active = false;
					}
				}
			}
			catch (...)
			{
				EndBroadcast();
				throw;
			}

			EndBroadcast();
		}

	private:
		using Callback = std::function<bool(Args...)>;

		struct CallbackEntry
		{
			std::uint64_t id;
			Callback callback;
			bool active;
		};

		DelegateHandle AddCallback(Callback callback)
		{
			const std::uint64_t id = mNextCallbackId++;
			CallbackEntry entry{ id, std::move(callback), true };

			if (mBroadcastDepth == 0)
			{
				mCallbacks.push_back(std::move(entry));
			}
			else
			{
				mPendingCallbacks.push_back(std::move(entry));
			}

			return DelegateHandle{ id };
		}

		static bool MarkInactive(List<CallbackEntry>& callbacks, std::uint64_t id)
		{
			for (CallbackEntry& entry : callbacks)
			{
				if (entry.id == id && entry.active)
				{
					entry.active = false;
					return true;
				}
			}
			return false;
		}

		void CompactCallbacks()
		{
			mCallbacks.erase(
				std::remove_if(
					mCallbacks.begin(),
					mCallbacks.end(),
					[](const CallbackEntry& entry)
					{
						return !entry.active;
					}
				),
				mCallbacks.end()
			);
		}

		void EndBroadcast()
		{
			if (mBroadcastDepth == 0 || --mBroadcastDepth != 0)
			{
				return;
			}

			CompactCallbacks();
			if (!mPendingCallbacks.empty())
			{
				mCallbacks.reserve(mCallbacks.size() + mPendingCallbacks.size());
				std::move(
					mPendingCallbacks.begin(),
					mPendingCallbacks.end(),
					std::back_inserter(mCallbacks)
				);
				mPendingCallbacks.clear();
			}
		}

		List<CallbackEntry> mCallbacks;
		List<CallbackEntry> mPendingCallbacks;
		std::uint64_t mNextCallbackId{ 1 };
		std::size_t mBroadcastDepth{ 0 };
	};
}

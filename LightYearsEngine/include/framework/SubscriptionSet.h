#pragma once

#include "framework/Delegate.h"
#include <functional>
#include <utility>
#include <vector>

namespace ly
{
	// Own this set as a listener member and clear it while the listener is alive.
	class SubscriptionSet
	{
	public:
		SubscriptionSet() = default;
		~SubscriptionSet() { Clear(); }
		SubscriptionSet(const SubscriptionSet&) = delete;
		SubscriptionSet& operator=(const SubscriptionSet&) = delete;

		SubscriptionSet(SubscriptionSet&& other) noexcept
		{
			mSubscriptions.swap(other.mSubscriptions);
		}

		SubscriptionSet& operator=(SubscriptionSet&& other) noexcept
		{
			if (this != &other)
			{
				Clear();
				mSubscriptions.swap(other.mSubscriptions);
			}
			return *this;
		}

		// The delegate must be owned by the guarded source. Returns false (and binds nothing)
		// when the source has already expired or the listener/callback is null.
		template<typename Listener, typename... Args>
		bool Bind(const weak_ptr<Object>& source, Delegate<Args...>& delegate, Listener* listener, void (Listener::*callback)(Args...))
		{
			shared_ptr<Object> sourceOwner = source.lock();
			if (!sourceOwner || listener == nullptr || callback == nullptr)
			{
				return false;
			}

			const DelegateHandle handle = delegate.BindAction(listener, callback);
			mSubscriptions.push_back({ source, true, [&delegate, handle]() { delegate.UnbindAction(handle); } });
			return true;
		}

		// Caller must Clear before a non-shared source or its delegate is destroyed.
		template<typename Listener, typename... Args>
		bool BindUnguarded(Delegate<Args...>& delegate, Listener* listener, void (Listener::*callback)(Args...))
		{
			if (listener == nullptr || callback == nullptr)
			{
				return false;
			}

			const DelegateHandle handle = delegate.BindAction(listener, callback);
			mSubscriptions.push_back({ {}, false, [&delegate, handle]() { delegate.UnbindAction(handle); } });
			return true;
		}

		void Clear()
		{
			std::vector<Subscription> subscriptions;
			subscriptions.swap(mSubscriptions);
			for (Subscription& subscription : subscriptions)
			{
				if (subscription.guarded)
				{
					if (shared_ptr<Object> owner = subscription.source.lock())
					{
						subscription.unbind();
					}
				}
				else
				{
					subscription.unbind();
				}
			}
		}

		bool IsEmpty() const { return mSubscriptions.empty(); }
		std::size_t Count() const { return mSubscriptions.size(); }

	private:
		struct Subscription
		{
			weak_ptr<Object> source;
			bool guarded;
			std::function<void()> unbind;
		};

		std::vector<Subscription> mSubscriptions;
	};
}

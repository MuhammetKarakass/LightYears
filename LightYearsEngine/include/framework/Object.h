#pragma once

#include "framework/Delegate.h"
#include <memory>
#include "framework/Core.h"

namespace ly
{
	class Object : public std::enable_shared_from_this<Object>
	{
	public:
		Object();
		virtual ~Object();

		virtual void Destroy();
		bool GetIsPendingDestroy() const { return mPendingDestroy; };

		Delegate<Object*> onDestory;
		weak_ptr<Object> GetWeakPtr();
		weak_ptr<const Object> GetWeakPtr() const;
		unsigned int GetUniqueID() { return mUniqueID; };

	private:
		bool mPendingDestroy;

		unsigned int mUniqueID;
		static unsigned int uniqueIDCounter;

		static unsigned int GetNextAvailableID();
	};
}
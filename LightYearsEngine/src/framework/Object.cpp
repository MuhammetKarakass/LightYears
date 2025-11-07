#include "framework/Object.h"
#include "framework/Core.h"

namespace ly
{
	Object::Object()
		: mPendingDestroy(false)
	{

	}
	Object::~Object()
	{

	}
	void Object::Destroy()
	{
		mPendingDestroy = true;
		onDestory.Broadcast(this);
	}
	weak_ptr<Object> Object::GetWeakPtr()
	{
		return weak_from_this();
	}
	weak_ptr<const Object> Object::GetWeakPtr() const
	{
		return weak_from_this();
	}
}
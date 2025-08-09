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
		LOG("Object destroyed");
	}
	void Object::Destroy()
	{
		mPendingDestroy = true;
	}
}
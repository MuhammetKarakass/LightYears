#pragma once

namespace ly
{
	class Object
	{
	public:
		Object();
		virtual ~Object();

		void Destroy();
		bool GetIsPendingDestroy() const { return mPendingDestroy; };
	private:
		bool mPendingDestroy;
	};
}
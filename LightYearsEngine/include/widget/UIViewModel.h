#pragma once

#include <cstdint>

namespace ly
{
	class UIRevision
	{
	public:
		void Bump()
		{
			++mValue;
			if (mValue == 0)
				++mValue;
		}

		std::uint32_t Get() const { return mValue; }

	private:
		std::uint32_t mValue{ 1 };
	};

	class UIRevisionWatcher
	{
	public:
		bool Consume(const UIRevision& revision)
		{
			if (mFirstConsume || mSeen != revision.Get())
			{
				mFirstConsume = false;
				mSeen = revision.Get();
				return true;
			}
			return false;
		}

		void Reset()
		{
			mSeen = 0;
			mFirstConsume = true;
		}

	private:
		std::uint32_t mSeen{ 0 };
		bool mFirstConsume{ true };
	};

	template<typename T>
	bool SetIfChanged(T& field, const T& value, UIRevision& revision)
	{
		if (field == value)
			return false;
		field = value;
		revision.Bump();
		return true;
	}
}

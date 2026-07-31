#pragma once

#include <map>
#include <string>

namespace sas
{
	class AbilityCooldownTracker
	{
	public:
		bool IsActive(const std::string& key) const;
		void Start(const std::string& key, float duration);
		void Tick(float deltaTime);
		void Clear();

	private:
		std::map<std::string, float> mCooldowns;
	};
}

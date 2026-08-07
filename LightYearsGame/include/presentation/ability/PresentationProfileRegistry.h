#pragma once

#include "gameplay/content/ContentIdSchema.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

namespace ly
{
	template <typename Profile>
	class PresentationProfileRegistry
	{
	public:
		static bool Register(Profile profile)
		{
			if (!content::ContentIdSchema::ValidateAbilityPresentationProfileId(
				profile.profileId
			))
			{
				return false;
			}

			const std::string profileId = profile.profileId;
			return GetProfiles().emplace(profileId, std::move(profile)).second;
		}

		static const Profile* Find(const std::string& profileId)
		{
			const auto found = GetProfiles().find(profileId);
			return found != GetProfiles().end() ? &found->second : nullptr;
		}

		static bool Contains(const std::string& profileId)
		{
			return Find(profileId) != nullptr;
		}

		static std::size_t Count()
		{
			return GetProfiles().size();
		}

	private:
		static std::unordered_map<std::string, Profile>& GetProfiles()
		{
			static std::unordered_map<std::string, Profile> profiles;
			return profiles;
		}
	};
}

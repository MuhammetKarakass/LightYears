#include "gameplay/content/DamageStatusBalanceCatalog.h"

#include "gameplay/content/DamageStatusBalanceLoader.h"

#include <stdexcept>

namespace ly::content
{
	namespace
	{
		DamageStatusBalance& GetBalanceStorage()
		{
			static DamageStatusBalance balance;
			return balance;
		}

		bool& GetLoadedState()
		{
			static bool loaded = false;
			return loaded;
		}

		bool Fail(std::string* failureReason, const std::string& message)
		{
			if (failureReason)
			{
				*failureReason = message;
			}
			return false;
		}
	}

	bool DamageStatusBalanceCatalog::LoadFromFile(
		const std::filesystem::path& filePath,
		std::string* failureReason
	)
	{
		GetLoadedState() = false;
		const DamageStatusBalanceLoader::Result loaded =
			DamageStatusBalanceLoader::LoadFromFile(filePath);
		if (!loaded.Succeeded())
		{
			return Fail(failureReason, loaded.error);
		}
		GetBalanceStorage() = loaded.balance;
		GetLoadedState() = true;
		return true;
	}

	const DamageStatusBalance& DamageStatusBalanceCatalog::Get()
	{
		if (!GetLoadedState())
		{
			throw std::logic_error{ "Damage status balance catalog is not loaded" };
		}
		return GetBalanceStorage();
	}

	bool DamageStatusBalanceCatalog::IsLoaded() noexcept
	{
		return GetLoadedState();
	}
}

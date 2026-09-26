#include "framework/Actor.h"
#include "gameplay/targeting/ActorHitCooldowns.h"
#include <iostream>

namespace
{
	bool Expect(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << "FAILED: " << message << '\n';
			return false;
		}
		return true;
	}
}

int main()
{
	ly::Actor firstTarget{ nullptr, "" };
	ly::Actor secondTarget{ nullptr, "" };
	ly::targeting::ActorHitCooldowns cooldowns;
	cooldowns.RecordHit(firstTarget, 2.f);
	if (!Expect(cooldowns.GetEntryCount() == 1, "A hit did not create one cooldown entry") ||
		!Expect(cooldowns.IsCoolingDown(firstTarget, 1.999f), "The first target was not blocked before expiry") ||
		!Expect(!cooldowns.IsCoolingDown(secondTarget, 1.f), "One target's hit blocked an independent target") ||
		!Expect(!cooldowns.IsCoolingDown(firstTarget, 2.f), "The expiry boundary remained blocked"))
	{
		return 1;
	}

	cooldowns.PruneExpired(2.f);
	if (!Expect(cooldowns.GetEntryCount() == 0, "Pruning did not remove an entry at its expiry boundary"))
	{
		return 1;
	}

	cooldowns.RecordHit(firstTarget, 3.f);
	cooldowns.RecordHit(secondTarget, 4.f);
	cooldowns.PruneExpired(3.f);
	if (!Expect(cooldowns.GetEntryCount() == 1, "Pruning did not retain only the unexpired entry") ||
		!Expect(cooldowns.IsCoolingDown(secondTarget, 3.f), "The unexpired target was not still blocked"))
	{
		return 1;
	}
	cooldowns.PruneExpired(4.f);
	if (!Expect(cooldowns.GetEntryCount() == 0, "Pruning all expired entries did not return the count to zero"))
	{
		return 1;
	}

	unsigned int previousTargetId = 0;
	{
		ly::Actor previousTarget{ nullptr, "" };
		previousTargetId = previousTarget.GetUniqueID();
		cooldowns.RecordHit(previousTarget, 10.f);
		previousTarget.Destroy();
	}
	ly::Actor replacementTarget{ nullptr, "" };
	if (!Expect(replacementTarget.GetUniqueID() != previousTargetId, "A replacement actor reused the destroyed actor ID") ||
		!Expect(!cooldowns.IsCoolingDown(replacementTarget, 0.f), "A replacement actor inherited the destroyed actor's cooldown"))
	{
		return 1;
	}
	cooldowns.PruneExpired(10.f);
	for (int index = 0; index < 32; ++index)
	{
		ly::Actor shortLivedTarget{ nullptr, "" };
		cooldowns.RecordHit(shortLivedTarget, 11.f);
	}
	if (!Expect(cooldowns.GetEntryCount() == 32, "Short-lived targets did not create 32 distinct cooldown entries"))
	{
		return 1;
	}
	cooldowns.PruneExpired(11.f);
	if (!Expect(cooldowns.GetEntryCount() == 0, "Pruning short-lived target cooldowns did not return the count to zero"))
	{
		return 1;
	}
	return 0;
}

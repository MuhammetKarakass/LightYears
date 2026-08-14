#include "gameplay/ability/loadout/DefaultAbilityLoadout.h"

#include "gameplay/ability/gravityAnomaly/GravityAnomalyContracts.h"
#include "gameplay/ability/echoProtocol/EchoProtocolContracts.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/ability/relayPrism/RelayPrismContracts.h"

namespace ly
{
	const std::vector<DefaultAbilityLoadoutEntry>& GetDefaultAbilityLoadout()
	{
		static const std::vector<DefaultAbilityLoadoutEntry> loadout{
			// Q is reserved for the direct cursor projectile ability.
			{ AbilityData::GravityAnomaly::AbilityId::Basic, sas::AbilitySlot::Ability1 },
			// E is the shared Ability2 input. Relay Prism is the default ability
			// assigned to that slot; the runtime loadout may replace it later.
			{ AbilityData::RelayPrism::AbilityId::Basic, sas::AbilitySlot::Ability2 },
			// F is the recorded-ability replay utility.
			{ AbilityData::EchoProtocol::AbilityId::Basic, sas::AbilitySlot::Ability3 },
			// R is the multi-projectile Overdrive ability.
			{ AbilityData::OverdriveCore::AbilityId::Basic, sas::AbilitySlot::Ability4 }
		};
		return loadout;
	}
}

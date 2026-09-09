#include "gameplay/ability/loadout/DefaultAbilityLoadout.h"

#include "gameplay/ability/lanceDrive/LanceDriveContracts.h"
#include "gameplay/ability/relayPrism/RelayPrismContracts.h"
#include "gameplay/ability/glacialPressure/GlacialPressureContracts.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerProtocolContracts.h"

namespace ly
{
	const std::vector<DefaultAbilityLoadoutEntry>& GetDefaultAbilityLoadout()
	{
		static const std::vector<DefaultAbilityLoadoutEntry> loadout{
			// Q: Lance Drive - kinetic V-shaped ram.
			{ AbilityData::LanceDrive::AbilityId::Basic, sas::AbilitySlot::Ability1 },
			// E: Relay Prism - beam relay prism.
			{ AbilityData::RelayPrism::AbilityId::Basic, sas::AbilitySlot::Ability2 },
			// F: Glacial Pressure - multi-segment ice push cone.
			{ AbilityData::GlacialPressure::AbilityId::Basic, sas::AbilitySlot::Ability3 },
			// R: Reclaimer Protocol - kill-window repair kits.
			{ AbilityData::ReclaimerProtocol::AbilityId::Basic, sas::AbilitySlot::Ability4 }
		};
		return loadout;
	}
}

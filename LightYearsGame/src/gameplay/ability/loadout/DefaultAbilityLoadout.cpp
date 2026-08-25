#include "gameplay/ability/loadout/DefaultAbilityLoadout.h"

#include "gameplay/ability/crescentReaver/CrescentReaverContracts.h"
#include "gameplay/ability/combatSentry/CombatSentryContracts.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeContracts.h"
#include "gameplay/ability/energySpear/EnergySpearContracts.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolContracts.h"
#include "gameplay/ability/railBurst/RailBurstContracts.h"
#include "gameplay/ability/relayPrism/RelayPrismContracts.h"

namespace ly
{
	const std::vector<DefaultAbilityLoadoutEntry>& GetDefaultAbilityLoadout()
	{
		static const std::vector<DefaultAbilityLoadoutEntry> loadout{
			// Q places the physical Crystal Barricade at the cursor.
			{ AbilityData::CrystalBarricade::AbilityId::Basic, sas::AbilitySlot::Ability1 },
			// E intentionally remains Relay Prism while the other slots are tested.
			{ AbilityData::RelayPrism::AbilityId::Basic, sas::AbilitySlot::Ability2 },
			// F is Return Protocol's default test slot; loadout ownership remains runtime-based.
			{ AbilityData::ReturnProtocol::AbilityId::Basic, sas::AbilitySlot::Ability3 },
			// Temporary test loadout: R uses Combat Sentry.
			{ AbilityData::CombatSentry::AbilityId::Basic, sas::AbilitySlot::Ability4 }
		};
		return loadout;
	}
}

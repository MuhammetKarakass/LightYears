#include "gameplay/ability/loadout/DefaultAbilityLoadout.h"

#include "gameplay/ability/cryostasis/CryostasisContracts.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromContracts.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/ability/seismicCharge/SeismicChargeContracts.h"

namespace ly
{
	// Fighter arena control kit: establish a Cryo zone, spread NanoPlague through
	// the group, detonate a charge in the cluster, then use Cryostasis to survive.
	const std::vector<DefaultAbilityLoadoutEntry>& GetDefaultAbilityLoadout()
	{
		static const std::vector<DefaultAbilityLoadoutEntry> loadout{
			// Q: Frost Maelstrom - Cryo control zone that slows and holds enemies.
			{ AbilityData::FrostMaelstrom::AbilityId::Basic, sas::AbilitySlot::Ability1 },
			// E: NanoPlague - spreads damage through the controlled enemy group.
			{ AbilityData::NanoPlague::AbilityId::Basic, sas::AbilitySlot::Ability2 },
			// F: Seismic Charge - explosive damage placed inside the controlled group.
			{ AbilityData::SeismicCharge::AbilityId::Basic, sas::AbilitySlot::Ability3 },
			// R: Cryostasis - defensive recovery and Cryo pressure during the hold.
			{ AbilityData::Cryostasis::AbilityId::Basic, sas::AbilitySlot::Ability4 }
		};
		return loadout;
	}
}

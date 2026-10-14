#include "gameplay/ability/loadout/DefaultAbilityLoadout.h"

#include "gameplay/ability/cryoBola/CryoBolaContracts.h"
#include "gameplay/ability/ionStorm/IonStormContracts.h"
#include "gameplay/ability/rocket/RocketContracts.h"
#include "gameplay/ability/solarBombardment/SolarBombardmentContracts.h"

namespace ly
{
	// Default kit provides one Cryo, Kinetic, Thermal, and Electric ability.
	const std::vector<DefaultAbilityLoadoutEntry>& GetDefaultAbilityLoadout()
	{
		static const std::vector<DefaultAbilityLoadoutEntry> loadout{
			// Q: Cryo Bola - direct Cryo projectile damage without displacement.
			{ AbilityData::CryoBola::AbilityId::Basic, sas::AbilitySlot::Ability1 },
			// E: Rocket - Kinetic projectile damage.
			{ AbilityData::Rocket::AbilityId::Basic, sas::AbilitySlot::Ability2 },
			// F: Ion Storm - sustained Electric field damage.
			{ AbilityData::IonStorm::AbilityId::Basic, sas::AbilitySlot::Ability3 },
			// R: Solar Bombardment - Thermal area damage.
			{ AbilityData::SolarBombardment::AbilityId::Basic, sas::AbilitySlot::Ability4 }
		};
		return loadout;
	}
}

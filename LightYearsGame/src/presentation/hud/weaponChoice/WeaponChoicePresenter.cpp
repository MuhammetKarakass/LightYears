#include "presentation/hud/weaponChoice/WeaponChoicePresenter.h"
#include "player/Player.h"
#include "player/PlayerManager.h"

#include <cctype>

namespace vega
{
	WeaponChoicePresenter::WeaponChoicePresenter() : mViewModel{ std::make_shared<WeaponChoiceViewModel>() }
	{
	}

	std::string WeaponChoicePresenter::MakeWeaponLabel(const std::string& weaponId)
	{
		// The third id segment is the weapon's name in CamelCase.
		std::string name = weaponId;
		const std::size_t first = weaponId.find('.');
		const std::size_t second = first == std::string::npos ? std::string::npos : weaponId.find('.', first + 1);
		if (second != std::string::npos)
		{
			const std::size_t third = weaponId.find('.', second + 1);
			name = weaponId.substr(second + 1, third == std::string::npos ? std::string::npos : third - second - 1);
		}
		std::string label;
		for (std::size_t index = 0; index < name.size(); ++index)
		{
			const unsigned char character = static_cast<unsigned char>(name[index]);
			if (index > 0 && std::isupper(character) && !std::isupper(static_cast<unsigned char>(name[index - 1]))) label.push_back(' ');
			label.push_back(name[index]);
		}
		return label;
	}

	void WeaponChoicePresenter::Tick(float deltaTime)
	{
		(void)deltaTime;
		const Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		const bool open = player && !player->GetPendingSecondaryWeaponChoices().empty();
		std::uint32_t instanceId = 0;
		std::vector<WeaponChoiceOption> options;
		if (open)
		{
			instanceId = player->GetPendingSecondaryWeaponChoices().front();
			for (const std::string& weaponId : player->GetSecondaryWeaponOptions()) options.push_back({ weaponId, MakeWeaponLabel(weaponId) });
		}
		SetIfChanged(mViewModel->open, open, mViewModel->revision);
		SetIfChanged(mViewModel->moduleInstanceId, instanceId, mViewModel->revision);
		SetIfChanged(mViewModel->options, options, mViewModel->revision);
	}

	bool WeaponChoicePresenter::Choose(std::size_t index)
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player || !mViewModel->open || index >= mViewModel->options.size()) return false;
		const bool chosen = player->ChooseSecondaryWeapon(mViewModel->moduleInstanceId, mViewModel->options[index].weaponId);
		Tick(0.f);
		return chosen;
	}
}

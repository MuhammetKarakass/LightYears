#include "framework/Core.h"

#include <iostream>

namespace
{
	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}
}

int main()
{
	using namespace ly;

	const GameplayTag parent{ "Ability.Offense" };
	const GameplayTag indexedOne = parent.WithIndex(1);
	const GameplayTag indexedTwo = parent.WithIndex(2);
	const GameplayTag child{ "Ability.Offense.SolarBombardment" };

	if (!parent.MatchesTagExact(parent) || !indexedOne.MatchesTagExact(indexedOne))
		return Fail("Exact gameplay-tag equality rejected identical tags");
	if (parent.MatchesTagExact(indexedOne) || indexedOne.MatchesTagExact(indexedTwo) ||
		parent.MatchesTagExact(child) || child.MatchesTagExact(parent) ||
		parent.MatchesTagExact(GameplayTag{ "ability.Offense" }) ||
		parent.MatchesTagExact(GameplayTag{ "Ability.Defense" }))
	{
		return Fail("Exact gameplay-tag matching accepted a different identity");
	}
	if (!child.MatchesTag(parent) || !indexedOne.MatchesTag(parent) ||
		!indexedOne.MatchesBase(parent) || parent.MatchesBase(indexedOne))
	{
		return Fail("Hierarchical or base gameplay-tag matching changed its established behavior");
	}

	GameplayTagContainer parentTags;
	parentTags.AddTag(parent);
	if (!parentTags.HasTag(parent, true) || parentTags.HasTag(indexedOne, true) ||
		parentTags.HasTag(child, true))
	{
		return Fail("GameplayTagContainer exact matching did not use exact tag identity");
	}
	GameplayTagContainer childTags;
	childTags.AddTag(child);
	if (!childTags.HasTag(parent) || childTags.HasTag(parent, true) || !childTags.HasTag(child, true))
		return Fail("GameplayTagContainer changed hierarchical matching or exact child matching");

	GameplayTagContainer indexedTags;
	indexedTags.AddTag(indexedOne);
	if (indexedTags.HasTag(parent, true) || !indexedTags.HasTag(parent))
		return Fail("GameplayTagContainer lost indexed base matching");

	return 0;
}

#include "framework/ComponentIdentity.h"
#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/reflection/Reflection.h"

namespace ly
{
	std::optional<ComponentIdentity> IdentifyComponent(const Actor& actor, const ActorComponent& target)
	{
		const auto* type = actor.GetEngineContext().Types().FindMostDerived(target);
		if (!type) return {};
		std::optional<ComponentIdentity> result;
		std::size_t occurrence = 0;
		actor.ForEachComponent([&](const ActorComponent& component)
		{
			if (actor.GetEngineContext().Types().FindMostDerived(component) != type) return;
			if (&component == &target) result = ComponentIdentity{ type->name, occurrence };
			++occurrence;
		});
		return result;
	}

	ActorComponent* FindComponentByTypeOccurrence(Actor& actor, const ComponentIdentity& identity)
	{
		ActorComponent* result = nullptr;
		std::size_t occurrence = 0;
		actor.ForEachComponent([&](ActorComponent& component)
		{
			const auto* type = actor.GetEngineContext().Types().FindMostDerived(component);
			if (type && type->name == identity.typeName && occurrence++ == identity.occurrence) result = &component;
		});
		return result;
	}
}

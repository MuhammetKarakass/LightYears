#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include <algorithm>

namespace ly
{
	namespace
	{
		using AbilityActorHandlerMap = Dictionary<
			GameplayTag,
			unique_ptr<AbilityActorTypeHandler>,
			GameplayTagHash
		>;

		AbilityActorHandlerMap& GetHandlers()
		{
			static AbilityActorHandlerMap handlers;
			return handlers;
		}

		const List<GameplayTag> EmptyTags{};
		const List<GameplayTag> GenericAbilityActorAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius,
			CommonAttributeIds::CollisionRadius
		};

		bool HasExactTag(const List<GameplayTag>& tags, const GameplayTag& expectedTag)
		{
			return std::any_of(tags.begin(), tags.end(), [&](const GameplayTag& tag)
			{
				return tag.MatchesTagExact(expectedTag);
			});
		}

		bool MatchesAnyRoot(const GameplayTag& tag, const List<GameplayTag>& roots)
		{
			return std::any_of(roots.begin(), roots.end(), [&](const GameplayTag& root)
			{
				return tag.MatchesTag(root);
			});
		}

		class GenericAbilityActorType final : public AbilityActorTypeHandler
		{
		public:
			const GameplayTag& GetActorTypeTag() const override
			{
				return AbilityActorSchema::Generic::TypeId;
			}

			const List<GameplayTag>& GetAllowedCommonAttributeIds() const override
			{
				return GenericAbilityActorAttributes;
			}

			weak_ptr<AbilityWorldActor> Spawn(const AbilityActorSpawnContext& context) const override
			{
				World* world = context.owner.GetWorld();
				return world
					? world->SpawnActor<AbilityWorldActor>(&context.owner, context.definition.texturePath)
					: weak_ptr<AbilityWorldActor>{};
			}
		};

		void EnsureBuiltInHandlers()
		{
			static const bool initialized = []
			{
				GetHandlers().emplace(
					AbilityActorSchema::Generic::TypeId,
					std::make_unique<GenericAbilityActorType>()
				);
				return true;
			}();
			(void)initialized;
		}

		const AbilityActorTypeHandler* FindHandler(const GameplayTag& actorTypeTag)
		{
			EnsureBuiltInHandlers();
			auto found = GetHandlers().find(actorTypeTag);
			return found != GetHandlers().end() ? found->second.get() : nullptr;
		}
	}

	AbilityActorValidationResult AbilityActorTypeHandler::ValidateDefinition(const AbilityActorDefinition& definition) const
	{
		return definition.actorDefinitionId.empty()
			? AbilityActorValidationResult{ false, "Ability actor definition requires an ID." }
			: AbilityActorValidationResult{ true, {} };
	}

	const List<GameplayTag>& AbilityActorTypeHandler::GetOwnedAttributeRoots() const
	{
		return EmptyTags;
	}

	const List<GameplayTag>& AbilityActorTypeHandler::GetAllowedCommonAttributeIds() const
	{
		return EmptyTags;
	}

	bool AbilityActorRegistry::RegisterHandler(unique_ptr<AbilityActorTypeHandler> handler)
	{
		EnsureBuiltInHandlers();
		if (!handler || !handler->GetActorTypeTag().IsValid())
		{
			return false;
		}
		return GetHandlers().emplace(handler->GetActorTypeTag(), std::move(handler)).second;
	}

	AbilityActorValidationResult AbilityActorRegistry::ValidateDefinition(const AbilityActorDefinition& definition)
	{
		const AbilityActorTypeHandler* handler = FindHandler(definition.actorTypeTag);
		if (!handler)
		{
			return { false, "No ability actor handler is registered for this actor type." };
		}

		List<GameplayTag> declaredAttributeIds;
		for (const sas::GameplayAttribute& attribute : definition.attributes)
		{
			if (!attribute.id.IsValid() || HasExactTag(declaredAttributeIds, attribute.id))
			{
				return { false, "Ability actor attributes must have valid, unique IDs." };
			}
			declaredAttributeIds.push_back(attribute.id);
			if (!HasExactTag(handler->GetAllowedCommonAttributeIds(), attribute.id) &&
				!MatchesAnyRoot(attribute.id, handler->GetOwnedAttributeRoots()))
			{
				return { false, "Ability actor attribute is not consumed by the selected actor type." };
			}
		}

		return handler->ValidateDefinition(definition);
	}

	weak_ptr<AbilityWorldActor> AbilityActorRegistry::Spawn(const AbilityActorSpawnContext& context)
	{
		const AbilityActorTypeHandler* handler = FindHandler(context.definition.actorTypeTag);
		return handler ? handler->Spawn(context) : weak_ptr<AbilityWorldActor>{};
	}
}

#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"
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
			CollisionAttributeIds::Radius
		};

		const GameplayTag AbilityActorAttributeRoot{ "Attribute.AbilityActor" };

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

		bool IsFamilyScopedAbilityActorAttributeRoot(
			const GameplayTag& root,
			const std::string& family
		)
		{
			// Only feature-local actor roots are constrained here. Roots such as
			// Attribute.Damage are intentionally reusable across families.
			if (!root.MatchesTag(AbilityActorAttributeRoot))
			{
				return true;
			}

			const std::string expectedPrefix =
				AbilityActorAttributeRoot.name + "." + family + ".";
			return root.name.compare(0, expectedPrefix.size(), expectedPrefix) == 0 &&
				root.name.size() > expectedPrefix.size();
		}

		class GenericAbilityActorType final : public AbilityActorTypeHandler
		{
		public:
			const GameplayTag& GetActorTypeTag() const override
			{
				return AbilityActorSchema::Generic::TypeTag;
			}

			const List<GameplayTag>& GetAllowedCommonAttributeIds() const override
			{
				return GenericAbilityActorAttributes;
			}

			weak_ptr<AbilityWorldActor> Spawn(const AbilityActorSpawnContext& context) const override
			{
				World* world = context.owner.GetWorld();
				return world
					? world->SpawnActor<AbilityWorldActor>(&context.owner, "")
					: weak_ptr<AbilityWorldActor>{};
			}
		};

		void EnsureBuiltInHandlers()
		{
			static const bool initialized = []
			{
				GetHandlers().emplace(
					AbilityActorSchema::Generic::TypeTag,
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
		std::string failureReason;
		content::ParsedFamilyRoleId actorId;
		if (!content::ContentIdSchema::ParseAbilityActorDefinitionId(
			definition.actorDefinitionId,
			actorId,
			&failureReason
		))
		{
			return { false, failureReason };
		}
		content::ParsedFamilyRoleId presentationId;
		if (!content::ContentIdSchema::ParseAbilityPresentationProfileId(
			definition.presentationProfileId,
			presentationId,
			&failureReason
		))
		{
			return { false, failureReason };
		}
		if (actorId.family != presentationId.family || actorId.role != presentationId.role)
		{
			return {
				false,
				"Ability actor and presentation profile IDs must use the same family and role."
			};
		}
		const GameplayTag expectedTypeTag{
			"AbilityActor." + actorId.family + "." + actorId.role
		};
		if (!definition.actorTypeTag.MatchesTagExact(expectedTypeTag))
		{
			return {
				false,
				"Ability actor type tag must match the family and role encoded by its definition ID."
			};
		}
		return { true, {} };
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
		std::string tagFailureReason;
		if (!GameplayTagSchema::Validate(
			definition.actorTypeTag,
			GameplayTagKind::AbilityActorType,
			&tagFailureReason
		))
		{
			return { false, "Ability actor type tag is invalid: " + tagFailureReason };
		}
		const AbilityActorTypeHandler* handler = FindHandler(definition.actorTypeTag);
		if (!handler)
		{
			return { false, "No ability actor handler is registered for this actor type." };
		}

		content::ParsedFamilyRoleId actorId;
		if (!content::ContentIdSchema::ParseAbilityActorDefinitionId(
			definition.actorDefinitionId,
			actorId,
			&tagFailureReason
		))
		{
			return { false, tagFailureReason };
		}
		for (const GameplayTag& root : handler->GetOwnedAttributeRoots())
		{
			if (!IsFamilyScopedAbilityActorAttributeRoot(root, actorId.family))
			{
				return {
					false,
					"Ability actor custom attribute roots must use "
					"Attribute.AbilityActor.<Family>.<Role>."
				};
			}
		}

		List<GameplayTag> declaredAttributeIds;
		for (const sas::GameplayAttribute& attribute : definition.attributes)
		{
			if (!GameplayTagSchema::Validate(
				attribute.id,
				GameplayTagKind::Attribute,
				&tagFailureReason
			) || HasExactTag(declaredAttributeIds, attribute.id))
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

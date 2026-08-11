#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/attributes/AttributeIdSchema.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include <algorithm>
#include <string_view>

namespace ly
{
	namespace
	{
		using AbilityActorHandlerMap = Dictionary<
			AbilityActorType,
			unique_ptr<AbilityActorTypeHandler>,
			std::hash<AbilityActorType>
		>;

		AbilityActorHandlerMap& GetHandlers()
		{
			static AbilityActorHandlerMap handlers;
			return handlers;
		}

		const List<sas::AttributeId> EmptyAttributeRoots{};
		const List<sas::AttributeId> EmptyAttributeIds{};
		const List<sas::AttributeId> GenericAbilityActorAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius,
			CollisionAttributeIds::Radius
		};

		const sas::AttributeId AbilityActorAttributeRoot{ "AbilityActor" };

		bool HasExactAttribute(
			const List<sas::AttributeId>& ids,
			const sas::AttributeId& expectedId
		)
		{
			return std::find(ids.begin(), ids.end(), expectedId) != ids.end();
		}

		bool MatchesAnyRoot(const sas::AttributeId& id, const List<sas::AttributeId>& roots)
		{
			return std::any_of(roots.begin(), roots.end(), [&](const sas::AttributeId& root)
			{
				const std::string_view name = id.GetName();
				const std::string_view prefix = root.GetName();
				return name.size() >= prefix.size() &&
					name.compare(0, prefix.size(), prefix) == 0 &&
					(name.size() == prefix.size() || name[prefix.size()] == '.');
			});
		}

		bool IsFamilyScopedAbilityActorAttributeRoot(
			const sas::AttributeId& root,
			const std::string& family
		)
		{
			// Only feature-local actor roots are constrained here. Roots such as
			// Damage are intentionally reusable across families.
			if (!AttributeIdSchema::IsInNamespace(root, AbilityActorAttributeRoot.GetName()))
			{
				return true;
			}

			const std::string expectedPrefix =
				std::string{ AbilityActorAttributeRoot.GetName() } + "." + family + ".";
			return root.GetName().compare(0, expectedPrefix.size(), expectedPrefix) == 0 &&
				root.GetName().size() > expectedPrefix.size();
		}

		class GenericAbilityActorType final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::Generic;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
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
					AbilityActorType::Generic,
					std::make_unique<GenericAbilityActorType>()
				);
				return true;
			}();
			(void)initialized;
		}

		const AbilityActorTypeHandler* FindHandler(AbilityActorType actorType)
		{
			EnsureBuiltInHandlers();
			auto found = GetHandlers().find(actorType);
			return found != GetHandlers().end() ? found->second.get() : nullptr;
		}
	}

	AbilityActorValidationResult AbilityActorTypeHandler::ValidateDefinition(const AbilityActorDefinition& definition) const
	{
		std::string failureReason;
		content::ParsedFamilyRoleId actorId;
		if (!content::ContentIdSchema::ParseAbilityActorDefinitionId(
			definition.actorDefinitionId.ToString(),
			actorId,
			&failureReason
		))
		{
			return { false, failureReason };
		}
		content::ParsedFamilyRoleId presentationId;
		if (!content::ContentIdSchema::ParseAbilityPresentationProfileId(
			definition.presentationProfileId.ToString(),
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
		return { true, {} };
	}

	const List<sas::AttributeId>& AbilityActorTypeHandler::GetOwnedAttributeRoots() const
	{
		return EmptyAttributeRoots;
	}

	const List<sas::AttributeId>& AbilityActorTypeHandler::GetAllowedCommonAttributeIds() const
	{
		return EmptyAttributeIds;
	}

	bool AbilityActorRegistry::RegisterHandler(unique_ptr<AbilityActorTypeHandler> handler)
	{
		EnsureBuiltInHandlers();
		if (!handler)
		{
			return false;
		}
		return GetHandlers().emplace(handler->GetActorType(), std::move(handler)).second;
	}

	AbilityActorValidationResult AbilityActorRegistry::ValidateDefinition(const AbilityActorDefinition& definition)
	{
		const AbilityActorTypeHandler* handler = FindHandler(definition.actorType);
		if (!handler)
		{
			return { false, "No ability actor handler is registered for this actor type." };
		}

		std::string idFailureReason;
		content::ParsedFamilyRoleId actorId;
		if (!content::ContentIdSchema::ParseAbilityActorDefinitionId(
			definition.actorDefinitionId.ToString(),
			actorId,
			&idFailureReason
		))
		{
			return { false, idFailureReason };
		}
		for (const sas::AttributeId& root : handler->GetOwnedAttributeRoots())
		{
			if (!IsFamilyScopedAbilityActorAttributeRoot(root, actorId.family))
			{
				return {
					false,
					"Ability actor custom attribute roots must use "
					"AbilityActor.<Family>.<Role>."
				};
			}
		}

		List<sas::AttributeId> declaredAttributeIds;
		for (const sas::GameplayAttribute& attribute : definition.attributes)
		{
			if (!AttributeIdSchema::Validate(attribute.id, nullptr) || HasExactAttribute(declaredAttributeIds, attribute.id))
			{
				return { false, "Ability actor attributes must have valid, unique IDs." };
			}
			declaredAttributeIds.push_back(attribute.id);
			if (!HasExactAttribute(handler->GetAllowedCommonAttributeIds(), attribute.id) &&
				!MatchesAnyRoot(attribute.id, handler->GetOwnedAttributeRoots()))
			{
				return { false, "Ability actor attribute is not consumed by the selected actor type." };
			}
		}

		return handler->ValidateDefinition(definition);
	}

	weak_ptr<AbilityWorldActor> AbilityActorRegistry::Spawn(const AbilityActorSpawnContext& context)
	{
		const AbilityActorTypeHandler* handler = FindHandler(context.definition.actorType);
		return handler ? handler->Spawn(context) : weak_ptr<AbilityWorldActor>{};
	}
}

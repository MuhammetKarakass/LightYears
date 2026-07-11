#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeSystem.h"
#include <string>

namespace ly
{
	struct AbilityActorSchema
	{
		struct Generic
		{
			inline static const GameplayTag TypeId{ "AbilityActor.Generic" };
		};

        struct SunBeam
        {
            inline static const GameplayTag SharedAttributeRoot{
                "Attribute.AbilityActor.SunBeam.Shared"
            };

			inline static const GameplayTag Width{
				"Attribute.AbilityActor.SunBeam.Shared.Width"
			};

			inline static const GameplayTag Length{
				"Attribute.AbilityActor.SunBeam.Shared.Length"
			};

            struct Strike
            {
                inline static const GameplayTag TypeId{
                    "AbilityActor.SunBeam.Strike"
                };

                inline static const GameplayTag AttributeRoot{
                    "Attribute.AbilityActor.SunBeam.Strike"
                };

                inline static const GameplayTag TelegraphDuration{
                    "Attribute.AbilityActor.SunBeam.Strike.TelegraphDuration"
                };

				inline static const GameplayTag ArrivalDuration{
					"Attribute.AbilityActor.SunBeam.Strike.ArrivalDuration"
				};

				inline static const GameplayTag ImpactVisualDuration{
					"Attribute.AbilityActor.SunBeam.Strike.ImpactVisualDuration"
				};
            };
        };
	};


	struct AbilityActorDefinition
	{
		std::string actorDefinitionId;
		GameplayTag actorTypeTag = AbilityActorSchema::Generic::TypeId;
		std::string texturePath;
		float lifeTime = 0.f;
		float spawnDistance = 0.f;
		GameplayAttributeList attributes;
		std::string telegraphVisualId;
		std::string visualId;
	};
}

namespace AbilityData
{
	const ly::AbilityActorDefinition* FindAbilityActorDefinition(const std::string& actorDefinitionId);
}



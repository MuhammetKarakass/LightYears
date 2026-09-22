# Identity and contract rules

This document records the current identity boundaries used by the runtime.
The concrete type names below are part of the contract; changing them requires
an explicit migration rather than a documentation-only alias.

## One owner per identity kind

- `GameplayTags` owns semantic, queryable and hierarchical gameplay values.
- `AttributeId` owns numeric runtime value keys. `AttributeIdSchema` validates
  only lexical structure; feature catalogs own which attributes exist.
- Content IDs remain string-backed record identities. Some fields expose them
  as `std::string` (`sas::AbilityDefinition::abilityId` and
  `sas::GameplayEffectDefinition::effectId`); other boundaries use the typed
  `ContentId` wrapper. The wrapper does not turn a content identity into a
  gameplay tag or numeric key.
- `AbilityActorDefinition::actorDefinitionId` and
  `AbilityActorDefinition::presentationProfileId` are `ContentId` values.
  They identify records and profiles; they are not gameplay tags or numeric
  attributes.
- `AbilityBehaviorType`, `AbilityActorType` and primary-weapon enums are
  closed implementation selectors for registry dispatch. They are not tags.
- `GameplayEffectBehaviorKey` is a narrow JSON/registry boundary key. It is not
  a semantic gameplay identity.
- Ability and weapon progression upgrade IDs are strings, not gameplay tags.
  The same rule applies to ability level/unlock upgrade IDs in JSON and game
  content contracts.
- `EncounterWaveSnapshot` is a read-only runtime projection, not a content ID,
  gameplay tag, attribute identity or mutable encounter state. `EncounterWaveRuntime`
  remains the sole owner of wave state and enemy-reference cleanup.

## Contract boundary

Feature contracts may own:

- stable feature-local content IDs;
- semantic tag references from `GameplayTags`;
- feature-local `AttributeId` roots and leaves;
- numeric setting contracts;
- feature-local effect/actor schema data that is consumed by that feature.

Contracts must not own registry maps, behavior/actor/weapon dispatch selectors,
global tag creation, runtime state, or cross-feature rules.

## Central semantic tags

Semantic tag values are aggregated and constructed once under
`gameplay/tags/GameplayTags.h`. Leaf declarations for ability families live
under the feature-local `tags/ability/` files and are included by the
aggregator. `GameplayTagSchema` validates domains and shared rules; it is not
a catalog of feature identities. Feature contracts may expose references to
central tags, but must not construct duplicate literal tags.

## Content and JSON boundary

JSON field names remain compatibility surface until a separate migration. The
loader converts string selectors into the narrow enum/key type immediately and
the runtime never uses those JSON strings as gameplay tags.

# GAS content authoring

Open **View > Content Authoring** or **Content > Author definitions**. Choose an
ability, effect, module, ship, or weapon on the left. Search matches IDs and display
names; the tag/category filter also matches hierarchical authored tags. Each entry
shows the number of incoming authored references. **Find references** lists source
definitions and field paths with jump links.

The right form comes from game-registered schema metadata: categories, defaults,
tooltips, number ranges/units, booleans, enums, tag and attribute pickers, references,
objects and lists. Tag pickers expose a searchable hierarchy. Suggestions combine
the game's `GameplayTagSchema` domains and authored tags accepted by that schema;
attribute suggestions come from authored IDs accepted by `AttributeIdSchema`.
Both schemas describe an extensible vocabulary, so **Edit ID** permits new names.
Reference fields offer **Jump**. Numeric per-level arrays use a level/value table
and curve preview. Optional fields are collapsed until needed. Unknown keys appear
under **Other**, read-only, and survive edits and saves.

**New** chooses a named game template or an existing definition. **Duplicate** uses
the selection as its template and requires a fresh ID. The `TimedModifier` effect
template owns its duration and modifier; source-parameterized effects instead get
numbers from an ability's `effectSpecs` and reject numeric defaults in `effects.json`.
The form surfaces those existing loader errors. Ability copies retain the existing
`baseId` contract and share global actor records with their base.

**Rename** previews the definition ID and every affected typed reference before
applying them in one undo step across files. Ordinary strings, tag identity and
opaque data are not references. **Delete** lists incoming references and refuses
deletion until they are removed. All form operations use the editor's undo/redo
history without dirtying the scene. The exit dialog handles unsaved content.

**Validate all** produces a report with jump links; errors for the selected
definition also appear above its form. Validation uses the production loaders and
existing SAS/game validators, including behavior validation, effect behavior/visual
registration, module rules, weapon execution rules and ship/module schematics.
Unsaved effect references use an injected lookup; existing ability behavior
validators read candidate numeric settings through `ScopedAbilityValidationView`,
a stack-scoped thread-local view that leaves runtime catalogs intact. Each existing
validator is fail-fast: the report includes the first semantic failure per
definition plus all missing typed references and duplicate IDs. Fix and revalidate
to reveal subsequent failures in that definition.

Save validates all loaded content, then replaces each changed file atomically via
a unique sibling temporary file. JSON node spans preserve existing key order,
whitespace, line endings, escapes and number spellings. Scalar edits replace only
their token. Structural edits rewrite separators in their changed container and
reuse the original bodies of unchanged definitions. New bodies follow the file's
detected indentation. Undo to the original document restores its exact bytes.
Files changed externally are refused at save time instead of overwritten. Files
are atomic individually; a multi-file save is not a filesystem transaction.

The game currently initializes its content catalogs for the process lifetime.
The panel displays **Applies on next Play (restart editor for compiled content
catalogs)**. Save, restart the editor, and start Play to use the new data. Content
edits are disabled during Play. Root ability IDs are also compiled behavior
selectors and cannot be renamed safely with the current runtime format; author
variants with `baseId` and rename those. Compiled C++ consumers are outside the
authored reference index. No active runtime objects are patched.

## Schema API

The engine owns the game-agnostic header
`framework/content/ContentSchema.h`. The game registers descriptors in
`GameContentSchemas.cpp`; `GameContentFields.inc` is its explicit authored field
vocabulary. It is not regenerated per frame or automatically by builds. When
deliberately adding supported fields, update the descriptor and vocabulary; the
source helper `node tools/generate-content-fields.cjs` can refresh field paths
and example defaults from reviewed shipped content. Unknown fields stay opaque
until deliberately registered. Empty list types without a row descriptor cannot
create rows until that descriptor is supplied.

```cpp
ly::content::ContentTypeSchema schema;
schema.type = "effect";
schema.file = "content/data/effects.json";
schema.collection = "effects";
ly::content::ContentField duration;
duration.path = "/duration";
duration.kind = ly::content::FieldKind::Number;
duration.defaultValue = 2.0;
duration.category = "Timing";
duration.tooltip = "Duration of a content-owned effect.";
duration.minimum = 0.0;
duration.unit = "s";
schema.fields.push_back(duration);
// /modifiers/*/attributeId describes every modifier row.
// Supply schema.validate using the game's existing loader and validators.
ly::content::ContentSchemaRegistry::Get().Register(std::move(schema));
```

Field patterns are JSON pointers relative to a definition; `*` matches an array
row. `Reference` fields specify `referenceType`. Registry predicates validate tag
and attribute vocabulary; optional type callbacks provide ID validation, templates
and format-aware duplication. The engine and editor contain no game headers for
this feature. Forms, reference indexes, suggestions, unknown-key previews and
validation results are cached by workspace revision and selection. JSON is only
copied for edits, validation and reload, not reconstructed every frame.

## Automation and verification

Run from `build/VegaEditor` with `--editor-script` and a disposable `--asset-root`.
Commands and JSON/path syntax are documented in
[the script README](../VegaEditor/tests/scripts/README.md). `content-validate`
fails the script on errors; `content-save` validates before writing;
`expect-content` reads the selected type/ID independently of the form selection.
`content-reload` reopens files from disk and refuses unsaved edits.

`VegaEditorGasContent` duplicates a templated effect, edits duration and a
modifier, references it from a duplicated ability, renames it with reference
updates, verifies undo/redo, validates, saves and reopens. It invokes the existing
game content-loader test executable on the result and verifies source hashes.
Its input is reset under `build/VegaEditor/gas-content-assets`; it never writes
to `LightYearsGame/assets`. The report and capture are build artifacts.
The same test runs an invalid unsaved numeric setting and checks that the script
fails through the existing Dash behavior validator without modifying saved data.
`VegaEditorContentWorkspace` tests reference identity, opaque data, cache
invalidation, deletion guards, one-step rename undo, exact unchanged bytes,
validation failures and reload/history behavior. `VegaEditorScript` covers parser
arity, nested paths, JSON values and malformed commands; `VegaEditorJsonDocument`
continues checking every shipped JSON file's byte-exact round trip.

The Ability System debugger docks with Inspector on first appearance in older
layouts. Existing authored dock/floating settings and the layout filename are
preserved; the default layout builder also reserves its Inspector tab.

## Optional engine module

The debugger is registered by `sas::RegisterEditorExtension` from the optional
`VegaGASEditor` target. Generic ability/effect field metadata lives in
`Modules/GAS`; LightYears retains its game schemas, ID vocabulary, templates
and production validators. `VegaEditorCore` builds without GAS. See
[engine modules](engine-modules.md) for runtime and editor setup.

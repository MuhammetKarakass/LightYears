# Reflection

Reflection describes the C++ properties shared by the Inspector, scene and
prefab serializers, and Lua. Include `framework/reflection/Reflection.h`; it
includes `ReflectionMacros.h` at the end. The macros are a thin spelling of the
existing `ly::reflect::Class<T, Base>` builder, which remains available.

## Runtime model

`TypeRegistry::Get()` is the process-wide registry. A `TypeInfo` contains its
registered name, base descriptor, root kind (Actor, Component, or Other), local
properties, instance predicate, optional spawn/add factories, removal policy,
and required component names. Register bases first: constructing a derived
builder resolves its base immediately. `RegisteredTypes()` enumerates
descriptors in registration order. Engine and game entry points guard repeated
calls; repeated builder registration reuses existing type/property descriptors.

`TypeInfo::AllProperties()` visits inherited properties before local ones;
`FindProperty(name)` checks local properties before the base. Requirements are
inherited. `FindMostDerived(actor)` and its component overload test registered
types against the live object and choose the deepest matching base chain. An
unregistered subclass uses its closest registered ancestor. SoundComponent is
an independent Other root outside actor-component discovery.

`PropertyInfo` stores the name, kind, erased getter and optional setter, range,
drag speed, tooltip, category, and enum/flag labels. Erased pointers must point
to the registered root (Actor or ActorComponent), allowing pointer adjustment
even under multiple inheritance. `PropertyValue` holds bool, int, float,
Vector2f, Color, string, ActorRef, StructValue, or PropertyList; enums and flags travel as int. `SetValue` checks
the value kind, finite numbers, declared enum labels/flag bits, and catches
setter exceptions. Read-only properties have no setter. Ranges describe UI
controls; property setters enforce domain rules, including clamping. Fields have no
setter, so `SetValue` clamps their numeric values to Range.

**Property names are scene keys and must not be renamed.** Registered type names,
component names, and enum/flag labels also appear in authored files. Rename a
C++ method while keeping its reflected string unchanged. Changing persistent
names requires an explicit migration of scene/prefab assets.

## Macro reference

Put `VEGA_CLASS(Type)` first in the class body in its header. Put the description
block in the class's own `.cpp`, inside its namespace. `Type` must be an
unqualified token; `Base` may be qualified. The block defines a helper with
`Self` and `Builder` aliases and a hidden-friend `VegaReflect(TypeTag<Type>)`
function. Its braces form the body of `Describe(Builder& type)`.

| Macro | Meaning |
| --- | --- |
| `VEGA_CLASS(Type)` | Declare the hidden friend used to connect a class to its reflection block, and let that block bind private members. Put it first in the class body. |
| `VEGA_REFLECT(Type, Base)` | Describe a class and its registered base. |
| `VEGA_REFLECT_ROOT(Type)` | Describe a class with no reflection base. |
| `VEGA_STRUCT(Type)` | Mark a plain struct as reflected, so it can be a property value or list element. Put it first in the struct body. See [Structs](#structs). |
| `VEGA_REFLECT_STRUCT(Type)` | Describe a struct; its fields use `VEGA_FIELD`. Register the struct before its users. |
| `VEGA_REGISTER(Type)` | Register a type through argument-dependent lookup (ADL). Include its header and register bases first. Explicit references retain static-library objects that the linker could otherwise discard. |
| `VEGA_PROPERTY("Name", Getter, Setter)` | Call `o.Getter()` and `o.Setter(v)` through lambdas; overloaded/default-argument setters work normally. |
| `VEGA_READONLY("Name", Getter)` | Register a getter without a setter. |
| `VEGA_ACTOR_REF("Target", Getter, Setter)` | Register an `ly::ActorRef`. Append `.RequiredType("Door")` to filter types, including subclasses. |
| `VEGA_LIST("Patrol", Getter, Setter)` | Register a `std::vector<T>`. Element metadata is inferred; enum lists support `.Labels(...)`. |
| `VEGA_FIELD("Name", Member)` | Bind a data member directly; no getter or setter is written. Chain `.Range(...)` to clamp numeric values. |
| `VEGA_FIELD_NOTIFY("Name", Member, OnChanged)` | As `VEGA_FIELD`, and call the member function `OnChanged` after each stored write. |
| `VEGA_FIELD_READONLY("Name", Member)` | Bind a data member with no setter. |
| `VEGA_CATEGORY("Group")` | Set the default category for subsequent properties, including direct `type.Property` calls. |
| `VEGA_SPAWNABLE()` | Use the `World*` constructor through `World::SpawnActor<Self>()`. |
| `VEGA_SPAWNABLE(factory)` | Supply a `shared_ptr<Actor>(World&)` factory for constructor arguments. |
| `VEGA_ADDABLE(args...)` | Call `actor.AddComponent<Self>(args...)`; no args uses the owner-only constructor. Addable components are removable by default. |
| `VEGA_REMOVABLE()` | Permit component removal without making it addable. |
| `VEGA_REQUIRES(ComponentName)` | Declare an inherited actor requirement with the stringified token. |

Property expressions return the builder: append `.Range(min, max)`, `.Speed(v)`,
`.Tooltip(text)`, `.Labels({ { value, "Label" } })`, `.Flags(labels)`, `.Category(text)`,
 or `.Asset(kind)` for string asset paths (see [asset paths](#asset-paths)). An explicit category overrides the default for that property
without changing following properties. A new default affects new properties
only. Use `type.Property(...)` for custom lambdas, such as texture loading with
error handling, and the fluent builder for policies such as
`type.Removable(false)`.

## Fields

`VEGA_FIELD(Name, Member)` binds a data member of the described class, private
members included, so plain data needs no getter or setter pair. Use it for flags,
sizes, labels, asset names and lists. Use `VEGA_PROPERTY` when the setter must
enforce a rule or the value is computed, such as normalising input or refusing a
change. `VEGA_FIELD_READONLY` exposes data without a setter. Members must be
declared by the described class; a base-class member needs a property with its
own accessors.

`VEGA_FIELD_NOTIFY(Name, Member, OnChanged)` calls a member function after each
stored write, as Unreal's PostEditChangeProperty does. Use it for side effects
such as reloading a texture or rebuilding a shape. The hook runs once per
successful write, after the new value is stored; refused writes do not run it.
`type.Field(Name, &Self::Member, hook)` also accepts a callable invocable as
`hook(Self&)`.

Range clamps fields: an Int or Float field with `.Range(min, max)` stores the
clamped value on every write from the Inspector, scene loads and Lua. The bounds
are read at write time, so `.Range` may follow the field. Vectors and
`VEGA_PROPERTY` setters are unaffected.

## Asset paths

A String property that holds an asset-root-relative path declares its kind with
`.Asset(kind)`. It works after `VEGA_PROPERTY`, `VEGA_FIELD` and `VEGA_READONLY`,
and after `type.Property` or `type.Field`:

```cpp
VEGA_FIELD("Icon", mIconPath).Asset(AssetKind::Texture);
VEGA_PROPERTY("Sound", GetSound, SetSound).Asset(AssetKind::Sound);
```

`AssetKind` lives in `ly::reflect` (`framework/reflection/AssetKind.h`). The value
stays a plain `std::string` with `/` separators, so scenes, prefabs, Lua and
serialization do not change. Kinds and the names they accept (case-insensitive;
prefab and scene need the compound suffix, so `level.json` is not a scene):

| Kind | Accepts |
| --- | --- |
| `Texture` | `.png` `.jpg` `.jpeg` `.bmp` `.tga` |
| `Sound` | `.wav` `.ogg` `.flac` `.mp3` |
| `Font` | `.ttf` `.otf` |
| `Prefab` | `*.prefab.json` |
| `Scene` | `*.scene.json` |
| `Script` | `.lua` |
| `Any` | any file name |

`AcceptsAsset(kind, path)` checks only the name; it does not touch the disk.
`.Asset()` is checked at registration: it throws `std::logic_error` on a
non-string property and for `AssetKind::None`. The registry golden records
`"asset"` only on asset-typed properties.

The editor Inspector draws these as a picker over the accepted files under the
asset root (searchable), a drop target for Content panel files, a Clear button, and
a `Missing asset` warning when the file does not exist. Texture values show a small
thumbnail. Dropping a file of another kind, a folder or a path outside the asset
root is refused with a message and leaves the value unchanged. Each accepted change
is one undo step. Read-only asset properties show the path only. Moving or
renaming an asset in the Content panel rewrites the matching strings in scenes and
prefabs like any other reference.

Engine properties tagged this way: SpriteComponent `Texture` (`Texture`) and
ScriptComponent `Script` (`Script`, read-only).

## Add a new class

Add a header and implementation to the game. CMake discovers game sources;
engine sources must also be listed in its `CMakeLists.txt`. Use constructor
components for default composition, as with Unreal's `CreateDefaultSubobject`:
`AddComponent` supplies ownership and lifecycle. Components here have no
independent transform hierarchy.

The reflection setup has three steps: add `VEGA_CLASS` to the header, describe
the type with `VEGA_REFLECT` in its `.cpp`, then add one `VEGA_REGISTER` line to
the module registration function. `VEGA_CLASS` is the small class hook analogous
to Unreal's `GENERATED_BODY()`, while the `VEGA_REFLECT` block declares Vega's
property metadata, playing a role similar to `UPROPERTY` declarations.

```cpp
// include/level/ExampleBeacon.h
#pragma once
#include <framework/Actor.h>

namespace ly
{
	class ExampleBeacon : public Actor
	{
		VEGA_CLASS(ExampleBeacon)
	public:
		explicit ExampleBeacon(World* world);
		bool IsLit() const { return mBrightness > 0.f; }
	private:
		float mBrightness = 0.5f;
	};
}
```

```cpp
// src/level/ExampleBeacon.cpp
#include "level/ExampleBeacon.h"
#include <framework/SpriteComponent.h>
#include <framework/reflection/Reflection.h>

namespace ly
{
	ExampleBeacon::ExampleBeacon(World* world) : Actor{ world }
	{
		AddComponent<SpriteComponent>();
	}

	VEGA_REFLECT(ExampleBeacon, Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_REQUIRES(SpriteComponent);
		VEGA_CATEGORY("Beacon");
		VEGA_FIELD("Brightness", mBrightness).Range(0.f, 1.f).Speed(0.01f);
		VEGA_READONLY("Lit", IsLit);
	}
}
```

Add `VEGA_REGISTER(ExampleBeacon);` to `ly::RegisterGameTypes()` after Actor has
been registered by `reflect::RegisterEngineTypes()`. Engine types are registered
directly in `ly::reflect::RegisterEngineTypes()`; ScriptComponent follows the
same pattern in `vega::script`. Keep module guards and custom scene
spawn/component serializers in their module entry points.

The registration file must include the class header: ADL on `TypeTag<Type>`
finds the hidden friend across translation units and avoids MSVC binding a
block-scope declaration to global scope.

The Hierarchy Create menu now includes ExampleBeacon. The Inspector shows
inherited Actor fields, a Beacon category with editable Brightness and read-only
Lit, and the SpriteComponent section with inherited Active and sprite fields.
Required SpriteComponent removal is disabled. Editing Brightness writes its field
clamped to Range, just as serialization and Lua do.

A sparse scene input can contain:

```json
{
	"format": "vega.scene",
	"version": 1,
	"settings": {},
	"entities": [{
		"guid": "a0000000000000000000000000000001",
		"name": "Gate beacon",
		"type": "ExampleBeacon",
		"properties": { "Location": [200, 300], "Brightness": 0.8 },
		"components": [{ "type": "SpriteComponent", "properties": { "Scale": [2, 2] } }]
	}]
}
```

Saving emits complete writable properties for plain entities. Lit is omitted
because it has no setter. Vectors use arrays, colors use `[r,g,b,a]`, enums use
label strings, and flags use arrays of bit labels. Components without `added`
address constructor instances by type occurrence. See [scene authoring](scene-authoring.md)
for composition changes and custom serializers.

A prefab can use `"base": { "type": "ExampleBeacon" }` and
`"properties": { "Brightness": 0.8 }`. A scene instance referencing the prefab
can override Brightness with `"properties": { "Brightness": 0.2 }`. The same
keys and setters apply after constructor defaults, parent prefabs, the prefab,
and instance overrides are resolved. Unchanged values inherit the baseline;
Inspector overrides appear blue and support Apply/Revert. Location belongs to
the instance. See [prefabs](prefabs.md) for full document and membership examples.

Lua attached to the beacon accesses these same registered names:

```lua
function on_create(self)
	local brightness = self.actor:get("Brightness")
	self.actor:set("Brightness", brightness + 0.1)
	local sprite = self.actor:component("SpriteComponent")
	if sprite then
		sprite:set("Scale", { x = 2, y = 2 })
	end
end
```

`actor:component(...)` returns the first matching component, including a match
through a reflected base. Handles resolve the actor and component again on
access. Unknown reads return nil; writes return success and an error string,
including read-only or incompatible-value errors. Reflection always writes through the
registered setter or field. Lua enum/flag values use integers; scene JSON uses labels.

## Actor references and lists

`ly::ActorRef` persists a scene GUID and caches a weak actor pointer. It never
owns its target. `ref.Resolve(world)` returns a live actor for the duration of a
call, or null for an empty, destroyed or missing target. Construct from `Actor&`
to capture scene identity, or from a GUID string for authored data. Failed lookups
are cached by the world's actor reference revision. Code directly changing a
public scene GUID must call `world.InvalidateActorReferences()`.

```cpp
ly::ActorRef GetTarget() const { return mTarget; }
void SetTarget(const ly::ActorRef& value) { mTarget = value; }
const std::vector<sf::Vector2f>& GetPatrol() const { return mPatrol; }
void SetPatrol(const std::vector<sf::Vector2f>& value) { mPatrol = value; }
// Inside VEGA_REFLECT:
VEGA_ACTOR_REF("Target", GetTarget, SetTarget).RequiredType("BlankPlayer");
VEGA_LIST("Patrol", GetPatrol, SetPatrol).Speed(1.f);
```

`PropertyKind::ActorRef`, `PropertyKind::List` and `PropertyKind::Struct` extend the existing kind IDs
without changing them. `PropertyList` holds `ScalarValue` elements, which are scalars or structs.
Supported vector elements are bool, int, float, Vector2f, Color, string, enum and
ActorRef. Lists are homogeneous, limited to 4,096 elements, and validated in full
before calling the setter. Wrong kinds, non-finite numbers and unlabelled enum
values refuse the complete edit. `.RequiredType(...)` also filters ActorRef lists.
Load resolves references after all entities exist; dangling GUIDs warn and survive
saving, while runtime reads remain null-safe.

The Inspector offers searchable, type-filtered choices, Pick in viewport (Esc
cancels), and Select target. Selected actors show dashed reference lines. Lists
have Add, Remove, Up and Down controls. Structural edits use one undo command;
value drags and Vec2 viewport point drags merge per gesture. Mixed lists show a
single em dash. Vec2 points use world coordinates. Property/overlay caches refresh
on scene/selection revisions, without per-frame JSON. Viewport handles operate
in scene Edit mode; prefab isolation offers the picker dropdown. Struct values and
lists of structs show a read-only summary until the struct editor lands; see [Structs](#structs).

```json
{"Target":{"guid":"door-guid"},"Patrol":[[100,200],[300,200]],"Spawn List":[{"guid":"enemy-a"},{"guid":""}]}
```

The registry golden now records `elementKind` (null on scalar properties) and
`requiredType` (empty when unrestricted). Asset-typed properties also record
`"asset"` (for example `"Texture"`); other entries have no such key. Existing order, kind IDs, labels and
other metadata remain identical.

## Structs

A plain struct is described once and then used as a property value, alone or as
`std::vector` elements, as Unreal's `USTRUCT` is used in a `UPROPERTY`. Mark it with
`VEGA_STRUCT(Type)` first in its body, describe it with `VEGA_REFLECT_STRUCT(Type)`, and
register it before any class that uses it:

```cpp
// include/level/WaveEntry.h
struct WaveEntry
{
	VEGA_STRUCT(WaveEntry)
	std::string prefab;
	int count = 1;
	float delay = 0.f;
};

// src/level/WaveEntry.cpp
VEGA_REFLECT_STRUCT(WaveEntry)
{
	VEGA_FIELD("Prefab", prefab).Asset(ly::reflect::AssetKind::Prefab);
	VEGA_FIELD("Count", count).Range(1.f, 50.f);
	VEGA_FIELD("Delay", delay).Range(0.f, 60.f);
}

// Its actor's VEGA_REFLECT block; WaveEntry is registered first, like a base.
VEGA_FIELD("Waves", mWaves);   // std::vector<WaveEntry> mWaves
```

`VEGA_STRUCT` is the struct form of `VEGA_CLASS`. It declares the hidden friend that
`Class` finds through ADL to recognise a struct, so a plain struct without it is refused
as a property. `VEGA_REFLECT_STRUCT` is `VEGA_REFLECT_ROOT`. Fields use the same
`VEGA_FIELD` lines, so `Range`, enum labels, asset kinds and `RequiredType` apply inside a
struct. A class that uses a struct which is not registered yet throws `std::logic_error` at
registration, as an unregistered base does.

**Scene and prefab JSON.** A struct is an object keyed by reflected field names, and a list
of structs is an array of those objects. Each field keeps its normal leaf encoding, so an
enum field is a label and an actor field is `{"guid": ...}`:

```json
{ "Boss": { "Prefab": "enemy.prefab.json", "Count": 3, "Delay": 1.5 },
  "Waves": [ { "Prefab": "a.prefab.json", "Count": 2, "Delay": 0.5 }, { "Prefab": "", "Count": 7, "Delay": 0.0 } ] }
```

Loading ignores unknown keys and keeps the struct default for missing keys. Saving writes
only writable fields. A struct property and a list property are each one prefab override
unit: changing any field of a struct overrides the whole struct, and changing any element
overrides the whole list, as lists already do.

**Lua.** `actor:get("Boss")` returns a table such as `{ Prefab = "...", Count = 3, Delay = 1.5 }`.
`actor:get("Waves")` returns the list table `{ n = 2, [1] = {...}, [2] = {...} }`, where each
element is a struct table. `actor:set` accepts the same shapes. Missing keys keep their
defaults. A value of the wrong type makes `set` return `false` with an error, as for any
other property.

**Limits in v1.** Fields are leaves only: bool, int, float, Vector2f, Color, string, an enum
as int, or ActorRef. A struct holding a struct or a list is refused at registration. A struct
value holds writable fields only. An ActorRef field keeps its `RequiredType` check when
values are written and its dangling-reference warning on load. The viewport draws reference
lines only for top-level references and list elements. The Inspector shows a summary line
(`Boss: WaveEntry {3 fields}`, `Waves: 4 × WaveEntry`) until the struct editor lands.

The registry golden writes `structType` only on struct-typed entries. A struct property
has kind `Struct` (10), and its `elementKind` is `Struct` for a list of structs.

## Compatibility checks

`LightYearsReflectionMacros` tests setter dispatch, metadata, factories and
inherited requirements. `LightYearsReflectionRegistryGolden` compares the full
engine, game and scripting descriptors, in registration order, against
`LightYearsEngine/tests/reflection-registry.golden.json`, captured from the
fluent registrations before conversion. It includes local property order,
setter availability and all authoring metadata. Intentional schema changes
require reviewing the golden change along with asset compatibility.

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
Vector2f, Color, string, ActorRef, or PropertyList; enums and flags travel as int. `SetValue` checks
the value kind, finite numbers, declared enum labels/flag bits, and catches
setter exceptions. Read-only properties have no setter. Ranges describe UI
controls; setters enforce domain rules, including clamping.

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
| `VEGA_CLASS(Type)` | Declare the hidden friend used to connect a class to its reflection block. Put it first in the class body. |
| `VEGA_REFLECT(Type, Base)` | Describe a class and its registered base. |
| `VEGA_REFLECT_ROOT(Type)` | Describe a class with no reflection base. |
| `VEGA_REGISTER(Type)` | Register a type through argument-dependent lookup (ADL). Include its header and register bases first. Explicit references retain static-library objects that the linker could otherwise discard. |
| `VEGA_PROPERTY("Name", Getter, Setter)` | Call `o.Getter()` and `o.Setter(v)` through lambdas; overloaded/default-argument setters work normally. |
| `VEGA_READONLY("Name", Getter)` | Register a getter without a setter. |
| `VEGA_ACTOR_REF("Target", Getter, Setter)` | Register an `ly::ActorRef`. Append `.RequiredType("Door")` to filter types, including subclasses. |
| `VEGA_LIST("Patrol", Getter, Setter)` | Register a `std::vector<T>`. Element metadata is inferred; enum lists support `.Labels(...)`. |
| `VEGA_CATEGORY("Group")` | Set the default category for subsequent properties, including direct `type.Property` calls. |
| `VEGA_SPAWNABLE()` | Use the `World*` constructor through `World::SpawnActor<Self>()`. |
| `VEGA_SPAWNABLE(factory)` | Supply a `shared_ptr<Actor>(World&)` factory for constructor arguments. |
| `VEGA_ADDABLE(args...)` | Call `actor.AddComponent<Self>(args...)`; no args uses the owner-only constructor. Addable components are removable by default. |
| `VEGA_REMOVABLE()` | Permit component removal without making it addable. |
| `VEGA_REQUIRES(ComponentName)` | Declare an inherited actor requirement with the stringified token. |

Property expressions return the builder: append `.Range(min, max)`, `.Speed(v)`,
`.Tooltip(text)`, `.Labels({ { value, "Label" } })`, `.Flags(labels)`, or
`.Category(text)`. An explicit category overrides the default for that property
without changing following properties. A new default affects new properties
only. Use `type.Property(...)` for custom lambdas, such as texture loading with
error handling, and the fluent builder for policies such as
`type.Removable(false)`.

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
#include <algorithm>

namespace ly
{
	class ExampleBeacon : public Actor
	{
		VEGA_CLASS(ExampleBeacon)
	public:
		explicit ExampleBeacon(World* world);
		float GetBrightness() const { return mBrightness; }
		void SetBrightness(float value) { mBrightness = std::clamp(value, 0.f, 1.f); }
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
		VEGA_PROPERTY("Brightness", GetBrightness, SetBrightness).Range(0.f, 1.f).Speed(0.01f);
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
Required SpriteComponent removal is disabled. Editing Brightness calls its
setter, just as serialization and Lua do.

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
including read-only or incompatible-value errors. Reflection always uses the
setter. Lua enum/flag values use integers; scene JSON uses labels.

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

`PropertyKind::ActorRef` and `PropertyKind::List` extend the existing kind IDs
without changing them. `PropertyList` holds non-list `ScalarValue` elements.
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
in scene Edit mode; prefab isolation offers the picker dropdown. Plain reflected
structs and nested lists are deferred to keep the variant/persistence schema small.

```json
{"Target":{"guid":"door-guid"},"Patrol":[[100,200],[300,200]],"Spawn List":[{"guid":"enemy-a"},{"guid":""}]}
```

The registry golden now records `elementKind` (null on scalar properties) and
`requiredType` (empty when unrestricted). Existing order, kind IDs, labels and
other metadata remain identical.

## Compatibility checks

`LightYearsReflectionMacros` tests setter dispatch, metadata, factories and
inherited requirements. `LightYearsReflectionRegistryGolden` compares the full
engine, game and scripting descriptors, in registration order, against
`LightYearsEngine/tests/reflection-registry.golden.json`, captured from the
fluent registrations before conversion. It includes local property order,
setter availability and all authoring metadata. Intentional schema changes
require reviewing the golden change along with asset compatibility.

# Actor hierarchy

`child.AttachTo(&parent, keepWorld)` defaults to keeping world values.
`GetParent()` and `GetChildren()` expose immediate links; World owns the actors.
Self, descendant, cross-world and destroyed parents are refused. `Detach()` keeps
world values. Destroying a parent destroys its subtree. Detach children first
when they should survive. Components still have no transforms.

There is no actor scale. SpriteComponent scale affects that sprite, not children.
Location uses ordinary XY rotation, positive degrees clockwise on screen:

```
worldLocation = parent.worldLocation + rotate(relativeLocation, parent.worldRotation)
worldRotation = parent.worldRotation + relativeRotation
```

Unparented relative values equal world values. World setters invert the parent
transform. Relative setters compose the equations above. This XY convention is
independent of Actor's legacy forward/right movement helpers (forward is up).
World getters remain constant-time. Transform edits synchronously update descendant
sprites, physical transforms and spatial bounds before returning. There is no
per-frame hierarchy traversal or scratch allocation. Transform edits cost O(subtree).
Tick order remains world actor order; render and physics see all completed edits.

Dynamic physics actors cannot attach: AttachTo returns false and logs a warning.
Use Static or disable physics first. Attached Static bodies use kinematic Box2D
bodies and follow the hierarchy; their authored type stays Static. Enabling dynamic
physics or changing an attached body to Dynamic detaches it, keeping world values.
Detaching restores the authored body type. No Box2D-driven movement is composed
back into attachment transforms.

## Scene data

Version 1 accepts an optional parent GUID. Location and Rotation are relative
when parented; old unparented scenes retain world values. Loading creates actors
before connecting links, so forward references work. Missing, cyclic or incompatible
links warn and leave the refused actor unparented. Saving orders parents first.

```json
{"guid":"root","type":"MyActor","properties":{"Location":[100,200],"Rotation":90}}
{"guid":"child","type":"MyActor","parent":"root","properties":{"Location":[10,0],"Rotation":15}}
```

The child has world Location [100,210] and Rotation 105. Nested children arrays
are accepted for subtree snapshots and prefab instances. Each child has an id,
stable among siblings; nesting scopes IDs. Scene GUIDs are independent of prefab
child IDs. Duplicate/paste remap subtree GUIDs and internal links together.
Serialized hierarchies are limited to 128 nesting levels.

## Prefab data

Version 1 remains compatible. Optional children use scene entity fields and stable
id values. A child may reference another prefab:

```json
{
  "format":"vega.prefab", "version":1, "name":"Assembly",
  "base":{"type":"MyActor"},
  "children":[
    {"id":"arm","name":"Arm","type":"MyActor","properties":{"Location":[10,0]}},
    {"id":"lamp","prefab":"prefabs/Lamp.prefab.json","properties":{"Location":[0,20]}}
  ]
}
```

Variants and nested children share cycle detection and a 64-asset stack limit.
Repeated references in separate branches are allowed. Child IDs must be nonempty,
unique among siblings, and contain no slash. An instance override is addressed by ID:

```json
{
  "guid":"instance", "name":"Assembly One", "prefab":"prefabs/Assembly.prefab.json",
  "properties":{"Location":[400,200]},
  "children":[{"id":"arm","properties":{"Rotation":30}}]
}
```

Children are live actors saved inside root overrides, never duplicate top-level
entities. Overrides merge by ID, independent of array order; nested arrays address
grandchildren. deleted:true removes a child. replaceChildren:true plus children
replaces membership. Child GUIDs and display names survive save/refresh/undo.
Creating a prefab captures the selected subtree. Child property/section revert and
apply address that child. Apply refreshes other instances while retaining overrides.
As with existing prefabs, undo restores scene instances, not asset file writes.

## Editor and Lua

Hierarchy rows are a collapsible tree. Drag onto a row to parent while keeping world
values. Alt-drop onto a row or the Detach context action unparents. The Inspector
shows Parent and a Relative Location / Rotation toggle. Gizmos move/rotate subtrees.
Selecting ancestor and descendant transforms the selected ancestor once. Delete,
duplicate and copy/paste include subtrees; each gesture is one undo step.

Reparenting an inherited prefab child makes it an added subtree at its new
address, keeping its values and world transform. The source instance records
removal. Undo restores its previous child IDs and inheritance. Apply the root's
structural changes before using per-child prefab Apply/Revert on an added child.

Lua exposes actor:parent(), actor:children(), actor:attach_to(other[, keep_world])
and actor:detach(). parent returns nil without a live parent; children allocates a
table of immediate live handles. attach_to returns success and defaults to keeping
world values. world.spawn of a prefab realizes the hierarchy and returns its root.

Script commands: parent <Child> <Parent>, unparent <Child>,
expect-parent <Child> <Parent>|none, expect-world-location <Target> x y [tolerance].
Targets resolve by name or GUID. VegaEditorHierarchy runs the hierarchy.vegascript
fixture in BlankEditor from build/VegaEditor against a disposable Blank asset copy.

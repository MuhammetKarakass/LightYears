# Editor script commands

Generated from the command registry. Arguments accept quoted strings; quotes preserve spaces, hashes and empty strings. JSON arguments preserve their original spelling. `#` starts a comment outside quotes. Optional arguments are in brackets; `...` repeats an argument. `text` is nonempty; `string` and `value` may be empty. Numeric arguments are finite; `uint` is an unsigned integer. Mode requirements are checked by the handler to preserve command-specific diagnostics.

## `add-component type:text`

Add a reflected component to the selected actor. Mode: Any.

## `add-script path:text`

Attach and prepare a Lua script on the selected actor in Edit mode. Mode: Edit.

## `apply-prefab-prop target:text property:text`

Apply an instance property to its prefab. Mode: Any.

## `asset-delete path:text confirmation:confirm`

Delete an unreferenced asset after confirmation. Mode: Edit.

## `asset-move source:text destination:text`

Move an asset and update references. Mode: Edit.

## `asset-rename path:text name:text`

Rename an asset and update references. Mode: Edit.

## `autosave-now`

Write a recovery snapshot of the active scene. Mode: Edit.

## `camera camera:editor|game`

Choose the editor or game camera. Mode: Any.

## `capture path:string`

Capture the complete editor window to an image file. Mode: Any.

## `capture-profile name:text`

Save captured profiler frames outside asset roots. Mode: Any.

## `capture-sky path:text`

Capture the sky preview to an image file. Mode: Edit.

## `clear-console`

Clear captured Console messages. Mode: Any.

## `close-prefab`

Close the active prefab. Mode: Edit.

## `colliders enabled:on|off`

Show or hide collider overlays. Mode: Any.

## `content-new type:text template:text id:text`

Create a content definition from a named template. Mode: Any.

## `content-open type:text id:text`

Open a content definition by type and ID. Mode: Any.

## `content-reload`

Reload content catalogs from the active asset root. Mode: Any.

## `content-rename type:text id:text new-id:text`

Rename a content definition and update references. Mode: Any.

## `content-save`

Save edited content catalogs. Mode: Any.

## `content-set path:text value:json`

Set a content field to a JSON value. Mode: Any.

## `content-validate`

Validate content definitions and their references. Mode: Any.

## `content-view view:grid|list`

Choose the Content panel grid or list view. Mode: Any.

## `copy`

Copy the selection. Mode: Any.

## `create type:text`

Create a reflected entity by type. Mode: Edit.

## `create-many type:text count:uint`

Create up to 10000 entities in a grid around the world view center. Mode: Any.

## `create-prefab path:text`

Save the selected entity as a prefab. Mode: Any.

## `create-variant parent:text path:text`

Create a prefab variant from its parent. Mode: Any.

## `delete`

Delete the selected entities. Mode: Any.

## `diagnostics visible:on|off`

Show or hide Console and Profiler panels. Mode: Any.

## `drop-asset target:text property:text path:text`

Assign an asset path to an asset property, as an Inspector drop does. Mode: Edit.

## `duplicate`

Duplicate the selected entity. Mode: Any.

## `expect-ability id:text state:ready|cooldown|active`

Assert an ability state on the debugger target. Mode: Play.

## `expect-asset path:text exists:true|false`

Assert whether an asset exists. Mode: Any.

## `expect-attribute id:text comparison:==|<|>|<=|>= value:real`

Compare a debugger attribute value with a finite number. Mode: Play.

## `expect-autosave path:text [exists:true|false]`

Assert whether a scene has an autosave snapshot (defaults to true). Mode: Any.

## `expect-backdrop id:text`

Assert the game-provided scene backdrop state. Mode: Any.

## `expect-camera camera:editor|game`

Assert the active camera kind. Mode: Any.

## `expect-component type:text presence:present|absent`

Assert whether the selected actor has a component of a type. Mode: Any.

## `expect-console level:Info|Warning|Error message:text`

Assert that Console contains a message at the requested severity. Mode: Any.

## `expect-content type:text id:text path:text value:json`

Assert a field in a content definition. Mode: Any.

## `expect-count type:string count:uint`

Assert the number of live actors of a type. Mode: Any.

## `expect-effect id:text exists:true|false`

Assert whether the debugger target has an effect. Mode: Play.

## `expect-entity-count count:uint`

Assert the number of authored entities. Mode: Any.

## `expect-export-ok`

Assert the previous export and its smoke test succeeded. Mode: Any.

## `expect-json pointer:string value:json`

Assert the value and scalar type at a JSON pointer. Mode: Any.

## `expect-json-dirty dirty:true|false`

Assert whether the open JSON document has unsaved changes. Mode: Any.

## `expect-list-size target:text property:text size:uint`

Assert the number of elements in a reflected list. Mode: Any.

## `expect-location x:real y:real [tolerance:real]`

Assert the selected actor location with optional nonnegative tolerance. Mode: Any.

## `expect-mode mode:edit|play`

Assert whether the editor is in Edit or Play mode. Mode: Any.

## `expect-mute-audio muted:true|false`

Assert the saved editor audio mute preference. Mode: Any.

## `expect-no-errors`

Assert that Console has no error or fatal messages. Mode: Any.

## `expect-overridden target:text property:text overridden:true|false`

Assert whether an instance property overrides its prefab. Mode: Any.

## `expect-parent child:text parent:text`

Assert an actor parent; none means no parent. Mode: Any.

## `expect-player-ship ship:text`

Assert the player ship in the live Play world. Mode: Play.

## `expect-prefab path:text`

Assert the selected entity prefab path. Mode: Any.

## `expect-profile-scope name:text`

Assert that a captured profiler frame contains a scope. Mode: Any.

## `expect-project-startup path:string`

Assert the project startup scene path. Mode: Any.

## `expect-prop target:text property:text value:value...`

Assert a reflected property, optionally followed by nonnegative tolerance. Mode: Any.

## `expect-prop-error message:text`

Consume an immediately preceding failed property write containing the supplied message. Mode: Any.

## `expect-recovery path:text [exists:true|false]`

Assert whether a scene is offered for recovery (defaults to true). Mode: Any.

## `expect-ref target:text property:text actor:text`

Assert an actor reference by name or GUID; none means no actor. Mode: Any.

## `expect-remembered-location`

Assert the selected actor matches its remembered location. Mode: Any.

## `expect-rotation-changed`

Assert that the selected actor rotation changed from its remembered value. Mode: Any.

## `expect-scene-dirty dirty:true|false`

Assert whether the scene has unsaved changes. Mode: Any.

## `expect-script-error message:text`

Assert a script error contains a message; none means no error. Mode: Any.

## `expect-script-prop property:text value:value...`

Assert a script property, optionally followed by nonnegative tolerance. Mode: Any.

## `expect-selected type:string`

Assert the selected actor type. Mode: Any.

## `expect-selection-count count:uint`

Assert the number of selected actors. Mode: Any.

## `expect-setting key:text value:json`

Assert an effective scene setting value. Mode: Any.

## `expect-sky path:string value:json`

Assert a sky field value. Mode: Edit.

## `expect-tag id:text exists:true|false`

Assert whether the debugger target owns a tag. Mode: Play.

## `expect-visible`

Assert the selected actor intersects the current world view. Mode: Any.

## `expect-world-location actor:text x:real y:real [tolerance:real]`

Assert an actor world location with optional nonnegative tolerance. Mode: Any.

## `export path:text [confirmation:overwrite]`

Export the game asynchronously and run its smoke test. Mode: Any.

## `focus`

Focus the editor camera on the selection. Mode: Any.

## `gas-activate id:text`

Activate a non-passive ability on the debugger target. Mode: Play.

## `gas-apply-effect id:text`

Apply a registered effect to the debugger target. Mode: Play.

## `gas-filter section:abilities|effects|attributes|tags|events filter:string`

Set a debugger section filter; an empty string clears it. Mode: Any.

## `gas-panel visible:on|off`

Show or hide the ability system debugger. Mode: Any.

## `gas-remove-effect id:text`

Remove matching effects from the debugger target. Mode: Play.

## `gas-reset-cooldowns`

Clear ability cooldowns on the debugger target. Mode: Play.

## `gas-select-player`

Select the player through the installed GAS resolver. Mode: Play.

## `grid enabled:on|off`

Show or hide the viewport grid. Mode: Any.

## `help [command:text]`

List registered commands or describe one command. Mode: Any.

## `json-revert`

Discard edits and reload the open JSON document. Mode: Any.

## `json-save`

Save the open JSON document. Mode: Any.

## `json-set pointer:string value:json`

Set a JSON pointer to a non-null scalar value. Mode: Any.

## `list-add target:text property:text value:value...`

Append a reflected list element using its descriptor type. Mode: Any.

## `list-remove target:text property:text index:uint`

Remove a reflected list element at a zero-based index. Mode: Any.

## `list-set target:text property:text index:uint value:value...`

Set a reflected list element at a zero-based index. Mode: Any.

## `move x:real y:real`

Set the selected actor location. Mode: Any.

## `move-selection x:real y:real`

Translate the selection by the supplied offset. Mode: Any.

## `mute-audio muted:true|false`

Set and persist the editor audio mute preference. Mode: Any.

## `new-scene`

Create a new scene after checking for unsaved changes. Mode: Edit.

## `open-json path:text`

Open a JSON document with no unsaved changes. Mode: Any.

## `open-prefab path:text`

Open a prefab in the prefab editor. Mode: Edit.

## `open-scene path:text`

Open a scene after checking for unsaved changes. Mode: Edit.

## `pan x:real y:real`

Pan the editor camera by a world-coordinate offset. Mode: Any.

## `parent child:text parent:text`

Attach a child actor to a parent by name or GUID. Mode: Any.

## `paste [x:real] [y:real]`

Paste at the supplied position or the world view center; supply both coordinates or neither. Mode: Any.

## `pause`

Pause the current world. Mode: Any.

## `place-prefab path:text [x:real] [y:real]`

Place a prefab at the supplied position or world view center; supply both coordinates or neither. Mode: Any.

## `play`

Unpause the current world. Mode: Any.

## `play-mode`

Start Play using a copy of the edit scene. Mode: Any.

## `profiler visible:on|off`

Enable or disable profiler recording. Mode: Any.

## `quit`

Finish the script and request orderly shutdown. Mode: Any.

## `recover-autosave path:text`

Restore a scene from its recovery snapshot. Mode: Edit.

## `redo`

Redo in the active tool history or scene history. Mode: Any.

## `reload-prefabs`

Reload prefab definitions and refresh instances. Mode: Any.

## `remember-location`

Remember the selected actor location for a later assertion. Mode: Any.

## `remember-rotation`

Remember the selected actor rotation. Mode: Any.

## `remove-component type:text [occurrence:uint]`

Remove a component by type and zero-based occurrence (defaults to zero). Mode: Any.

## `rename name:string...`

Rename the selected entity; unquoted words are joined with spaces. Mode: Any.

## `reset-setting key:text`

Remove an authored scene setting override. Mode: Edit.

## `revert-prefab-prop target:text property:text`

Revert an instance property to its prefab value. Mode: Any.

## `rotate degrees:real`

Set the selected actor rotation in degrees. Mode: Any.

## `save-prefab`

Save the active prefab. Mode: Edit.

## `save-scene [path:text]`

Save the active scene, optionally to a new path. Mode: Edit.

## `select number:uint`

Select an actor by numeric ID. Mode: Any.

## `select-add actor:text`

Add an actor to the selection by name, GUID or numeric ID. Mode: Any.

## `select-all`

Select every live actor. Mode: Any.

## `select-name name:string...`

Select an entity by name; unquoted words are joined with spaces. Mode: Any.

## `select-rect x1:real y1:real x2:real y2:real`

Select actors within a world-coordinate rectangle. Mode: Any.

## `select-remove actor:text`

Remove an actor from the selection by name, GUID or numeric ID. Mode: Any.

## `select-type type:string [index:uint]`

Select the zero-based occurrence of an actor type (defaults to zero). Mode: Any.

## `set-project-startup path:string`

Set the project startup scene path. Mode: Any.

## `set-prop target:text property:text value:value...`

Set a reflected property using its descriptor type. Mode: Any.

## `set-prop-all property:text value:value...`

Set a common reflected property on every selected actor. Mode: Any.

## `set-ref target:text property:text actor:text`

Set an actor reference by name or GUID; use none to clear it. Mode: Any.

## `set-script-prop property:text value:value...`

Set a script property using its declared type. Mode: Any.

## `set-setting key:text value:json`

Set a scene setting to a JSON value; null selects an optional default. Mode: Edit.

## `simulate-unclean-exit`

Leave the autosave session marked unclean for recovery testing. Mode: Edit.

## `sky-drag handle:text x:real y:real`

Move a sky handle by a pixel delta. Mode: Edit.

## `sky-open id:text`

Open a sky by its opaque asset ID. Mode: Edit.

## `sky-save`

Save the authored sky document. Mode: Edit.

## `sky-set path:string value:json`

Set a sky field using a JSON scalar or array. Mode: Edit.

## `snap enabled:on|off`

Enable or disable transform snapping. Mode: Any.

## `stop`

Stop Play and restore the edit scene. Mode: Any.

## `tool tool:move|rotate`

Choose the transform gizmo tool. Mode: Any.

## `undo`

Undo in the active tool history or scene history. Mode: Any.

## `unparent child:text`

Detach an actor while preserving its world transform. Mode: Any.

## `wait number:uint`

Wait the supplied number of completed editor frames. Mode: Any.

## `wait-seconds seconds:real`

Wait a finite duration between zero and 86400 seconds. Mode: Any.

## `zoom factor:real`

Zoom the editor camera by a strictly positive finite factor. Mode: Any.


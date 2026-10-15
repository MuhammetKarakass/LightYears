Run the editor from its executable directory:

```text
VegaEditor.exe --editor-script <file>
```

The runner waits for the first live world, executes one command per completed UI
frame, and exits at EOF or `quit`. It continues after failed lines so the report
can include later checks. Exit code is 0 only when every executed line and report
write succeeded. Results are logged through `LY_GAME_INFO` and written alongside
the script to `<file>.report.txt` as `OK|FAIL <source line> -- <detail>`.

Commands are case-sensitive and whitespace-separated. Blank lines and `#`
comments are ignored. Arguments cannot contain whitespace or `#`. Numbers must
be finite; IDs, indices, counts and frame counts must be non-negative integers.

```text
pause
play
wait <frames>
select <id>
select-type <TypeName> [index=0]
move <x> <y>
rotate <degrees>
undo
redo
expect-selected <TypeName>
expect-location <x> <y> [tolerance=0.5]
expect-count <TypeName> <count>
remember-location
expect-remembered-location
capture <png-path>
quit
```

`select-type` uses the unqualified demangled RTTI type name and the world's live
actor order, excluding pending-destroy actors. `move` and `rotate` use the selected
actor and the same command history as the Inspector. Each script edit is a
separate undo step. `wait N` leaves N complete frames between that command and the
next command. `expect-location` checks each coordinate with the stated tolerance.
`remember-location` stores the selected actor's position for a later
`expect-remembered-location` check (default tolerance 0.5).

Capture paths are relative to the process working directory. The capture copies
the editor window after ImGui renders; no `--visual-capture` option is needed.
Existing game dev options remain available. Script runs still require an SFML
window and graphics context. Actor deletion cannot be undone; UI deletion asks for
confirmation, executes without adding an undo entry, and invalidates redo.

# Galaxy route tools

`build_route.py` builds `LightYearsGame/assets/content/data/galaxy_route.json`, the real-star part of the
campaign route from the Sun toward Sgr A*. The procedural tail to Sgr A* is generated in C++ (`GalaxyRoute`).
Full source, acknowledgement and selection rules: `docs/GALAXY_ROUTE_DATA.md`.

## Regenerate

Python 3 standard library only.

```powershell
& 'C:\Users\dfdfg\AppData\Local\python\bin\python.exe' 'D:\LightYears\tools\galaxy\build_route.py'
```

- Queries the NASA Exoplanet Archive TAP service (`pscomppars`) and caches the raw CSV in `cache/pscomppars.csv`.
- `--offline` reuses the cached CSV (no network).
- `--out <path>` writes the JSON somewhere else.

The cache folder has its own `.gitignore`, so the raw CSV is not committed.

## Output summary

The script prints the stop count, the first ten stops and the farthest real stop. The JSON is commit-ready; the
`generated` field records the run date.

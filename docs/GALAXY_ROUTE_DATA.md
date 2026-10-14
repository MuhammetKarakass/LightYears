# Galaxy route data

The campaign runs from the Sun to the galactic centre (Sgr A*). The first part is real exoplanet host stars
(`galaxy_route.json`); the rest is generated procedurally in C++ from the galaxy model (`GalaxyRoute`).

## Source

- Catalogue: NASA Exoplanet Archive, table `pscomppars` (Planetary Systems Composite Parameters), TAP service
  `https://exoplanetarchive.ipac.caltech.edu/TAP/sync`, queried with CSV output.
- Columns used: `pl_name, hostname, ra, dec, sy_dist, st_teff, st_rad, st_lum, st_mass, st_spectype, sy_pnum,
  pl_orbsmax, pl_rade, pl_bmasse, pl_eqt, discoverymethod`.
- Retrieved by `tools/galaxy/build_route.py` on 2026-10-13: 6445 planet rows for 4842 host names.
- The HYG catalogue and Gaia are not used.

## Licence and acknowledgement

Checked on 2026-10-13 at `https://exoplanetarchive.ipac.caltech.edu/docs/acknowledge.html` (page dated
13 August 2025).

- Required acknowledgement, to be included "in any published material that makes use of the NASA Exoplanet
  Archive's services": "This research has made use of the NASA Exoplanet Archive, which is operated by the
  California Institute of Technology, under contract with the National Aeronautics and Space Administration under
  the Exoplanet Exploration Program."
- Preferred citation: Christiansen et al. (2025), the Archive's paper accepted by the Planetary Science Journal
  (it replaces Akeson et al. 2013).
- Literature references: cite a specific paper directly when using its data.
- Dataset-specific wording applies to WASP, KELT, FDL INARA, CUTE and ROME/REA data. Several of these surveys
  contribute planets to `pscomppars`, so check the relevant section of the page before publishing credits that
  name them.
- The page states no formal licence, data-use policy or redistribution terms. The acknowledgement and citation
  are the only stated requirements. The JSON `acknowledgement` field carries the standard sentence.

## Selection rules

Coordinates are converted from J2000 RA/Dec to galactic (l, b) with the IAU rotation (NGP RA 192.85948 deg,
Dec 27.12825 deg, l of the NCP 122.93192 deg). A frame check requires Sgr A* to map to l ~ 0, b ~ 0.

1. Candidates are hosts with a known `sy_dist`, |l| <= 35 deg and |b| <= 12 deg (toward the centre), and at
   least one of mass, luminosity, radius or temperature.
2. Position along the Sun-centre axis: x = d cos b cos l (pc). Only 0 <= x < 8000 pc is used.
3. Bins along x, edges (pc): 0, 5, 10, 20, 40, 80, 150, 300, 600, 1000, 2000, 3000, 4500, 6000, 8000.
4. In each bin, hosts are ranked by more planets, then a known (not estimated) temperature, then more planets in
   the temperate range 180-320 K, then name. The top three per bin are kept.
5. If more than 35 real hosts remain, the lowest-ranked pick is dropped from the most crowded bin. The route
   therefore has Sol plus 35 real hosts (36 stops).
6. Stops are sorted by x (Sun to centre).

Stop 0 is Sol (T 5772 K, L 1, R 1, M 1, G2V). The planets are Earth (a 1.000 AU, R 1.0 R_E, M 1.0 M_E,
equilibrium 255 K) and Jupiter (a 5.204 AU, R 11.21 R_E, M 317.8 M_E, equilibrium 110 K). These are standard
reference values written in from memory; they were not fetched in this run.

### Estimated star values

Missing star values are filled from main-sequence relations and marked `"estimated": true`:

- M from mass, else luminosity (inverse of L ~ M^4), else radius (R ~ M^1.25), else temperature (T = 5772 K M^0.6).
- L = M^4 for M >= 0.43 Msun, 0.23 M^2.3 below. R = M^0.8. T from Stefan-Boltzmann with the Sun at 5772 K.

The archive's `st_lum` is log10(L/Lsun) and is converted to linear.

## Output

`LightYearsGame/assets/content/data/galaxy_route.json`, version 1: `stops[]` with `id, name, real, distancePc, l, b,
radiusKpc, heightKpc, azimuthRad, star{...}, planets[]`. Galactocentric values use the frame in
`GalaxyModel.h` (Sun at R = 8.2 kpc, z = 0.02 kpc, azimuth 0 on the Sun-centre line). Floats are rounded to 4
significant digits. Unknown planet fields are `null`.

## Regenerate

```powershell
& 'C:\Users\dfdfg\AppData\Local\python\bin\python.exe' 'D:\LightYears\tools\galaxy\build_route.py'
& 'C:\Users\dfdfg\AppData\Local\python\bin\python.exe' 'D:\LightYears\tools\galaxy\build_route.py' --offline
```

The first run needs network access and refreshes `tools/galaxy/cache/pscomppars.csv`. `--offline` reuses that
cache. The data changes when the archive is updated, so commit the regenerated JSON with its `generated` date.

## Procedural tail (C++)

`GalaxyRoute::AppendProceduralStops(count, seed)` continues from the last real stop to Sgr A*. Progress follows
`visual::galaxy::RoutePosition`. Azimuth interpolates linearly to 0. Each stop samples its host temperature from
`ComputeSky(...).stellarMix` using `std::mt19937(seed)`. Mass, luminosity and radius come from the same
main-sequence relations, with mass inverted from temperature. Planets: Poisson count (mean 1.5, at most 8); each
is a giant with probability 0.06 * 10^(2 [Fe/H]), capped at 0.5 (Fischer & Valenti 2005). The final stop is
Sgr A* (22 000 K, 20 Msun, at 0.0001 kpc, no planets).

Tests: `LightYearsGame/tests/GalaxyRouteTests.cpp` (target `LightYearsGalaxyRouteTests`).

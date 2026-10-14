#!/usr/bin/env python3
"""Builds the LightYears campaign route data (real exoplanet hosts, Sun -> galactic centre).

Reads the NASA Exoplanet Archive pscomppars table (TAP, CSV), converts host positions to
galactocentric cylindrical coordinates and writes LightYearsGame/assets/content/data/galaxy_route.json.
Python 3 standard library only. See tools/galaxy/README.md and docs/GALAXY_ROUTE_DATA.md.
"""

import argparse
import bisect
import csv
import datetime
import io
import json
import math
import os
import re
import sys
import urllib.parse
import urllib.request

TAP_URL = "https://exoplanetarchive.ipac.caltech.edu/TAP/sync"
ADQL = (
    "select pl_name,hostname,ra,dec,sy_dist,st_teff,st_rad,st_lum,st_mass,st_spectype,"
    "sy_pnum,pl_orbsmax,pl_rade,pl_bmasse,pl_eqt,discoverymethod from pscomppars"
)
# Standard acknowledgement from https://exoplanetarchive.ipac.caltech.edu/docs/acknowledge.html
ACKNOWLEDGEMENT = (
    "This research has made use of the NASA Exoplanet Archive, which is operated by the California "
    "Institute of Technology, under contract with the National Aeronautics and Space Administration "
    "under the Exoplanet Exploration Program. Preferred citation: Christiansen et al. (2025)."
)
REQUIRED_COLUMNS = ("pl_name", "hostname", "ra", "dec", "sy_dist", "st_teff", "st_rad", "st_lum",
                    "st_mass", "st_spectype", "sy_pnum", "pl_orbsmax", "pl_rade", "pl_bmasse",
                    "pl_eqt", "discoverymethod")

HERE = os.path.dirname(os.path.abspath(__file__))
CACHE_DIR = os.path.join(HERE, "cache")
CACHE_FILE = os.path.join(CACHE_DIR, "pscomppars.csv")
OUT_FILE = os.path.normpath(os.path.join(HERE, "..", "..", "LightYearsGame", "assets", "content",
                                         "data", "galaxy_route.json"))

# Galactocentric frame: galactic centre at the origin, Sun at R0 on the x axis (IAU-style values).
R0_KPC = 8.2
Z0_KPC = 0.02
SUN_TEFF_K = 5772.0

# Candidate selection: toward the centre (|l| <= 35 deg, |b| <= 12 deg).
MAX_ABS_L_DEG = 35.0
MAX_ABS_B_DEG = 12.0
# Bins along x (projected distance toward the centre, pc); edges widen with distance.
BIN_EDGES_PC = [0, 5, 10, 20, 40, 80, 150, 300, 600, 1000, 2000, 3000, 4500, 6000, 8000]
PER_BIN = 3
TARGET_REAL_STOPS = 35  # with Sol the route has 36 stops
TEMPERATE_EQT_K = (180.0, 320.0)
# Allowed radius increase between consecutive stops (projection noise, kpc). Same value as GalaxyRoute.cpp.
RADIUS_ORDER_TOLERANCE_KPC = 0.25

# J2000 -> galactic (IAU 1958 pole and node).
RA_NGP_DEG = 192.85948
DEC_NGP_DEG = 27.12825
L_NCP_DEG = 122.93192

SGR_A_RA_DEG = 266.4168
SGR_A_DEC_DEG = -29.0078


def equatorial_to_galactic(ra_deg, dec_deg):
    """Returns (l, b) in degrees, l wrapped to [-180, 180)."""
    a = math.radians(ra_deg)
    d = math.radians(dec_deg)
    a_ngp = math.radians(RA_NGP_DEG)
    d_ngp = math.radians(DEC_NGP_DEG)
    # sin b = sin d sin d_ngp + cos d cos d_ngp cos(a - a_ngp)
    sin_b = math.sin(d) * math.sin(d_ngp) + math.cos(d) * math.cos(d_ngp) * math.cos(a - a_ngp)
    b = math.asin(max(-1.0, min(1.0, sin_b)))
    # tan(l_ncp - l) = cos d sin(a - a_ngp) / (cos d_ngp sin d - sin d_ngp cos d cos(a - a_ngp))
    y = math.cos(d) * math.sin(a - a_ngp)
    x = math.cos(d_ngp) * math.sin(d) - math.sin(d_ngp) * math.cos(d) * math.cos(a - a_ngp)
    l = math.radians(L_NCP_DEG) - math.atan2(y, x)
    l = (l + math.pi) % (2.0 * math.pi) - math.pi
    return math.degrees(l), math.degrees(b)


def galactocentric(distance_pc, l_deg, b_deg):
    """Heliocentric (d, l, b) -> galactocentric (radius kpc, height kpc, azimuth rad).

    x = d cos b cos l, y = d cos b sin l           (kpc, heliocentric projection, x points to the centre)
    R = sqrt((R0 - x)^2 + y^2)                     (cylindrical radius from the galactic centre)
    azimuth = atan2(y, R0 - x)                     (0 on the Sun-centre line, Sun at 0)
    z = z0 + d sin b                               (height above the plane)
    """
    d = distance_pc / 1000.0
    l = math.radians(l_deg)
    b = math.radians(b_deg)
    x = d * math.cos(b) * math.cos(l)
    y = d * math.cos(b) * math.sin(l)
    radius = math.hypot(R0_KPC - x, y)
    azimuth = math.atan2(y, R0_KPC - x)
    height = Z0_KPC + d * math.sin(b)
    return radius, height, azimuth


def projected_axis_pc(distance_pc, l_deg, b_deg):
    """Distance along the Sun -> centre axis (pc); used to order and bin the stops."""
    return distance_pc * math.cos(math.radians(b_deg)) * math.cos(math.radians(l_deg))


def ms_luminosity(mass):
    # L ~ M^4 for M >= 0.43 Msun, L ~ 0.23 M^2.3 below.
    return mass ** 4 if mass >= 0.43 else 0.23 * mass ** 2.3


def ms_mass_from_luminosity(lum):
    return lum ** 0.25 if lum >= ms_luminosity(0.43) else (lum / 0.23) ** (1.0 / 2.3)


def ms_radius(mass):
    # R ~ M^0.8 on the main sequence.
    return mass ** 0.8


def ms_mass_from_temperature(teff):
    # With L ~ M^4 and R ~ M^0.8, Stefan-Boltzmann gives T = 5772 K * M^0.6.
    return (teff / SUN_TEFF_K) ** (1.0 / 0.6)


def to_float(text, positive=False):
    if text is None:
        return None
    text = str(text).strip()
    if text == "":
        return None
    try:
        value = float(text)
    except ValueError:
        return None
    if not math.isfinite(value):
        return None
    if positive and value <= 0.0:
        return None
    return value


def first_value(rows, key, positive=False):
    for row in rows:
        value = to_float(row.get(key), positive)
        if value is not None:
            return value
    return None


def first_text(rows, key):
    for row in rows:
        text = (row.get(key) or "").strip()
        if text:
            return text
    return None


def host_star(rows):
    """Star block for a host; missing fields are filled from main-sequence relations (estimated = True)."""
    teff = first_value(rows, "st_teff", positive=True)
    lum_log = first_value(rows, "st_lum")  # archive gives log10(L / Lsun)
    rad = first_value(rows, "st_rad", positive=True)
    mass = first_value(rows, "st_mass", positive=True)
    lum = 10.0 ** lum_log if lum_log is not None else None
    if teff is None and lum is None and mass is None and rad is None:
        return None

    estimated = False
    if mass is None:
        estimated = True
        if lum is not None:
            mass = ms_mass_from_luminosity(lum)
        elif rad is not None:
            mass = rad ** 1.25
        else:
            mass = ms_mass_from_temperature(teff)
    if lum is None:
        lum = ms_luminosity(mass)
        estimated = True
    if rad is None:
        rad = ms_radius(mass)
        estimated = True
    if teff is None:
        teff = SUN_TEFF_K * (lum / (rad * rad)) ** 0.25
        estimated = True
    return {
        "temperatureK": teff,
        "luminositySolar": lum,
        "radiusSolar": rad,
        "massSolar": mass,
        "spectralType": first_text(rows, "st_spectype"),
        "estimated": estimated,
    }


def planet_entry(row):
    name = (row.get("pl_name") or "").strip()
    return {
        "name": name,
        "semiMajorAU": to_float(row.get("pl_orbsmax")),
        "radiusEarth": to_float(row.get("pl_rade")),
        "massEarth": to_float(row.get("pl_bmasse")),
        "eqTempK": to_float(row.get("pl_eqt")),
        "method": (row.get("discoverymethod") or "").strip() or None,
    }


def build_candidates(rows_by_host):
    candidates = []
    for host_name, rows in rows_by_host.items():
        ra = first_value(rows, "ra")
        dec = first_value(rows, "dec")
        distance = first_value(rows, "sy_dist", positive=True)
        if ra is None or dec is None or distance is None:
            continue
        l_deg, b_deg = equatorial_to_galactic(ra, dec)
        if abs(l_deg) > MAX_ABS_L_DEG or abs(b_deg) > MAX_ABS_B_DEG:
            continue
        axis_pc = projected_axis_pc(distance, l_deg, b_deg)
        if not (0.0 <= axis_pc < BIN_EDGES_PC[-1]):
            continue
        star = host_star(rows)
        if star is None:
            continue
        planets = [planet_entry(row) for row in rows if (row.get("pl_name") or "").strip()]
        planets = [p for p in planets if p["name"]]
        if not planets:
            continue
        planets.sort(key=lambda p: (p["semiMajorAU"] is None, p["semiMajorAU"] or 0.0, p["name"]))
        temperate = sum(1 for p in planets if p["eqTempK"] is not None
                        and TEMPERATE_EQT_K[0] <= p["eqTempK"] <= TEMPERATE_EQT_K[1])
        candidates.append({
            "name": host_name,
            "distancePc": distance,
            "l": l_deg,
            "b": b_deg,
            "axisPc": axis_pc,
            "star": star,
            "teffReal": first_value(rows, "st_teff", positive=True) is not None,
            "temperate": temperate,
            "planets": planets,
        })
    return candidates


def select_stops(candidates):
    bins = [[] for _ in range(len(BIN_EDGES_PC) - 1)]
    for candidate in candidates:
        index = bisect.bisect_right(BIN_EDGES_PC, candidate["axisPc"]) - 1
        bins[index].append(candidate)
    picks = []
    for group in bins:
        group.sort(key=lambda c: (-len(c["planets"]), not c["teffReal"], -c["temperate"], c["name"]))
        picks.append(group[:PER_BIN])
    # Trim the lowest-ranked pick from the most crowded bin until the target count is reached.
    while sum(len(p) for p in picks) > TARGET_REAL_STOPS:
        index = max(range(len(picks)), key=lambda i: (len(picks[i]), i))
        picks[index].pop()
    chosen = [c for p in picks for c in p]
    return sorted(chosen, key=lambda c: c["axisPc"])


def sig(value, digits=4):
    if value is None:
        return None
    if value == 0:
        return 0.0
    return float(f"{value:.{digits}g}")


def slugify(name, used):
    slug = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-") or "star"
    base = slug
    counter = 2
    while slug in used:
        slug = f"{base}-{counter}"
        counter += 1
    used.add(slug)
    return slug


def sol_stop():
    radius, height, azimuth = galactocentric(0.0, 0.0, 0.0)
    return {
        "id": "sol",
        "name": "Sol",
        "real": True,
        "distancePc": 0.0,
        "l": 0.0,
        "b": 0.0,
        "radiusKpc": sig(radius),
        "heightKpc": sig(height),
        "azimuthRad": sig(azimuth),
        "star": {
            "temperatureK": 5772.0,
            "luminositySolar": 1.0,
            "radiusSolar": 1.0,
            "massSolar": 1.0,
            "spectralType": "G2V",
            "estimated": False,
        },
        "planets": [
            {"name": "Earth", "semiMajorAU": 1.0, "radiusEarth": 1.0, "massEarth": 1.0,
             "eqTempK": 255.0, "method": "Solar System reference"},
            {"name": "Jupiter", "semiMajorAU": 5.204, "radiusEarth": 11.21, "massEarth": 317.8,
             "eqTempK": 110.0, "method": "Solar System reference"},
        ],
    }


def real_stop(candidate, used_ids):
    radius, height, azimuth = galactocentric(candidate["distancePc"], candidate["l"], candidate["b"])
    star = candidate["star"]
    return {
        "id": slugify(candidate["name"], used_ids),
        "name": candidate["name"],
        "real": True,
        "distancePc": sig(candidate["distancePc"]),
        "l": sig(candidate["l"]),
        "b": sig(candidate["b"]),
        "radiusKpc": sig(radius),
        "heightKpc": sig(height),
        "azimuthRad": sig(azimuth),
        "star": {
            "temperatureK": sig(star["temperatureK"]),
            "luminositySolar": sig(star["luminositySolar"]),
            "radiusSolar": sig(star["radiusSolar"]),
            "massSolar": sig(star["massSolar"]),
            "spectralType": star["spectralType"],
            "estimated": star["estimated"],
        },
        "planets": [
            {
                "name": p["name"],
                "semiMajorAU": sig(p["semiMajorAU"]),
                "radiusEarth": sig(p["radiusEarth"]),
                "massEarth": sig(p["massEarth"]),
                "eqTempK": sig(p["eqTempK"]),
                "method": p["method"],
            }
            for p in candidate["planets"]
        ],
    }


def check_frame():
    """Sanity check: Sgr A* must map to l ~ 0, b ~ 0 and the Sun to R0."""
    l_deg, b_deg = equatorial_to_galactic(SGR_A_RA_DEG, SGR_A_DEC_DEG)
    if abs(l_deg) > 0.2 or abs(b_deg) > 0.2:
        raise SystemExit(f"frame check failed: Sgr A* maps to l={l_deg:.3f}, b={b_deg:.3f}")


def fetch_csv():
    query = urllib.parse.urlencode({"query": ADQL, "format": "csv"})
    request = urllib.request.Request(TAP_URL + "?" + query,
                                     headers={"User-Agent": "LightYears-galaxy-route/1.0"})
    with urllib.request.urlopen(request, timeout=300) as response:
        return response.read().decode("utf-8")


def read_rows(text):
    reader = csv.DictReader(io.StringIO(text))
    header = reader.fieldnames or []
    missing = [column for column in REQUIRED_COLUMNS if column not in header]
    if missing:
        raise SystemExit("unexpected archive response (missing columns: " + ", ".join(missing) + ")")
    return list(reader)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--offline", action="store_true", help="reuse tools/galaxy/cache/pscomppars.csv")
    parser.add_argument("--out", default=OUT_FILE, help="output JSON path")
    args = parser.parse_args(argv)

    check_frame()

    if args.offline:
        if not os.path.isfile(CACHE_FILE):
            raise SystemExit(f"--offline requested but {CACHE_FILE} does not exist")
        with open(CACHE_FILE, "r", encoding="utf-8") as handle:
            text = handle.read()
        print(f"using cached archive table {CACHE_FILE}")
    else:
        print("querying NASA Exoplanet Archive pscomppars ...")
        text = fetch_csv()
        os.makedirs(CACHE_DIR, exist_ok=True)
        with open(CACHE_FILE, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        print(f"cached raw CSV at {CACHE_FILE}")

    rows = read_rows(text)
    rows_by_host = {}
    for row in rows:
        host = (row.get("hostname") or "").strip()
        if host:
            rows_by_host.setdefault(host, []).append(row)
    print(f"archive rows: {len(rows)}, hosts: {len(rows_by_host)}")

    candidates = build_candidates(rows_by_host)
    print(f"candidate hosts toward the centre: {len(candidates)}")
    chosen = select_stops(candidates)

    used_ids = {"sol"}
    stops = [sol_stop()] + [real_stop(c, used_ids) for c in chosen]

    previous = None
    for stop in stops:
        if previous is not None and stop["radiusKpc"] > previous + RADIUS_ORDER_TOLERANCE_KPC:
            raise SystemExit(f"stop order violated at {stop['name']}: radius {stop['radiusKpc']} after {previous}")
        previous = stop["radiusKpc"]

    document = {
        "version": 1,
        "generated": datetime.date.today().isoformat(),
        "source": "NASA Exoplanet Archive pscomppars",
        "acknowledgement": ACKNOWLEDGEMENT,
        "stops": stops,
    }
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(document, handle, indent=2, ensure_ascii=False)
        handle.write("\n")

    real_stops = [s for s in stops if s["real"] and s["id"] != "sol"]
    farthest = max(real_stops, key=lambda s: s["distancePc"]) if real_stops else None
    print(f"wrote {args.out}")
    print(f"stops: {len(stops)} (Sol + {len(real_stops)} real hosts)")
    for stop in stops[:10]:
        print(f"  {stop['name']:<28} {stop['distancePc']:>10} pc  R={stop['radiusKpc']} kpc")
    if farthest is not None:
        print(f"farthest real stop: {farthest['name']} at {farthest['distancePc']} pc")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

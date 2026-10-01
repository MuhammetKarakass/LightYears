# Light Years — Ship & Primary Weapon Design Catalog

Bu dosya, yüklenen güncel altı gemi + primary weapon tasarım dosyasının birleştirilmiş halidir. İçerikler değiştirilmeden korunmuştur.

## İçindekiler

1. Fighter + Rapid Laser
2. Breacher + Rapid Shotgun
3. Interceptor + Dual Kinetic Blaster
4. Conductor + Electric Launcher
5. Aegis + Continuous Heat Laser
6. Cryo Controller + Cryo Projector

---


# Bölüm 1 — Fighter + Rapid Laser

# Fighter + Rapid Laser Design Notes

## 1. Fighter

**Role:** Balanced Fighter / All-Rounder  
**Primary Weapon:** Fighter Rapid Laser

Fighter is the baseline/reference ship. It is intended to be reliable in most situations without becoming the strongest specialist in any single area.

---

## Core Stats

| Stat | Level 1 | Per Level |
|---|---:|---:|
| Max Health | 250 | +40 |
| Attack Power | 39 | +3 |
| Energy Power | 35 | +2 |
| Armor | 2 | +2 |
| Luck | 0 | +1 |

### Example Progression

| Ship Level | Max Health | AP | EP | Armor | Luck |
|---:|---:|---:|---:|---:|---:|
| 1 | 250 | 39 | 35 | 0 | 0 |
| 2 | 295 | 42 | 37 | 2 | 1 |
| 3 | 340 | 45 | 39 | 4 | 2 |
| 4 | 385 | 48 | 41 | 6 | 3 |
| 5 | 430 | 51 | 43 | 8 | 4 |
| 10 | 655 | 66 | 53 | 18 | 9 |
| 15 | 880 | 81 | 63 | 28 | 14 |

Armor and Luck use diminishing returns. Around 400 rating they reach approximately 83% effectiveness.

---

## Movement

| Movement Stat | Value |
|---|---:|
| Forward / Reverse Thrust | 650 / 190 |
| Lateral Thrust | 270 |
| Turn Speed / Response | 400 / 5 |
| Linear Damping | 0.7 |
| Max Speed | 520 |
| Input Response | 12 |
| Mouse Aim Dead Zone | 32 |
| Horizontal / Vertical Rating -> Thrust | 60 / 60 |
| Rating -> Max Speed | 20 |
| Movement Rating Reduction Scale | 20 |

---

## Energy / Shield / Afterburner

| Stat | Value |
|---|---:|
| Base Max Shield | 115 |
| Shield Full Recharge Time | 6 s |
| Shield Recharge Delay | 6 s |
| Afterburner Capacity | 65 |
| Afterburner Full Recharge Time | 6 s |
| Afterburner Recharge Delay | 1.5 s |
| Afterburner Speed Multiplier | 1.55x |
| Afterburner Acceleration Multiplier | 1.8x |
| Energy Consumption | 33/s |
| Shield Affinity | 0.5 |
| Afterburner Affinity | 0.5 |
| Afterburner Engage Time | 0.22 s |
| Afterburner Disengage Time | 0.45 s |
| Afterburner Maneuver Multiplier | 0.9 |

Energy Power also contributes to the ship's shield and afterburner systems according to the global Energy system.

---

# 2. Fighter Rapid Laser

**Role:** Balanced / Reliable baseline primary  
**Damage Type:** Photonic  
**Fire Rate:** 4 shots/s  
**Range:** 1600  
**Magazine:** 48  
**Reload:** 2 s

Rapid Laser is the baseline weapon used to evaluate the risk/reward of the more specialized primaries.

---

## Normal Damage

```text
BaseDamage(L) = 10 + 5 * (L - 1)

APScale = 0.41

NormalDamage = BaseDamage(L) + AP * APScale
```

### Level 1 Fighter Reference

Fighter Level 1:

```text
AP = 39
```

Rapid Laser Level 1:

```text
BaseDamage = 10
APScale = 0.41

NormalDamage
= 10 + 39 * 0.41
= 25.99
```

Normal DPS:

```text
25.99 * 4
= 103.96 DPS
```

So the new Level 1 baseline remains approximately:

**104 normal DPS**

This preserves the old Rapid Laser baseline even though Fighter now starts at 39 AP instead of 35 AP.

---

## Weapon Level Base Damage

| Weapon Level | Base Damage |
|---:|---:|
| 1 | 10 |
| 2 | 15 |
| 3 | 20 |
| 4 | 25 |
| 5 | 30 |

Formula:

```text
BaseDamage(L) = 10 + 5 * (L - 1)
```

---

## Current Empowered Reference

The Rapid Laser's empowered mechanic has not yet been fully rebalanced around the new Fighter stats.

Current authored/reference behavior:

- Every 6 successful hits triggers an empowered shot.
- Old empowered bonus reference: +5.5 damage at the initial level.

This section should be revisited separately before treating the empowered scaling as final.

---

# 3. Design Intent

Fighter + Rapid Laser define the roster baseline.

- Fighter starts with strong but not extreme AP.
- AP naturally grows by +3 per ship level.
- EP also grows, keeping the ship broadly useful rather than purely kinetic.
- Health progression is moderate at +45 per level.
- Armor and Luck grow naturally but remain far from specialist values.
- Rapid Laser provides high range and reliable uptime.
- Its Level 1 normal DPS remains around 104 after increasing Fighter's starting AP from 35 to 39.
- Specialized weapons such as Rapid Shotgun should exceed Rapid Laser under their ideal combat conditions, while accepting more risk or setup requirements.


---


# Bölüm 2 — Breacher + Rapid Shotgun

# Breacher + Rapid Shotgun Design Notes

## 1. Breacher

**Role:** Bruiser / Breacher / Close-range assault ship  
**Primary Weapon:** Rapid Shotgun

Breacher is designed to force its way into close range, survive contact, and keep the shotgun cone on target. It has lower top speed than Fighter, but stronger acceleration and turning so it can enter, correct its angle, and stay threatening at short range.

### Core Stats

| Stat | Level 1 | Per Level |
|---|---:|---:|
| Max Health | 300 | +60 |
| Attack Power | 45 | +2 |
| Energy Power | 34 | +0 |
| Armor | 3 | +2 |
| Luck | 0 | +1 |

### Movement

| Movement Stat | Value |
|---|---:|
| Max Speed | 490 |
| Forward Thrust | 750 |
| Reverse Thrust | 220 |
| Lateral Thrust | 245 |
| Turn Speed | 450 |
| Turn Response | 6.0 |
| Linear Damping | 0.7 |

### Movement Identity

- Lower top speed than Fighter.
- Strong forward acceleration.
- Better turning and response at close range.
- Lower lateral agility than a true interceptor.
- Intended to reach shotgun range quickly without becoming a high-speed skirmisher.

### Afterburner Direction

Current design target:

- Afterburner Speed Multiplier: ~1.65x
- Normal Top Speed: 490
- Approx. Afterburner Top Speed: 808

This keeps Breacher slower in normal flight while allowing short, aggressive breach entries.

---

## 2. Rapid Shotgun

**Role:** Bruiser / Off-Tank / Breacher primary  
**Damage Model:** Instant cone blast / hitscan-like wave  
**Normal Damage Type:** Photonic  
**Fire Rate:** 1.75 shots/s  
**Range:** ~400

The shotgun no longer uses pellets. Each trigger pull creates a short-range cone blast. Every valid target inside the cone receives one damage event.

### Normal Damage

```text
BaseDamage(L) = 38 + 8 * (L - 1)

APScale(L) = 0.60 + 0.04 * (L - 1)

NormalDamage = BaseDamage(L) + AP * APScale(L)
```

### Empowered Trigger

Every 3rd trigger pull becomes empowered.

Current empowered secondary scaling:

```text
EPScale = 0.15

HPScale = 0.03

EmpoweredBonus = EP * 0.15 + MaxHealth * 0.03

EmpoweredDamage = NormalDamage + EmpoweredBonus
```

The empowered attack is intended to be the shotgun's heavier special blast. Ignite behavior is not yet locked.

---

## 3. Level 1 Reference — Breacher + Shotgun

Breacher Level 1 stats:

```text
MaxHealth = 290
AP = 45
EP = 34
```

Normal shotgun hit:

```text
BaseDamage = 38
APScale = 0.60

NormalDamage
= 38 + 45 * 0.60
= 65
```

Normal DPS:

```text
65 * 1.75 = 113.75 DPS
```

Empowered bonus:

```text
34 * 0.15 + 290 * 0.03
= 5.1 + 8.7
= 13.8
```

Empowered hit:

```text
65 + 13.8 = 78.8
```

Average sustained DPS with every 3rd shot empowered:

```text
(65 + 13.8 / 3) * 1.75
= 121.8 DPS
```

### L1 Summary

| Metric | Value |
|---|---:|
| Normal Hit | 65 |
| Normal DPS | 113.75 |
| Empowered Bonus | 13.8 |
| Empowered Hit | 78.8 |
| Average Sustained DPS | ~121.8 |

---

## 4. Design Intent

Breacher and Rapid Shotgun are balanced as one package.

- Fighter is the safer, longer-range baseline.
- Breacher accepts lower top speed and very short weapon range.
- In return, Breacher gets stronger acceleration, better close-range turning, more hull durability, and higher starting AP.
- Shotgun should outperform Rapid Laser when Breacher successfully maintains ideal close range.
- AP is the shotgun's main damage stat.
- Max Health contributes to empowered damage.
- EP contributes to empowered damage but does not naturally grow on Breacher.
- Breacher starts with usable EP, but its natural progression does not turn it into an Energy-focused ship.

The intended combat loop is:

**Approach -> accelerate into breach range -> keep nose on target -> maintain cone pressure -> leverage every 3rd empowered blast.**


---


# Bölüm 3 — Interceptor + Dual Kinetic Blaster

# Interceptor + Dual Kinetic Blaster Design Notes

## 1. Interceptor

**Role:** Interceptor / Skirmisher / Sustained Armor Breaker  
**Primary Weapon:** Dual Kinetic Blaster

Interceptor is a fragile, mobile ship built around staying on a target, maintaining weapon uptime, and accelerating the Dual Kinetic Blaster's ArmorPen ramp through natural Attack Speed growth.

---

## Core Stats

| Stat | Level 1 | Per Level |
|---|---:|---:|
| Max Health | 200 | +30 |
| Attack Power | 41 | +3 |
| Energy Power | 28 | +1 |
| Armor | 1 | +1 |
| Luck | 1 | +1 |
| Natural Attack Speed Bonus | +1% | +1% |

Natural Attack Speed follows ship level directly:

```text
NaturalASBonus(L) = L * 1%
```

Examples:

| Ship Level | Natural AS Bonus |
|---:|---:|
| 1 | +1% |
| 5 | +5% |
| 10 | +10% |
| 15 | +15% |

### Example Stat Progression

| Ship Level | HP | AP | EP | Armor | Luck | Natural AS |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 200 | 41 | 28 | 1 | 1 | +1% |
| 5 | 320 | 53 | 32 | 5 | 5 | +5% |
| 10 | 470 | 68 | 37 | 10 | 10 | +10% |
| 15 | 620 | 83 | 42 | 15 | 15 | +15% |

Armor and Luck use the global diminishing-return system.

---

## Movement Identity

Interceptor should be the roster's agile skirmisher:

- High top speed
- Strong lateral movement
- High turn speed and response
- Lower durability than Fighter and Breacher
- Designed to stay attached to a target rather than win through raw initial damage

### Current Movement Draft

| Movement Stat | Draft Value |
|---|---:|
| Max Speed | 590 |
| Forward Thrust | 720 |
| Reverse Thrust | 240 |
| Lateral Thrust | 340 |
| Turn Speed | 480 |
| Turn Response | 6.5 |

These movement values are still provisional and can be tuned separately.

---

# 2. Dual Kinetic Blaster

**Role:** Sustained Kinetic pressure / ArmorPen ramp  
**Damage Type:** Kinetic  
**Base Fire Rate:** 7 shots/s  
**Range:** ~650  
**Magazine:** 64

Dual Kinetic Blaster intentionally starts with modest raw damage. Its reward comes from maintaining successful hits on the same target and gradually applying ArmorPen.

---

## Normal Damage

```text
BaseDamage(L) = 3 + 2 * (L - 1)

APScale(L) = 0.20 + 0.02 * (L - 1)

NormalDamage = BaseDamage(L) + AP * APScale(L)
```

The Interceptor's natural Attack Speed modifies the weapon's actual fire rate:

```text
ActualFireRate = 7 * (1 + NaturalASBonus)
```

At ship level 1:

```text
NaturalASBonus = 0.01

ActualFireRate = 7 * 1.01
               = 7.07 shots/s
```

---

## Empowered Hit

Every **8 successful hits** triggers one empowered hit.

```text
EmpoweredFlat(L) = 5 + 2 * (L - 1)

EmpoweredAPScale = 0.08

EmpoweredEPScale(L) = 0.25 + 0.04 * (L - 1)

EmpoweredBonus =
    EmpoweredFlat(L)
    + AP * 0.08
    + EP * EmpoweredEPScale(L)

EmpoweredDamage = NormalDamage + EmpoweredBonus
```

After the empowered hit deals damage:

```text
+1 ArmorPen stack
```

Normal hits do not apply ArmorPen.

---

## ArmorPen

Global ArmorPen progression:

| Stacks | Armor Penetration |
|---:|---:|
| 1 | 6% |
| 2 | 12% |
| 3 | 18% |
| 4 | 30% |

The new stack is applied **after the current hit**, so the empowered hit that creates a stack does not benefit from that newly-created stack.

---

# 3. Level 1 Reference — Interceptor + Dual

Level 1 Interceptor:

```text
AP = 41
EP = 28
Natural AS = +1%
```

Level 1 Dual:

```text
BaseDamage = 3
APScale = 0.20
BaseFireRate = 7/s
```

### Normal Hit

```text
NormalDamage
= 3 + 41 * 0.20
= 11.2
```

### Empowered Bonus

```text
EmpoweredBonus
= 5 + 41 * 0.08 + 28 * 0.25
= 5 + 3.28 + 7
= 15.28
```

### Empowered Hit

```text
EmpoweredDamage
= 11.2 + 15.28
= 26.48
```

### Actual Fire Rate

```text
7 * 1.01
= 7.07 shots/s
```

### Average Raw Sustained DPS

With one empowered hit per 8 successful hits:

```text
AverageDamagePerShot
= 11.2 + 15.28 / 8
= 13.11

AverageRawDPS
= 13.11 * 7.07
≈ 92.7 DPS
```

### Level 1 Summary

| Metric | Value |
|---|---:|
| Normal Hit | 11.2 |
| Empowered Bonus | 15.28 |
| Empowered Hit | 26.48 |
| Actual Fire Rate | 7.07/s |
| Average Raw DPS | ~92.7 |
| ArmorPen Trigger | Every 8 successful hits |

---

# 4. Design Intent

Interceptor and Dual Kinetic Blaster are balanced as one package.

- Level 1 raw DPS intentionally sits around **90-95 DPS**.
- Fighter Rapid Laser is safer and has stronger immediate baseline damage.
- Rapid Shotgun has much higher ideal-range raw damage.
- Dual begins lower because its true payoff is sustained ArmorPen.
- Interceptor naturally gains Attack Speed every ship level.
- Higher Attack Speed increases both raw DPS and the rate at which empowered ArmorPen hits occur.
- AP remains the primary normal-damage stat.
- EP mainly contributes to empowered hits.
- Interceptor is intentionally fragile, starting at only 200 HP.
- Its survivability comes primarily from mobility and target control through positioning rather than hull durability.

The intended combat loop is:

**Acquire target -> stay attached through mobility -> maintain successful hits -> trigger empowered shots -> build ArmorPen -> win the sustained engagement.**


---


# Bölüm 4 — Conductor + Electric Launcher

# Conductor + Electric Launcher Design Notes

## 1. Conductor

**Role:** Support / Conductor / Ability-Weaving  
**Primary Weapon:** Electric Launcher

Conductor is built around high Energy Power, ability usage, Electric vulnerability, and strong utility scaling rather than high raw weapon DPS.

---

## Core Stats

| Stat | Level 1 | Per Level |
|---|---:|---:|
| Max Health | 220 | +40 |
| Attack Power | 27 | +1 |
| Energy Power | 41 | +3 |
| Armor | 1 | +1 |
| Luck | 2 | +2 |

### Example Progression

| Ship Level | HP | AP | EP | Armor | Luck |
|---:|---:|---:|---:|---:|---:|
| 1 | 230 | 27 | 41 | 1 | 2 |
| 5 | 390 | 31 | 53 | 5 | 10 |
| 10 | 590 | 36 | 68 | 10 | 20 |
| 15 | 790 | 41 | 83 | 15 | 30 |

Armor and Luck use the global diminishing-return system.

---

## Movement

| Movement Stat | Value |
|---|---:|
| Max Speed | 505 |
| Forward Thrust | 620 |
| Reverse Thrust | 200 |
| Lateral Thrust | 285 |
| Turn Speed | 410 |
| Turn Response | 5.3 |

### Movement Identity

- Slightly slower than Fighter in top speed.
- Better lateral movement than Breacher.
- Moderate turning and response.
- Not intended to compete with Interceptor mobility.
- Positioned as a stable mid-range support / ability-weaving platform.

---

# 2. Electric Launcher

**Role:** Support / Conductor / Ability-Weaving / Horde  
**Normal Fire Rate:** 2 shots/s

Electric Launcher deliberately has lower personal raw DPS than dedicated damage primaries. Its main value comes from chain hits, empowered bursts, and Electric vulnerability.

---

## Normal Damage

The weapon adapts to whichever offensive stat is higher.

```text
DominantStat = max(AP, EP)
SecondaryStat = min(AP, EP)

BaseDamage(L) = 20 + 2 * (L - 1)

DominantScale(L) = 0.40 + 0.02 * (L - 1)

SecondaryScale(L) = 0.10 + 0.01 * (L - 1)

NormalDamage =
    BaseDamage(L)
    + DominantStat * DominantScale(L)
    + SecondaryStat * SecondaryScale(L)
```

Normal Electric Launcher attacks deal Electric damage but do **not** apply Electric stacks.

---

## Empowered Trigger

After a successful ability cast:

```text
NextEmpoweredShots = 2
```

These shots fire at:

```text
EmpoweredFireRate = 4 shots/s
```

A new successful ability cast refreshes the empowered shot count to 2. It does not stack to 4.

---

## Empowered Damage

```text
EmpoweredFlat(L) = 3 + 1 * (L - 1)

EmpoweredEPScale(L) = 0.08 + 0.02 * (L - 1)

EmpoweredDamage =
    NormalDamage
    + EmpoweredFlat(L)
    + EP * EmpoweredEPScale(L)
```

Each empowered hit applies:

```text
+1 Electric stack
```

Chain-hit targets also receive +1 Electric stack.

---

## Electric Vulnerability

| Electric Stacks | Increased Damage Taken |
|---:|---:|
| 1 | +3% |
| 2 | +6% |
| 3 | +9% |
| 4 | +16% |

Maximum stacks: 4.

---

# 3. Level 1 Reference — Conductor + Electric Launcher

Level 1 Conductor:

```text
AP = 27
EP = 41
```

Since EP is higher:

```text
DominantStat = 41
SecondaryStat = 27
```

### Normal Hit

```text
NormalDamage
= 20 + 41 * 0.40 + 27 * 0.10
= 20 + 16.4 + 2.7
= 39.1
```

### Normal DPS

```text
39.1 * 2
= 78.2 DPS
```

### Empowered Bonus

```text
EmpoweredBonus
= 3 + 41 * 0.08
= 6.28
```

### Empowered Hit

```text
EmpoweredDamage
= 39.1 + 6.28
= 45.38
```

The empowered shots are fired at 4 shots/s, but only the next 2 shots receive the empowered state.

---

## Level 1 Summary

| Metric | Value |
|---|---:|
| Normal Hit | 39.1 |
| Normal DPS | 78.2 |
| Empowered Bonus | 6.28 |
| Empowered Hit | 45.38 |
| Normal Fire Rate | 2/s |
| Empowered Fire Rate | 4/s |
| Empowered Charges | 2 after ability cast |

---

# 4. Design Intent

Conductor and Electric Launcher are balanced as one package.

- Conductor starts with clearly higher EP than AP.
- EP is the ship's main offensive and utility stat.
- AP remains relevant through the adaptive dominant/secondary weapon formula.
- Electric Launcher intentionally starts around 78 raw DPS instead of competing with dedicated damage primaries.
- The real payoff comes from chaining targets and applying Electric vulnerability through empowered shots.
- Ability usage is directly tied to weapon pressure through the two-shot empowered burst.
- Luck grows quickly to support the ship's broader proc / buildcraft identity.
- Conductor remains less mobile than Interceptor and less durable than Breacher.

The intended combat loop is:

**Use ability -> gain 2 empowered shots -> rapidly apply Electric stacks -> amplify follow-up damage -> continue ability/weapon weaving.**


---


# Bölüm 5 — Aegis + Continuous Heat Laser

# Aegis + Continuous Heat Laser Design Notes

## 1. Aegis

**Role:** Heavy Tank / Anchor / Shield-Reactor Specialist  
**Primary Weapon:** Continuous Heat Laser

Aegis is designed as a durable anchor ship that relies less on raw hull than Breacher and more on Energy Power, Armor scaling, shields, and sustained beam pressure.

---

## Core Stats

| Stat | Level 1 | Per Level |
|---|---:|---:|
| Max Health | 270 | +50 |
| Attack Power | 25 | +1 |
| Energy Power | 43 | +3 |
| Armor | 4 | +3 |
| Luck | 0 | +1 |

### Example Progression

| Ship Level | HP | AP | EP | Armor | Luck |
|---:|---:|---:|---:|---:|---:|
| 1 | 270 | 25 | 43 | 4 | 0 |
| 5 | 470 | 29 | 55 | 16 | 4 |
| 10 | 720 | 34 | 70 | 31 | 9 |
| 15 | 970 | 39 | 85 | 46 | 14 |

Armor and Luck use the global diminishing-return system.

---

## Tank Identity

Aegis is intentionally different from Breacher:

- Lower raw Max Health than Breacher.
- Much higher Energy Power growth.
- Much stronger Armor growth.
- Primary weapon does not use AP.
- Intended to tank through Armor, shield/reactor strength, and sustained positioning.
- Functions best as a mid-range anchor rather than a point-blank bruiser.

---

# 2. Continuous Heat Laser

**Role:** EP Heavy Tank / Anchor / Anti-Shield Beam  
**Damage Tick Interval:** 0.25 s  
**Damage Ticks:** 4 per second  
**Primary Damage Stat:** Energy Power  
**AP Scaling:** None  
**MaxHealth Damage Scaling:** None

The beam becomes stronger as Heat rises. Its highest Heat state becomes an empowered Energy beam with additional effectiveness against shields.

---

## Base Tick Damage

```text
BaseTickDamage(L) = 13 + 2 * (L - 1)

EPScale(L) = 0.19 + 0.02 * (L - 1)

RawTickDamage =
    BaseTickDamage(L)
    + EP * EPScale(L)
```

---

## Heat Regions

### Low Heat — 0% to 35%

```text
DamageMultiplier = 1.00x
```

### Mid Heat — 35% to 75%

```text
DamageMultiplier = 1.15x
```

### Empowered Heat — 75%+

```text
DamageMultiplier = 1.40x
```

At 75%+ Heat:

- Beam damage becomes **Energy damage**.
- Global Energy shield multiplier applies.

```text
ShieldDamage = FinalDamage * 1.50
```

Hull damage remains at the normal 1.00x Energy damage value.

---

# 3. Level 1 Reference — Aegis + Heat Laser

Level 1 Aegis:

```text
EP = 43
```

Level 1 Heat Laser:

```text
BaseTickDamage = 13
EPScale = 0.19
TickInterval = 0.25 s
```

### Raw Tick

```text
RawTickDamage
= 13 + 43 * 0.19
= 21.17
```

### Low Heat DPS

```text
21.17 * 4
= 84.68 DPS
```

### Mid Heat DPS

```text
84.68 * 1.15
= 97.38 DPS
```

### Empowered Heat DPS

```text
84.68 * 1.40
= 118.55 DPS
```

### Empowered Heat vs Shield

```text
118.55 * 1.50
= 177.83 Shield DPS
```

---

## Level 1 Summary

| Heat State | Multiplier | Hull DPS | Shield DPS |
|---|---:|---:|---:|
| 0–35% | 1.00x | 84.7 | 84.7 |
| 35–75% | 1.15x | 97.4 | 97.4 |
| 75%+ | 1.40x | 118.6 | 177.8 |

---

# 4. Design Intent

Aegis and Continuous Heat Laser are balanced as one package.

- Aegis starts with high EP at 43.
- EP naturally grows by +3 per ship level.
- Armor grows faster than on Breacher, at +3 per level.
- Aegis has less raw hull than Breacher but more effective defensive scaling.
- Continuous Heat Laser starts below high-risk dedicated damage weapons.
- Its sustained damage rises as the player maintains beam uptime and Heat.
- The highest Heat region does not need extreme raw hull DPS because it gains the global 1.50x Energy shield multiplier.
- The weapon intentionally has no AP scaling.
- MaxHealth does not currently contribute to weapon damage.

The intended combat loop is:

**Hold position -> maintain beam contact -> build Heat -> enter empowered Energy state -> pressure shields while relying on Armor and EP-driven defenses.**


---


# Bölüm 6 — Cryo Controller + Cryo Projector

# Cryo Controller + Cryo Projector Design Notes

## 1. Cryo Controller

**Role:** Controller / Area Control / Battlefield Sweeper  
**Primary Weapon:** Cryo Projector

Cryo Controller is designed as a lower-damage but more durable control ship. Its handling stays close to Fighter, while its stronger lateral movement and turning help it line up wide Cryo waves across groups of enemies.

---

## Core Stats

| Stat | Level 1 | Per Level |
|---|---:|---:|
| Max Health | 250 | +40 |
| Attack Power | 27 | +1 |
| Energy Power | 40 | +2 |
| Armor | 2 | +2 |
| Luck | 1 | +1 |

### Example Progression

| Ship Level | HP | AP | EP | Armor | Luck |
|---:|---:|---:|---:|---:|---:|
| 1 | 250 | 27 | 40 | 2 | 1 |
| 5 | 410 | 31 | 48 | 10 | 5 |
| 10 | 610 | 36 | 58 | 20 | 10 |
| 15 | 810 | 41 | 68 | 30 | 15 |

Armor and Luck use the global diminishing-return system.

---

## Movement

| Movement Stat | Fighter | Cryo Controller |
|---|---:|---:|
| Max Speed | 520 | 520 |
| Forward Thrust | 650 | 610 |
| Reverse Thrust | 190 | 210 |
| Lateral Thrust | 270 | 310 |
| Turn Speed | 400 | 425 |
| Turn Response | 5.0 | 5.5 |

### Movement Identity

- Same top speed as Fighter.
- Slightly weaker forward acceleration.
- Better reverse and lateral movement.
- Better turning and response.
- Designed to reposition and sweep wide waves through groups rather than chase targets like Interceptor.

---

# 2. Cryo Projector

**Role:** Controller / Horde Control / Wide-Area Primary  
**Fire Rate:** 2.5 shots/s  
**Normal Damage Type:** Cryo

Cryo Projector has intentionally low single-target raw damage because each wave can hit multiple enemies and its real value comes from Frost Meter, Whiteout, Cryo stacks, and area control.

Normal waves do **not** apply Cryo stacks.

---

## Normal Damage

```text
BaseDamage(L) = 20 + 2 * (L - 1)

APScale(L) = 0.30 + 0.02 * (L - 1)

NormalDamage = BaseDamage(L) + AP * APScale(L)
```

### Level 1 Reference

Cryo Controller Level 1:

```text
AP = 27
```

Normal hit:

```text
NormalDamage
= 20 + 27 * 0.30
= 28.1
```

Normal single-target DPS:

```text
28.1 * 2.5
= 70.25 DPS
```

---

# 3. Frost Meter

```text
MaxFrostMeter = 4
```

A normal wave can gain Frost Meter from two independent conditions:

- Hits 2 or more different enemies -> +1 Frost Meter
- Hits at least one target that already had a Cryo stack before the wave -> +1 Frost Meter

If both conditions are met:

```text
+2 Frost Meter
```

Maximum Frost Meter gain per wave:

```text
+2
```

Examples:

| Wave Result | Frost Meter Gain |
|---|---:|
| 1 normal target | 0 |
| 1 Cryo-stacked target | +1 |
| 2+ normal targets | +1 |
| 2+ targets and at least one already has Cryo | +2 |

When Frost Meter reaches 4, Whiteout begins automatically.

---

# 4. Whiteout

```text
WhiteoutDuration = 4 s
```

During Whiteout, attacks alternate:

```text
Empowered -> Normal -> Empowered -> Normal -> ...
```

Whiteout always begins with an empowered wave.

Attack Speed can increase the number of total waves fired during the Whiteout window.

Frost Meter is not generated during Whiteout.

A short post-Whiteout Frost Meter lockout is planned; current working value is approximately 3 seconds.

---

# 5. Empowered Wave

Empowered waves provide:

- +1 Cryo stack to every valid target hit
- Increased range
- Increased wave size / width
- Moderate bonus damage

Current range target:

```text
EmpoweredRangeBonus ≈ +250
```

Empowered damage:

```text
EmpoweredFlat(L) = 5 + 2 * (L - 1)

EmpoweredEPScale(L) = 0.07 + 0.01 * (L - 1)

EmpoweredDamage =
    NormalDamage
    + EmpoweredFlat(L)
    + EP * EmpoweredEPScale(L)
```

If the target already had 4 Cryo stacks before the empowered hit:

```text
FinalEmpoweredDamage = EmpoweredDamage * 1.15
```

Cryo stacks are not consumed.

---

# 6. Level 1 Empowered Reference

Cryo Controller Level 1:

```text
AP = 27
EP = 40
```

Normal hit:

```text
28.1
```

Empowered bonus:

```text
5 + 40 * 0.07
= 7.8
```

Empowered hit:

```text
28.1 + 7.8
= 35.9
```

Against a target that already has 4 Cryo stacks:

```text
35.9 * 1.15
= 41.285
≈ 41.3
```

---

## Level 1 Summary

| Metric | Value |
|---|---:|
| Normal Hit | 28.1 |
| Normal Single-Target DPS | 70.25 |
| Empowered Bonus | 7.8 |
| Empowered Hit | 35.9 |
| Empowered Hit vs Pre-existing 4 Cryo | ~41.3 |
| Fire Rate | 2.5/s |

---

# 7. Cryo Status

Global Cryo stack values:

| Cryo Stacks | Slow |
|---:|---:|
| 1 | 4% |
| 2 | 8% |
| 3 | 12% |
| 4 | 20% |

Maximum Cryo stacks: 4.

---

# 8. Design Intent

Cryo Controller and Cryo Projector are balanced as one package.

- Cryo Controller has lower offensive stats than Fighter and dedicated damage ships.
- It starts with only 27 AP, keeping Cryo Projector single-target damage low.
- EP remains strong enough to support empowered waves, shields, afterburner, and control-focused abilities.
- It has the same top speed as Fighter but better lateral movement and turning.
- It is slightly more control-oriented and positionally stable than Fighter rather than simply faster.
- Its durability comes from moderate HP, higher starting Armor, and EP-driven defenses.
- Cryo Projector is intentionally weak in single-target raw DPS because its wave can hit many enemies at once.
- Its real payoff comes from battlefield coverage, Frost Meter generation, automatic Whiteout windows, and Cryo stack application.

The intended combat loop is:

**Reposition laterally -> line up multiple enemies -> sweep with normal waves -> build Frost Meter -> enter Whiteout -> spread Cryo stacks with enlarged empowered waves -> maintain control of the battlefield.**
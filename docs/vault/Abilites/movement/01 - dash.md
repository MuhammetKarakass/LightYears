---
type: ability
id: "movement.dash"
name: "Dash"
category: movement
slot: 1
status: implemented
priority: high
source: "Abilites/yetenekler.md"
source_section: "2.3. Dash"
activation_policy: OnPressed
lifetime_policy: Duration
direction_policy: MovementInputOrMouseWorld
max_level: 5
evolve_count: 5
features:
  - Movement burst
  - Input or mouse direction
  - Relative camera zoom
  - Cooldown progression
scaling:
  - "Cooldown decreases by 0.12 seconds per level"
tags:
  - ability
  - ability/movement
---

# Dash

> Kategori: **Movement** | Durum: **Implemented**

## Kaynak ozellikler

| Ozellik | Deger |
|---|---:|
| Ability ID | `Ability.Movement.Dash.Basic` |
| Behavior ID | `GameAbilityBehavior.Dash` |
| Cooldown | 2.0 saniye |
| Duration | 0.24 saniye |
| Base distance | 260.0 birim |
| Relative zoom-out | 0.15 |
| Max charges | 1 |
| Direction | `MovementInputOrMouseWorld` |

## Algoritma

1. F tusu ve cooldown kontrol edilir.
2. Yonu, hareket girdisi varsa hareket girdisinden; yoksa mouse world konumundan alir.
3. Gemi 0.24 saniyelik hareket penceresinde 260 birimlik dash uygular.
4. Kamera goreli olarak 0.15 zoom-out yapar.
5. Dash bitince hiz ve kamera durumu normale doner; cooldown baslatilir.

## Seviye Gelisimi ve Olcekleme (Level Progression - 5 Seviye)

- **Seviye Basi Azalis (L2 - L5)**:
  - Bekleme suresi: **-0.12 saniye** / seviye
  - Bu, güncel JSON'da authored sabit **-0.12 saniye** / seviye değeridir;
    yüzde kuralı olarak genellenmez.
- **Seviyelere gore bekleme sureleri**:
  - L1: **2.00 saniye**
  - L2: **1.88 saniye**
  - L3: **1.76 saniye**
  - L4: **1.64 saniye**
  - L5: **1.52 saniye**
- **Hurda maliyetleri**: `[40, 50, 65, 80]` Hurda.

## Evolve'ler

| Seviye | Evolve adi | Durum | Degisim |
|---:|---|---|---|
| 1 | Basic Dash | implemented | Cooldown 2.00 saniye |
| 2 | Quick Dash | implemented | Cooldown 1.88 saniye |
| 3 | Rapid Dash | implemented | Cooldown 1.76 saniye |
| 4 | Tactical Dash | implemented | Cooldown 1.64 saniye |
| 5 | Instant Dash | implemented | Cooldown 1.52 saniye |

## Notlar

- Ikon: `SpaceShooterRedux/PNG/Power-ups/star_gold.png`.
- Renk: `sf::Color(120, 220, 255, 255)`.

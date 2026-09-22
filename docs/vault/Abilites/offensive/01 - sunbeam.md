---
type: ability
id: "offensive.sunbeam"
name: "Sun Beam"
category: offensive
slot: 1
status: implemented
priority: high
source: "Abilites/yetenekler.md"
source_section: "2.2. Sun Beam"
activation_policy: OnPressed
lifetime_policy: Instant
targeting: MouseWorld
max_level: 5
evolve_count: 5
features:
  - Direct damage
  - Area impact
  - Telegraph
  - Radius scaling
scaling:
  - "Damage +8 per level from L2 to L5"
  - "Radius +8 per level from L2 to L5"
tags:
  - ability
  - ability/offensive
---

# Sun Beam

> Kategori: **Offensive** | Durum: **Implemented**

## Kaynak ozellikler

| Ozellik                |                                Deger |
| ---------------------- | -----------------------------------: |
| Ability ID             | `Ability.Offense.SunBeam.Strike.Basic` |
| Behavior ID            |        `GameAbilityBehavior.SunBeam` |
| Cooldown               |                           1.0 saniye |
| Base damage            |                                 40.0 |
| Radius                 |                           96.0 birim |
| Width                  |                           72.0 birim |
| Length                 |                          720.0 birim |
| Telegraph duration     |                           0.5 saniye |
| Arrival duration       |                           0.2 saniye |
| Impact delay           |                          0.05 saniye |
| Impact visual duration |                          0.22 saniye |
| Actor                  | `Actor.Ability.SunBeam.Strike.Basic` |

## Algoritma

1. Yetenek `OnPressed` ile ve cooldown hazirsa tetiklenir.
2. `MouseWorld` konumu hedef olarak alinir.
3. Isinin uzunluk, genislik ve yaricap parametreleriyle telegraph alani gosterilir.
4. Telegraph tamamlaninca hedef hattina hasar uygulanir.
5. Impact gecikmesi ve gorsel temizlik tamamlanir; cooldown baslatilir.

## Seviye Gelisimi ve Olcekleme (Level Progression - 5 Seviye)

- **Seviye Basi Artis (L2 - L5)**:
  - Hasar: **+8.0 Damage** / seviye
  - Yaricap: **+8.0 Radius** / seviye
- **Hurda Maliyetleri**: `[40, 50, 65, 80]` Hurda

## Evolve'ler

| Seviye | Evolve adi | Durum | Degisim |
|---:|---|---|---|
| 1 | Basic Beam | implemented | 40 damage, 96 radius |
| 2 | Wider Beam | implemented | +8 damage, +8 radius |
| 3 | Focused Beam | implemented | +8 damage, +8 radius |
| 4 | Solar Lance | implemented | +8 damage, +8 radius |
| 5 | Full Spectrum | implemented | +8 damage, +8 radius |

## Notlar

- Renk: `sf::Color(255, 190, 70, 255)`.
- Ikon: `SpaceShooterRedux/PNG/Lasers/laserBlue01.png`.

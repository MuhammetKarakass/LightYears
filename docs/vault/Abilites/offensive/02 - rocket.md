---
type: ability
id: "offensive.rocket"
name: "Rocket"
category: offensive
slot: 2
status: implemented
priority: high
source: "Abilites/yetenekler.md"
source_section: "2.5. Rocket"
activation_policy: OnPressed
lifetime_policy: Instant
max_level: 15
evolve_count: 5
features:
  - Projectile damage
  - Kinetic area explosion
  - AttackPower scaling
  - Explosion radius progression
scaling:
  - "Damage + Owner.AttackPower * 1.25"
  - "Five evolve bands summarize the 15-level progression"
tags:
  - ability
  - ability/offensive
---

# Rocket

> Kategori: **Offensive** | Durum: **Implemented**

## Kaynak ozellikler

| Ozellik | Deger |
|---|---:|
| Ability ID | `Ability.Offense.Rocket.Basic` |
| Behavior ID | `GameAbilityBehavior.Rocket` |
| Cooldown | 3.0 saniye |
| Base damage | 55.0 Kinetic AoE |
| Projectile speed | 1000.0 birim/sn |
| Range | 1100.0 birim |
| Explosion radius | 55.0 birim |
| Collision radius | 8.0 birim |
| Spawn distance | 42.0 birim |
| Cleanup grace | 0.25 saniye |

## Algoritma

1. R tusu ve cooldown kontrol edilir.
2. Rocket, geminin 42 birim onunden hedef yonde dogar.
3. Projectile 1000 birim/sn hizla ilerler ve hedefe veya menzil sonuna kadar gider.
4. Carpismada 55 damage uygulanir ve 55 birimlik kinetic patlama alani olusturulur.
5. Actor cleanup grace suresi sonunda silinir; cooldown baslatilir.

## Seviye Gelisimi ve Olcekleme (Level Progression - 15 Seviye)

- **Nitelik olcekleme**:
  - Hasar += `Owner.AttackPower * 1.25`
- **Seviye basi artis (L2 - L15)**:
  - Hasar: **+4.0 Damage** / seviye
  - Bekleme suresi: **-0.12 saniye** / seviye
  - Patlama yaricapi: **+1.0 birim** / seviye
  - Mermi hizi ve menzil sabit tutulur.
- **Hurda maliyetleri**: 14 gelistirme seviyesinin her biri **60 Hurda**.

## Evolve'ler

Kaynakta 15 seviye bulunuyor. Dashboard sistemindeki 5 evolve satirina uc seviye birer evolve bandi olarak gruplanmistir.

| Evolve | Kaynak seviyeleri | Durum | Her seviyedeki degisim |
|---:|---|---|---|
| 1 | L1-L3 | implemented | Damage +4, cooldown -0.12, explosion radius +1 |
| 2 | L4-L6 | implemented | Ayni artisin 3 seviye daha uygulanmasi |
| 3 | L7-L9 | implemented | Ayni artisin 3 seviye daha uygulanmasi |
| 4 | L10-L12 | implemented | Ayni artisin 3 seviye daha uygulanmasi |
| 5 | L13-L15 | implemented | Ayni artisin 3 seviye daha uygulanmasi |

## Notlar

- Mermi hizi ve menzil sabit tutulur.
- Ikon: `SpaceShooterRedux/PNG/Lasers/laserRed04.png`.
- Renk: `sf::Color(255, 115, 75, 255)`.

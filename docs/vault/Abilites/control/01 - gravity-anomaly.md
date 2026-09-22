---
type: ability
id: "control.gravity-anomaly"
name: "Gravity Anomaly"
category: control
slot: 2
status: implemented
priority: high
source: "Abilites/yetenekler.md"
source_section: "2.4. Gravity Anomaly"
activation_policy: OnPressed
lifetime_policy: Instant
max_level: 15
evolve_count: 5
features:
  - Area control
  - Pull force
  - Slow effect
  - Projectile delivery
scaling:
  - "Radius + Owner.MaxHealth * 0.20"
  - "Duration + Owner.MaxHealth * 0.0025"
  - "Five evolve bands summarize the 15-level progression"
tags:
  - ability
  - ability/control
  - control
---

# Gravity Anomaly

> Kategori: **Control** | Durum: **Implemented**

## Kaynak ozellikler

| Ozellik | Deger |
|---|---:|
| Ability ID | `Ability.Control.GravityAnomaly.Basic` |
| Behavior ID | `GameAbilityBehavior.GravityAnomaly` |
| Cooldown | 8.0 saniye |
| Cast range | 900.0 birim |
| Projectile speed | 2000.0 birim/sn |
| Base duration | 2.5 saniye |
| Base radius | 220.0 birim |
| Pull strength | 500.0 birim/sn2 |
| Slow magnitude | 0.20 |
| Spawn distance | 42.0 birim |
| Effect | `Effect.GravityAnomaly.Inside` |

## Algoritma

1. Q tusu ve cooldown kontrol edilir.
2. Projectile, hedef konuma dogru 2000 birim/sn hizla gonderilir.
3. Projectile alana ulastiginda `Effect.GravityAnomaly.Inside` alanini olusturur.
4. Alan icindeki hedefler mesafeye gore `F = PullStrength * (1 - d / Radius)^2 * dt` ile merkeze cekilir.
5. Hedeflere yuzde 20 yavaslatma uygulanir; alan suresi bitince alan temizlenir.

## Seviye Gelisimi ve Olcekleme (Level Progression - 15 Seviye)

- **Nitelik olcekleme**:
  - Alan yaricapi += `Owner.MaxHealth * 0.20`
  - Alan suresi += `Owner.MaxHealth * 0.0025`
- **Seviye basi artis (L2 - L15)**:
  - Bekleme suresi: **-0.10 saniye** / seviye
  - Alan suresi: **+0.03 saniye** / seviye
  - Alan yaricapi: **+2.0 birim** / seviye
  - Cekim kuvveti: **+10.0 birim/sn2** / seviye
  - Yavaslatma orani: **+0.005** / seviye
  - Mermi hizi: **+25.0 birim/sn** / seviye
  - Atis menzili: **+5.0 birim** / seviye
- **Hurda maliyetleri**: 14 gelistirme seviyesinin her biri **60 Hurda**.

## Evolve'ler

Kaynakta 15 seviye bulunuyor. Dashboard sistemindeki 5 evolve satirina uc seviye birer evolve bandi olarak gruplanmistir.

| Evolve | Kaynak seviyeleri | Durum | Her seviyedeki degisim |
|---:|---|---|---|
| 1 | L1-L3 | implemented | Cooldown -0.10, duration +0.03, radius +2, pull +10, slow +0.005, speed +25, range +5 |
| 2 | L4-L6 | implemented | Ayni artisin 3 seviye daha uygulanmasi |
| 3 | L7-L9 | implemented | Ayni artisin 3 seviye daha uygulanmasi |
| 4 | L10-L12 | implemented | Ayni artisin 3 seviye daha uygulanmasi |
| 5 | L13-L15 | implemented | Ayni artisin 3 seviye daha uygulanmasi |

## Notlar

- Hurda maliyeti: 14 gelistirme seviyesinin her biri 60 hurda.
- Ikon: `SpaceShooterRedux/PNG/Lasers/laserBlue04.png`.
- Renk: `sf::Color(135, 95, 255, 255)`.

---
type: ability
id: "defensive.shield"
name: "Shield"
category: defensive
slot: 1
status: implemented
priority: high
source: "Abilites/yetenekler.md"
source_section: "2.1. Shield"
activation_policy: OnPressed
lifetime_policy: Duration
max_level: 5
evolve_count: 5
features:
  - Self barrier
  - MaxHealth scaling
  - Armor scaling
  - Barrier break thrust boost
scaling:
  - "Barrier capacity + Owner.MaxHealth * 0.20"
  - "Barrier capacity + Owner.Armor * 50.0"
tags:
  - ability
  - ability/defensive
---

# Shield

> Kategori: **Defensive** | Durum: **Implemented**

## Kaynak ozellikler

| Ozellik | Deger |
|---|---:|
| Ability ID | `Ability.Shield.Basic` |
| Behavior ID | `GameAbilityBehavior.Shield` |
| Cooldown | 8.0 saniye |
| Duration | 5.0 saniye |
| Max charges | 1 |
| Uygulanan effect | `Effect.Barrier.Basic` |
| Hedef | Self |

## Algoritma

1. Q tusuna basildiginda cooldown ve charge kontrol edilir.
2. Barrier kapasitesi `30 + MaxHealth * 0.20 + Armor * 50.0` formuluyle hesaplanir.
3. `Effect.Barrier.Basic` self hedefe 5 saniyelik sureyle uygulanir.
4. Barrier kirilirsa `Event.Owner.BarrierBroken` olayi dinlenir.
5. Kirilma olayinda 2 saniyelik `Effect.Test.BarrierBreak.ThrustBoost` uygulanir.

## Seviye Gelisimi ve Olcekleme

- Kaynak dokumanda seviye basi artis degerleri belirtilmemistir.
- **Nitelik olcekleme**:
  - Barrier capacity += `Owner.MaxHealth * 0.20`
  - Barrier capacity += `Owner.Armor * 50.0`
- Bes evolve icin kapasite, sure, yenilenme ve kirilma etkisi dengelemesi ayri tasarlanacak.

## Evolve'ler

| Seviye | Evolve adi | Durum | Degisim |
|---:|---|---|---|
| 1 | Basic Barrier | implemented | Kaynak dokumandaki temel kalkan |
| 2 | Barrier Capacity | planned | Evolve degeri ayrica tasarlanacak |
| 3 | Reinforced Armor | planned | Evolve degeri ayrica tasarlanacak |
| 4 | Fast Recharge | planned | Evolve degeri ayrica tasarlanacak |
| 5 | Break Thrust | planned | Mevcut kirilma etkisi evolve olarak ayrilacak |

## Notlar

- Kaynak dokuman temel davranisi tanimliyor; bes evolve icin ayri dengeleme karari gerekiyor.
- Ikon: `SpaceShooterRedux/PNG/Power-ups/shield_gold.png`.
- Renk: `sf::Color(80, 200, 255, 255)`.

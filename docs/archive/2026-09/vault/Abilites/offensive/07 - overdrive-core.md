---
type: ability
id: "offensive.overdrive-core"
name: "Overdrive Core"
category: offensive
slot: 2
status: planned
priority: high
source: "Abilites/yetenekler.md"
source_section: "4. Overdrive Core"
activation_policy: OnPressed
lifetime_policy: Duration
duration: 5.0
max_level: 5
evolve_count: 5
features:
  - AttackPower buff
  - AttackSpeed buff
  - Movement speed buff
  - Health regeneration lock
scaling:
  - "AttackPower Multiply +35%"
  - "AttackSpeed Multiply +35%"
  - "MoveSpeedHorizontal and MoveSpeedVertical Multiply +35%"
  - "HealthRegenPerSecond Override 0"
tags:
  - ability
  - ability/offensive
  - planned
---

# Overdrive Core

> Kategori: **Offensive** | Durum: **Planned**

## Kaynak ozellikler

| Ozellik | Deger |
|---|---|
| Aktivasyon | `OnPressed` |
| Duration | 5 saniye |
| AttackPower | `Multiply +35%` |
| AttackSpeed | `Multiply +35%` |
| MoveSpeedHorizontal | `Multiply +35%` |
| MoveSpeedVertical | `Multiply +35%` |
| HealthRegenPerSecond | `Override 0` |

## Algoritma

1. Yetenek basildiginda 5 saniyelik buff instance olusturulur.
2. AttackPower, AttackSpeed ve iki hareket hizina Multiply modifier uygulanir.
3. HealthRegenPerSecond niteligine Override 0 uygulanir; bu modifier diger toplama/carpma modifierlarindan once gelir.
4. Duration bitince tum modifier handle'lari kaldirilir ve can yenileme normal sisteme doner.
5. Actor veya effect lifecycle temizligi iki kez uygulanmayacak sekilde idempotent olmalidir.

## Seviye Gelisimi ve Olcekleme

- **Temel buff degerleri**:
  - AttackPower: **+35% Multiply**
  - AttackSpeed: **+35% Multiply**
  - MoveSpeedHorizontal: **+35% Multiply**
  - MoveSpeedVertical: **+35% Multiply**
  - HealthRegenPerSecond: **0 Override**
- **Sure**: **5 saniye**.
- Kaynak dokumanda L2-L5 sayisal progression degerleri henuz belirtilmemistir.

## Evolve'ler

| Seviye | Evolve adi | Durum | Degisim |
|---:|---|---|---|
| 1 | Basic Overdrive | planned | Temel 5 saniyelik buff |
| 2 | Turbo Output | planned | Hasar ve atis hizi paketi |
| 3 | Vector Boost | planned | Hareket hizi paketi |
| 4 | Reactor Lock | planned | Can yenileme kilidinin netlestirilmesi |
| 5 | Master Overdrive | planned | Son dengeleme paketi |

## Notlar

- Can yenilemesinin `Override` ile 0 yapilmasi bu yetenegin ana riskidir.
- Kaynak dokumanda input, cooldown, enerji maliyeti ve gorsel profil degerleri ayrica tanimlanmali.

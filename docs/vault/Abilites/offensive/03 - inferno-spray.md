---
type: ability
id: "offensive.inferno-spray"
name: "Inferno Spray"
category: offensive
slot: 3
status: planned
priority: high
source: "Abilites/yetenekler.md"
source_section: "2. Inferno Spray"
activation_policy: WhileHeld
lifetime_policy: WhileInputHeld
targeting: MouseWorld
max_level: 5
evolve_count: 5
features:
  - Cone flame scan
  - Thermal damage
  - Thermal stack status
  - Burn damage over time
scaling:
  - "Thermal rules from DamageTypeSystem"
  - "First stack applies immediately"
  - "Thermal table is 1/2/3/5 DPS for 5 seconds"
tags:
  - ability
  - ability/offensive
  - planned
---

# Inferno Spray

> Kategori: **Offensive** | Durum: **Planned**

## Kaynak ozellikler

| Ozellik | Deger |
|---|---|
| Aktivasyon | `WhileHeld` |
| Yasam suresi | `WhileInputHeld` |
| Hasar etiketi | `Thermal` |
| Hedef yonu | MouseWorld |
| Vurus tipi | Yuksek frekansli scan tick |
| Thermal stack | 1–4 stack |
| Stack tablosu | 1 / 2 / 3 / 5 hasar/sn |
| Tam guc suresi | 5 saniye; sonra 1 stack/sn decay |

## Algoritma

1. Oyuncu inputu basili tutuldugu surece konik tarama alani aktif kalir.
2. Her tick, koni icindeki uygun dusmanlar bulunur ve Thermal hasar kurali uygulanir.
3. Ilk isabetten itibaren canonical Thermal stack ve stack'e ait DPS uygulanir.
4. Yeniden isabet stack ekler ve 5 saniyelik tam guc suresini yeniler; isabet kesilirse stackler decay olur.
5. Input birakildiginda scan, tick timer ve aktif gorseller temizlenir.

## Seviye Gelisimi ve Olcekleme

- Kaynak dokumanda sayisal level progression degerleri henuz belirtilmemistir.
- Planlanan scaling alanlari:
  - Konik alan acisi ve menzili
  - Tick frekansi ve Thermal hasari
  - Ignite buildup esigi ve burn hasari

## Evolve'ler

| Seviye | Evolve adi | Durum | Degisim |
|---:|---|---|---|
| 1 | Pilot Flame | planned | Temel konik tarama |
| 2 | Wider Cone | planned | Konik alan genisligi |
| 3 | Hotter Fuel | planned | Tick hasari |
| 4 | Deep Ignite | planned | Ignite buildup ve burn |
| 5 | Inferno Core | planned | Son dengeleme paketi |

## Notlar

- Kaynak dokumanda actor/config sayisal degerleri henuz tamamlanmamis.
- Implementasyon tamamlaninca cooldown, energy cost, cone angle, range ve tick rate eklenmeli.

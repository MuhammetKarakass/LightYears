---
type: ability
id: "offensive.swarm-micro-missiles"
name: "Swarm Micro-Missiles"
category: offensive
slot: 4
status: planned
priority: medium
source: "Abilites/yetenekler.md"
source_section: "5. Swarm Micro-Missiles"
activation_policy: OnPressed
lifetime_policy: Instant
missile_count: 6
targeting: ClosestDistinctTarget
max_level: 5
evolve_count: 5
features:
  - Six missile salvo
  - Closest distinct target selection
  - Target distribution
  - Photonic or kinetic damage
scaling:
  - "Each distinct eligible enemy receives at least one missile when possible"
  - "Remaining missiles may repeat targets when enemy count is lower than six"
tags:
  - ability
  - ability/offensive
  - planned
---

# Swarm Micro-Missiles

> Kategori: **Offensive** | Durum: **Planned**

## Kaynak ozellikler

| Ozellik | Deger |
|---|---|
| Aktivasyon | `OnPressed` |
| Yasam suresi | `Instant` |
| Missile count | 6 |
| Hedef secimi | `closest distinct target` |
| Hasar etiketi | `Photonic` veya `Kinetic` |
| Actor davranisi | 6 bagimsiz missile actor |

## Algoritma

1. Aktivasyon aninda uygun dusmanlar menzil ve gorus kurallarina gore toplanir.
2. Hedefler mesafeye gore siralanir ve birbirinden farkli hedefler secilir.
3. Ilk dagitim turunda her uygun hedefe en fazla bir missile atanir.
4. Hedef sayisi 6'dan azsa kalan missile'lar en yakin hedeflere tekrar atanir.
5. Her missile bagimsiz actor olarak dogar; carpismada hasar verir ve cleanup sonrasi yok edilir.

## Seviye Gelisimi ve Olcekleme

- **Temel salvo**: **6 missile**.
- **Hedef dagitimi**:
  - Uygun dusmanlar arasindan en yakin farkli hedefler secilir.
  - Her farkli hedefe mumkunse en az 1 missile atanir.
  - Hedef sayisi 6'dan azsa kalan missile'lar tekrar atanabilir.
- Projectile speed, range, collision radius ve damage progression degerleri kaynakta henuz belirtilmemistir.

## Evolve'ler

| Seviye | Evolve adi | Durum | Degisim |
|---:|---|---|---|
| 1 | Micro Salvo | planned | 6 missile temel salvosu |
| 2 | Smart Lock | planned | Hedef secim onceligi |
| 3 | Split Guidance | planned | Hedef dagitimi ve takip |
| 4 | Dense Payload | planned | Hasar veya etki guclendirmesi |
| 5 | Swarm Core | planned | Son dengeleme paketi |

## Notlar

- Kaynak dokumanda projectile speed, range, collision radius, damage ve cleanup degerleri henuz yok.
- Bu yetenek primary fire silahlarindan ayridir; `Ability` katalogunda offensive slot 4 olarak tutulur.

---
type: ability
id: "defensive.null-pulse"
name: "Null Pulse"
category: defensive
slot: 3
status: design
priority: medium
max_level: 5
evolve_count: 5
created: 2026-08-05
features: []
scaling: []
tags:
  - ability
  - ability/defensive
---

# Null Pulse

> Kategori: **Defensive** | Durum: **design**

Null Pulse, oyuncunun çevresindeki projectile baskısını anında temizleyen ve yakındaki düşmanların sistemlerini kısa süreliğine devre dışı bırakan savunma ve kontrol odaklı bir yetenektir.

Yetenek yüksek hasar vermek için kullanılmaz. Temel amaçları:

- Tehlikeli düşman mermilerini temizlemek
- Oyuncuya yeniden pozisyon alma fırsatı vermek
- Yakındaki düşmanları kısa süre durdurmak
- Oyuncunun kendi projectile’larını da silerek zamanlama bedeli oluşturmak

Oyuncu yetenek tuşuna bastığında geminin merkezinden dışarı doğru dairesel bir enerji dalgası yayılır.

İşlem sırası:

1. Oyuncunun çevresindeki pulse alanı hesaplanır.
2. Alan içindeki bütün geçerli projectile actor’ları yok edilmek üzere işaretlenir.
3. Alan içindeki düşmanlar tespit edilir.
4. Her düşmana düşük miktarda Energy hasarı uygulanır.
5. Her uygun düşmana stun GameplayEffect’i uygulanır.
6. Görsel ve işitsel darbe başlatılır.
7. Ability anında tamamlanır.
8. Cooldown başlar.

Null Pulse, projectile’ın sahibini veya takımını dikkate almaz.

### Silinen projectile’lar

- Düşman mermileri
- Oyuncunun primary weapon projectile’ları
- Oyuncunun Rocket projectile’ı
- Mikro füzeler
- Fiziksel projectile olarak çalışan ability saldırıları
- Projectile tabanlı düşman ve boss saldırıları

### Silinmeyen nesneler

- Beam saldırıları
- Sürekli lazerler
- Gravity Anomaly alanı
- Kalıcı area effect’ler
- Düşmanlar
- Oyuncu
- Pickup’lar
- Asteroidler
- Arena hazard’ları
- Patlama alanları
- Projectile özelliği taşımayan görsel effect’ler

Oyuncunun kendi projectile’larının da silinmesi, yeteneğin temel kullanım bedelidir.

Bu davranış:

- Luck ile engellenmez.
- EnergyPower ile değiştirilmez.
- Ability level ile kaldırılmaz.
- Temel ability içinde oyuncu projectile’larını koruyan istisna içermez.

Kendi projectile’larını koruyan bir davranış ileride ayrı bir Evolve veya Essence olabilir.


| Ozellik     |                          Deger |
| ----------- | -----------------------------: |
| Ability ID  | `Ability.Control.NullPulse.Basic` |
| Behavior ID | `GameAbilityBehavior.NullPuls` |
| Cooldown    |                    10.0 saniye |
| Base damage |                             10 |
| Radius      |                      300 birim |
| Stun Base   |                     1.0 saniye |

## Algoritma

1. Tetiklenme kosulunu kontrol et.
2. Hedefi veya etki alanini belirle.
3. Temel etkiyi uygula.
4. Scaling ve evolve degerlerini hesapla.
5. Cooldown, duration ve temizleme islemlerini uygula.

## Seviye Gelisimi ve Olcekleme

- Seviye basi artis (L2 - L5):
  - Hasar:
  - Yaricap:
  - Bekleme suresi:
- Nitelik olcekleme:
  - Owner statlari:
  - Formul:

## Evolve'ler

| Seviye | Evolve adi | Durum | Degisim |
|---:|---|---|---|
| 1 | | planned | |
| 2 | | planned | |
| 3 | | planned | |
| 4 | | planned | |
| 5 | | planned | |

## Notlar

- Bagimliliklar:
- Test senaryolari:
- Dengeleme notlari:

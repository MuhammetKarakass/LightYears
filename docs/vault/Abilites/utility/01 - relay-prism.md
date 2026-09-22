---
type: ability
name: Relay Prism
category: utility
slot: 2
status: implemented
ability_id: Ability.Utility.RelayPrism.Basic
behavior: RelayPrism
features: [mouse-world, projectile-capture, clone-scatter, luck-bonus]
scaling: source-damage transfer + AttackPower * 0.25
source: LightYearsGame/assets/content/data/abilities.json
---

# Relay Prism

Bu notun eski loadout snapshot'ında E/Ability2'ye bağlanan Relay Prism, fare
konumunda 4 saniyelik 100 birim capture volume oluşturur. Güncel default slot
eşlemesi bu nottan değil `DefaultAbilityLoadout.cpp`'den okunur. Yakalanan uygun
ability projectile'i destroy edilir ve
merkezden dört dost clone üretilir. Clone hasarı kaynak hasarının %15'i ile
`AttackPower * 0.25` toplamıdır. Luck, en fazla dört ek clone'a olasılıklı katkı
verir; saçılma 30°–90° aralığındadır.

Her projectile lineage kaydı taşır. Aynı capture volume'u daha önce ziyaret eden
bir projectile yeniden yakalanmaz; bu kural clone üretim döngüsünü engeller.
Prism fizik dışı bir query volume'dür ve normal silah çarpışmalarını bloklamaz.

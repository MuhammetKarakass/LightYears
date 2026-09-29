---
type: ability
name: Shield Harvest
category: defensive
slot: 3
status: implemented
ability_id: Ability.Defense.ShieldHarvest.Basic
behavior: ShieldHarvest
features: [area-count, temporary-overshield, completion-telegraph]
scaling: per-enemy shield
source: LightYearsGame/assets/content/data/abilities.json
---

# Shield Harvest

Ability3 input'u bu ability'e bağlandığında 1.5 saniye odaklanır; 700 birim
içindeki karşıt savaşçıları
tamamlanma anında bir kez sayar. Her hedef için 40 temporary overshield verir.
Overshield 5 saniye tutulur ve ardından saniyede 100 azalır. 14 saniye cooldown'a
sahiptir. L2–L15: her seviye -0.25 s cooldown ve hedef başına +5 shield.

Telegraph owner'ı takip eder; sayım tamamlandıktan sonra kısa completion feedback
fazı gösterir. Sonradan alana giren veya çıkan düşmanlar verilmiş shield miktarını
değiştirmez.

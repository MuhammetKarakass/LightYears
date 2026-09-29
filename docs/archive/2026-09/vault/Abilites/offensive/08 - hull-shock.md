---
type: ability
name: Hull Shock
category: offensive
slot: 1
status: implemented
ability_id: Ability.Offense.HullShock.Basic
behavior: HullShock
features: [hold-to-charge, electric-area, radial-telegraph, action-lock]
scaling: MaxHealth * 0.30 damage
source: LightYearsGame/assets/content/data/abilities.json
---

# Hull Shock

Ability1 input'u bu ability'e bağlandığında basılı tutulurken en fazla iki
saniye şarj olur; tuş
bırakılınca veya süre dolunca Electric alan darbesi boşaltır. Taban hasar 30,
maksimum yarıçap 600, minimum şarj yarıçapı 300'dür. En düşük şarjda hasar
çarpanı 0.25'tir; hasar ayrıca `MaxHealth * 0.30` ile ölçeklenir.

Yarıçap 1.25 saniyede tam değere ulaşır. Electric stack ilk uygulamadan itibaren
etkilidir; effect 4 saniye tam güçte kalır, en fazla 4 stack'tir ve alınan hasarı
canonical olarak +%3 / +%6 / +%9 / +%16 artırır. Şarj sırasında ability activation ve primary fire
action-lock tag'leri eklenir. Hasar yarıçapı, telegraph'ın kullandığı aynı radial
resolver ile hesaplanır.

---
type: ability
name: Energy Spear
category: functional
slot: 1
status: implemented
ability_id: Ability.Movement.EnergySpear.Basic
behavior: EnergySpear
features: [held-charge, movement, energy-contact, endpoint-damage]
scaling: AttackPower * 1.0
source: LightYearsGame/assets/content/data/abilities.json
---

# Energy Spear

Basılı tutulan şarjlı hareket saldırısıdır. 1.5 saniyede şarj olur, 200–600
birim arasında 2400 speed ile ilerler ve Energy temas hasarı verir. Taban hasar
10 + `AttackPower * 1.0`; tam şarjda 2.5x, endpointte 2.0x damage multiplier
uygulanır. L2–L15: +2 hasar ve -0.20 saniye cooldown.

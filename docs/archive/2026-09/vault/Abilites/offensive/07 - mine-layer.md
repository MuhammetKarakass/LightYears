---
type: ability
name: Mine Layer
category: offensive
slot: 1
status: implemented
ability_id: Ability.Offense.MineLayer.Basic
behavior: MineLayer
features: [mines, trigger-radius, energy, stun, knockback]
scaling: AttackPower * 0.75
source: LightYearsGame/assets/content/data/abilities.json
---

# Mine Layer

3 Energy mayını 120 birim aralıkla bırakır. Her mayın 8 saniye yaşar; 90 birim
tetik yarıçapında 30 hasar ve 120 birim patlama alanı üretir, 0.5 saniye stun ve
300 knockback uygular. Hasar `AttackPower * 0.75` ile ölçeklenir; Luck bonus mayın
sayısına katkı verir. L2–L15: +3 hasar ve -0.20 saniye cooldown.

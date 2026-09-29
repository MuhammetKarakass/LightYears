---
type: ability
name: Crescent Reaver
category: offensive
slot: 1
status: implemented
ability_id: Ability.Offense.CrescentReaver.Basic
behavior: CrescentReaver
features: [kinetic, projectile, bounce, mouse-direction]
scaling: AttackPower * 0.80; Luck * 0.05 bounce count
source: LightYearsGame/assets/content/data/abilities.json
---

# Crescent Reaver

Eski loadout snapshot'ında Q/Ability1'e bağlanan Crescent Reaver, fare yönüne
giden Kinetic projectile 26 hasar, 1300
speed ve 5 bounce ile başlar. Her bounce hasarı %15 büyütür ve bounce cooldown'u
0.3 saniye azaltır. Hasar `AttackPower * 0.80`, bounce sayısı `Luck * 0.05` ile
ölçeklenir. L2–L15: +3 hasar ve -0.20 saniye cooldown.

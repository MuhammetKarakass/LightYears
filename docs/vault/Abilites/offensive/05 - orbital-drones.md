---
type: ability
name: Orbital Drones
category: offensive
slot: 4
status: implemented
ability_id: Ability.Offense.OrbitalDrones.Basic
behavior: OrbitalDrones
features: [orbit, kinetic-contact, duration, attack-power-scaling]
scaling: AttackPower * 0.50
source: LightYearsGame/assets/content/data/abilities.json
---

# Orbital Drones

Altı saniye boyunca sahibin çevresinde dört drone döndürür. Dronlar temasla
Kinetic hasar verir; aynı hedefe tekrar vurma 0.5 saniyelik bekleme ile
sınırlandırılır. Runtime değerleri: 12 s cooldown, 500 birim yörünge yarıçapı,
18 taban hasar, 2.5 radyan/sn ve 12 birim temas yarıçapı. `AttackPower * 0.50`
hasara eklenir. L2–L15 her seviyede +2 hasar ve -0.25 s cooldown uygular.

Varsayılan loadout'ta değildir; content slotu Ability4/R'dir ve loadout manager
tarafından yeniden takılabilir.

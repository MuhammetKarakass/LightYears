---
type: ability
name: Scorch Drive
category: offensive
slot: 3
status: implemented
ability_id: Ability.Offense.ScorchDrive.Basic
behavior: ScorchDrive
features: [thermal, fire-trail, area-segments, burn]
scaling: AttackPower * 0.30; MaxHealth lifetime scale
source: LightYearsGame/assets/content/data/abilities.json
---

# Scorch Drive

Eski loadout snapshot'ında F/Ability3'e bağlanan Scorch Drive, beş saniye
boyunca hareket hattına 60 birim arayla
70×80 Thermal fire segment'leri bırakır. Segmentler 0.25 saniye tick atar;
canonical Thermal status ilk stack'ten itibaren 1/2/3/5 hasar/sn tablosunu
kullanır. Scorch Drive'ın özel burn modu ayrıca `BurnDamagePerTick` ve
`BurnTickInterval` snapshot'ı ile çalışır; bu mod canonical DPS ile çiftlenmez.
Taban hasar 8 + `AttackPower * 0.30`; segment ömrü MaxHealth
ölçeğiyle uzayabilir. L2–L15: +2 hasar ve -0.25 saniye cooldown.

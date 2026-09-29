---
type: ability
name: Execution Drive
category: offensive
slot: 4
status: implemented
ability_id: Ability.Offense.ExecutionDrive.Basic
behavior: ExecutionDrive
features: [kill-stacks, attack-power-effect, chase-movement, duration-refresh]
scaling: AttackPower + movement resolver
source: LightYearsGame/assets/content/data/abilities.json
---

# Execution Drive

Beş saniyelik agresif güçlendirmedir; 14 saniye cooldown'a sahiptir. Başlangıçta
+10 AttackPower effect'i verir. Sahip gemi bir düşmanı öldürdüğünde +3 AttackPower
stack kazanır ve active süreyi tam başlangıç süresine yeniler. Düşük canlı ve
hareket yönünün önünde kalan düşmana yaklaşırken hareket bonusu; %10 base + stack
başına %2 + harici AttackPower'ın 0.001 katsayısı ile hesaplanır. Hedefleme
menzili 700, yön eşiği 0.25'tir.

Effect ve ship runtime modifier, ability bittiğinde birlikte kaldırılır; stack
sayısı sonraki aktivasyona taşınmaz.

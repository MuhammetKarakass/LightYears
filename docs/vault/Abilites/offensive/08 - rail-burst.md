---
type: ability
name: Rail Burst
category: offensive
slot: 4
status: implemented
ability_id: Ability.Offense.RailBurst.Basic
behavior: RailBurst
features: [energy, precision, piercing, critical-pierce-retention, procedural-rail-visual]
scaling: AttackPower * 1.50
source: LightYearsGame/assets/content/data/abilities.json
---

# Rail Burst

Eski loadout snapshot'ında R/Ability4'e bağlanan Rail Burst, düz hatlı,
homing/alan/patlama üretmeyen
Energy rail shot; aynı hedefi yalnız bir kez vurur ve farklı hedefleri deler.
Her isabetten sonra hasar `1 - effectivePierceLoss` ile azalır:

```text
effectivePierceLoss = 0.20 / (1 + CriticalChance * 1.50)
```

Taban hasar 80, AttackPower katkısı `*1.50`, speed 2400, collision radius 8 ve
runtime JSON menzili 1400'dür. L2–L15: +6 hasar, -0.15 s cooldown. Görsel,
sprite mermi yerine procedurally çizilen beyaz çekirdek, cyan dış gövde, uzun
trail/afterimage ve kısa Energy impact pulse'lardan oluşur.

> Tasarım notundaki 800 menzil hedefi kodda uygulanmış değildir. Bu değer
> değişirse actor lifetime/range validation ve regresyon testleri birlikte
> güncellenmelidir.

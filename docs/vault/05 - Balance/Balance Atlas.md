---
type: balance
status: active
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - ability, damage, movement and presentation integrations
  - GasLiteCoreTests and balance-related content configurations
source_files:
  - SpaceAbilitySystem/include/attributes/AttributeMath.h
  - LightYearsGame/include/gameConfigs/ship/ShipStructs.h
  - LightYearsGame/src/gameplay/progression/ShipProgression.cpp
  - LightYearsGame/assets/content/data/weapons.json
  - LightYearsGame/include/gameConfigs/combat/WeaponStructs.h
  - LightYearsGame/include/gameConfigs/ability/movement/DashConfig.h
  - LightYearsGame/include/gameConfigs/ability/offensive/SunBeamConfig.h
  - LightYearsGame/include/gameConfigs/ability/offensive/RocketConfig.h
  - LightYearsGame/include/gameConfigs/ability/control/GravityAnomalyConfig.h
  - LightYearsGame/src/gameplay/ability/actions/AbilityActionAttributeResolver.cpp
symbols:
  - sas::AttributeMath::SaturatingFraction
  - ly::ShipProgression::GetXPRequiredForNextLevel
  - ly::AbilityActionAttributeResolver::ResolveAttributes
  - WeaponProgressionProfile
related:
  - "[[Derived Attributes]]"
  - "[[Modifier Operations]]"
  - "[[Ability Skill Progression System]]"
  - "[[Weapon System]]"
  - "[[Combat and Damage System]]"
  - "[[Balance Data and Runtime Resolution Flow]]"
---

# Balance Atlas

## Amaç ve sınır

Bu atlas, kodda etkin olan balance girdilerinin konumunu ve runtime'a hangi sırayla girdiğini haritalar. Bir hedef değer, playtest sonucu veya tasarım onayı değildir. `docs/BALANCE_AND_ROADMAP_NOTEBOOK.md` içindeki boş/gelecek deney alanları uygulanmış kabul edilmez; mevcut değer için burada belirtilen kaynak kod esas alınır.

## Balance kaynak haritası

| Alan | Etkin kaynak | Runtime tüketicisi | Durum |
|---|---|---|---|
| Rating eğrileri | `SpaceAbilitySystem/include/attributes/AttributeMath.h` | combat, cooldown, loot ve derived stat resolver'ları | Implemented |
| Ship hareket tabanı ve rating katkısı | `gameConfigs/ship/ShipStructs.h` | `MovementComponent` | Implemented |
| Ship XP ve level growth | `ShipStructs.h`, `gameplay/progression/ShipProgression.cpp` | Player-owned `ShipProgression` / `AttributeSystem` | Implemented |
| Ability base değerleri, level step ve scrap | `assets/content/data/abilities.json`, `GameAbilityDefinition` | `GameAbility`, `AbilityActionAttributeResolver` | Implemented |
| Weapon base değerleri ve owner scaling | `assets/content/data/weapons.json`, `WeaponStructs.h` | `GameAbilityActionExecutor`, primary-weapon handler'ları | Implemented |
| Weapon level milestone ve scrap | `WeaponStructs.h::WeaponProgressionProfile` ve `weapons.json` | primary-fire ability conversion / weapon runtime | Implemented |
| Damage türü, armor, crit ve shield çözümü | combat/damage config ve runtime | `CombatRuntime`, `DamageTypeSystem`, `ShieldComponent` | Implemented; ayrıntı [[Combat and Damage System]] |
| Playtest hedefleri, TTK ve yayın hedefleri | mevcut kaynakta merkezi, ölçümlü bir atlas olarak bulunmadı | Doğrulanamadı | Unclear |

## Koddan doğrulanan ortak formüller

| Konu | Etkin formül / kural | Kaynak |
|---|---|---|
| Yüzde rating doygunluğu | `clamp(1 - exp(-rating / scale), 0, 1)` | `AttributeMath::SaturatingFraction` |
| Ability haste cooldown çarpanı | `1 - SaturatingFraction(haste, 100)` | `GetAbilityCooldownMultiplier` |
| Crit chance | base chance ile kalan olasılık üzerinden rating katkısı | `GetCriticalChance` |
| Armor reduction | Base multiplier `100 / (max(0, armor) + 100)`; DR bunun `1 - multiplier` eşdeğeridir. Penetration ile runtime multiplier'ı doğrudan `baseMultiplier + penetration × (1 - baseMultiplier)` çözer; hard cap yoktur (50 Armor ≈ %33.333 DR, 100 Armor = %50 DR, 200 Armor ≈ %66.667 DR, 300 Armor = %75 DR, 500 Armor ≈ %83.333 DR). | `GetArmorDamageMultiplier / GetArmorDamageReduction` |
| Ship XP ihtiyacı | `max(1, baseXP × currentLevel^xpExponent)` | `ShipProgression::GetXPRequiredForNextLevel` |
| Ship level bonusu | `(level - 1) × configured perLevel` | `ShipProgression::RebuildLevelModifiers` |
| Ability/weapon source scaling | Hedef attribute için sırayla Add/Multiply/Override; sonuç en az `0` | `AbilityActionAttributeResolver::ResolveAttributes`, altta `sas::ApplyAttributeScalings` |
| Movement rating katkısı | `contribution × scale × (1 - exp(-max(0,rating)/scale))` | `ShipMovementAttributes::GetDiminishingRatingContribution` |

Sıra önemlidir: scaling rule'ları definition içindeki sırayla uygulanır. Attribute modifier sırası için [[Modifier Operations]], rating eğrilerinin ayrıntısı için [[Derived Attributes]] kullanılır.

## Şu anki içerik profilleri

| Profil | Kodda görünen seviye/maliyet modeli | Önemli örnek |
|---|---|---|
| Player primary weapons | Fighter BasicRapidLaser: L1–L15 (L15: 152 D, 1.10 AP, 16 Empowered D, 0.52 EP, 40..105 scrap); Diğer birincil silahlar: Maksimum level 4, L2/L3/L4 için `40 / 50 / 65` scrap | `weapons.json` & `WeaponProgressionProfile` |
| Dash | L1–L5; `40 / 50 / 65 / 80` scrap | Güncel JSON her step için authored `-0.12` cooldown modifier ekler |
| Sun Beam | L1–L5; `40 / 50 / 65 / 80` scrap | Her step `+8 Damage`, `+8 Radius` |
| Rocket | L1–L15; her sonraki level için `60` scrap | Config'te damage/cooldown/radius progression; owner AttackPower scaling'i |
| Gravity Anomaly | L1–L15; her sonraki level için `60` scrap | Config'te cooldown/duration/radius/pull/slow/delivery progression; MaxHealth scaling'i |

Bu tablo yalnız config'teki mevcut numeric girdileri sınıflandırır; tek hedef DPS, encounter TTK veya değerlerin “nihai” olduğu sonucu çıkarmaz.

## Ship başlangıç ve growth varsayılanları

Güncel production balance kapsamı yalnız `Ship.Player.Fighter.Basic` profilidir.
Breacher, Interceptor, Conductor, Aegis ve Cryo Controller henüz production
ship/profile değildir; bu beş kimlik için base veya natural-growth sayıları bu
aşamada üretilmeyecektir.

`ShipProgressionDefinition` varsayılanı `baseXP = 100`, `xpExponent = 1.25` değerlerini taşır. Stat büyümesi merkezi base değer ve multiplier yerine ship'e ait kontrollü `naturalGrowth` girdileriyle tanımlanır: `{attributeId, perLevel}`. Toplam ek değer `(level - 1) × perLevel` olur; eksik attribute büyümez. Whitelist dışındaki hedefler, duplicate girdiler ve geçersiz değerler reddedilir.

Fighter'ın eski migration compatibility growth değerleri Phase 3A.3 ile kaldırılmış, üretim profili olarak aşağıdaki değerler devreye alınmıştır:
- L1 Base Statlar: MaxHealth 250, AttackPower 35 (`baseOwnerAttributes`), EnergyPower 35, Armor 0, Luck 0, AttackSpeed 0, CriticalChance 0, AbilityHaste 0, MoveSpeedHorizontal 0, MoveSpeedVertical 0.
- Natural Growth: MaxHealth +75, AttackPower +3, EnergyPower +2, Armor +2, Luck +1. AttackSpeed, Crit, Haste ve Movement Fighter'da doğal büyüme almaz.
- L1 / L5 / L10 / L15 Seviyeleri:
  - L1: HP 250, AP 35, EP 35, Armor 0, Luck 0 | Shield 150, AB 100, ShieldRegen 25, ABRegen ~16.67, Armor DR 0%
  - L5: HP 550, AP 47, EP 43, Armor 8, Luck 4 | Shield 158, AB 108, ShieldRegen ~26.33, ABRegen 18, Armor DR ~7.41%
  - L10: HP 925, AP 62, EP 53, Armor 18, Luck 9 | Shield 168, AB 118, ShieldRegen 28, ABRegen ~19.67, Armor DR ~15.25%
  - L15: HP 1300, AP 77, EP 63, Armor 28, Luck 14 | Shield 178, AB 128, ShieldRegen ~29.67, ABRegen ~21.33, Armor DR 21.875%

Bu değerler Fighter'a özel güncel üretim değerleridir; playtest ile tune edilebilir ve diğer gemiler için global bir standart değildir. Diğer beş gemi ertelenmiştir; altı gemilik nihai karşılaştırma yapılmamıştır.

AttackSpeed, CriticalChance ve AbilityHaste raw rating olarak yazılır; `PercentageRatingScale = 100`dür. Bu nedenle `2.0` iki rating puanı, `0.02` ise yüzde iki değil `0.02` puandır. Primary weapon'ın mevcut AttackSpeed → FireRate additive tüketimi bu progression aşamasında değiştirilmemiştir.

## Okuma sırası

1. [[Balance Data and Runtime Resolution Flow]]
2. [[Derived Attributes]] ve [[Modifier Operations]]
3. [[Ability Skill Progression System]]
4. [[Weapon System]]
5. [[Combat and Damage System]]

## Doğrulanamayan alanlar

- Merkezi bir balance data asset'i, ekonomi sink/source tablosu veya encounter TTK hedef matrisi bulunmadı.
- Config değerleri için playtest onayı ya da release-quality tuning sonucu koddan çıkarılamaz.
- Ayrı Evolve sistemi doğrulanmadığından evolve balance atlasına eklenmedi.

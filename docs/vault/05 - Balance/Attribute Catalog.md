---
type: balance
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - GameplayAttribute MovementSlow addition
  - effect and ability feature-local attribute schemas
  - GasLiteCoreTests
source_files:
  - LightYearsGame/include/gameplay/attributes/AttributeIds.h
  - SpaceAbilitySystem/include/attributes/GameplayAttribute.h
  - LightYearsGame/include/gameConfigs/combat/EffectStructs.h
  - LightYearsGame/include/gameConfigs/combat/DamageTypeConfig.h
  - LightYearsGame/include/gameConfigs/combat/WeaponStructs.h
  - LightYearsGame/include/gameConfigs/ability/offensive/RocketConfig.h
  - LightYearsGame/include/gameConfigs/ability/offensive/SunBeamConfig.h
  - LightYearsGame/include/gameConfigs/ability/control/GravityAnomalyConfig.h
  - LightYearsGame/src/gameplay/combat/CombatRuntime.cpp
  - LightYearsGame/src/gameplay/ship/ShipRuntime.cpp
symbols:
  - ly::OwnerAttributeIds
  - ly::ShipAttributeIds
  - ly::CommonAttributeIds
  - BarrierEffectSchema
  - DamageTypeSchema::Attribute
related:
  - "[[Attribute System]]"
  - "[[Gameplay Tag System]]"
  - "[[Derived Attributes]]"
  - "[[Modifier Operations]]"
---

# Attribute Catalog

## Katalog sınırı

Bu katalog attribute tag ailelerini ve gerçek runtime storage yerlerini gösterir. Her ability/weapon değerini ayrıntılandırmaz; feature-local aileleri kaynak kökü olarak listeler.

## Önemli attribute envanteri

| Attribute | Kategori | Base kaynak | Runtime sahibi | Modifier alıyor mu? | Ana kullanıcılar | Durum |
|---|---|---|---|---|---|---|
| `OwnerAttributeIds::MaxHealth` | Health | `CombatRuntime::InitializeOwnerAttributes(maxHealth)`; ship/progression mutation | `CombatRuntime::AttributeSystem` | Evet | `HealthComponent`, `ShipRuntime`, scaling | Active |
| `HealthRegen` | Regeneration | Başlangıç 0; `MaxHealth / 1200` | Owner AttributeSystem | Evet | `SpaceShip::UpdateRegeneration` | Derived active |
| `EnergyPower` | Energy/Utility | Ship profile başlangıcı; progression/config | Owner AttributeSystem | Evet | `ShipRuntime`, ability scaling | Active |
| `AttackPower` | Offensive | Başlangıç 0; ship/progression/config | Owner AttributeSystem | Evet | Ability/weapon scaling | Active |
| `AttackSpeed` | Offensive | Başlangıç 0; ship/progression/config | Owner AttributeSystem | Evet | Weapon cadence/heat scaling | Active |
| `AbilityHaste` | Utility | Başlangıç 0 | Owner AttributeSystem | Evet | Cooldown multiplier, movement reduction helper test | Active |
| `MoveSpeedHorizontal` | Movement | Başlangıç 0 | Owner AttributeSystem | Evet | `MovementComponent`, Dash scaling | Active |
| `MoveSpeedVertical` | Movement | Başlangıç 0 | Owner AttributeSystem | Evet | `MovementComponent`, Dash scaling | Active |
| `MovementSlow` | Movement | İlk modifier'da auto-register 0 | Owner AttributeSystem | Evet | `MovementComponent`, slow effect'leri | Active, dirty worktree |
| `Armor` | Armor/Defense | Başlangıç 0 | Owner AttributeSystem | Evet | `CombatRuntime::ProcessIncomingDamage` | Active |
| `Luck` | Utility | Başlangıç 0 | Owner AttributeSystem | Evet | combat/loot proc hesabı | Active |
| `CriticalChance` | Offensive | Başlangıç 0 rating | Owner AttributeSystem | Evet | `CombatRuntime::GetCriticalChance` | Active |
| `CriticalDamage` | Offensive | Başlangıç 1.5 multiplier | Owner AttributeSystem | Evet | `CombatRuntime::GetCriticalDamageMultiplier` | Active |
| `ShipAttributeIds::MaxShield` | Shield | Energy config + EnergyPower | `ShipRuntime::AttributeSystem` | Evet | `ShieldComponent` | Derived active |
| `ShieldRegen` | Shield/Regeneration | MaxShield / recharge duration | Ship AttributeSystem | Evet | `SpaceShip::UpdateRegeneration` | Derived active |
| `AfterburnerCapacity` | Energy | Energy config + EnergyPower | Ship AttributeSystem | Evet | `EnergyComponent` | Derived active |
| `AfterburnerRegen` | Regeneration | Capacity / recharge duration | Ship AttributeSystem | Evet | `EnergyComponent` | Derived active |
| `CommonAttributeIds::Damage` | Definition-local offensive | Ability/weapon/effect config | `GameplayAttributeList` / resolved state | Evet, helper/resolver ile | Ability/weapon/effect producer | Active |
| `CommonAttributeIds::Cooldown` | Definition-local utility | Ability config | Ability resolved list/state | Evet, helper/resolver ile | `GameAbility` / ability resolver | Active |
| `BarrierEffectSchema::Capacity` | Effect-local defense | Effect definition/spec | `ActiveGameplayEffect::runtimeAttributes` | Behavior değiştirir | Barrier behavior | Active, dirty worktree |

“Modifier alıyor mu?” sütunu, mutlaka owner `AttributeSystem` modifier map'ine girdiği anlamına gelmez. Definition-local listeler `CalculateModifiedAttributeValue` veya feature resolver'larıyla değişebilir; effect-local runtime attribute'lar behavior tarafından doğrudan güncellenebilir.

## Owner attribute'ları

Kaynak: `OwnerAttributeIds`, `LightYearsGame/include/gameplay/attributes/AttributeIds.h`.

| Sembol | Tag | Başlangıç / üretim | Durum |
|---|---|---|---|
| `MaxHealth` | `Attribute.Owner.MaxHealth` | `InitializeOwnerAttributes(maxHealth)` | Active |
| `HealthRegen` | `Attribute.Owner.HealthRegen` | 0; `ShipRuntime` MaxHealth / 1200 yazar | Derived active |
| `EnergyPower` | `Attribute.Owner.Energy.Power` | Ship profile başlangıcı; progression/config değiştirir | Active |
| `AttackPower` | `Attribute.Owner.AttackPower` | 0 | Active |
| `AttackSpeed` | `Attribute.Owner.AttackSpeed` | 0 | Active |
| `AbilityHaste` | `Attribute.Owner.AbilityHaste` | 0 | Active |
| `MoveSpeedHorizontal` | `Attribute.Owner.MoveSpeedHorizontal` | 0 | Active |
| `MoveSpeedVertical` | `Attribute.Owner.MoveSpeedVertical` | 0 | Active |
| `MovementSlow` | `Attribute.Owner.MovementSlow` | İlk modifier'da auto-register 0 | Active in dirty worktree |
| `Armor` | `Attribute.Owner.Armor` | 0 | Active |
| `Luck` | `Attribute.Owner.Luck` | 0 | Active |
| `CriticalChance` | `Attribute.Owner.CriticalChance` | 0 | Active rating |
| `CriticalDamage` | `Attribute.Owner.CriticalDamage` | 1.5 | Active multiplier; 1.5 = 150% total critical damage |

`MovementSlow`, `InitializeOwnerAttributes` içinde açıkça register edilmez. `AttributeSystem::AddModifier` eksik attribute'i 0 ile register ettiği için effect ilk kez uygulandığında oluşur.

`EnergyPower Reference = 100` gelecekteki reaktör-gücü tasarımı için ortak
referanstır. Bu bir normalize etme kuralı değildir. Resource hesapları
tamamen lineerdir: `ReactorBudget = EnergyPower × 2`; hard cap veya diminishing
return yoktur. `ShieldAffinity + AfterburnerAffinity = 1.0` ship-profile
invariant'ı budget'ın iki resource'a dağıtımını tanımlar.

## Ship attribute'ları

Kaynak: `ShipAttributeIds`; storage sahibi `ShipRuntime::mAttributeSystem`.

| Grup | Tag'ler | Durum |
|---|---|---|
| Shield | MaxShield, ShieldRegen, ShieldRechargeDelay | Active; bazıları EnergyPower/config'den derived |
| Afterburner kapasite | AfterburnerCapacity, AfterburnerRegen, AfterburnerRechargeDelay | Active; kapasite/regen derived |
| Afterburner hareket | EnergyDrainPerSecond, SpeedMultiplier, AccelerationMultiplier | Active config values |
| Afterburner ramp | RampUpDuration, RampDownDuration, ManeuverabilityMultiplier | Active config values |

## Ortak definition attribute'ları

`CommonAttributeIds` değerleri genellikle `GameplayAttributeList` içinde definition/spec-local taşınır; otomatik olarak owner `AttributeSystem`'a register edilmez.

| Sembol | Tag | Tipik kullanım |
|---|---|---|
| `Cooldown` | `Attribute.Common.Cooldown` | Ability cooldown |
| `Damage` | `Attribute.Common.Damage` | Ability/weapon payload |
| `Radius` | `Attribute.Common.Radius` | Genel radius |
| `Duration` | `Attribute.Common.Duration` | Definition/runtime süresi |
| `Interval` | `Attribute.Common.Interval` | Tick/period aralığı |
| `FireRate` | `Attribute.Common.FireRate` | Weapon cadence |
| `Range` | `Attribute.Common.Range` | Delivery menzili |
| `ProjectileCount` | `Common.ProjectileCount` | Ability/projectile adetinin ortak base değeri |
| `PierceDamageLoss` | `Common.PierceDamageLoss` | Her başarılı pierce sonrasında kaybedilen hasar oranı; `0.20` mevcut hasarın `%20` azalmasıdır |
| `CollisionRadius` | `Attribute.Collision.Radius` | Collision boyutu |
| `AreaRadius` | `Attribute.Area.Radius` | Alan boyutu |

## Feature-local aileler

| Aile | Kaynak | Storage / kullanım | Bu aşamadaki ayrıntı |
|---|---|---|---|
| Barrier runtime | `EffectStructs.h::BarrierEffectSchema` | `ActiveGameplayEffect::runtimeAttributes` | Capacity, absorption, regen, delay |
| Damage payload/status | `DamageTypeConfig.h::DamageTypeSchema::Attribute` | Definition/config resolution | Ayrıntı Damage System aşamasına ertelendi |
| Primary weapon delivery/features | `WeaponStructs.h` | Weapon definition/runtime | Sonlu `PierceCount` ayrı kalır; hasar kaybı kullanan projectile aileleri `Common.PierceDamageLoss` kullanır |
| Rocket/Sun Beam/Gravity Anomaly | feature config header'ları | Ability actor definitions/spec | Ayrıntı Ability aşamasına ertelendi |
| Attachment | `AttachmentDefinition.h` | Attachment definition/runtime | Ayrıntı Attachment aşamasına ertelendi |

## Kritik gerçek kod

Dosya: `LightYearsGame/include/gameplay/attributes/AttributeIds.h`  
Sınıf veya namespace: `ly::OwnerAttributeIds`  
Fonksiyon: owner kimlik kataloğu  
Görevi: Çekirdek combat owner stat'lerini adlandırır.

```cpp
		inline static const GameplayTag AttackPower{ "Attribute.Owner.AttackPower" };
		inline static const GameplayTag AttackSpeed{ "Attribute.Owner.AttackSpeed" };
		inline static const GameplayTag AbilityHaste{ "Attribute.Owner.AbilityHaste" };
```

Dosya: `LightYearsGame/include/gameplay/attributes/AttributeIds.h`  
Sınıf veya namespace: `ly::CommonAttributeIds`  
Fonksiyon: ortak kimlik kataloğu  
Görevi: Definition-local ortak değer anahtarlarını adlandırır.

```cpp
		inline static const GameplayTag Cooldown{ "Attribute.Common.Cooldown" };
		inline static const GameplayTag Damage{ "Attribute.Common.Damage" };
		inline static const GameplayTag Radius{ "Attribute.Common.Radius" };
		inline static const GameplayTag Duration{ "Attribute.Common.Duration" };
```

Dosya: `LightYearsGame/src/gameplay/combat/CombatRuntime.cpp`  
Sınıf veya namespace: `ly::CombatRuntime`  
Fonksiyon: `InitializeOwnerAttributes`  
Görevi: Owner stat storage'ının başlangıç kayıtlarını oluşturur.

```cpp
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AttackPower, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AttackSpeed, 0.f);
		mAttributeSystem.RegisterAttribute(OwnerAttributeIds::AbilityHaste, 0.f);
```

## Durum ayrımı

### Aktif ve koddan doğrulandı

- Owner, ship, common ve barrier tablolarında Active olarak işaretlenen değerlerin runtime tanım ve tüketicileri bulundu.

### Planlanan

- Yukarıdaki tablolarda **Active** olarak işaretlenenler kodda sembol ve kullanım ile doğrulandı.
- `BALANCE_AND_ROADMAP_NOTEBOOK.md` içindeki boş ArmorScale, Shield multiplier ve Crit multiplier satırları şablondur; uygulanmış veya onaylanmış yeni formül değildir.
- Yeni attribute registry/linter veya data-driven schema planı doğrulanamadı.

### Tanımlı fakat aktif kullanımı doğrulanamadı

Merkezi `OwnerAttributeIds`, `ShipAttributeIds` ve `CommonAttributeIds` içindeki önemli üyelerin production tüketicileri bulundu. Feature-local yüzeyin tamamı bu görevde izlenmedi; özellikle her weapon/ability delivery attribute'ünün aktif handler tüketimi kapsam dışı olduğu için burada tek tek “Active” sayılmadı.

### Test-only

`GasLiteCoreTests.cpp` extension schema altında test-only attribute tag'leri tanımlar. Bunlar production kataloğuna eklenmemiştir.

## Doğrulama

- Literal araması tüm `Attribute.` tag ailelerini ilgili include/config köklerinde taradı.
- Kaynaklar generated/build/third-party içerikten ayrıldı.
- 2026-07-29 tamamlanan SAS geçişinden sonra alınan Debug build ve
  `LightYearsGasLiteCore`/`LightYearsEngineLifetime` 2/2 CTest sonucu
  tarihsel kayıttır; bu docs-only denetiminde yeniden çalıştırılmadı.
- Commit tabanı aynı; `MovementSlow` ve bazı feature-local şemalar dirty worktree'ye aittir.

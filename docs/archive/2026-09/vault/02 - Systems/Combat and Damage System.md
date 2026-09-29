---
type: system
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - damage type, effect behavior and combat integrations
  - GasLiteCoreTests
source_files:
  - LightYearsGame/include/gameplay/combat/Combatant.h
  - LightYearsGame/include/gameplay/combat/CombatRuntime.h
  - LightYearsGame/src/gameplay/combat/CombatRuntime.cpp
  - LightYearsGame/src/gameplay/combat/Combatant.cpp
  - LightYearsGame/include/gameplay/damage/DamageContext.h
  - LightYearsGame/src/gameplay/damage/DamageTypeSystem.cpp
  - LightYearsGame/src/spaceShip/SpaceShip.cpp
symbols:
  - ly::ApplyCombatDamage
  - ly::DamageContext
  - ly::DamagePayload
  - ly::CombatRuntime::ProcessIncomingDamage
  - ly::CombatRuntime::NotifyDamageResolved
  - ly::DamageTypeSystem::BuildPayload
related:
  - "[[CombatRuntime]]"
  - "[[Damage and Shield Resolution Flow]]"
  - "[[Shield System]]"
  - "[[Gameplay Effect System]]"
---

# Combat and Damage System

## 7 Eylül 2026 kaynak kontrolü

ApplyCombatDamage kaynak/target damage protection ve critical kararından DamageContext/ReceiveDamage yoluna gider. SpaceShip::ReceiveDamage combat mitigation/effect, shield, health ve NotifyDamageResolved sırasını izler. DamageTypeSystem::BuildPayload damage tag kimliği + source attribute değerlerini kullanır; tag tek başına balance statüsü değildir. Tüm status kombinasyonları çalıştırılmadı.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Kapsam

Bu not, combat hedefe gelen damage'in girişinden `DamageContext`, effect/armor/status işleme, ship shield/health uygulaması ve event bildirimlerine kadar doğrulanan üst seviye akışı açıklar. Projectile impact üretimi [[Projectile System]], weapon payload üretimi [[Weapon System]], status effect lifecycle ayrıntısı ise [[Gameplay Effect System]] kapsamındadır.

## Ana roller

| Bileşen | Doğrulanmış sorumluluk |
|---|---|
| `Combatant` | Combat-aware actor'ın `CombatRuntime` ve `ReceiveDamage` kontratı |
| `ApplyCombatDamage` | Crit çözümü, `DamageContext` kurulumu ve combat/non-combat ayrımı |
| `DamageContext` | Tek hit'in mutable sayısal akış kaydı |
| `DamagePayload` | Damage type'tan çözülmüş shield, armor, status ve crit parametreleri |
| `CombatRuntime` | Effect behavior, ortak hull Armor hesabı, status, ability event ve delegate koordinasyonu |
| `SpaceShip::ReceiveDamage` | Effect sonrası kalan damage'i legacy `ShieldComponent`, ortak hull Armor hesabı ve health'e uygular |
| `DamageTypeSystem` | Tag + source attribute'tan payload, status effect spec'leri ve effect behavior kayıtları |

## Giriş ve context

`ApplyCombatDamage`, pozitif incoming damage için önce source `Combatant` varsa critical chance'i çözer. Payload crit'e izin veriyorsa damage critical multiplier ile çarpılabilir. Target `Combatant` ise helper şu alanlarla `DamageContext` kurup `ReceiveDamage` çağırır: source, target, original/remaining damage, critical flag, damage tags ve payload. Combatant olmayan target için engine'in hafif `Actor::ApplyDamage` yolu kullanılır.

```cpp
struct DamageContext
{
    Actor* source = nullptr;
    Actor* target = nullptr;
    float originalDamage = 0.f;
    float remainingDamage = 0.f;
    float absorbedDamage = 0.f;
    float mitigatedDamage = 0.f;
    float modifiedDamage = 0.f;
    float appliedDamage = 0.f;
    bool wasCritical = false;
    List<GameplayTag> damageTags;
    DamagePayload payload;
};
```

## CombatRuntime processing

`SpaceShip::ReceiveDamage`, önce invulnerability ve non-positive gate'lerini uygular; sonra `CombatRuntime::ProcessIncomingDamage` çağırır. Bu methodun doğrulanan sırası:

1. Active gameplay-effect incoming-damage hook'ları çalışır ve pending effect event'leri ability sistemine gönderilir.
2. `DamageTypeSystem::ApplyStatusEffects`, incoming effect aşamasında remaining damage pozitifse target effect sistemine status spec'lerini uygular.
3. `SpaceShip`, kalan source damage'i `ShieldComponent::AbsorbDamage`a verir. Dönen değer source-damage cinsinden shield tarafından emilen miktardır; shield kapasitesinin tüketimi multiplier nedeniyle farklı olabilir.
4. Shield sonrasında kalan hull hasarı `CombatRuntime::ApplyHullDamageMitigation` ile ortak Armor hesabından geçer:
   - `baseReduction = max(0, Armor) / (max(0, Armor) + 100)`
   - `effectiveReduction = baseReduction * (1 - armorPenetration)`
   - `remainingDamage = damageBeforeArmor * (1 - effectiveReduction)`
   - Runtime eşdeğer sonucu sayısal güvenlik için doğrudan multiplier ile çözer: `finalMultiplier = baseMultiplier + armorPenetration * (1 - baseMultiplier)`.
   - Azalan miktar `mitigatedDamage` olur. Negatif Armor ilave bonus hasar üretmez (`max(0, Armor)`). Hard cap yoktur; sonlu Armor değerlerinde reduction %100'e ulaşmaz.
   - Breakpoint'ler: 50 Armor ≈ %33.333 DR, 100 Armor = %50 DR, 200 Armor ≈ %66.667 DR, 300 Armor = %75 DR, 500 Armor ≈ %83.333 DR.
   - Pipeline sırası: Crit -> incoming effects/status application -> Persistent Ship Shield/overshield -> hull Armor + Armor Penetration -> Hull/Health. Armor shield-only hit'leri değiştirmez.
5. Applied ignite status için source combatant'a source-ignite ability event'i gönderilir.
6. `onDamageProcessed` delegate'i yayınlanır; ship/summon kendi health çözümünden sonra `NotifyDamageResolved` çağırır.

Bu aşamadan sonra ship hedefi legacy shield ve health uygulamasını yapar. Ayrıntılı sıra: [[Damage and Shield Resolution Flow]].

## Damage type payload'ı

`DamageTypeSystem::BuildPayload`, hiyerarşik damage tag'lerinden ilk eşleşen type davranışını kurar, sonra source attributes ile declared override uygular ve güvenli aralıklara clamp eder. Doğrulanmış type varsayımları:

| Damage type | Payload etkisi |
|---|---|
| Energy | Shield damage multiplier ve ek shield regeneration delay |
| Kinetic | Stack tablosu + kaynak penetration bonusu |
| Thermal | Stack tablosu; yalnız özel periodic Burn parametreleri source-owned kalır |
| Cryo | Stack tablosu ve `MovementSlow` modifier çözümlemesi |
| Electric | Stack tablosu ve incoming-damage multiplier |

Status efektleri hedefin mevcut ASC effect instance'larına uygulanır. Dört seçili
status için `DecayAfterDuration` policy'si açıkça tanımlıdır; generic `Stack`
effect'leri bu davranışı miras almaz. Sayısal tablo `DamageStatusBalance` içinde
tek kaynaktır; effect JSON yalnız lifecycle/content sözleşmesini taşır.

## Event ve sonuç bildirimi

`CombatRuntime::NotifyDamageResolved`, `appliedDamage > 0` olduğunda hedefe `Event.Owner.DamageTaken`, source combatant'a `SourceDamageDealt` event'i gönderir; ardından `onDamageResolved` yayınlanır. `appliedDamage`, `SpaceShip` yolunda legacy shield'ın emdiği source damage ile health'ten gerçekten düşen miktarın toplamıdır.

## Sınırlar

- Damage pipeline aynı anda effect-based barrier ve legacy ship shield ile çalışabilir; ayrım için [[Shield System]]. Her iki shield katmanından sonra kalan hull hasarı tek ortak Armor hesabından geçer.
- Non-combat `Actor` hedeflerde armor/effect/shield context pipeline'ı yoktur.
- Exact crit rastgelelik dağılımı, status balance değerleri ve tüm combatant subclass'larının `ReceiveDamage` varyasyonları bu notta ayrıntılandırılmadı.
- Damage type seçiminde çoklu farklı type tag'in öncelik/birleşim matrisi, implementation'daki sıralı `if / else if` davranışı dışında ayrıca tasarım kuralı olarak doğrulanmadı.

## Kaynak doğrulaması

- İncelenen durum: dirty worktree.
- Directly read: Combatant helper, CombatRuntime, DamageContext/Payload, DamageTypeSystem ve SpaceShip receive path.
- 21 Eylül 2026 doğrulaması: `cmake --build build --parallel 4`, `cmake --build build --target LightYearsGame --parallel 4` ve `LightYearsGasLiteTests.exe` başarıyla çalıştı; test executable'ı exit code `0` döndürdü.

---
type: balance
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - ability cooldown, attachment and attribute integrations
source_files:
  - LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h
  - SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
  - SpaceAbilitySystem/include/attributes/AttributeMath.h
symbols:
  - sas::GameplayAbilityInstance::GetCooldownDuration
  - sas::AttributeMath::GetAbilityHasteReduction
  - sas::AttributeMath::GetAbilityCooldownMultiplier
related:
  - "[[Ability Cooldown and Charge Flow]]"
  - "[[AbilityInstance]]"
  - "[[Derived Attributes]]"
  - "[[Attribute System]]"
---

# Ability Cooldown and Charge Rules

## Aktif kurallar

| Parametre | Gerçek kaynak | Aktif formül/davranış | Kullanan sembol | Durum |
|---|---|---|---|---|
| Base cooldown | `GameAbilityDefinition::cooldown` (`sas::AbilityDefinition` alanı) | Instance definition kopyasının başlangıç cooldown değeri | `GameAbility::ResolveCooldownDuration` | Active |
| Final cooldown | Definition modifier'ları, ability attachment modifier'ları, owner haste | `max(0, attachmentModifiedCooldown × hasteMultiplier)` | `GetCooldownDuration` | Active |
| AbilityHaste | `OwnerAttributeIds::AbilityHaste` resolved current value | `GetCurrentValue` ile okunur | `GetCooldownDuration` | Active |
| Haste reduction | `AttributeMath::SaturatingFraction` | `clamp(1-exp(-haste/100), 0, 1)` | `GetAbilityHasteReduction` | Active |
| Cooldown multiplier | Haste reduction | `1 - hasteReduction` | `GetAbilityCooldownMultiplier` | Active |
| Minimum cooldown | Final dönüş değeri | `max(0, final)`; pozitif haste için eğri çarpanı sıfıra ulaşmaz | `GetCooldownDuration` | Active |
| Maximum charges | `GameAbilityDefinition::maxCharges` (`sas::AbilityDefinition` alanı) | Charge-enabled instance için üst değer | constructor, `UpdateCooldown` | Active |
| Initial charges | `definition.maxCharges` | Constructor'da doğrudan atanır | `sas::GameplayAbilityInstance::GameplayAbilityInstance` | Active |
| Charge recharge duration | Ayrı runtime/config alanı bulunamadı | Ayrı süre yok; cooldown tamamlanması reset tetikler | `UpdateCooldown` | Not supported |
| Recharge multiplier | Ayrı formül bulunamadı | Ayrı multiplier yok | — | Not supported |
| Cooldown-on-activation | `TryActivate` | Başarılı activation tek başına cooldown yazmaz; `Instant` policy aynı çağrıda `EndAbility`'ye iner | `TryActivate`, `EndAbility` | Active (policy-dependent) |
| Cooldown-on-end | `EndAbility` | Effective cooldown ile `mRuntimeState.EndActivation(cooldownOnEnd, maxCharges)` | `sas::AbilityRuntimeState` | Active |
| Cooldown-on-charge-depleted | Özel branch bulunamadı | Son charge için ayrı cooldown başlangıç kuralı yok; ability end olunca aynı end kuralı çalışır | `TryActivate`, `EndAbility` | Not supported |
| Trigger cooldown | `sas::AbilitySystemRuntime` içindeki ayrı tracker state'i | `GameplayAbilityInstance` cooldown/charge state'inden ayrıdır; `AbilityCooldownTracker` ile tick edilir | `AbilitySystemRuntime::HandleGameplayEvent` | Active |

## Okunabilir formül

```text
LeveledCooldown = CalculateModifiedAttributeValue(BaseCooldown, definition.attributeModifiers)
AttachmentCooldown = ApplyAttachmentModifiers(LeveledCooldown)
HasteReduction = clamp(1 - exp(-AbilityHaste / 100), 0, 1)
HasteMultiplier = 1 - HasteReduction
FinalCooldown = max(0, AttachmentCooldown × HasteMultiplier)
```

## Örnek hesap

**Örnek; aktif balance değeri değildir.** Base/leveled/attachment cooldown'ın `10 s`, AbilityHaste'in `100` olduğu varsayılırsa:

```text
HasteReduction = 1 - exp(-1) ≈ 0.6321
HasteMultiplier ≈ 0.3679
FinalCooldown ≈ 10 × 0.3679 = 3.679 s
```

## Sınırlar

- `maxCharges > 0` iken gate, charge `0` veya altındaysa activation'ı reddeder; başarılı activation bir charge düşürür; cooldown sonunda değer `maxCharges` olarak atanır.
- `sas::AbilityRuntimeState` içinde kademeli/per-charge refill yoktur. Geçerli non-negative `maxCharges` definition'larında normal akış `0..maxCharges` aralığında kalır; cooldown tamamlanınca charge sayısı doğrudan `maxCharges` değerine döner.
- AbilityHaste için negatif değer `SaturatingFraction` içinde `0` reduction üretir. Pozitif sonlu haste, eğrinin asimptotik karakteri nedeniyle multiplier'ı sıfıra indirmez.

## Sonraki inceleme

- Definition validation'ın negatif `maxCharges` veya geçersiz cooldown değerlerini nasıl reddettiği.
- Attachment/level modifier producer'larının tümü.
- Trigger cooldown'ın event sistemiyle ayrıntılı ilişkisi.

---
type: system
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - barrier effect, damage and presentation integrations
source_files:
  - LightYearsGame/include/gameplay/ShieldComponent.h
  - LightYearsGame/src/gameplay/ShieldComponent.cpp
  - LightYearsGame/src/spaceShip/SpaceShip.cpp
  - SpaceAbilitySystem/include/AbilitySystemComponent.h
  - LightYearsGame/src/gameplay/effects/LightYearsEffectBehaviorRuntime.cpp
  - LightYearsGame/src/gameplay/effects/content/barrier/BarrierEffectBehavior.cpp
  - LightYearsGame/src/gameplay/ability/shield/ShieldAbility.cpp
symbols:
  - ly::ShieldComponent
  - ly::ShieldComponent::AbsorbDamage
  - ly::BarrierEffectBehavior::ProcessIncomingDamage
  - ly::ShieldAbility::Validate
related:
  - "[[Combat and Damage System]]"
  - "[[Damage and Shield Resolution Flow]]"
  - "[[ShieldComponent]]"
  - "[[Gameplay Effect System]]"
---

# Shield System

## 20 Eylül 2026 kaynak kontrolü

ShieldComponent source-owned TemporaryOvercapLedger ile temporary overshield hold/decay/consume yönetir; HealthComponent tarafında da overcap desteği vardır. Normal shield ve Barrier effect ayrı kalır. SpaceShip::UpdateRegeneration temporary decay'i normal recharge'dan ayrı tick eder. Ayrıntı: [[Shield and Energy]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Önemli ayrım

Kodda iki farklı shield katmanı vardır; bunlar aynı nesne veya aynı lifecycle değildir.

| Katman | State sahibi | Damage sırasındaki konum | Regeneration |
|---|---|---|---|
| Ship shield | `SpaceShip::mShieldComponent` | Incoming effect hook sonrasında, hull Armor öncesinde kalan damage | `SpaceShip::UpdateRegeneration` / ship attributes |
| Barrier effect | `sas::AbilitySystemComponent` içindeki typed `ActiveGameplayEffect::runtimeAttributes` | `CombatRuntime::ProcessIncomingDamage` içindeki effect hook aşaması | `BarrierEffectBehavior::Tick` |

Bu katmanlar aynı hit üzerinde ardışık çalışabilir. Barrier ve diğer incoming effect behavior'ları önce damage'i azaltabilir; ship shield ardından kalan source damage'i emer; shield sonrasında kalan hull damage'i ortak Armor hesabı azaltır; en son health etkilenir.

## Ship shield: ShieldComponent

`ShieldComponent`, current/max shield, base recharge delay ve pending recharge delay state'ini taşır. `AbsorbDamage(sourceDamage, shieldDamageMultiplier, extraRechargeDelay)`, shield capacity'den multiplier'lı miktarı harcar fakat çağırana emilen *source damage* miktarını döndürür; böylece yüksek shield multiplier kısmi kırılmada ekstra hull damage üretmez.

Damage, recharge delay'i en az `base recharge delay + payload extra delay` değerine taşır. `Tick`, recharge izinliyse önce delay'i tüketir, kalan frame zamanında regeneration uygular. `SpaceShip`, max shield/recharge delay'i `ShipRuntime` attribute'larından yeniler ve recharge iznini `!IsAfterburnerRechargeBlocked()` ile belirler.

`onShieldChanged` ve `onShieldDamaged` delegate'leri component yüzeyinde vardır; bu aşamada bütün UI subscriber'ları doğrulanmadı.

## Barrier effect

`BarrierEffectBehavior`, effect runtime attributes içindeki capacity, absorption ratio ve regeneration değerlerini kullanır. Incoming damage'da capacity, `remainingDamage * absorptionRatio * shieldDamageMultiplier` kadar harcanır; context remaining/absorbed alanları source-damage eşdeğeriyle güncellenir. Capacity sıfıra düşerse effect removal talep eder ve barrier broken event'i üretir.

Barrier damage sonrası runtime regeneration delay, effect base delay ve payload extra shield delay'e göre yükselir. Effect tick'i delay'i tüketir ve capacity'yi base capacity'ye kadar yeniler. Stack eklendiğinde capacity base/current değeri stack capacity ile artar.

`ShieldAbility`, yalnız OnActivate phase'inde `ApplyEffectAction` içeren definition'ı kabul eden behavior validation sınıfıdır; shield gameplay etkisini doğrudan kendisi spawn etmez.

## Temporary overshield: Shield Harvest

`ShieldHarvestAbility`, Barrier effect değildir. 1.5 saniyelik odaklanma sonunda
700 birim içindeki opposing combatant'ları bir kez sayar ve `ShieldComponent`
üzerinden her hedef başına 40 temporary overshield verir. Bu katman 5 saniye
tutulur, ardından 100/s hızla azalır. Sayım sonrası telegraph completion feedback
gösterir; alanın sonraki değişimleri grant miktarını değiştirmez.

## Presentation sınırı

Shield effect visual content ve `ShieldVisual` dosyaları vardır. Gameplay shield state'in sahibi değildirler; effect visual registry üzerinden active effect snapshot/state'i tüketirler. Görsel ayrıntılar bu aşamanın dışındadır.

## Sınırlar ve belirsizlikler

- Legacy ship shield ile barrier effect'in tasarımsal olarak birlikte mi yoksa alternatif mi kullanılması gerektiği koddan kesinleşmedi; mevcut pipeline ikisini ardışık destekler.
- ShieldComponent'in player/enemy dışındaki tüm `Combatant` implementasyonlarındaki kullanımı ayrıntılı taranmadı.
- Energy/afterburner sisteminin recharge blocking kuralları yalnız shield bağlantısı düzeyinde doğrulandı; ayrı Energy aşamasında ele alınmalıdır.

## Kaynak doğrulaması

- Son doğrulanan commit: `b2e24c11d157c64b89bd1cf47c1390bf72784056`.
- Directly read: ShieldComponent, SpaceShip receive/regeneration, barrier behavior ve shield ability validation.
- Test/build bu aşamada çalıştırılmadı.

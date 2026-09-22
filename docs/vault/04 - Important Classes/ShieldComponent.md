---
type: important-class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsGame/include/gameplay/ShieldComponent.h
  - LightYearsGame/src/gameplay/ShieldComponent.cpp
  - LightYearsGame/src/spaceShip/SpaceShip.cpp
symbols:
  - ly::ShieldComponent
  - ly::ShieldComponent::AbsorbDamage
  - ly::ShieldComponent::Tick
related:
  - "[[Shield System]]"
  - "[[Damage and Shield Resolution Flow]]"
  - "[[Combat and Damage System]]"
---

# ShieldComponent

## 7 Eylül 2026 kaynak kontrolü

Temporary overcap kaynaklarını TemporaryOvercapLedger izler; GrantTemporaryOvershield, TickTemporaryOvershields ve AbsorbDamage bu ledger'a bağlı. Normal regen blocking source-id ile yönetilir. [[Shield and Energy]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Rol

`ly::ShieldComponent`, ship üzerinde kalıcı current/max shield ve recharge delay state'ini tutan value component'tir. `SpaceShip` bunu owner olarak tutar; temporary barrier gameplay effect'i bu sınıf değildir ve [[Shield System]] içinde ayrı katman olarak açıklanır.

## Public yüzey

Dosya: `LightYearsGame/include/gameplay/ShieldComponent.h`

```cpp
float AbsorbDamage(float sourceDamage, float shieldDamageMultiplier,
    float extraRechargeDelay);
void Tick(float deltaTime, float regenerationPerSecond,
    bool allowRecharge = true);
void SetMaxShield(float maxShield, bool preserveShieldPercent = false);
void SetRechargeDelay(float rechargeDelay);
```

## Damage dönüş değeri

`AbsorbDamage`, shield capacity'den harcanan değeri değil, buna karşılık gelen source damage miktarını döndürür. Örneğin multiplier `> 1` ise aynı source damage shield'dan daha fazla capacity harcar; shield kısmi kırıldığında çağırana yanlışlıkla fazla remaining damage bırakılmaz.

Method her positive damage girişinde recharge delay'i `max(mevcut delay, base delay + extra delay)` ile uzatır. Shield veya multiplier sıfırsa damage emmez; yine delay güncellemesi koşulları methodun girişinde uygulanır.

## Recharge ve state değişimi

`Tick`, recharge izinli değilse, non-positive delta ise veya shield zaten full ise çıkış yapar. Önce pending delay'i delta içinden tüketir; ardından kalan sürede positive regeneration per second ile current shield'i max shield'e kadar artırır.

`SetMaxShield`, yeni maximum'u clamp eder. İstenirse previous shield yüzdesini korur; önceki maximum sıfırken positive maximum verilirse shield'i full başlatır. `SetRechargeDelay` yalnız base delay'i değiştirir; mevcut remaining delay'i doğrudan resetlemez.

## Bildirimler ve sahiplik

- `onShieldChanged(delta, current, max)`: current shield gerçekten değiştiğinde yayınlanır.
- `onShieldDamaged(absorbedShieldDamage, current, max)`: shield capacity harcandığında yayınlanır.
- Component `SpaceShip::mShieldComponent` value member'ıdır; actor lifetime'ıyla yaşar.
- `SpaceShip::UpdateRegeneration`, ship runtime attribute'larından regen/deley değerlerini aktarır ve afterburner durumuna göre recharge'i durdurabilir.

## Sınırlar

- Component armor, status, crit veya gameplay effect hook'larını uygulamaz; bunlar combat pipeline'ın diğer katmanlarındadır.
- Energy/afterburner state'i yalnız `allowRecharge` flag'i üzerinden bu sınıfa ulaşır.
- UI delegate subscriber'larının tamamı bu aşamada doğrulanmadı.

## İlgili notlar

- [[Shield System]]
- [[Damage and Shield Resolution Flow]]
- [[Combat and Damage System]]

## Kaynak doğrulaması

- Son doğrulanan commit: `b2e24c11d157c64b89bd1cf47c1390bf72784056`.
- Header/implementation ve SpaceShip çağrı noktaları doğrudan okundu; test/build bu aşamada çalıştırılmadı.

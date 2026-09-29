---
type: walkthrough
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - SpaceAbilitySystem static-library migration (Attribute phase)
  - attribute revision consumers and runtime integrations
  - GameplayAttribute and movement integration
source_files:
  - LightYearsEngine/include/framework/Delegate.h
  - SpaceAbilitySystem/include/attributes/AttributeSystem.h
  - SpaceAbilitySystem/src/attributes/AttributeSystem.cpp
  - LightYearsGame/src/gameplay/ship/ShipRuntime.cpp
  - LightYearsGame/src/spaceShip/SpaceShip.cpp
  - LightYearsGame/src/gameplay/ability/actions/AbilityActionAttributeResolver.cpp
  - LightYearsGame/src/gameplay/ability/actions/FireWeaponActionRuntime.cpp
symbols:
  - sas::AttributeSystem::Recalculate
  - sas::AttributeSystem::GetRevision
  - sas::AttributeSystem::onAttributeChanged
  - ly::ShipRuntime::OnOwnerAttributeChanged
  - ly::SpaceShip::OnRuntimeAttributeChanged
  - ly::AbilityActionAttributeResolver::ResolveAttributes
  - ly::FireWeaponActionRuntime::Execute
  - ly::FireWeaponActionRuntime::Tick
  - ly::Delegate::BindAction
  - ly::Delegate::Broadcast
related:
  - "[[Attribute System]]"
  - "[[Attribute Resolution Flow]]"
  - "[[Derived Attributes]]"
  - "[[Frame Update Flow]]"
---

# Attribute Change Notification Flow

## Amaç

Resolved attribute değişiminden delegate subscriber'larına ve revision tabanlı cache invalidation'a kadar iki ayrı notification mekanizmasını gösterir.

Notification üreticisi artık `sas::AttributeSystem` olarak statik
`SpaceAbilitySystem` target'ındadır. Subscriber'lar oyun tarafında kalır; bu
bağımlılık yönü SAS → Game değil, Game → SAS olarak korunur.

## Akış

```mermaid
sequenceDiagram
    participant Mut as Attribute mutation
    participant AS as AttributeSystem
    participant Ship as ShipRuntime
    participant Actor as SpaceShip
    participant Cache as FireWeaponActionRuntime cache
    Mut->>AS: Recalculate(id)
    AS->>AS: compare previous/current
    alt value changed
        AS->>AS: ++mRevision
        AS-->>Ship: onAttributeChanged(id, old, new)
        AS-->>Actor: onAttributeChanged(id, old, new)
        Ship->>AS: SetBaseValue(derived values)
        Actor->>Actor: refresh components/movement
    end
    Cache->>AS: GetRevision()
    alt revision differs
        Cache->>Cache: rebuild resolved attributes
    end
```

## İki kanal

1. **Push:** `onAttributeChanged` yalnız exact float değeri değiştiğinde yayınlanır.
2. **Pull/cache key:** `GetRevision()` tüketicilerin önbelleğinin güncel olup olmadığını anlamasını sağlar.

`ShipRuntime`, owner `MaxHealth`/`EnergyPower` değişimini dinleyip derived owner ve ship değerlerini tekrar yazar. `SpaceShip`, owner ve ship attribute delegate'lerini ayrı ayrı dinleyip health/shield/energy/movement component state'ini günceller.

## Binding, sıra ve lifetime

- `SpaceShip` `GetWeakPtr()` overload'unu kullanır. Listener ölürse callback `false` döndürür ve sonraki `Broadcast` sırasında listeden silinir.
- `ShipRuntime` raw `this` overload'unu kullanır. Bu overload lifetime izlemez; açık unbind API'si bulunamadı. Güvenlik, `ShipRuntime` ile owner attribute delegate'inin sahiplik sırasına bağlıdır.
- Callback'ler vector ekleme sırasıyla dolaşılır. Broadcast sırasında yeni callback bind edilmesi veya aynı delegate collection'ının reentrant değiştirilmesi için açık güvence yoktur.
- `AddModifier` ve `RemoveModifier` her handle için ayrı `Recalculate` çağırır. Effect birden fazla modifier taşıyorsa notification batch edilmez; gerçekten değişen her attribute için ayrı broadcast oluşabilir.
- Aynı resolved değere ulaşılırsa `previousValue != value` false olur ve `onAttributeChanged` çalışmaz.

## Kritik gerçek kod

Dosya: `SpaceAbilitySystem/include/attributes/AttributeSystem.h`  
Sınıf veya namespace: `sas::AttributeSystem`  
Fonksiyon: notification yüzeyi  
Görevi: Revision getter ve üç delegate'i dışarı açar.

```cpp
		uint64_t GetRevision() const { return mRevision; }
```

```cpp
		Delegate<GameplayTag, float, float> onAttributeChanged;
		Delegate<GameplayTag> onAttributeRegistered;
		Delegate<> onAttributesCleared;
```

Dosya: `LightYearsGame/src/gameplay/ship/ShipRuntime.cpp`  
Sınıf veya namespace: `ly::ShipRuntime`  
Fonksiyon: `OnOwnerAttributeChanged`  
Görevi: Yalnız derived değerleri etkileyen owner stat'lerinde yeniden hesaplar.

```cpp
		if (attributeId == OwnerAttributeIds::MaxHealth || attributeId == OwnerAttributeIds::EnergyPower)
		{
			RecalculateAttributes();
		}
```

Dosya: `LightYearsGame/src/spaceShip/SpaceShip.cpp`  
Sınıf veya namespace: `ly::SpaceShip`  
Fonksiyon: `BeginPlay`  
Görevi: Owner ve ship attribute sistemlerine weak-owner delegate binding kurar.

```cpp
		mCombatRuntime.GetAttributes().onAttributeChanged.BindAction(GetWeakPtr(), &SpaceShip::OnRuntimeAttributeChanged);
		mShipRuntime.GetAttributes().onAttributeChanged.BindAction(GetWeakPtr(), &SpaceShip::OnShipAttributeChanged);
		RefreshMovementAttributesFromRuntime();
```

Dosya: `LightYearsGame/src/gameplay/ability/actions/FireWeaponActionRuntime.cpp`  
Sınıf veya namespace: anonymous namespace helper  
Fonksiyon: `ResolveAttributes`  
Görevi: Attribute, attachment veya configuration revision değişince resolved weapon attribute cache'ini yeniler.

```cpp
			if (!state.hasResolvedAttributes ||
				state.resolvedAttributeRevision != attributeRevision ||
				state.resolvedAttachmentRevision != attachmentRevision ||
				state.resolvedConfigurationRevision != configurationRevision)
			{
```

Dosya: `LightYearsEngine/include/framework/Delegate.h`  
Sınıf veya namespace: `ly::Delegate`  
Fonksiyon: weak `BindAction` callback'i  
Görevi: Ölü Object listener'ını çağırmaz ve cleanup için `false` döndürür.

```cpp
					(static_cast<ClassName*>(obj.lock().get())->*callback)(args...);
```

Dosya: `LightYearsEngine/include/framework/Delegate.h`  
Sınıf veya namespace: `ly::Delegate`  
Fonksiyon: raw-pointer `BindAction`  
Görevi: Lifetime kontrolü olmadan object pointer'ını çağırır.

```cpp
				if (obj != nullptr)
				{
					(obj->*callback)(args...);
					return true;
				}
				return false;
```

## Test ve kaynak doğrulaması

- Attribute değer sonuçları çekirdek testte doğrulanır.
- `ShipRuntime` derived değerleri ve component kapasite güncellemeleri entegrasyon testlerinde dolaylı kapsanır.
- Delegate'in tam bir kez yayınlanması, unchanged value sessizliği ve revision artış miktarı için izole assertion bulunamadı.
- `GetRevision` production tüketicisi artık `FireWeaponActionRuntime.cpp` içindeki resolved-attribute cache helper'ıdır; literal arama `ShipRuntime` ve `SpaceShip` binding'lerini doğruladı.
- Debug/Release `exit 0` ve 2026-07-29 CTest 2/2 sonucu tarihsel kayıttır;
  bu docs-only denetiminde güncel build/test çalıştırılmadı.

## Riskler

- Callback zinciri synchronous'tur; callback içinden başka attribute değişimi yeni broadcast üretebilir.
- Exact float `!=`, küçük hesap farklarını notification kabul eder.
- `RegisterAttribute` değer değişiminde revision'ı `Recalculate` içinde ve kayıt sonunda iki kez artırabilir; revision “değişiklik sayısı” değil cache epoch'u olarak ele alınmalıdır.
- Raw-pointer binding için automatic expiration/unsubscribe yoktur; `ShipRuntime` lifetime sırası korunmalıdır.

## Okuma sırası

1. `AttributeSystem` delegate/revision yüzeyi
2. `Recalculate`
3. `ShipRuntime::InitializeFromShipDefinition`
4. `ShipRuntime::OnOwnerAttributeChanged`
5. `SpaceShip::BeginPlay` ve callback'ler
6. `FireWeaponActionRuntime::ResolveAttributes` ve `AbilityActionAttributeResolver::ResolveAttributes`

---
type: architecture
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - GAS-Lite gameplay-effect runtime ownership and presentation cleanup
  - ability, damage, movement and test integrations
source_files:
  - LightYearsEngine/include/framework/Application.h
  - LightYearsEngine/include/framework/World.h
  - LightYearsEngine/include/framework/Actor.h
  - LightYearsEngine/include/framework/Object.h
  - LightYearsEngine/include/framework/TimerManager.h
  - LightYearsEngine/src/framework/TimerManager.cpp
  - LightYearsGame/include/spaceShip/SpaceShip.h
  - LightYearsGame/include/gameplay/ability/actors/AbilityWorldActor.h
  - LightYearsGame/include/presentation/effects/GameplayEffectVisual.h
symbols:
  - ly::Application
  - ly::World
  - ly::Actor
  - ly::Object::GetWeakPtr
  - ly::Timer
  - ly::AbilityWorldActor
  - ly::GameplayEffectVisual
related:
  - "[[Object Lifecycle]]"
  - "[[Actor and World System]]"
  - "[[Object]]"
  - "[[TimerManager]]"
  - "[[CombatRuntime]]"
  - "[[GameplayEffectSystem]]"
---

# Ownership and Lifetime

> GAS-Lite doğrulama eki: `CombatRuntime`, `mAbilitySystemComponent` üzerinden attribute, owned-tag, effect ve ability sistemlerini value/owned üyeler olarak sahiplenir. Typed effect runtime, active effect state'i value olarak tutar; source/context kontratı non-owning pointer/shared ownership ayrımını korur. Ayrıntı: [[CombatRuntime]], [[GameplayEffectSystem]] ve [[Gameplay Effect Duration and Removal Flow]].

## Sahiplik özeti

| İlişki | Saklama biçimi | Bellek sahibi mi? | Doğrulanmış anlam |
|---|---|---|---|
| `main` → Application | `std::unique_ptr<Application>` | Evet | Process-level uygulama sahibi |
| Application → World | `shared_ptr` current + pending | Evet | Aktif ve geçiş bekleyen world |
| World → Actor | `List<shared_ptr<Actor>>` | Evet | Pending ve aktif actor sahipliği |
| Actor → World | raw `World*` | Hayır | Actor’ın runtime container erişimi |
| World → view target | `weak_ptr<Actor>` | Hayır | Kamera hedefi actor yaşamını uzatmaz |
| Object callbacks/timers | `weak_ptr<Object>` | Hayır | Listener ölünce callback/timer geçersizleşir |
| SpaceShip → components | value members | Evet | Component ömrü gemi nesnesine bağlı |
| `AbilityWorldActor` → owner | `weak_ptr<Actor>` | Hayır | Owner ömrünü uzatmaz; expired olduğunda güvenli null |
| `AbilityWorldActor` → source ability | `AbilityHandle` + owner ASC lookup | Hayır | Loadout değişiminde ham ability pointer saklanmaz |
| Bazı feature-local visual actor → owner | raw veya weak (sınıfa göre) | Hayır | Henüz tek engine-wide owner semantiği yok |
| GameplayEffectVisual → owner | `weak_ptr<Actor>` | Hayır | Görsel owner’ı yaşatmaz |

Generic `Actor` sınıfında owner/parent alanı yoktur. `mOwningWorld` container bağlantısıdır. “Owner” ilişkisi feature-local sınıflarda tanımlanır; dolayısıyla tek bir engine-wide owner semantiği bulunmaz.

## Kritik gerçek kod

Dosya: `LightYearsEngine/include/framework/Application.h`  
Sınıf veya namespace: `ly::Application`  
Fonksiyon: sınıf alanları / `LoadWorld`  
Görevi: Current ve pending world’leri güçlü referansla tutmak.

```cpp
shared_ptr<World> mCurrentWorld;
shared_ptr<World> mPendingWorld;
```

```cpp
auto newWorld = std::make_shared<WorldType>(this);
mPendingWorld = newWorld;
return newWorld;
```

Dosya: `LightYearsEngine/include/framework/World.h`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: sınıf alanları  
Görevi: Actor/HUD/stage sahipliğini ve Application’a non-owning bağlantıyı tanımlamak.

```cpp
Application* mOwningApp;
bool mBeganPlay;
bool mIsPaused;

List<shared_ptr<Actor>> mActors;
List<shared_ptr<Actor>> mPendingActors;
List<shared_ptr<GameStage>> mGameStages;
List<shared_ptr<GameStage>>::iterator mCurrentStage;
shared_ptr<HUD> mHUD;
shared_ptr<HUD> mOverlayHUD;
```

Dosya: `LightYearsEngine/include/framework/Actor.h`  
Sınıf veya namespace: `ly::Actor`  
Fonksiyon: sınıf alanı  
Görevi: Actor’ın world’e non-owning raw pointer tuttuğunu göstermek.

```cpp
World* mOwningWorld;
bool mBeganPlay;
```

Dosya: `LightYearsGame/include/spaceShip/SpaceShip.h`  
Sınıf veya namespace: `ly::SpaceShip`  
Fonksiyon: sınıf alanları  
Görevi: Component-benzeri nesnelerin value composition ile SpaceShip tarafından sahiplenildiğini göstermek.

```cpp
HealthComponent mHealthComponent;
ShieldComponent mShieldComponent;
EnergyComponent mEnergyComponent;
CombatRuntime mCombatRuntime;
ShipRuntime mShipRuntime;
MovementComponent mMovementComponent;
```

Dosya: `LightYearsGame/include/gameplay/ability/actors/AbilityWorldActor.h`  
Sınıf veya namespace: `ly::AbilityWorldActor`  
Fonksiyon: owner alanı  
Görevi: Projectile/ability actor owner ilişkisinin weak ve non-owning olduğunu göstermek.

```cpp
weak_ptr<Actor> mOwner;
sas::AbilityHandle mSourceAbilityHandle;
float mDamage;
List<GameplayTag> mDamageTags;
```

Dosya: `LightYearsGame/include/presentation/effects/GameplayEffectVisual.h`  
Sınıf veya namespace: `ly::GameplayEffectVisual`  
Fonksiyon: `GetVisualOwner` / owner alanı  
Görevi: Bir visual actor türünün owner yaşamını `weak_ptr` ile izlemesini göstermek.

```cpp
shared_ptr<Actor> GetVisualOwner() const { return mOwner.lock(); }
virtual void TickVisual(float deltaTime) = 0;

private:
	weak_ptr<Actor> mOwner;
```

Dosya: `LightYearsEngine/src/framework/TimerManager.cpp`  
Sınıf veya namespace: `ly::Timer`  
Fonksiyon: `IsExpired`  
Görevi: Listener yok olduğunda veya pending-destroy olduğunda timer’ı geçersiz saymak.

```cpp
return mIsExpired || mListener.first.expired() || mListener.first.lock()->GetIsPendingDestroy();
```

## Koddan doğrulanan davranışlar

- World, actor belleğinin gerçek sahibidir; caller’a `weak_ptr` döndürür.
- Caller ayrıca `shared_ptr` kilitler ve saklarsa actor, world koleksiyonundan çıkarıldıktan sonra da C++ nesnesi olarak yaşamaya devam edebilir. Ancak `pendingDestroy` true kalır ve runtime katılımı durur.
- Actor’ın world pointer’ı world’ü yaşatmaz. Normal world yıkımında actor container’ları world member destruction sırasında bırakıldığı için actor destructor’ları world ömrünün sonunda çalışır; destructor kodu world pointer’ını kullanmaz.
- Generic parent-child transform veya generic component container yoktur. Component örnekleri value member’dır ve owner class ile aynı ömre sahiptir.
- Timer callback’leri `weak_ptr<Object>` listener saklar; listener expired/pending ise callback çağrılmadan timer expired olur.
- Weak delegate binding expired listener’ları broadcast sırasında temizler. Raw-pointer `Delegate::BindAction` overload’u ise lifetime kontrolü yapmaz.
- `AbilityWorldActor` owner’ı weak pointer, source ability'yi handle olarak
  tutar ve gerektiğinde owner `Combatant` ASC'sinden çözer. Stack üzerinde
  oluşturulan izole test owner'ları için weak üretilemediğinde dar bir unmanaged
  fallback vardır; World-spawn runtime owner'larında kullanılmaz.
- `ContinuousBeamVisualActor` gibi bazı feature-local visual actor'larda raw
  owner hâlâ bulunur; `GameplayEffectVisual` owner’ı weak pointer tutar. Owner
  semantiği tüm presentation sınıflarında henüz aynı değildir.
- `CombatRuntime`, `mAbilitySystemComponent` ve effect presentation binding'i value member olarak sahiplenir; SAS component attribute/tag/effect/ability storage'ını kendi lifetime'ında tutar.
- `ActiveGameplayEffect`, source object/scope'u non-owning `const void*`, typed runtime context'i `shared_ptr` olarak saklar; hedef owner erişimi `CombatRuntime`/game component callback bağından gelir.
- `AbilitySystemComponent`/typed effect runtime süre için TimerManager kullanmaz. Normal `CombatRuntime::Clear`, ability/effect cleanup ve presentation temizliğini component sınırları üzerinden uygular.

## Registry yaşam döngüsü kuralları (zorunlu)

Cross-frame veya persistent registry'ler (observer listeleri, reflection/interception kayıtları, alan kayıtları) şu kurallara uyar:

1. **Kimlik:** Registry anahtarı olarak ham `Actor*` / `Object*` kullanılamaz. `Object::GetUniqueID()` kullanılır; değer monotoniktir ve geri dönüştürülmez.
2. **Erişim:** Entry actor'a yalnızca `weak_ptr<Actor>` üzerinden erişir; `lock()` ile yaşam doğrulanır, expired girdi temizlenir. Ham pointer dereference edilmez.
3. **Non-Object receiver'lar:** Actor olmayan bir politika nesnesi saklanacaksa sahiplik **RAII kayıt token'ı** ile ifade edilir; token'ın yıkıcısı kaydı siler. Böylece doğruluk, `End()`/teardown callback'inin her yok etme yolunda çağrılmasına bağlı kalmaz.
4. **Kayıt semantiği:** Aynı anahtar için ikinci kayıt sessizce eskisini koruyamaz; açıkça **replace** veya **reject** edilir. Eski token yeni kaydı silemez (generation kontrolü).
5. **Kapsam:** World'e özgü gameplay registry'leri process-global ömür taşımamalıdır; hedef, World-sahipli bir servistir.

**Kanonik örnek:** `ClosedCircuitFieldActor` — `unordered_map<unsigned int, weak_ptr<ClosedCircuitFieldActor>>`, `GetUniqueID()` ile anahtarlı, weak değer.
**Düzeltilmiş karşı örnek:** `ProjectileReflectionService::Receivers()` — ham `Actor*` anahtar + ham `ProjectileReflectionReceiver*` değer, fonksiyon-içi `static`. `GetUniqueID()` + `weak_ptr<Actor>` + RAII `Registration` token'ına taşındı.
**Aynı sınıfın düzeltilmiş ikinci örneği:** `GameHUD::mObservedDamageShips` — ham `SpaceShip*` set'i; `GetUniqueID()` set'ine taşındı.

> Not: 5. madde (World-sahipli servis) henüz uygulanmadı. `ProjectileReflectionService` yaşam güvenliği açısından düzeltildi; kapsam olarak hâlâ fonksiyon-içi `static` ve bu nedenle tüm World'ler tarafından paylaşılıyor. Entry'ler weak handle ile doğrulandığı için cross-world etkileşim oluşmaz; kalan iş kapsülleme iyileştirmesidir.

## İncelenmesi gereken riskler

- **Kalan tasarım riski:** `ContinuousBeamVisualActor` ve bazı feature-local
  visual actor owner alanları raw pointer'dır. Ortak `AbilityWorldActor` owner ve
  source ability dangling riski weak/handle çözümüne taşınmıştır.
- **Doğrulanmış tasarım riski:** `Delegate::BindAction(ClassName*)` overload’u yalnızca null kontrolü yapar; pointee ömrünü izlemez. Kullanım yerlerinin hepsinin lifetime garantisi bu görevde doğrulanmadı.
- **Doğrulanmış tasarım riski:** Raw-pointer delegate binding owner ömrünü otomatik izlemez. Delegate artık handle/unbind API’si sunsa da her raw subscriber’ın handle saklama ve lifecycle sonunda unbind etme kontratı kullanım yerinde doğrulanmalıdır.
- **Teorik/inceleme gerekli:** Combat owner `CombatRuntime::Clear` çağrılmadan yok edilirse value member destructor'ları belleği bırakır; ancak normal `RemoveEffectAt` tag/modifier/visual callback sırasının çalıştığı doğrulanamadı.
- **Teorik/inceleme gerekli:** Effect source/sourceScope non-owning olduğundan effect source'tan uzun yaşarsa bu pointer'ları kullanan feature behavior'larının validity kontratı ayrıca test edilmelidir.
- **Teorik/inceleme gerekli:** Effect delegate callback'i removal/apply yaparak `mActiveEffects` listesini reentrant değiştirirse iterator/reference güvenliği için açık kontrat veya test yoktur.
- **Teorik/inceleme gerekli:** Dış kod bir actor’ın `shared_ptr`ını destroy sonrası tutabilir. Bu bellek açısından geçerlidir fakat runtime-dışı, pending bir nesneye method çağırma semantiği merkezi olarak engellenmez.
- **Teorik/inceleme gerekli:** `Application::LoadWorld` sonrası `mPendingWorld`, promotion tamamlandığında resetlenmez; current ve pending aynı world’e iki `shared_ptr` tutar. Switch/shutdown kodu ikisini de bırakır, fakat bu çift referansın gelecekteki geçiş varsayımları test edilmelidir.
- **Teorik/inceleme gerekli:** `Application::Run` içinden exception çıkarsa `Application` default destructor’ı subsystem shutdown sırasını açıkça çağırmaz. Normal kapanış yolu güvenli; exception cleanup davranışı izole test edilmemiştir.

## Scene/world kapanışı

World switch sırasında eski `mCurrentWorld` bırakılır, game/global timer’lar temizlenir, Box2D world temizlenip yeniden kurulur, sonra pending world current olur ve `BeginPlayInternal` alır. Normal application shutdown’da pending/current world’ler önce bırakılır; böylece component destructor’ları bağlı oldukları servisler hâlâ hayattayken çalışır. Ardından audio, timer, physics, shader ve asset manager’lar kapatılır.

## Test ve doğrulama

### Mevcut testler

`GasLiteCoreTests.cpp`, world’den dönen weak pointer’ları lock eder ve bazı actor cleanup senaryolarında weak expiration kontrol eder. Camera follow testleri `weak_ptr<Actor>` kullanımını; projectile/telegraph testleri world koleksiyon sahipliğinin bırakılmasını dolaylı doğrular.

### Doğrudan test edilmeyen davranışlar

Raw owner actor’dan önce yok olduğunda child/visual davranışı, world switch sırasında haricî `shared_ptr` tutulan actor, raw delegate listener ömrü ve exception sırasında Application cleanup.

### Manuel doğrulama gereken noktalar

Owner’dan uzun yaşayan ability/visual actor senaryoları ve world kapatılırken dış sistemlerde saklanan raw pointer’lar.

### Önerilen fakat henüz bulunmayan testler

- Owner’ı yok edilen raw-owner actor/visual için sanitizers ile lifecycle testi.
- Destroy sonrası dış `shared_ptr` tutulan actor’ın world’den çıkarıldığı fakat nesnenin pending kaldığı testi.
- World switch sonrası timer/delegate callback’lerinin eski world nesnelerine gitmediği testi.

## Kod Okuma Sırası

1. `LightYearsEngine/include/framework/Application.h` — world sahipliği.
2. `LightYearsEngine/include/framework/World.h` — actor/HUD/stage sahipliği.
3. `LightYearsEngine/include/framework/Object.h` — weak reference tabanı.
4. `LightYearsEngine/include/framework/Actor.h` — raw world bağlantısı.
5. `LightYearsEngine/include/framework/TimerManager.h` — weak listener modeli.
6. `LightYearsGame/include/spaceShip/SpaceShip.h` — value component örneği.
7. `LightYearsGame/include/gameplay/ability/actors/AbilityWorldActor.h` — feature-local raw owner örneği.

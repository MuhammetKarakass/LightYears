---
type: audit
status: implemented-in-dirty-worktree
verified_date: 2026-07-27
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/include/framework/Delegate.h
  - LightYearsEngine/include/framework/TimerManager.h
  - LightYearsEngine/src/framework/TimerManager.cpp
  - LightYearsEngine/include/framework/World.h
  - LightYearsEngine/src/framework/World.cpp
  - LightYearsEngine/include/framework/PhysicsSystem.h
  - LightYearsEngine/src/framework/PhysicsSystem.cpp
  - LightYearsEngine/include/framework/SoundComponent.h
  - LightYearsEngine/src/framework/SoundComponent.cpp
  - LightYearsEngine/src/framework/Application.cpp
  - LightYearsEngine/tests/EngineLifetimeTests.cpp
related:
  - "[[Performance Monitoring]]"
  - "[[Ownership and Lifetime]]"
  - "[[Engine Runtime Services]]"
  - "[[Audio System]]"
  - "[[Technical Debt and Roadmap]]"
---

# Engine Performance and Memory

## 7 Eylül 2026 kaynak kontrolü

World::Render view-bounds culling ve render candidate/culled/submitted sayaçları içerir; background/foreground eleme dışında kalır. World query body-less grid kullanır. World::SpawnActor yeni allocation yapar; scratch/bucket reuse actor pool değildir. LightYearsEngine/src/framework/World.cpp ve include/framework/World.h doğrulandı; performans benchmark çalıştırılmadı.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Sonuç

2026-07-27 incelemesinde engine’in temel sahiplik modeli genel olarak güvenli bulundu; en yüksek riskler frame içi geçici allocation’lar, callback sırasında container mutation ve raw delegate aboneliklerinin yaşam döngüsüydü. Dirty worktree içinde bu alanlara yönelik optimizasyon ve lifecycle düzeltmeleri uygulandı.

| Alan | Önceki maliyet / risk | Uygulanan değişiklik |
|---|---|---|
| World render | Her render layer için tüm actor listesi yeniden taranıyordu: yaklaşık `O(layer × actor)` | Actor’lar tek geçişte kalıcı render bucket’larına ayrılıyor; çizim yaklaşık `O(actor + layer)` |
| Pending actor promotion | Geçici vector, kopyalama ve tekrar büyüme ihtimali | `reserve` + move ile doğrudan `mActors` içine taşıma |
| Actor cleanup | Her erase kalan `shared_ptr`ları tekrar kaydırabiliyordu | Tek `remove_if` + toplu erase |
| Physics shape rebuild | Her çağrıda `new[]/delete[]` | `PhysicsSystem` üyesi scratch buffer tekrar kullanılıyor |
| Timer callback mutation | Update sırasında timer ekleme/silme iterator/reference invalidation riski taşıyordu | Update sırasında eklenenler pending map’e alınıyor; expired ve pending kayıtlar tur sonunda flush ediliyor |
| Delegate mutation | Broadcast sırasında bind/unbind/clear güvenli değildi; raw subscriber açıkça ayrılamıyordu | Stable `DelegateHandle`, `UnbindAction`, deferred callback ekleme ve broadcast-depth tabanlı cleanup |
| SoundComponent lifetime | Raw `this` ile bağlanan callback component yok olduktan sonra kalabiliyordu | Abonelik handle’ı saklanıyor ve destructor’da, AudioManager hâlâ yaşıyorsa, unbind ediliyor |
| Shutdown sırası | World içindeki component’ler AudioManager kapandıktan sonra yok olabiliyordu | Pending/current world önce bırakılıyor, sonra process-level servisler kapatılıyor |
| Game post-build copy | Engine output klasörünün tamamı kopyalanıyor, build metadata’sı ve test dosyaları da Game output’una taşınıyordu | Yalnız SFML/Box2D target dosyaları `copy_if_different` ile kopyalanıyor |

## Bellek tahsis modeli

- `World`, aktif ve pending actor’ları `shared_ptr` içeren contiguous listelerde tutar; dışarıya verilen ana referans türü `weak_ptr`dır.
- Render bucket’ları `Actor*` tutan non-owning scratch listelerdir. Her frame `clear()` edilir fakat capacity korunur; actor sahipliği hâlâ `mActors`dadır.
- `PhysicsSystem::mShapeScratchBuffer`, fixture/shape yeniden kurulumlarında capacity’yi korur. Peak shape sayısına kadar büyüyebilir ve PhysicsSystem kapanana kadar bu kapasiteyi saklar.
- `TimerManager` aktif ve callback sırasında eklenen timer’ları iki hash map’te tutar. Pending map yalnız update turu boyunca geçici sahiplik taşır.
- Delegate callback’leri `std::function` içinde tutulur. Küçük callable’lar implementation’ın small-buffer optimizasyonundan yararlanabilir; büyük capture’lar heap allocation yapabilir.
- Audio allocation ayrıntıları [[Audio System]] notundadır.

## Doğrulama

- `LightYearsEngineLifetimeTests`, delegate self-unbind/bind/clear reentrancy’sini, timer callback sırasında add/clear davranışını ve audio shutdown sonrası `SoundComponent` yıkımını kapsar.
- 2026-07-27’de mevcut değişiklikler Release ve AddressSanitizer build’lerinde
  iki CTest hedefiyle doğrulandı; her iki matriste de `2/2` test geçti. Bu
  tarihsel kayıttır; 20 Eylül docs-only denetiminde build/test çalıştırılmadı.
- SFML 3.1.0 yükseltmesi temiz Release configure/build ile doğrulandı; o
  tarihteki CTest çalıştırmasında iki gerçek hedef `2/2` geçti. Ayrıntı:
  [[Audio System]].

## Kalan riskler

- Render bucket’larının toplam capacity’si bir kez görülen peak actor dağılımına göre yüksek kalabilir. Bu leak değildir; world ömrü boyunca retained capacity’dir.
- Physics scratch buffer da peak shape sayısını retain eder. Çok uç içerik geçişlerinde `shrink_to_fit` ancak ölçümle gerekçelendirilmelidir; per-frame kullanılması önerilmez.
- `Delegate::BindAction(raw pointer, ...)` hâlâ owner lifetime’ını otomatik izlemez. Her raw binding handle saklamalı ve publisher subscriber’dan uzun yaşayabiliyorsa explicit unbind etmelidir.
- `std::function`, `shared_ptr` ve hash map kullanımı güvenlik ve sadelik sağlar fakat allocation-free değildir. Hot-path değişiklikleri ölçüm olmadan özel allocator’a taşınmamalıdır.
- Performance monitor yalnız sayaç raporlar; frame time percentile, allocation count/byte ve GPU timing henüz yoktur. Ayrıntı: [[Performance Monitoring]].

---
type: system
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/include/framework/Delegate.h
  - LightYearsEngine/include/framework/TimerManager.h
  - LightYearsEngine/src/framework/TimerManager.cpp
  - LightYearsEngine/include/framework/camera/CameraManager.h
  - LightYearsEngine/src/framework/camera/CameraManager.cpp
  - LightYearsEngine/src/framework/Application.cpp
  - LightYearsEngine/src/framework/World.cpp
symbols:
  - ly::Delegate
  - ly::TimerManager
  - ly::CameraManager
  - ly::Application::TickInternal
  - ly::World::UpdateCamera
related:
  - "[[Game Loop]]"
  - "[[TimerManager]]"
  - "[[Delegate]]"
  - "[[CameraManager]]"
  - "[[Timer, Delegate and Camera Frame Flow]]"
---

# Engine Runtime Services

## 7 Eylül 2026 kaynak kontrolü

AssetManager cache/CleanCycle ve TimerManager deferred mutation kaynakları okundu. Timer repeat kalan dt taşımıyor. Global timer pause sırasında, game timer pause dışında ilerler; actor temporal domain bu manager zamanlarını otomatik ölçeklemez. Application::TickInternal doğrulandı. Asset hata tüketicilerinin tümü incelenmedi.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Kapsam

Bu not `LightYearsEngine` içindeki event callback, timer ve kamera servislerini kapsar. `World`/Actor sahipliği için [[Actor and World System]] ve [[Ownership and Lifetime]] temel notlardır; burada onların üstünde çalışan servislerin yaşam döngüsü gösterilir.

| Servis | Sorumluluk | Sahiplik / yaşam döngüsü |
|---|---|---|
| `Delegate<Args...>` | Observer callback listesi ve yayın | Publisher value-member'ı callback'leri tutar; weak binding, ilk başarısız yayınında listener'ı listeden çıkarır |
| `TimerManager` | Gecikmeli veya tekrar eden callback | Üç lazy singleton: default, global ve game; application shutdown hepsini clear + reset yapar |
| `CameraManager` | World view follow, look-ahead, zoom, shake ve bounds | `World` value-member'ıdır; follow target weak pointer'dır |

## Delegate sözleşmesi

`BindAction(weak_ptr<Object>, member)` callback'i weak owner ile sarar. Owner expired olduğunda callback `false` döner; `Broadcast` bu kaydı erase eder. `BindAction(raw pointer, member)` yalnız null kontrolü yapar: bu overload için lifetime çağıranın sorumluluğundadır.

`BindAction` artık `DelegateHandle` döndürür ve `UnbindAction` explicit unsubscribe sağlar. Broadcast sırasında eklenen callback’ler pending listede tutulur; unbind/clear kayıtları inactive işaretler ve en dış broadcast tamamlandığında compact edilir. Böylece self-unbind, callback içinden bind ve nested broadcast iterator invalidation üretmeden çalışır. Raw-pointer overload owner ömrünü hâlâ izlemez; subscriber handle saklayıp uygun lifecycle noktasında unbind etmelidir.

## Timer domainleri

`Application::TickInternal` global timer'ları her frame günceller. World pause değilse game timer'lar ve physics güncellenir; bu nedenle game timer'lar pause sırasında ilerlemez, global timer'lar ilerler. Pending World geçişinde game ve global timer listeleri temizlenir. Default `GetTimerManager()` için bu tick fonksiyonunda otomatik update çağrısı görülmedi; kullanımına göre ayrıca güncellenmesi gerekir.

Timer owner'ı expired veya `pending destroy` olursa `Timer::IsExpired` true döner. `UpdateTimer` expired kayıtları bir sonraki update döngüsünün başında siler. Repeat timer callback'i çalıştıktan sonra sayacı `0` yapar; deltaTime artığı taşınmaz.

## Kamera sözleşmesi

World, `SetViewTarget` ile weak Actor hedefini `CameraManager`a verir; `World::TickInternal` üzerinden `UpdateCamera`, render öncesi runtime state'i günceller. `GetWorldView` center/zoom/shake sonucu üretir ve `World::Render` actor katmanlarını bu view ile çizer.

Kamera cursor look-ahead ile dış velocity kaynaklı movement look-ahead'i birleştirir ve clamp eder. Speed zoom, ek mutlak zoom ve göreli zoom katmanları `GetDesiredZoom` içinde bileşir. World bounds, view merkezini ve view bounds'tan büyükse merkezi sınırlar. `ArenaLevel` oyuncu velocity, mouse world position, afterburner/dash zoom katmanlarını World wrapper'ları üzerinden besler.

## Doğrulanmayan / sınır alanlar

- Timer callback’i sırasında add/clear pending mutation modeliyle korunur ve engine lifetime testinde doğrulanır.
- Delegate self-unbind/bind/clear reentrancy’si testlidir; raw-pointer pointee ömrü otomatik izlenmediği için kullanım yerleri yine lifecycle sorumluluğu taşır.
- Kamera davranışının tüm arena/level türlerinde aynı şekilde bağlandığı doğrulanmadı; ayrıntılı game-side örnek `ArenaLevel`dır.

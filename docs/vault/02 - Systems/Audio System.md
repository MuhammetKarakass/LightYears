---
type: system
status: implemented-in-dirty-worktree
verified_date: 2026-07-27
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/CMakeLists.txt
  - LightYearsEngine/include/framework/AudioManager.h
  - LightYearsEngine/src/framework/AudioManager.cpp
  - LightYearsEngine/include/framework/SoundComponent.h
  - LightYearsEngine/src/framework/SoundComponent.cpp
  - LightYearsEngine/src/framework/Application.cpp
symbols:
  - ly::AudioManager
  - ly::SoundComponent
  - sf::PlaybackDevice
related:
  - "[[Engine Performance and Memory]]"
  - "[[Ownership and Lifetime]]"
  - "[[Game Loop]]"
---

# Audio System

## Mimari

`AudioManager` lazy process singleton’ıdır. `Application::TickInternal` her frame `Update` çağırır; yaklaşık iki saniyelik clean cycle içinde bitmiş sesler pool’a döner. Normal shutdown’da world’ler önce bırakıldığı için `SoundComponent` abonelikleri AudioManager yaşamaya devam ederken çözülür, ardından `AudioManager::ShutdownAudioManager` aktif müzik ve sesleri durdurup sahip olduğu kaynakları bırakır.

## Bellek ve allocation

- Aktif kısa sesler `ActiveSound` kayıtlarında tutulur: `sf::Sound*`, buffer’ı yaşatan `shared_ptr<sf::SoundBuffer>`, tip, pitch ve volume.
- Boş pool yoksa yeni `sf::Sound` heap’te oluşturulur. Bitmiş sesler durdurulup en fazla 32 elemanlık `unique_ptr<sf::Sound>` pool’una alınır; pool doluysa silinir.
- Pool capacity yeniden kullanım sayesinde yoğun SFX sırasında allocation churn’ünü azaltır. Buna karşılık peak sonrası en fazla 32 `sf::Sound` AudioManager kapanana kadar retained kalabilir.
- `SoundBuffer` sahipliği AssetManager cache’i ile aktif ses kaydı arasında `shared_ptr` üzerinden paylaşılır.
- Müzik ve crossfade stream’leri `shared_ptr<sf::Music>` olarak saklanır; shutdown bunları explicit stop + reset eder.
- World-time-scale callback’i raw component pointer’ı kullanır fakat artık `DelegateHandle` saklanır ve component destructor’ında explicit unbind edilir.

## WASAPI device invalidation

Gözlenen hata:

```text
miniaudio ERROR: [WASAPI] Failed to retrieve internal buffer ...
HRESULT = -2004287484. Stopping device.
```

`-2004287484`, hex olarak `0x88890004` ve Windows Core Audio tarafında `AUDCLNT_E_DEVICE_INVALIDATED` değeridir. Çıkış endpoint’i çıkarıldığında, devre dışı kaldığında, yeniden yapılandırıldığında, sürücü resetlendiğinde veya sistem sleep/hibernate sonrasında artık kullanılamadığında görülebilir.

Projede kullanılan SFML 3.0.1, varsayılan cihazı bile miniaudio’ya explicit device ID olarak geçiriyordu. miniaudio explicit ID gördüğünde otomatik default-device stream routing’i kapatır; bu nedenle invalidation sonrasında playback device durup ses kalıcı olarak kesilebiliyordu.

## Uygulanan çözüm

`LightYearsEngine/CMakeLists.txt` içindeki SFML bağımlılığı `3.0.1` → `3.1.0` yükseltildi. SFML 3.1.0 varsayılan seçimde device ID geçmeyerek miniaudio’nun otomatik stream routing yolunu etkinleştirir ve sleep/resume ile default playback device değişimlerini yönetir.

Projede `sf::PlaybackDevice::setDevice()` ile özel/sabit cihaz seçimi yoktur; bu yüzden varsayılan yeni davranış doğrudan geçerlidir. İleride UI üzerinden belirli bir cihaz seçilirse, o cihaz kaybolduğunda uygulama-level state notification ve fallback politikası ayrıca tasarlanmalıdır.

## Doğrulama

- SFML 3.1.0 ile temiz `Release` configure ve 528-adımlı ilk build başarıyla tamamlandı.
- Post-build kopyalama, engine output klasörünün tamamını taşımak yerine yalnız SFML/Box2D target dosyalarını `copy_if_different` ile kopyalayacak şekilde daraltıldı. Böylece `CTestTestfile.cmake` gibi build metadata’sı Game output’una taşınmıyor.
- 2026-07-27 tarihli son configure/build sonrasında CTest yalnız iki gerçek
  testi listeledi; `LightYearsGasLiteCore` ve `LightYearsEngineLifetime`
  testleri `2/2` geçti. Bu tarihsel kayıttır; 20 Eylül docs-only denetiminde
  build/test çalıştırılmadı.

## Riskler ve manuel test

- Donanım/sürücü geçişi otomatik testte gerçek WASAPI endpoint invalidation üretmeden tam doğrulanamaz.
- Windows üzerinde oyun ses çalarken şu senaryolar manuel denenmelidir: default output değiştirme, Bluetooth/USB cihaz çıkarma-takma ve sleep/resume.
- Default-device modunda beklenen sonuç sesin yeni default endpoint’e devam etmesidir. Explicit-device özelliği eklenirse sessiz fallback ile hoparlöre otomatik geçiş arasındaki ürün kararı açık olmalıdır.
- Otomatik testler API/lifecycle regresyonlarını kapsar; gerçek endpoint değişimini doğrulamak için aşağıdaki donanım senaryoları yine manuel çalıştırılmalıdır.

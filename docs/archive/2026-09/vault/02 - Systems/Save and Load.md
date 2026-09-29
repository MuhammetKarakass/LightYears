---
type: system
verified_on: 2026-09-07
verification: source-review-only
verified_worktree_state: dirty
---

# Save and Load

## Mevcut durum: uygulama yolu bulunmadı

Kaynak envanteri, grafik Save araması ve üretim kaynaklarında SaveGame/LoadGame/serialize/ofstream taraması bir oyun kaydetme/yükleme servisi göstermedi; ofstream eşleşmesi log içindi. Bu sınırlı negatif kanıttır, haricî araç veya bütün olası farklı isimler için mutlak yokluk iddiası değildir.

`LightYearsGame/src/player/Player.cpp`, `SpawnSpaceShip` ve `RestorePurchasedAbilityLevels`, aynı Player nesnesinin bellekte tuttuğu ilerlemeyi yeni gemiye bağlar. `ResetRunProgression` ve `PlayerManager::Reset` koşu durumunu temizler. Bu respawn restore'dur; uygulama kapanıp açıldığında devam etme değildir. JSON content loader'ları savegame loader değildir.

## Tasarım kararı / öneri

Kalıcı tutulacak veriler henüz bu denetimde belirlenmedi. Run snapshot, meta unlock ve ayarlar farklı ihtiyaçlar; önce oynanabilir run ve ekonomi bağlantısı tamamlanmalı. Sonra sürümlü küçük bir veri kontratı seçilmeli; Actor/World nesne grafiğini bütünüyle serialize etme önerilmez.

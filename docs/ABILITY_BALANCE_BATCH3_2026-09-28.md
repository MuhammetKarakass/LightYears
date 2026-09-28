# Ability catalog üçüncü paket

## Değişiklik sözleşmesi

Amaç: katalogdaki 18'den sonraki dokuz farklı yeteneğin yeni davranış ve
denge formüllerini uygulamak: FrozenThrong, WingSentinels, CombatSentry,
NanoPlague, StrikeRun, SeismicCharge, ArcScythes, LanceDrive, InertialWake.
Katalogda ArcScythes iki kez 25 olarak yazılmış ve 26 yoktur; kullanıcı
InertialWake dahil dokuz farklı yetenek kapsamını açıkça onayladı.
EmberSwarm'a ve son beş eski progression yeteneğine geçilmeyecek.

Sahiplik: üç Luna max ajanı sırasıyla ilk, orta ve son üç ailenin doğrudan
C++ gameplay/config/contract/presentation dosyalarını düzenler. Codex ortak
abilities.json, gerekli ortak entegrasyon, kaynak incelemesi, final doğrulama,
bu rapor ve commit sahibidir. Ortak dosya değişiklikleri önce orkestratörde
birleştirilir. Başlangıç çalışma ağacı temiz, HEAD 98a2702.

FrozenThrong için ortak izinli sınır: DamageContext ve CombatRuntime mevcut
ölüm-öncesi Cryo snapshot'ına stack sayısını ekler. Hasar/status çözüm sırası
değişmez; aile ölüm callback'iyle temizlenmiş hedefi okumaz.

İnvariantlar: runtime loadout, diğer ability kayıtları, belirtilmeyen değerler,
mevcut seviye sınırları ve maliyetler korunur. Kimlik, damage/status ve typed
presentation sahipliği değişmez. Kalıcı kayıtlar unique ID/weak_ptr ve uygun
RAII ömrü kullanır. Genel motor davranışı aile gereksinimi olmadan değiştirilmez.

Kanıt: direct owner, çağrı akışı, JSON ve diff incelemesi; entegrasyon sonunda
oyun hedefinin derlenmesi. Kullanıcının yeni talebiyle AGENTS.md içindeki test
ve GasLite zorunlulukları kaldırıldı. Bu pakette test çalıştırma zorunluluğu yok.
Önceki Scorch loader düzeltmesinin runtime kabulü henüz doğrulanmadı.

## Sonuç ve sınır

Üç Luna max ajanı aile dosyalarında dokuz yeteneği uyguladı. Orkestratör ortak
`abilities.json`, ölüm-öncesi Cryo stack snapshot'ı, JSON/validator entegrasyonu,
AGENTS.md kural değişikliği ve commit sahibidir. ArcScythes'in katalogdaki çift
satırı tek yetenek sayıldı; EmberSwarm ve son beş eski progression işlenmedi.

- FrozenThrong: Cryo stacks olmadan da tetiklenir; ölüm anı 0/1/2/3/4 stack
  çarpanı .75/1/1.20/1.40/1.70, AP ve EP eşit formal ölçek.
- WingSentinels: iki drone kendi konumundan bağımsız hedef seçer; ateş hızı
  `1.5 + 1.5 × floor(AttackSpeed / 40)`.
- CombatSentry: turret HP/Armor owner ölçekleri, AP hasarı ve lineer
  AttackSpeed ateş hızı katalog değerlerine bağlandı.
- NanoPlague: `1 + floor(Luck / 100)` yayılımı, nesil sınırı olmadan; aynı
  hedefte eşzamanlı ikinci infection engellenir.
- StrikeRun: beş impact, 1400 hat, 350 aralık, .8 telegraph ve .1 cadence;
  katalog AP/level hasar katsayıları.
- SeismicCharge: 3 saniye fuse, 2 saniye büyüyen wave, hedef başına tek hit;
  faz ömrü uzun frame'lerde sabit actor timeout'una yenilmez.
- ArcScythes: ortak cadence ve hedef başına tek tick korunur; EP/level hasarı
  ve Electric stack canonical payload üzerinden yürür.
- LanceDrive: gerçek temas hızı × EP ile güçlenen dönüşüm, geçici enemy-body
  collision geçişi ve aktivasyon kilidi.
- InertialWake: gerçek hız × AP ile güçlenen dönüşüm; 220 taban alan uzunluğu,
  speed ratio ile büyüyen geometri ve mevcut re-hit/CC davranışı.

JSON kontrolü: 55 kayıt, yalnız seçili dokuz kayıt değişti; dokuzunun da
14 yükseltme adımı ve scrap maliyetleri korundu. `git diff --check` exit 0.
`cmd /c 'call vcvars64.bat ... && set "VSLANG=1033" && cmake --build
D:\LightYears\build --target LightYearsGame --parallel 6'` exit 0;
son log `build/balance-batch3-game-final.log` içinde 3 değişmiş C++ nesnesi ve
oyun executable link'i görülür. Başlangıçtaki geniş Game/GasLite/E2E derleme
komutunda oyun ve GasLite ikilileri linklendi, fakat E2E ikilisi eski Nano
imzalı nesne nedeniyle `LNK2019` ile linklenmedi. Bu paket için E2E veya
GasLite çalıştırılmadı; runtime davranış kabulü iddia edilmiyor. Önceki
Scorch loader düzeltmesi de çalıştırılarak doğrulanmış sayılmıyor.

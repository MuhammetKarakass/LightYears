# Legendary modül temeli — L0

Tasarım sahibi [modules.md, Legendary bölümü](modules.md#legendary-modüller--12),
sayısal kaynak [denge belgesi](LEGENDARY_BALANCE_PROPOSAL.md). Çelişkide katalog
önceliklidir. L0 yalnız içerik, yükleme, çözümleme ve yaşam döngüsü sağlar;
Legendary davranışları sonraki görevlerdedir. Loot havuzuna ekleme yapılmaz.

## JSON sözleşmesi

`ModuleDefinition::legendary`, modül kökündeki `legendary` nesnesinden yüklenir:

```json
{
  "id": "Module.Primary.SecondHardpoint.Legendary",
  "displayName": "Second Hardpoint",
  "description": "Carry a reserve Primary weapon; switching weapons marks targets for bonus damage.",
  "family": "Ability.Primary",
  "rarity": "Legendary",
  "levelCount": 3,
  "legendary": {
    "kind": "SecondHardpoint",
    "markDamageBonus": [0.20, 0.25, 0.30],
    "markSeconds": 5,
    "switchSeconds": 1
  },
  "sizeVariants": [{ "shape": [[0, 0], [1, 0], [1, 1]] }]
}
```

`kind`, `ModuleLoaderLegendary.cpp` içinde ilgili somut kural tipine dönüştürülür.
`ModuleLegendaryRule`, bu 12 tipin kapalı `std::variant` birleşimidir; runtime
string selector veya yeni GameplayTag kullanmaz. Kural tipleri
`include/gameplay/module/rules/legendary/Legendary<Name>Rules.h` altındadır.

Seviye alanları mevcut `ModuleLevelTable<float>` kullanır. JSON dizileri doğal
L1–L3 değerleridir; varsayılan doğrusal uzatma aynı tablonun L4/L5 değerlerini
verir. Mevcut `<field>Growth` sözleşmesi de desteklenir. Sabitler scalar'dır;
tam sayı adetler integer olmalıdır. Yükleyici etkin L1–L5 değerlerinin sonlu ve
pozitif olduğunu denetler. Eksik alan, bilinmeyen alan/kind, yanlış aile,
birden fazla boyut, yan veya ortak yan havuzu reddedilir.

`Fraction`, `Bonus`, `Coefficient` ve `Multiplier` alanları oran/çarpandır:
örneğin `0.20`, %20 demektir. Underdog'un adında `PercentPerWeight` bulunan
AS/Haste alanları yüzde puanıdır (`0.375`, +%0,375); hareket alanı rating'dir.
Diğer alanların birimleri adlarında belirtilir; mesafe ve yarıçap dünya birimi,
`steamDeviationDegrees` derece, `magnetSpeed` dünya birimi/sn'dir.
Primed tavanı bağımsız veri değildir: `1 + powerPerReadySecond × readyWindowSeconds`.
Ion Comet global CombatTick'i kullanmalıdır; ikinci tick saati/verisi eklenmedi.

## Sonraki davranış görevlerinin giriş noktası

`ModuleResolver`, mevcut bölge bonusundan sonra `ResolveLegendaryRules` çağırır.
Çıktı `ResolvedModuleEffects::legendaryRules` içindeki modüle özgü listelerdir;
her kayıt placement `instanceId` ve etkin seviyede çözülmüş scalar değerleri taşır.

`ShipModuleApplier`, 12 ayrı `Legendary<Name>Runtime` nesnesinin tek sahibidir.
Yeniden kurmada ilgili liste `Replace` ile gönderilir, her frame `Tick` çağrılır,
applier yıkılırken `Clear` çağrılır. Runtime destructor'ı da `Clear` çağırır.
`Replace` önce eski durumu temizler. L0 runtime'ları yalnız değer kopyası tutar;
delegate, actor registry, modifier veya oynanış etkisi oluşturmaz.

Sonraki görev kendi runtime `.h/.cpp` çiftindeki TODO'yu doldurur. Ability/combat
bağlantısı constructor'da hazırdır. Gereken subsystem olayları ilgili sahibi
üzerinden bağlanmalı; tüm kayıtlar/effect handle'ları `Clear` tarafından RAII ile
serbest bırakılmalı, actor referansları weak olmalı ve kimlik unique ID olmalıdır.
İlk aşamada ortak loader/resolver/applier/CMake değişikliği gerekmez; yeni olay
sınırı veya ek veri gerekirse yalnız o sözleşme genişletilir. `src/` altındaki
runtime kaynakları mevcut ortak CMake glob'u ile oyuna ve E2E'lere bağlanır;
loader ve saf çözümleme kaynağı gameplay core kaynak listesine dahil edilmiştir.

## Belirsizlikler ve temsil kararları

- L yönü mevcut `[[0,0],[1,0],[1,1]]`, domino `[[0,0],[1,0]]`; Twin Rack karesi
  aynı başlangıç hücresinden 2×2 olarak yazıldı. Bunlar şeklin başlangıç yönüdür;
  mevcut döndürme sözleşmesi korunur.
- Oranlar JSON'da kesir, Underdog AS/Haste ise açıkça yüzde puanı olarak seçildi.
  Bu bir veri birimi kararıdır; oyun matematiğine uygulanması sonraki görevindir.
- Primed'in 6⅔ sn penceresi JSON'da `6.666666666666667` ile temsil edilir;
  float çözümlemenin olağan yuvarlaması geçerlidir.
- Meltdown odaklanırken Second Hardpoint ile silah değiştirmenin etkisi katalogda
  hâlâ açık. L0 bu etkileşime karar vermez. Şarjörsüz Primary ısı modeli katalog
  gereği ertelenmiştir; önceki 6/2 önerisi içerik sözleşmesine eklenmedi.
- Katalogdaki son “her şey tamam” onayları eski denge belgesindeki açık boyut,
  Blink eşleme, Ion fren/charge, Reaction tetik, Hardlight işlem sırası ve
  Parasite kontrol/menzil notlarından üstündür. Bu değerler tahmin edilmedi.

## Kanıt sınırı

`LightYearsLegendaryModulesE2E`, shipped JSON → loader → inventory/install →
resolver → gerçek geminin applier/runtime zincirini, L1–L5/bölge bonusunu,
şekilleri, negatif yükleme örneklerini ve sökme temizliğini doğrular. JSON
artefaktı `build/e2e-artifacts/legendary-modules.json` altındadır. Bu senaryo
gelecekteki Legendary davranışları veya oynanış dengesi için kanıt değildir.

# Modül uygulama kanıtları

Bu belge 2026-10-13–14 uygulama kayıtlarını korur. Son düzenleme: 2026-10-23. Buradaki build ve E2E sonuçları tarihli kanıttır; bu belge düzenlenirken yeniden çalıştırılmadı. Güncel tasarım ve uygulama sınırı [katalogdadır](modules.md#uygulama-durumu).

**Tamamlayıcı paket —2026-10-14:** Tempered Hull/Bastion/Field Medic, Speed Lance/Echo/Charged, Release Valve, Gold Plating, Polar Shield ve Stripped Hull geçişi uygulandı. Field Medic30sn ve Bastion5sn yeniden Armor hesabıyla sıfırlanmaz; hız mızrağı900 menzil/1800 hızla tüm hedefleri tam hasarla, her hedefe mızrak başına bir kez deler. Release halkasının sınırları600 ve900 dahil; iç/dış bölge hasarsız. Polar normal havuzları bağımsız, geçici katman ortak; kaynaksız/yönü belirlenemeyen hasar normal havuzları oranlı tüketir. Pozitif normal kalkan artışı daha eksik havuzu eşitler; kapasite değişimleri ve sökme toplam kalkan üretmez. Gold bonusu mevcut cüzdan üzerinden tam10’luk dilimlerden, tavanlı hesaplanır; `RunGold::TrySpend` yetersiz bakiyeyi reddeder. Gold Armor, Armor Conversion ve Rapid Alloy’a dahildir; mağaza akışı eklenmedi.

`cmake --build build --target LightYearsGame LightYearsModuleGoldHullE2ETests LightYearsModulePolarShieldE2ETests LightYearsModuleReleaseValveE2ETests LightYearsModuleSpeedLanceE2ETests LightYearsModuleTemperedHullE2ETests LightYearsModuleDefenseLayerE2ETests LightYearsModulePlanTransitionE2ETests --parallel 2` —vcvars64 14.38 sonrası `VSLANG=1033`, `_CL_=/Z7`; exit0. 2026-10-14 kaydında aşağıdaki yedi focused E2E geçti; tam suite çalıştırılmadı. Test artefaktları somut mekanik/entegrasyon kanıtıdır, oyuncu dengesi veya görsel kabulü değildir.

| E2E | Geçen kontrol | Kanıt |
|---|---:|---|
| ModuleGoldHull | 98 | [JSON](../build/e2e-artifacts/ModuleGoldHull-2026-10-14.json) |
| ModulePolarShield | 97 | [JSON](../build/e2e-artifacts/ModulePolarShield-2026-10-14.json) |
| ModuleReleaseValve | 120 | JSON — `../build/e2e-artifacts/ModuleReleaseValve-2026-10-14.json` (tarihsel referans; bu checkout’ta dosya yok) |
| ModuleSpeedLance | 229 | JSON — `../build/e2e-artifacts/ModuleSpeedLance-2026-10-14.json` (tarihsel referans; bu checkout’ta dosya yok) |
| ModuleTemperedHull | 534 | JSON — `../build/e2e-artifacts/ModuleTemperedHull-2026-10-14.json` (tarihsel referans; bu checkout’ta dosya yok) |
| ModuleDefenseLayer | 227 | JSON — `../build/e2e-artifacts/ModuleDefenseLayer-2026-10-14.json` (tarihsel referans; bu checkout’ta dosya yok) |
| ModulePlanTransition | 212 | JSON — `../build/e2e-artifacts/ModulePlanTransition-2026-10-14.json` (tarihsel referans; bu checkout’ta dosya yok) |

Toplam **1517** geçen kontrol. İlk test hataları kapalı collision mask, doğrudan maksimum-kalkan fixture ayarı, no-op Apply dönüşü ve float sınır örneklemesiydi; bu düzeltmeler üretim bug iddiası değildir. Son build sırasında başka Ninja ile nesne dosyası çakışmaları ve açık exe link kilidi yaşandı; ayrı test hedefleriyle devam edilip final Game linki tamamlandı.

**Epic dışı kalan4:** Danger Pay—Threat/koşullu Gold+Scrap ödülü; Silent Running—Threat seyahat artışı; Interest Ledger—nokta sonu olayı; Survey Relay—rota ödül ailesi verisi. [GalaxyRoute](../LightYearsGame/include/gameplay/galaxy/GalaxyRoute.h) yıldız ve konak duraklarını taşır, bu oyun sözleşmelerini karşılamaz. Bu modüller için sahte tetik veya yeni Threat/rota sistemi kurulmadı. Epic/Legendary ve nadirliği açık modüller bu uygulama kapsamının dışında planlıdır.

**İkinci paket —2026-10-13:** Armor Conversion2/3+RapidAlloy/BalancedOutput, Overshield2/3+ShieldRush/ReserveShell, StatusPrimer sabit2 uygulandı. Geçici modül kalkanı normal kalkan eksikken de6sn erir; tekrar aynı kaynak yenilenir, sökme başka yeteneğin kalkanına dokunmaz. StatusPrimer eşit stackte deterministik Thermal/Cryo/Electric/Kinetic sırası kullanır; mevcut stack tavanları korunur ve yeni modül tetiklemez. `cmake --build build --target LightYearsGame LightYearsModuleDefenseLayerE2ETests LightYearsModulePlanTransitionE2ETests --parallel 2`(vcvars/VSLANG1033),exit0. [Yeni227kontrol](../build/e2e-artifacts/module-defense-layer-2026-10-13.json) ve önceki212kontrol — `../build/e2e-artifacts/module-plan-transition-after-defense-2026-10-13.json` (tarihsel referans; bu checkout’ta dosya yok),exit0. Yeni yan seviyeleri kabul edilen başlangıç değerinde sabit; seçilmemiş büyüme eklenmedi. Bastion5sn ve Speed sınırsız/tamhasar delme o paketten sonra uygulandı; güncel kanıt yukarıdadır.

**İlk tasarım geçişi —2026-10-13:** `cmake --build build --target LightYearsGame LightYearsModulePlanTransitionE2ETests --parallel 2`,vcvars64 sonrasıVSLANG=1033,_CL_=/Z7;exit0. `build/LightYearsGame/LightYearsModulePlanTransitionE2ETests.exe build/e2e-artifacts/module-plan-transition-2026-10-13.json`:exit0,[212 kontrol kanıtı](../build/e2e-artifacts/module-plan-transition-2026-10-13.json). Arc üç seviyede hedef/range/cooldown/no-chain/removal/küçükboyut kilidi; dört hasar statüsü/Frozen süresi/donma beklemesi; dört Hunt taşıyıcısında gerçek Lucky iadesi; Defense-only kalkan/Armor/expiry ve canlı katalog dönüşümü. Tam test suite çalıştırılmadı. Eski Lucky/Cold/Shatter ana-modül veya Spare yan beklentili tarihsel E2E senaryoları eski tasarım içindir; bu kanıt onların çalıştığı iddiası değildir.

Önceki tarihli canlı revizyon: Arc Extender 1 hücre; Double Feed 2 hücre; Death Burst 400 yarıçap/%15; Trail L1 30+%30 AP Kinetic (L2/L3 ajan35+%35/40+%40); Hunt Fuel yerine Kill Ward. Omni Uncommon. İlgili Game build ve focused E2E sonuçları:

- Boyut/mayın/patlama — `../build/e2e-artifacts/module-sizing-mines-burst-2026-10-13.json` (tarihsel referans; bu checkout’ta dosya yok): 379 kontrol.
- Hunt Fuel kaldırma — `../build/e2e-artifacts/loot-shield-no-hunt-fuel-2026-10-13.json` (tarihsel referans; bu checkout’ta dosya yok): 192 kontrol (Kill Ward eklenmeden önceki tarihli sınır).
- Kill Ward — `../build/e2e-artifacts/loot-shield-kill-ward-2026-10-13.json` (tarihsel referans; bu checkout’ta dosya yok): 108 kontrol; üç seviye, küçük boyut kilidi, kill/refresh/no-stack/expiry/removal.

İlk tasarım geçişi uygulandı: Zone Cooler2,Arc1/2+Discharge/Resonator+O1/O2/O5,Hunt Clock G5 yerineLucky,Spare yansız,Barrier/BraceCommon. Lucky/Cold/Shatter bağımsız kayıtları ve Cold/Shatter/Spare özel yanları kaldırıldı. Armor/Overshield/StatusPrimer ikinci pakette uygulandı. Tempered/Speed/Polar/Release/Gold ve Stripped geçişi de uygulandı. Epic dışındaki dört bağımlı ana modül aşağıda açık olarak listelenir. Loot,kalıcılık,yükseltme harcaması,ekonomi/rota bağımlıları ve Rare sunumu ayrı işlerdir. Test/JSON varlığı oyuncunun edinme veya görsel kabul kanıtı değildir. Güncel uygulama sonuçları [Uncommon planında](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md) ve [Rare planında](MODULE_RARE_IMPLEMENTATION_PLAN.md).


**2026-10-23 erişilebilirlik kontrolü:** Yukarıda “bu checkout’ta dosya yok” işaretli artefaktlar tarihli kayıtlarda anılmıştır; bu denetimde dosyaları yeniden incelenemedi. Eski test sonucu kaydı korunur, mevcut dosya veya yeniden koşulmuş test kanıtı olarak sunulmaz.

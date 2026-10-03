# Modül Sistemi Tasarımı

**Tarih:** 2026-10-03 · **Durum:** Modül runtime ve ekranı mevcut; fiziksel sistem genişlemesi · **Kaynak:** kullanıcıyla 2026-10-01 ve 2026-10-03 tasarım konuşmaları · **Mimari:** [MODULE_SYSTEM_ARCHITECTURE.md](MODULE_SYSTEM_ARCHITECTURE.md)

Bu doküman sıfırdan düşünülmüştür; `Designs.md`'deki eski slot/Star fikirleri referans alınmaz. Eski yetenek başına attachment sistemi kaldırıldı; yerine bu modül sistemi gelir. Rakamlar başlangıç önerisidir, prototipte ayarlanır.

---

## 1. Özet

Gemi, Tetris benzeri bir **şematik** üzerinde modüller taşır. Kalkan, afterburner ve HUD göstergeleri de birer modüldür: varsayılan olarak takılı gelir, oyuncu isterse söker ya da bulduğu bir varyantla değiştirir (NieR plug-in chip mantığı). Modüller koşu içinde düşer ve **her zaman** takılıp sökülebilir.

Gemi kimliği üç şeyden gelir:

1. Bölgelerin boyut dağılımı (bir gemide Çekirdek büyük, birinde Motor),
2. Gövdenin (serbest bölge) büyüklüğü, yani esneklik,
3. Varsayılan sistem modülleri.

Toplam kapasite tüm gemilerde aynıdır.

## 2. Kararlar

| Konu | Karar |
|---|---|
| Temel model | Gemi şematiği üzerinde modül sistemi; yeteneğe özel soket **yok** |
| Kapasite | Sistemler ve boş alan birlikte **46 hücre**, tüm oyuncu gemilerinde aynı (Gövde dahil). Varsayılan sistemler 20 hücre, başlangıçta boş alan 26 hücre |
| Bölgeler | 5 aile bölgesi + Gövde (serbest bölge), Tetris şekilli |
| Aileler | Yetenek kategorileriyle aynı: **Primary, Offense, Defense, Movement, Utility**. Tanklık Defense içindedir |
| Yerleşim | Her modül her bölgeye girer (sistem modülleri hariç) |
| Bölge bonusu | Ailesiyle eşleşen bölgede ana değer **+%25** |
| Gövde (serbest bölge) | Her aileye **+%10**, tüm gemilerde var, boyutu gemiye göre değişir |
| Eşleşmeyen bölge | Ceza yok, taban değerin %100'ü |
| Döndürme | 90° döndürme var |
| Değişim | Her zaman, savaş içi dahil; modül ekranı oyunu **durdurmaz** |
| Savaş içi değişim | Takılan modül kısa süre **"Sistem yeniden başlatılıyor"** durumunda kalır |
| Savaş tanımı | Son 3 saniyede hasar alındı ya da verildi |
| Varsayılan modüller | Life Support, Flight Control, Primary Weapon, Q/E/F/R, kalkan, afterburner, HP/armor ve hasar göstergeleri; sistemler toplam kapasitenin içinde yer tutar |
| Seviye büyümesi | Kalkan/afterburner seviye artışı gemide kalır, takılı sistem modülünün üstüne eklenir |
| Boyut | Büyük modül daha çok güç verir; kalite hücre maliyetini **düşürmez** |
| Boyut türleri | Karma: ölçeklenen, genişleyen, sabit (bkz. §5) |
| Nadirlik | Var (bkz. §6) |
| Komşuluk | İlk sürümde yok |

## 3. Bölgeler ve aileler

Modül aileleri yeteneklerin kategorileriyle **aynıdır**; ayrı bir aile listesi yoktur. Bir modülün ailesi hem bölge bonusunu hem de (yetenek kapsamlı modüllerde) hangi yetenekleri etkilediğini belirler.

| Bölge | Konum | Bonus aldığı aile | Varsayılan modül |
|---|---|---|---|
| **Burun** | Ön | Primary (ana silah) | — |
| **Kanatlar** | Yanlar | Offense | — |
| **Gövde** | Orta | **Serbest:** tüm aileler +%10 | — |
| **Çekirdek** | Merkez | Defense (kalkan, HP, armor dahil) | Kalkan jeneratörü (2) |
| **Motor** | Arka | Movement | Afterburner (2) |
| **Sensör** | Kokpit | Utility | HP/armor göstergesi (1), hasar göstergesi (1) |

Yetenek kategori değişiklikleri (2026-10-01, uygulandı): Scorch Drive ve Lance Drive `Movement`'a taşındı. Dört Control yeteneği (Cryo Bola, Frost Maelstrom, Gravity Anomaly, Null Pulse) `Utility` oldu ve `Ability.Control` kategorisi silindi. Her yetenek tek kategori taşır.

Kurallar:

- Bonus yalnız modülün **ana değerine** uygulanır; nadirlikten gelen yan statlara uygulanmaz.
- Bonus için modül **tamamen tek bir bölgenin içinde** olmalıdır. İki bölgeye taşan modül bonus almaz.
- Arayüzde eşleşme yeşil "+%25" (aile bölgesi) / "+%10" (Gövde) etiketiyle gösterilir; eşleşmeme için uyarı gösterilmez.

## 4. Sistem modülleri

Kalkan, afterburner ve HUD göstergeleri **sistem modülüdür**:

- Kalkan → Çekirdek, afterburner → Motor, göstergeler → Sensör, Life Support → Gövde, Flight Control → Motor, Primary Weapon → Burun bölgelerine takılır. Bulunan varyantlar da aynı kurala uyar. Q/E/F/R slot modülleri herhangi bir bölgeye yerleşebilir; tuşu yetenek kategorisine sabitlemezler.
- Her kategoriden gemide en fazla **bir** tane bulunur (iki kalkan jeneratörü takılamaz).
- Sökülebilir. Söküldüğünde ilgili sistem çalışmaz: kalkan yok, afterburner yok ya da HUD öğesi görünmez.
- Varsayılanların hücreleri toplam 46 hücreye dahildir. Söküldüklerinde açılan hücreler normal modül yerleşimi için kullanılabilir.

### 4.1 Fiziksel yaşam, hareket ve silah/yetenek sistemleri (2026-10-03)

| Sistem | Hücre | Sökülünce |
|---|---:|---|
| Life Support Unit | 2 | Oyuncu ölür; taşıma ve ilk kurulum ölüm tetiklemez |
| Flight Control Unit | 2 | A/S/D hareket girdileri kapanır; W ve aim yönüne dönüş devam eder |
| Primary Weapon | 2 | Ana silah ateşi kapanır |
| Q / E / F / R Ability | Her biri 2 | Yalnız ilgili slot kullanımı kapanır |

Yeni sistemler toplam 14 hücredir. Kalkan (2), afterburner (2) ve iki gösterge (1+1) ile 20 hücre doludur; tam donanımlı gemide 26 hücre boş kalır. Bir yeteneği sökmek 2 hücre açar. Life Support dışındaki sistemler kullanım ile alan arasında seçim sunar. Life Support sökme sonucu arayüzde açıkça belirtilmelidir.

Life Support zorunludur: sökülmesi mevcut gemiyi öldürür; yaşam desteği olmayan koşu yerleşimiyle yeni gemi doğamaz. İlk doğuşta varsayılan sistem yerleşimi kurulmadan önce veya bir modül yeniden başlatılırken ölüm tetiklenmez.

Silah/yetenek sistemini söküp takmak öğrenilmiş yetenek seviyesini, cooldown veya şarjörü sıfırlayan bir edinim işlemi değildir. Savaşta yeniden kurulum mevcut 1,5 saniyelik başlatma kuralına uyar. Q/E/F/R tuş slotlarıdır; varsayılan yetenek eşlemesinin sahibi `DefaultAbilityLoadout.cpp` olarak kalır.

Kalkan varyantı örnekleri:

| Varyant | Etki | Hücre |
|---|---|---|
| Standart jeneratör | Geminin varsayılanı | 2 |
| Hızlı şarj | Kalkan %20 düşük, şarj belirgin hızlı | 2 |
| Ağır jeneratör | +60 kalkan | 3 |
| Yansıtıcı | Kalkan düşük, kırılınca hasar yansıtır | 2 |

Kural: saf güçlendirme fazladan hücreye mal olur, takas yapan varyantlar aynı boyutta kalabilir.

Varsayılan göstergeler 1 hücredir. Bunları sökmek NieR'deki gibi "son hücreye ihtiyacım var" anı yaratır.

## 5. Modül arketipleri

| Arketip | Kural | Örnek |
|---|---|---|
| **Ölçeklenen** | 2 ve 3 hücre sürümü var, büyük olan hücre başına daha az verimli | Zırh Plakası: 2 hücre +40, 3 hücre +55 |
| **Genişleyen** | Büyük sürüm ek bir özellik açar | Kalkan Takviyesi: 2 hücre +40; 3 hücre +60 ve kırılınca kısa dokunulmazlık |
| **Sabit** | Tek boyutta var | Aşırı Yük Çekirdeği (yalnız 3–4 hücre, büyük güç); sensör modülleri (yalnız 1 hücre) |

- Modül şekilleri: 1 (tek), 2 (domino), 3 (I veya L), 4 (yalnız özel/legendary modüller).
- Aynı güçteki iki modülden biri bölgeye sığar, diğeri sığmaz. Şekil boyut kararının parçasıdır.

## 6. Nadirlik

Boyut "ne kadar güç" sorusunu cevaplar, nadirlik "ne kadar ilginç" sorusunu. Nadirlik hücre sayısını değiştirmez.

| Nadirlik | Ne ekler |
|---|---|
| **Common** | Yalnız ana değer |
| **Rare** | Ana değer ~%10 yüksek ve 1 küçük yan stat |
| **Epic** | Ana değer ~%10 yüksek ve 2 yan stat |
| **Legendary** | Sabit, benzersiz bir mekanik; yalnız belli modüllerde çıkar |

- Yan statlar modülün ailesinin havuzundan gelir (kalkan modülüne savunma statı çıkar).
- Legendary ilk sürümde yok. İlk sürümde nadirlik ana değer ve yan stat olarak kalır.

## 7. Savaş içi değişim: "Sistem yeniden başlatılıyor"

- Modül ekranı her zaman açılabilir ve **oyunu durdurmaz**. Savaş içinde envanter açmak gerçek risktir; değişim doğal olarak sakin anlara kayar.
- Savaş sırasında (son 3 saniyede hasar alındı ya da verildi) takılan modül **1,5 saniye** etki vermez (önerilen başlangıç değeri), ikonunda yükleme göstergesi bulunur.
- Sökülen sistem modülü hemen kapanır. Yeniden takılırsa başlatma süresini bekler; kalkan bu süre boyunca boş kalır.
- Savaş dışında değişim anında devreye girer.

## 8. Önceki gemi şemaları (2026-10-01 tarihsel örnek)

Bu bölümün 28 boş hücre ve ek sistem alanı modeli 2026-10-03 kararıyla kaldırıldı. Aşağıdaki dağılım eski tasarım örneğidir; güncel kapasite sistemler dahil 46 hücredir. Güncel bölge şekilleri ve varsayılan yerleşimler `ships.json` içindeki `schematic` alanına aittir. Roller [gemi tasarım dokümanından](Light_Years_Ships_Primary_Weapons_Combined.md) alınmıştır.

| Gemi | Rol | Burun (Primary) | Kanat (Offense) | Gövde (serbest) | Çekirdek (Defense) | Motor (Movement) | Sensör (Utility) |
|---|---|---|---|---|---|---|---|
| Fighter | Dengeli | 4 | 5 | 7 | 5 | 4 | 3 |
| Breacher | Yakın menzil saldırı | 7 | 6 | 6 | 4 | 2 | 3 |
| Interceptor | Skirmisher | 3 | 4 | 6 | 4 | 9 | 2 |
| Conductor | Yetenek odaklı destek | 2 | 4 | 7 | 5 | 3 | 7 |
| Aegis | Ağır tank | 2 | 3 | 4 | 12 | 4 | 3 |
| Cryo | Alan kontrolü | 3 | 4 | 6 | 4 | 4 | 7 |

- Hücre sayısının yanında bölgenin **şekli** de kimliktir. Aegis'in Çekirdeği 3×4 ise L modüller rahat oturur, dağınıksa küçük modüller değerlenir.
- Dar bölge doğal zayıflıktır. Örneğin Breacher'ın 2 hücrelik motoru 3 hücrelik motor modüllerini bonuslu taşıyamaz.
- Tarihsel modelde sistem modülleri bu sayıların üstüne ekleniyordu: Çekirdek +2, Motor +2, Sensör +2; toplam 34 hücre. Bu ek kapasite artık yoktur; sistemler toplam 46 hücrenin içindedir.

## 9. İlk modül listesi önerisi (16)

Değerler örnektir. "Aile" sütunu bonus aldığı bölgeyi gösterir.

| Modül | Aile → Bölge | Arketip | Hücre | Etki |
|---|---|---|---|---|
| Odaklı Namlu | Primary → Burun | Ölçeklenen | 2/3 | Ana silah hasarı +%10 / +%14 |
| Delici Uç | Primary → Burun | Sabit | 2 | Ana silah mermileri +1 hedef deler |
| Atış Hızlandırıcı | Primary → Burun | Ölçeklenen | 2/3 | Ana silah atış hızı +10 / +14 |
| Saldırı Matrisi | Offense → Kanat | Ölçeklenen | 2/3 | Offense yetenek hasarı +%12 / +%16 |
| Yan Bataryalar | Offense → Kanat | Genişleyen | 2/3 | Offense yetenek hasarı +%10; 3 hücrede yanlara ek atış |
| Aşırı Yük Çekirdeği | Offense → Kanat | Sabit | 4 | Offense yetenek hasarı +%25, cooldown +%10 |
| Zırh Plakası | Defense → Çekirdek | Ölçeklenen | 2/3 | Armor +4 / +5,5 |
| Gövde Takviyesi | Defense → Çekirdek | Ölçeklenen | 2/3 | MaxHP +40 / +55 |
| Kalkan Takviyesi | Defense → Çekirdek | Genişleyen | 2/3 | Kalkan +40; 3 hücrede +60 ve kırılınca dokunulmazlık |
| İtki Artırıcı | Movement → Motor | Ölçeklenen | 2/3 | Maks. hız +%8 / +%11 |
| Manevra İticisi | Movement → Motor | Sabit | 1 | Yatay hareket +%8 |
| Plazma İzi | Movement → Motor | Genişleyen | 2/3 | Afterburner izi hasar verir; 3 hücrede iz yavaşlatır |
| Soğutma Devresi | Utility → Sensör | Ölçeklenen | 2/3 | AbilityHaste +8 / +11 |
| Enerji Hücresi | Utility → Sensör | Ölçeklenen | 2/3 | EnergyPower +10 / +14 |
| Hedefleme Bilgisayarı | Utility → Sensör | Ölçeklenen | 1/2 | Kritik şansı +3 / +5 |
| Hurda Tarayıcı | Utility → Sensör | Sabit | 1 | Ganimet düşme şansı artar |

## 10. Mevcut koda etkisi

Bugün kalkan ve afterburner **geminin** değerleri. `ships.json` → `energy` bloğunda `baseMaxShield`, `maxShieldPerLevel`, `shieldFullRechargeDuration`, `baseShieldRechargeDelay`, `baseAfterburnerCapacity`, `afterburnerCapacityPerLevel` ve diğer afterburner alanları var. Bunlar Faz 1 gemi planında taban + seviye artışı olarak kuruldu.

- Kalkan/afterburner taban ve seviye değerleri `ships.json`'da kalır. Sistem modülü sistemi **açar** ve değerleri göreli değiştirir (Hızlı şarj: kalkan ×0,8, şarj ×1,5; Ağır: +60). Ayrıntı: [mimari](MODULE_SYSTEM_ARCHITECTURE.md) §6.1.
- Modül sökülünce ilgili sistem devre dışı kalır (kalkan 0, afterburner kullanılamaz). Seviye büyümesi gemide kalır; modül geri takılınca yeniden uygulanır.
- HUD gösterge modülleri, MVVM/Presenter katmanında ilgili View'ın görünürlüğünü kontrol eder.
- Yetenek kapsamlı modüller (ör. "Offense yetenek hasarı +%12") mevcut `ScopedAbilityRule` ile yetenek kategorisine göre uygulanır.
- Modül tanımları JSON'da tutulur (`modules.json`); gemi şeması `ships.json` içinde ASCII ızgaradır.

## 11. Uygulama sırası

Ayrıntılı adımlar ve kanıtlar: [mimari](MODULE_SYSTEM_ARCHITECTURE.md) §9.

## 12. Açık sorular

Sonraya bırakılanlar:

1. Modüller nereden düşer: normal düşman, elite, boss, mağaza?
2. Koşu sonunda modüller kaybolur mu, yoksa bir kısmı meta ilerlemeye mi taşınır?
3. Taşıma kapasitesi: takılı olmayan modüller için envanter sınırı var mı?

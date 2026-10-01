# UI Tabanı Planı — MVVM + Presenter

Orkestratör: Sol. Uygulayıcı ve inceleyici alt ajanlar: Luna. Sol her ADIM'ı doğrudan ayrı bir Luna görevine çevirir; `orchestrator.py` kullanılmaz. Build ve test doğrulamasını her adımın uygulayıcısı yapar.

## Amaç

Çok sayıda UI elemanı gelecek. Kompakt, esnek ve lifetime açısından güvenli bir UI tabanı kur, sonra `GameHUD`'daki player vitals parçasını (health, shield, energy, life, score) bu desene taşıyarak tabanı kanıtla.

## Mimari sözleşme (tüm adımlar için bağlayıcı)

Veri tek yönde akar:

```
Gameplay (Model) ──delegate/poll──▶ Presenter ──SetIfChanged──▶ ViewModel ◀──revision okur (Tick)── View
                                        ▲                                                           │
                                        └──────────────── niyet delegate'i (onClicked vb.) ─────────┘
```

1. **Model gameplay'in kendisidir.** UI için gameplay durumunun ikinci bir kopyası veya cache'i tutulmaz. ViewModel yalnız gösterilecek türetilmiş değerleri tutar.
2. **ViewModel** düz bir struct'tır ve SFML widget'ına bağımlı değildir (`sf::Color` gibi değer tipleri kullanılabilir). Her alan değişikliği `SetIfChanged` üzerinden yapılır ve bir `UIRevision` sayacını artırır. Tek Presenter `Tick`'inde birden fazla alan değişirse revision birden fazla artabilir; sayaç bir değişiklik işaretidir, frame veya olay sayacı değildir.
3. **ViewModel'den View'a callback yoktur.** View her `Tick`'te `UIRevisionWatcher::Consume` ile revision'a bakar ve yalnız değişiklik varsa widget değerlerini günceller. İki View `Tick`'i arasında birden fazla alan değişse de sonraki `Tick` son durumu bir kez uygular. Görünür widget'lar normal `Draw` akışında her frame çizilmeye devam eder; revision çizim önbelleği sağlamaz.
4. **Presenter** model'e bağlanır, lifetime'ı yönetir (player/ship değişimi, respawn) ve ViewModel'e yazar. Bağlantılar `SubscriptionSet` ile tutulur. Ham `DelegateHandle` alanı biriktirilmez.
5. **View** yalnız widget kompozisyonudur ve oyun mantığı içermez. Input'u delegate olarak dışarı verir.
6. View, ViewModel'i `shared_ptr<const VM>` ile tutar. Ham pointer veya referans tutmaz, böylece Presenter/View yok olma sırası dangling üretmez.
7. **Geriye uyumluluk:** Mevcut `HUD::AddWidget`, `SetWidgetLocation` ve `MainMenuHUD`/`PauseMenuHUD`/`GameOverHUD` davranışı değişmez. Layout almayan widget eskisi gibi mutlak konumla çalışır.

## Ortak kurallar (her Luna adımı)

- Luna yalnız kendisine atanan ADIM'ı uygular. Git komutu (commit, stash, restore, checkout) çalıştırmaz. Çalışma ağacındaki ilgisiz dirty değişiklikler korunur.
- Kod yazmadan önce ilgili mevcut sahip dosyalar okunur (adımda listelenenler).
- C++17, SFML 3 (`event.getIf<sf::Event::X>()`, `sf::FloatRect{position,size}`), namespace `ly`, tab girinti, mevcut dosyaların adlandırma ve yorum yoğunluğu.
- Engine kodu Game'e bağımlı olamaz. Yeni engine dosyaları `D:\LightYears\LightYearsEngine\include\...` ve `src\...` altına eklenir ve `D:\LightYears\LightYearsEngine\CMakeLists.txt` kaynak listesine eklenir.
- Yeni Game kaynakları `LIGHT_YEARS_UI_HUD_SOURCES` adlı tek bir CMake değişkeninde toplanır (`LIGHT_YEARS_ENCOUNTER_HUD_SOURCES` deseni gibi). Bu değişken hem oyun hedef listesine hem de `LIGHT_YEARS_GAS_LITE_TEST_SOURCES` listesine eklenir. Dosya: `D:\LightYears\LightYearsGame\CMakeLists.txt`.
- İlgisiz refactor, denge değişikliği veya biçimlendirme yapılmaz. `TEMPORARY TEST UI` işaretli parçalar (hız yazısı, hasar sayıları, ability stat readout) taşınmaz.
- **Luna kendi adımını derler ve testlerini koşar** (aşağıdaki komutlar, adımın "Doğrulama" satırındaki hedef ve regex ile). Build veya test kırmızıysa adım bitmiş sayılmaz.
- **Rapor biçimi (Sol'a):** değişen dosyalar; yapılanların özeti; çalıştırılan komutlar ve exit kodları; geçen/kalan test sayısı; belirsizlikler ve kalan riskler. Çalıştırılmayan doğrulama "çalıştırılmadı" diye açıkça yazılır.

## Doğrulama komutları

```bat
cmd /c "call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && set VSLANG=1033 && cmake --build build --target <HEDEFLER> --parallel 4"
ctest --test-dir build -R "<REGEX>" --output-on-failure --timeout 60
```

`VSLANG=1033` zorunludur (bkz. `AGENTS.md`). Header değişikliğinden sonra link veya davranış tuhafsa bir kez temiz build alınır. Yeni test hedefi eklenen adımlarda önce `cmake -S . -B build` ile yeniden yapılandırma gerekebilir.

## Sol için orkestrasyon notları

- **Roller:** Adım başına bir **Luna uygulayıcı** (kod, build, test). ADIM 2, 4, 8 ve 10'dan sonra ayrıca bir **Luna inceleyici** (salt okunur; kod değiştirmez) diff'i aşağıdaki kontrol listesine göre denetler. İnceleyici bulgu verirse Sol düzeltmeyi aynı veya yeni bir uygulayıcıya verir, sonra bir sonraki adıma geçer.
- **Seri yürüt, paralel değil.** Adımlar aynı `build` dizinini, aynı CMake dosyalarını ve aynı test dosyasını (`UIFoundationTests.cpp`) paylaşıyor. Aynı anda iki build veya aynı dosyaya iki yazar çakışma üretir. Aynı anda en fazla bir uygulayıcı çalışır. İnceleyici, uygulayıcı bittikten sonra çalışır.
- **Sıra ve bağımlılık:** 1 → 2 → 3 → 4 zorunlu sıradır (her biri öncekinin API'sini kullanır). 5 ve 6 teknik olarak yalnız 1'e bağlıdır ama seri kuralı gereği 4'ten sonra yapılır. 7 için 5+6, 8 için 2+3+4+7, 9 için 8, 10 için 9 gerekir.
- **Her adıma bağlam:** Luna'ya bu dosyanın "Mimari sözleşme", "Ortak kurallar" ve "Doğrulama komutları" bölümleri ile yalnız ilgili ADIM verilir. Önceki adımların kabul edilmiş raporlarının kısa özeti eklenir (yeni API isimleri gibi).
- **En riskli adımlar:** ADIM 2 (tüm widget'ların temel sınıfı; eski HUD'ları bozma riski) ve ADIM 8 (`GameHUD`'dan söküm; `mObservedPlayerSpaceShip` ortak kullanımı). İnceleyici bu iki adımda geriye uyumluluğu ve silinen üyelerin başka yerde kullanılmadığını özellikle denetler.
- **İnceleyici kontrol listesi:** mimari sözleşme maddeleri 1–7; engine→game bağımlılığı yok; View'da oyun mantığı yok; VM'den View'a callback yok; ham `DelegateHandle` alanı biriktirilmemiş; yeni kaynaklar CMake'te; testler görsel değil davranışsal; raporlanan build/test sonuçları gerçekten çalıştırılmış.
- **Görsel kabul** otomatik testle yapılamaz. ADIM 8 sonrası kullanıcı oyunu açıp sol alttaki vitals'ı ve respawn'ı gözle kontrol etmeli.

---

## ADIM 1: UILayout ve rect çözümleme matematiği (engine, saf)

**Amaç:** Pencere veya SFML çizimi gerektirmeyen, test edilebilir anchor/pivot layout matematiği.

**Yeni dosyalar**
- `D:\LightYears\LightYearsEngine\include\widget\UILayout.h`
- `D:\LightYears\LightYearsEngine\src\widget\UILayout.cpp`
- `D:\LightYears\LightYearsEngine\tests\UIFoundationTests.cpp`

**Sözleşme**
```cpp
struct UIRect { sf::Vector2f position{0,0}; sf::Vector2f size{0,0}; };

enum class UIAnchor { TopLeft, Top, TopRight, Left, Center, Right, BottomLeft, Bottom, BottomRight };

struct UILayout
{
	sf::Vector2f anchorMin{0,0};   // parent rect içinde 0..1
	sf::Vector2f anchorMax{0,0};   // anchorMin == anchorMax ise o eksen "nokta", değilse "stretch"
	sf::Vector2f pivot{0,0};       // kendi rect'inde 0..1, yalnız nokta eksende anlamlı
	sf::Vector2f offset{0,0};      // nokta eksen: anchor noktasına göre piksel kayma
	sf::Vector2f size{0,0};        // nokta eksen: 0 ise intrinsic boyut kullanılır
	sf::Vector2f insetMin{0,0};    // stretch eksen: sol/üst iç boşluk
	sf::Vector2f insetMax{0,0};    // stretch eksen: sağ/alt iç boşluk

	static UILayout Anchored(UIAnchor anchor, const sf::Vector2f& offset = {0,0}, const sf::Vector2f& size = {0,0});
	static UILayout Stretch(const sf::Vector2f& insetMin = {0,0}, const sf::Vector2f& insetMax = {0,0});
};

UIRect ResolveLayout(const UILayout& layout, const UIRect& parent, const sf::Vector2f& intrinsicSize);
```

**Kurallar**
- `Anchored` fonksiyonu anchorMin, anchorMax ve pivot değerlerini aynı köşe veya merkez değerine ayarlar. Örnek: `BottomRight` için anchor (1,1), pivot (1,1).
- Nokta eksen: `extent = size>0 ? size : intrinsic`, `pos = parent.pos + parent.size*anchor + offset - pivot*extent`.
- Stretch eksen: `pos = parent.pos + parent.size*anchorMin + insetMin`, `extent = max(0, parent.size*(anchorMax-anchorMin) - insetMin - insetMax)`.
- Sonuç pozisyonu `std::round` ile tam piksele yuvarlanır (bulanık metni önlemek için), boyut yuvarlanmaz.
- Eksenler bağımsızdır: x stretch, y nokta olabilir.

**Test hedefi** (`D:\LightYears\LightYearsEngine\CMakeLists.txt` içinde `BUILD_TESTING` bloğuna, `LightYearsWorldSpatialQueryTests` desenini kopyalayarak):
`add_executable(LightYearsUIFoundationTests tests/UIFoundationTests.cpp)`, engine'e link ve `add_test(NAME LightYearsUIFoundation COMMAND LightYearsUIFoundationTests)`.

Test dosyası basit bir `Check(cond, msg)` yardımcısı ve `main` içerir. Hata olursa stderr'e yazar ve 1 döndürür. Test framework eklenmez. Kapsam:
- 9 anchor'un her biri 1280x720 parent içinde 100x40 kutu ile beklenen konumu verir.
- `size=0` olunca intrinsic boyut kullanılır.
- Tam stretch ve inset'ler, x-stretch/y-nokta karışık eksen.
- Parent offset'i (parent.position ≠ 0) sonuca eklenir.
- Negatif stretch boyutu 0'a kırpılır, yarım piksel yuvarlanır.

**Yapılmayacaklar:** `Widget` veya `HUD` değişmez.

**Kabul:** Saf fonksiyonlar, global durum yok, testler anchor/pivot/stretch/inset/yuvarlama davranışını kapsıyor.

**Doğrulama:** hedef `LightYearsUIFoundationTests`, ctest `^LightYearsUIFoundation$`.

## ADIM 2: Widget ağacı, layout çözümü ve alpha mirası (engine)

**Amaç:** `Widget`'a parent/child ağacı, layout uygulama, özyinelemeli tick/draw/event ve miras alınan alpha eklemek. Mevcut alt sınıflar (`TextWidget`, `ImageWidget`, `ValueGauge`, `Button`) değişmeden çalışmaya devam eder.

**Değişen dosyalar**
- `D:\LightYears\LightYearsEngine\include\widget\Widget.h`, `D:\LightYears\LightYearsEngine\src\widget\Widget.cpp`
- `D:\LightYears\LightYearsEngine\src\widget\TextWidget.cpp` (yalnız `SetString`/`SetTextSize` sonunda `InvalidateLayout()` çağrısı)
- `D:\LightYears\LightYearsEngine\src\widget\ImageWidget.cpp` (yalnız `SetImage` sonunda `InvalidateLayout()`)
- `D:\LightYears\LightYearsEngine\tests\UIFoundationTests.cpp` (yeni testler)

**Eklenecek API (Widget)**
```cpp
template<typename T, typename... Args> weak_ptr<T> AddChild(Args&&... args); // HUD::AddWidget gibi new T(...)
void RemoveChild(const weak_ptr<Widget>& child);  // anında silmez, DestroyWidget ile işaretler
const List<shared_ptr<Widget>>& GetChildren() const;
Widget* GetParent() const;

void SetLayout(const UILayout& layout);           // mHasLayout = true, InvalidateLayout()
void ClearLayout();                               // eski mutlak konum davranışına döner
bool HasLayout() const;
const UIRect& GetResolvedRect() const;
void InvalidateLayout();                          // kendini dirty yapar, parent zincirine subtreeDirty yayar
void ResolveLayoutTree(const UIRect& parentRect, bool force); // HUD çağırır

bool NativeHandleEvent(const sf::Event& event);   // görünmezse false; çocuklar TERS sırada, sonra kendi HandleEvent
float GetEffectiveAlpha() const;                  // mAlpha * parent effective alpha

protected:
virtual void ArrangeChildren(const UIRect& selfRect, bool force); // varsayılan: her çocuğa ResolveLayoutTree(selfRect, force)
void PlaceAt(const UIRect& rect);                 // container'lar için: çocuğu verilen rect'e taşır
virtual sf::Vector2f GetIntrinsicSize() const;    // varsayılan: GetBound().size
```

**Davranış kuralları**
- `PlaceAt` ve layout uygulaması origin'den bağımsızdır: `delta = rect.position - GetBound().position`, ardından `SetWidgetLocation(GetWidgetLocation() + delta)`. Böylece `CenterOrigin` kullanan widget'lar da doğru yerleşir.
- Layout'suz widget'ın `mResolvedRect` değeri `GetBound()`'dur. Çocukları bu rect'e göre çözülür.
- `ResolveLayoutTree` yalnız `force || mLayoutDirty || mSubtreeDirty` ise çalışır. Kendi rect'i değiştiyse çocuklara `force=true` geçilir. Bittiğinde dirty bayrakları temizlenir.
- `SetVisibility` değişince parent'ın layout'u geçersiz olur (container gizli çocuğu atlar).
- `NativeTick` sırası: animasyon, `Tick`, çocukların `NativeTick`'i, sonra süresi dolan veya işaretli çocukların silinmesi. Döngü, başlangıçtaki çocuk sayısı üzerinde index ile yürür. Tick sırasında eklenen çocuk bir sonraki frame'de tick alır. Silinen çocuğun `mParent` değeri `nullptr` yapılır.
- `NativeDraw`: görünürse `Draw`, ardından çocuklar ekleme sırasıyla çizilir (sonra eklenen üstte kalır).
- Alpha: `SetAlpha` kendi `mAlpha` değerini saklar ve `ApplyAlpha(GetEffectiveAlpha())` çağırır, sonra çocuklara yayar. Fade animasyonu parent'ta çalışınca tüm alt ağaç söner.
- `mParent` ham pointer'dır. Parent çocuklarının sahibidir, bu yüzden güvenlidir.

**Testler** (sabit `GetBound` döndüren test-only bir `TestBoxWidget` alt sınıfıyla, font veya texture yüklemeden):
- Nokta anchor ile çocuk parent rect'e göre yerleşir, parent taşınınca `ResolveLayoutTree` çocuğu da taşır.
- Dirty olmayan ağaç ikinci `ResolveLayoutTree` çağrısında yeniden yerleştirme yapmaz (test widget'ı `LocationUpdated` çağrılarını sayar).
- Event'i en son eklenen çocuk önce alır. Çocuk `true` döndürünce kardeşlere ve parent'a gitmez. Görünmez çocuk event almaz.
- Parent alpha 0.5, çocuk alpha 0.5 → çocuğun uygulanan alpha değeri 0.25.
- Tick içinde `DestroyWidget` edilen çocuk aynı tick sonunda silinir ve crash olmaz.

**Kabul:** Eski HUD'lar kod değişikliği olmadan derlenir. Mevcut alt sınıf imzaları değişmez.

**Doğrulama:** hedefler `LightYearsUIFoundationTests LightYearsGame`, ctest `^LightYearsUIFoundation$`.

## ADIM 3: Panel ve StackPanel container'ları (engine)

**Yeni dosyalar**
- `D:\LightYears\LightYearsEngine\include\widget\Panel.h`, `D:\LightYears\LightYearsEngine\src\widget\Panel.cpp`
- `D:\LightYears\LightYearsEngine\include\widget\StackPanel.h`, `D:\LightYears\LightYearsEngine\src\widget\StackPanel.cpp`

**Panel:** Kutu tabanlı widget.
- `Panel(const sf::Vector2f& size = {0,0})`. `SetPanelSize`, `SetBackgroundColor` (varsayılan şeffaf; alfa 0 ise çizilmez).
- `GetBound()` layout varsa resolved rect'i, yoksa konum + panel boyutunu döndürür.
- Layout çözümünde kutu boyutu resolved rect boyutuna eşitlenir, böylece stretch panel gerçekten uzar.
- `ApplyAlpha` arka plan rengine uygulanır.

**StackPanel : Panel**
```cpp
enum class UIOrientation { Horizontal, Vertical };
enum class UIAlign { Start, Center, End };
StackPanel(UIOrientation orientation, float spacing = 0.f);
void SetSpacing(float); void SetPadding(const sf::Vector2f&); void SetCrossAlign(UIAlign);
```
- `ArrangeChildren` override: görünür çocukları sırayla yerleştirir. Çocuk boyutu, layout'u varsa ve `size` sıfırdan büyükse o boyut, değilse `GetIntrinsicSize()` olur. Çapraz eksende `UIAlign`'a göre hizalanır. Çocuğun kendi anchor'u yok sayılır, `offset` ek kayma olarak eklenir. Yerleştirme `PlaceAt` ile yapılır, ardından çocuğun kendi alt ağacı `ArrangeChildren` ile çözülür.
- `GetIntrinsicSize` override: padding\*2 + ana eksende çocuk boyutları + spacing toplamı, çapraz eksende en büyük çocuk. Layout'suz StackPanel ve içerik sığdırma bununla çalışır.
- Gizli çocuk yer kaplamaz.

**Testler:** Yatay ve dikey dizilim; spacing ve padding; Center/End hizalama; gizli çocuk atlanır; çocuk boyutu değişip `InvalidateLayout` çağrılınca kardeşler kayar; iç içe StackPanel intrinsic boyutu doğru.

**Doğrulama:** hedef `LightYearsUIFoundationTests`, ctest `^LightYearsUIFoundation$`.

## ADIM 4: HUD katmanları ve viewport (engine)

**Değişen dosyalar:** `D:\LightYears\LightYearsEngine\include\widget\HUD.h`, `D:\LightYears\LightYearsEngine\src\widget\HUD.cpp`, testler; `MainMenuHUD.cpp`, `PauseMenuHUD.cpp`, `GameOverHUD.cpp` yalnız event yönlendirme sırası için.

**API**
```cpp
enum class UILayer : std::uint8_t { Hud = 0, Menu, Modal, Tooltip, Count };
weak_ptr<Panel> GetLayer(UILayer layer);
template<typename T, typename... Args> weak_ptr<T> AddToLayer(UILayer layer, Args&&... args);
void SetViewportSize(const sf::Vector2u& size);   // testler pencere olmadan kullanır
sf::Vector2u GetViewportSize() const;
```

**Davranış**
- Katmanlar HUD constructor'ında `UILayout::Stretch()` ile tam ekran `Panel` olarak oluşturulur.
- `NativeInit` pencere pointer'ını saklar ve `SetViewportSize(window.getSize())` çağırır. `HUD::Tick` her frame pencere boyutunu karşılaştırır, değiştiyse viewport'u günceller ve katmanlara `force` ile layout çözümü yaptırır. Pencere yoksa (testler) son `SetViewportSize` değeri kullanılır.
- `HUD::Tick` sırası: legacy `mWidgets` tick ve temizlik (mevcut kod aynen kalır), katman `NativeTick`, katman `ResolveLayoutTree(viewportRect, viewportChanged)`.
- `HUD::Draw`: önce legacy `mWidgets`, sonra katmanlar Hud → Menu → Modal → Tooltip sırasıyla çizilir.
- `HUD::HandleEvent`: event Tooltip → Modal → Menu → Hud sırasıyla `NativeHandleEvent`'e gider. Modal katmanında görünür en az bir çocuk varsa Menu ve Hud'a event inmez ve `true` döner. `sf::Event::Resized` gelirse viewport güncellenir.
- `GameHUD::Draw` ve `GameHUD::Tick` içindeki base çağrılar korunur. Mevcut menü `HandleEvent` override'ları butonları base HUD'dan önce çalıştırdığı için modal engellemesini aşar ve short-circuit durumunda resize/katman yönlendirmesini atlar. `MainMenuHUD.cpp`, `PauseMenuHUD.cpp`, `GameOverHUD.cpp` handler'ları önce `if (HUD::HandleEvent(event)) return true;` çalıştırır, sonra aynı mevcut buton/klavye akışını korur; sonda base ikinci kez çağrılmaz. Menülerin katmanlara taşınması bu adımın kapsamı dışındadır. `GameHUD.cpp` bu adımda okunur, değiştirilmez.

**Testler:** HUD alt sınıfı test içinde tanımlanır. Viewport 1280x720 iken BottomRight çocuk doğru konumda; viewport 1920x1080'e değişince bir sonraki Tick'te yeniden konumlanır; Modal'da görünür çocuk varken Hud katmanındaki çocuk event almaz, Modal çocuğu gizlenince alır.

**Doğrulama:** hedefler `LightYearsUIFoundationTests LightYearsGame`, ctest `^LightYearsUIFoundation$`.

## ADIM 5: SubscriptionSet (engine)

**Yeni dosya:** `D:\LightYears\LightYearsEngine\include\framework\SubscriptionSet.h` (header-only). CMake kaynak listesine header olarak eklenir.

**API**
```cpp
class SubscriptionSet
{
public:
	SubscriptionSet() = default;
	~SubscriptionSet() { Clear(); }
	SubscriptionSet(const SubscriptionSet&) = delete; SubscriptionSet& operator=(const SubscriptionSet&) = delete;
	SubscriptionSet(SubscriptionSet&&) noexcept; SubscriptionSet& operator=(SubscriptionSet&&) noexcept; // taşınan boş kalır

	// Kaynak shared_ptr ile yönetiliyorsa: kaynak ölmüşse Clear unbind'i atlar.
	template<typename Listener, typename... Args>
	void Bind(const weak_ptr<Object>& source, Delegate<Args...>& delegate, Listener* listener, void (Listener::*callback)(Args...));

	// Kaynak shared_ptr ile yönetilmiyorsa (ör. deque'deki Player): ÇAĞIRAN, kaynak ölmeden önce Clear
	// çağrılacağını garanti eder (ör. PlayerManager::onPlayerAboutToBeDestroyed içinde).
	template<typename Listener, typename... Args>
	void BindUnguarded(Delegate<Args...>& delegate, Listener* listener, void (Listener::*callback)(Args...));

	void Clear();          // idempotent, broadcast sırasında çağrılabilir (Delegate bunu destekler)
	bool IsEmpty() const; std::size_t Count() const;
};
```
- İç kayıt `{ weak_ptr<Object> source; bool guarded; std::function<void()> unbind; }` biçimindedir. `unbind`, `delegate.UnbindAction(handle)` çağırır. Bağlama, `Delegate`'in ham pointer `BindAction` overload'ıyla yapılır. Bu güvenlidir çünkü set listener'ın üyesidir ve listener yok olurken `Clear` çalışır.
- Başlık yorumunda kısa bir sözleşme olur: set, listener'ın üyesi olmalıdır.

**Testler** (`UIFoundationTests.cpp`): Bind sonrası broadcast listener'a ulaşır; `Clear` sonrası ulaşmaz; guarded kaynak (shared_ptr<Object> test nesnesi ve üyesi Delegate) yok edildikten sonra `Clear` crash olmaz; callback içinden `Clear` güvenli; move sonrası kaynak set boştur ve hedef set bağlantıları yönetir; destructor unbind eder.

**Doğrulama:** hedef `LightYearsUIFoundationTests`, ctest `^LightYearsUIFoundation$`.

## ADIM 6: UIRevision / ViewModel yardımcıları ve UIStyle (engine)

**Yeni dosyalar**
- `D:\LightYears\LightYearsEngine\include\widget\UIViewModel.h` (header-only)
- `D:\LightYears\LightYearsEngine\include\widget\UIStyle.h`, `D:\LightYears\LightYearsEngine\src\widget\UIStyle.cpp`

**UIViewModel.h**
```cpp
class UIRevision { public: void Bump(); std::uint32_t Get() const; private: std::uint32_t mValue{1}; }; // 0 atlanır
class UIRevisionWatcher { public: bool Consume(const UIRevision& r); void Reset(); private: std::uint32_t mSeen{0}; };
template<typename T> bool SetIfChanged(T& field, const T& value, UIRevision& revision);
```
Yeni watcher ilk `Consume`'da her zaman `true` döner (widget değerlerinin ilk güncellemesi garanti). Son görülen revision'dan farklı bir değer için bir kez `true`, aynı değerle sonraki çağrılarda `false` döner. İki `Consume` arasında birden fazla `Bump` olması ayrı ayrı View güncellemesi gerektirmez.

**UIStyle**
```cpp
enum class UIFontRole { Body, Title, Count };
enum class UIColorRole { Text, TextMuted, Accent, Positive, Warning, Danger, PanelBackground, GaugeBackground, Count };
enum class UITextSize { Small, Body, Large, Title, Count };
struct UIStyle
{
	std::array<std::string, (size_t)UIFontRole::Count> fonts;
	std::array<sf::Color, (size_t)UIColorRole::Count> colors;
	std::array<unsigned int, (size_t)UITextSize::Count> textSizes;
	const std::string& Font(UIFontRole) const; sf::Color Color(UIColorRole) const; unsigned int TextSize(UITextSize) const;
	static UIStyle MakeDefault();
	static const UIStyle& Get(); static void Set(const UIStyle& style);
};
```
- Varsayılan font `SpaceShooterRedux/Bonus/kenvector_future.ttf`. Renkler mevcut `GameHUD` renklerinden alınır: gauge arka planı (128,128,128), Danger (255,0,0), Accent (80,160,255). Boyutlar: 10/16/20/32.
- Mevcut widget constructor varsayılanları **değişmez**. Yeni View'lar stili `UIStyle::Get()` üzerinden okur.

**Testler:** revision/watcher/SetIfChanged davranışı (aynı değer bump etmez, değişen her alan bir kez bump eder, watcher ilk okumada `true` döner; birden fazla alan değişikliğinden sonra bir `Consume` `true`, aynı revision'la sonraki `Consume` `false` döner); `UIStyle::Set` ardından `Get` yeni değeri döndürür, test sonunda varsayılan geri yüklenir.

**Doğrulama:** hedef `LightYearsUIFoundationTests`, ctest `^LightYearsUIFoundation$`.

## ADIM 7: Vitals ViewModel ve Presenter (game)

**Yeni dosyalar**
- `D:\LightYears\LightYearsGame\include\presentation\hud\vitals\VitalsViewModel.h`
- `D:\LightYears\LightYearsGame\include\presentation\hud\vitals\VitalsPresenter.h`, `D:\LightYears\LightYearsGame\src\presentation\hud\vitals\VitalsPresenter.cpp`

Önce şu mevcut sahip okunur: `D:\LightYears\LightYearsGame\src\widget\GameHUD.cpp` içindeki `RefreshHealthBar`, `PlayerHealthUpdated` (barrier capacity overcap hesabı dahil), `PlayerShieldUpdated`, `RefreshEnergyBar`, `ConnectStatus`/`BindStatusToPlayer`/`UnbindStatusFromPlayer`/`OnPlayerAboutToBeDestroyed`/`OnPlayerCreated`/`DisconnectStatus`, `RefreshPlayerHUDState`. Davranış birebir korunur.

**VitalsViewModel**
```cpp
struct VitalsViewModel
{
	bool hasShip{false};
	float health{0.f}, healthMax{1.f};          // ham HealthComponent
	float displayHealth{0.f}, displayHealthMax{1.f}; // barrier capacity dahil
	float shield{0.f}, shieldMax{1.f};
	float energy{0.f}, energyMax{1.f};
	bool hasPlayer{false};
	unsigned int life{0}, score{0};
	UIRevision revision;
};
sf::Color ComputeHealthBarColor(const VitalsViewModel& vm); // mevcut PlayerHealthUpdated renk mantığının saf kopyası
```
Renk kuralları mevcut koddan birebir alınır: `healthMax<=0` → (255,0,0); `displayHealthMax>healthMax` → (80,160,255); aksi halde yüzdeye göre kırmızı-sarı-yeşil gradyan.

**VitalsPresenter** (Object değildir; sahibi ADIM 8'deki controller'dır)
```cpp
class VitalsPresenter
{
public:
	VitalsPresenter();                          // PlayerManager delegate'lerine BindUnguarded (singleton, ömür boyu yaşar)
	void Tick();                                // kimlik kontrolü + gemi değerlerini poll
	shared_ptr<const VitalsViewModel> GetViewModel() const;
private:
	void OnPlayerCreated(Player* player);
	void OnPlayerAboutToBeDestroyed(Player* player); // bağlı player ise mPlayerSubs.Clear()
	void OnLifeChanged(int life); void OnScoreChanged(int score);
	void BindPlayer(Player* player);            // mPlayerSubs.Clear, BindUnguarded(onLifeChange/onScoreChange), ilk değerleri yaz
	shared_ptr<VitalsViewModel> mViewModel;
	SubscriptionSet mManagerSubs, mPlayerSubs;
	unsigned int mBoundPlayerId{0};
};
```
- **Player (life/score):** event ile güncellenir. Player `std::deque` içinde yaşar ve shared_ptr ile yönetilmez. Bu yüzden `BindUnguarded` kullanılır ve `onPlayerAboutToBeDestroyed` içinde `mPlayerSubs.Clear()` zorunludur. `Tick` içinde `GetPlayer()` kimliği `mBoundPlayerId`'den farklıysa yeniden bağlanır.
- **Gemi (health, shield, energy, overcap):** Mevcut `GameHUD` bunları zaten her frame okuyor. Parite için `Tick` içinde poll edilir ve yalnız `SetIfChanged` ile yazılır. Değer değişmedikçe revision artmaz. Gemi yoksa, süresi dolmuşsa veya `GetIsPendingDestroy()` ise `hasShip=false`, `displayHealth=0/1`, `shield=0/1`, `energy=0/1` olur. Böylece ship-destroyed timer'ına gerek kalmaz.
- Tüm VM yazımları `SetIfChanged(..., vm.revision)` ile yapılır.

**Doğrulama:** hedef `LightYearsGame` (test ADIM 9'da). CMake: `LIGHT_YEARS_UI_HUD_SOURCES` değişkeni burada oluşturulur ve iki listeye eklenir.

## ADIM 8: VitalsView, controller ve GameHUD'dan vitals çıkarma (game)

**Yeni dosyalar**
- `D:\LightYears\LightYearsGame\include\presentation\hud\vitals\VitalsView.h`, `D:\LightYears\LightYearsGame\src\presentation\hud\vitals\VitalsView.cpp`
- `D:\LightYears\LightYearsGame\include\presentation\hud\vitals\VitalsHUDController.h`, `D:\LightYears\LightYearsGame\src\presentation\hud\vitals\VitalsHUDController.cpp`

**VitalsView : Panel**
- Constructor `shared_ptr<const VitalsViewModel>` alır.
- Kompozisyon: `UILayout::Anchored(UIAnchor::BottomLeft, {20,-20})` ile yerleşen yatay `StackPanel` (spacing ~10, cross align End). Sol çocuk dikey `StackPanel` (spacing 6): energy (220x18), shield (220x18), health (220x30) `ValueGauge`. Sağ çocuk yatay `StackPanel`: life ikonu (`SpaceShooterRedux/PNG/pickups/playerLife1_blue.png`), life text (20), score ikonu (`SpaceShooterRedux/PNG/Power-ups/star_gold.png`), score text (20). Renkler ve boyutlar mevcut `GameHUD` constructor'ındaki değerlerle aynıdır. Piksel piksel eşlik şart değil, yaklaşık yerleşim yeterli.
- `Tick`: `mWatcher.Consume(mViewModel->revision)` true ise gauge değerleri, `ComputeHealthBarColor` sonucu ve life/score metinleri güncellenir. Test için `GetRefreshCount()` sayacı tutulur.

**VitalsHUDController : HUDController** (`D:\LightYears\LightYearsGame\include\presentation\hud\HUDController.h`)
- Constructor `weak_ptr<GameHUD>` alır, `VitalsPresenter`'ı sahiplenir. İlk `Tick`'te HUD kilitlenebiliyorsa `hud->AddToLayer<VitalsView>(UILayer::Hud, presenter.GetViewModel())` ile view oluşturulur.
- `Tick`: `mPresenter.Tick()`.
- Destructor: view hâlâ yaşıyorsa `DestroyWidget()` çağrılır.
- `D:\LightYears\LightYearsGame\src\level\GameLevel.cpp` içindeki `CreateGameHUD`'a `AddHUDController(std::make_shared<VitalsHUDController>(mGameHUD));` eklenir (mevcut controller'ların yanına).

**GameHUD temizliği** (`D:\LightYears\LightYearsGame\include\widget\GameHUD.h`, `D:\LightYears\LightYearsGame\src\widget\GameHUD.cpp`)
- Kaldırılacaklar: `mPlayerHealthBar`, `mPlayerShieldBar`, `mPlayerEnergyBar`, `mPlayerLifeIcon`, `mPlayerLifeText`, `mPlayerScoreIcon`, `mPlayerScoreText`; bunların constructor, `Init`, `Draw` satırları; `RefreshHealthBar`, `RefreshHealthBarDeferred`, `PlayerHealthUpdated`, `PlayerShieldUpdated`, `RefreshEnergyBar`, `PlayerSpaceShipDestroyed`, `ConnectStatus`, `BindStatusToPlayer`, `UnbindStatusFromPlayer`, `OnPlayerAboutToBeDestroyed`, `OnPlayerCreated`, `DisconnectStatus`, `RefreshPlayerHUDState`, `PlayerLifeUpdated`, `PlayerScoreUpdated` ve yalnız bunların kullandığı üyeler (`mRefreshHealthBarTimerHandle`, `mIsPlayerManagerObserved`, player handle'ları, `mObservedPlayerId`).
- **Dikkat:** `mObservedPlayerSpaceShip` dosyada 6 yerde geçiyor. Hasar sayıları, hız veya uyarı kodu tarafından hâlâ kullanılıyorsa korunur. Worker her kullanımı okuyup karar verir ve raporda gerekçesini yazar.
- Korunacaklar: frame rate, hız yazısı, hasar sayıları, gameplay uyarıları, boss barı, timer, dinamik bildirimler, `GameHUDDamageE2ETestAccess`.
- `GameHUDPlayerRestartTestAccess` friend'i ve ilgili erişimler kaldırılır. Test ADIM 9'da yeniden yazılır. `D:\LightYears\LightYearsGame\tests\GameHUDPlayerRestartTests.cpp` bu adımda derlenebilir kalacak şekilde güncellenmelidir: aynı `RunGameHUDPlayerRestartTests()` imzası korunur, erişim `VitalsPresenter` üzerinden yapılır (life/score kontrolü `GetViewModel()` ile).

**Kabul:** `GameHUD` vitals kodu içermez; oyun açıldığında barlar, can ve skor sol altta görünür; respawn sonrası yeni gemiye bağlanır.

**Doğrulama:** hedefler `LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests`, ctest `^(LightYearsGameHUDDamageEventE2E|LightYearsPlayerLevelTestProgressionE2E|LightYearsGameHUDPlayerRestart)$`. Player restart senaryosu `LightYearsGameHUDPlayerRestart` adlı izole CTest kaydıyla çalıştırılır.

## ADIM 9: Vitals testleri (game)

**Dosyalar**
- `D:\LightYears\LightYearsGame\tests\GameHUDPlayerRestartTests.cpp` (presenter tabanlı hale getirilir, mevcut senaryolar korunur: player yeniden oluşturma ve kaldırma sırasında mutation)
- Yeni: `D:\LightYears\LightYearsGame\tests\VitalsHUDTests.cpp`. Yeni executable `LightYearsVitalsHUDTests`, `LightYearsPlayerLevelTestProgressionE2ETests` CMake deseniyle (`LIGHT_YEARS_GAS_LITE_TEST_SOURCES`, include ve link listesi, `LIGHT_YEARS_PROJECT_SOURCE_DIR`) oluşturulur. `add_test(NAME LightYearsVitalsHUD ...)`, `TIMEOUT 60`. Dünya ve oyuncu kurulumu `D:\LightYears\LightYearsGame\tests\PlayerLevelTestProgressionE2E.cpp`'den örnek alınır.

**Senaryolar**
1. `ComputeHealthBarColor`: maxHealth 0 → kırmızı; overcap → mavi; %100 → (0,255,0); %50 → (255,255,0); %0 → (255,0,0).
2. Presenter bağlanınca VM değerleri geminin health/shield/energy değerlerine eşit ve `hasShip=true`.
3. Hasar sonrası bir `Tick` → health ve displayHealth geminin güncel değerlerini yansıtır, revision değişir. Birden fazla alan değişebileceği için artışın tam olarak bir olması beklenmez. Değişiklik olmadan iki `Tick` → revision değişmez.
4. Gemi yok edilince sonraki `Tick`'te `hasShip=false` ve barlar 0/1. Respawn sonrası yeni gemiye bağlanır, eski geminin değerleri VM'ye sızmaz.
5. Life/score delegate'leri VM'yi günceller. Player yeniden oluşturulunca yeni player'a bağlanır. Eski player kaldırılırken crash olmaz ve eski player'dan gelen broadcast VM'yi değiştirmez.
6. `VitalsView`: ilk `Tick` widget değerlerini uygular. Değişiklik yokken ardışık Tick'lerde `GetRefreshCount()` artmaz; iki View Tick'i arasında birden fazla VM alanı değişse de sonraki `Tick` son değerleri uygular ve sayaç bir kez artar. View, presenter/controller yok edildikten sonra da güvenle tick alır (shared_ptr sahipliği).

**Doğrulama:** hedefler `LightYearsVitalsHUDTests LightYearsGasLiteTests`, ctest `^(LightYearsVitalsHUD|LightYearsUIFoundation|LightYearsGameHUDPlayerRestart)$`. Restart senaryosu ayrı `LightYearsGameHUDPlayerRestart` CTest kaydıyla doğrulanır.

## ADIM 10: Dokümantasyon ve "yeni UI elemanı" tarifi

**Dosya:** `D:\LightYears\docs\PROJECT_DOCUMENTATION.md`. Mevcut HUD/UI bölümüne eklenir, yoksa yeni "UI Mimarisi" bölümü açılır. Diğer bölümler değiştirilmez.

İçerik:
- Veri akışı diyagramı ve 7 maddelik mimari sözleşme (bu planın başından özetlenir).
- Engine yapı taşları: `UILayout`/`UIAnchor`, `Widget` ağacı, `Panel`/`StackPanel`, `UILayer`, `SubscriptionSet`, `UIRevision`/`SetIfChanged`, `UIStyle`. Her biri için tek cümle ve dosya yolu.
- **Yeni UI elemanı ekleme tarifi (5 adım):** ViewModel struct'ı → Presenter (bağlama + `SetIfChanged`) → View (`Panel` alt sınıfı, `UIRevisionWatcher`) → `HUDController` ile `AddToLayer` → `LIGHT_YEARS_UI_HUD_SOURCES` + test. Referans uygulama: `presentation/hud/vitals/`.
- Bilinen sonraki işler (bu planın kapsamı dışında): `AbilityUIController`'ın desene taşınması, boss barı ve uyarıların taşınması, hasar sayıları için dünya-uzayı anchor'u, menü HUD'larının katmanlara taşınması, gerekirse JSON layout.

**Doğrulama:** doküman adımı build gerektirmez. Uçtan uca son kontrol için tüm hedefler tek build sürecinde derlenir:

~~~bat
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && set VSLANG=1033 && cmake --build build --target LightYearsGame LightYearsUIFoundationTests LightYearsVitalsHUDTests LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests LightYearsPlayerLevelTestProgressionE2ETests --parallel 4'
~~~

Ardından CTest kayıtları çalıştırılır:

~~~powershell
ctest --test-dir build -R '^(LightYearsUIFoundation|LightYearsVitalsHUD|LightYearsGameHUDDamageEventE2E|LightYearsPlayerLevelTestProgressionE2E|LightYearsGameHUDPlayerRestart)$' --output-on-failure --timeout 60
~~~

### Uygulama sonucu ve doğrulama (2026-09-30)

ADIM 10 dokümantasyonu [PROJECT_DOCUMENTATION.md](PROJECT_DOCUMENTATION.md) §8 içine eklendi. Gerçek sahip yolları ve Vitals boyutları kaynakla karşılaştırıldı; veri akışı sözleşmesi, 5 adımlı ekleme tarifi ve kapsam dışı işler eklendi.

Uygulama sırasında netleşen noktalar:

- `Panel::GetBound()` döndürdüğü değer gerçek arka plan şeklinin global bounds'udur. `Panel::PlaceAt()` panel boyutunu ayarlar ve `Widget::PlaceAt()` ile temel konum hareketini tamamlar.
- Vitals controller `GameLevel::CreateGameHUD()` içine değil, `GameLevel::CreateHUDControllers()` içine kaydedilir; View'ı HUD `HasInit()` olduktan sonra ekler.
- Player unique ID `0` geçerli olduğundan Presenter, `mHasBoundPlayer` ve `mHasPlayerBeingDestroyed` bayraklarını ID değerinden ayrı tutar.
- Player restart doğrulaması `LightYearsGameHUDPlayerRestart` adlı izole CTest kaydıdır. Vitals uçtan uca testi `LightYearsVitalsHUDTests` çalıştırılabilir dosyasındadır; hasar event testi mevcut `LightYearsContinuousBeamWallE2ETests` çalıştırılabilir dosyasını kullanır. Üç menüdeki base-first event yönlendirmesi ADIM 4 davranışıdır; menü HUD'larının UILayer'a taşındığı anlamına gelmez.

Tek seri build komutu:

~~~bat
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && set VSLANG=1033 && cmake --build build --target LightYearsGame LightYearsUIFoundationTests LightYearsVitalsHUDTests LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests LightYearsPlayerLevelTestProgressionE2ETests --parallel 4'
~~~

Sonuç: exit code 0; altı hedef tamamlandı. CTest komutu:

~~~powershell
ctest --test-dir build -R '^(LightYearsUIFoundation|LightYearsVitalsHUD|LightYearsGameHUDDamageEventE2E|LightYearsPlayerLevelTestProgressionE2E|LightYearsGameHUDPlayerRestart)$' --output-on-failure --timeout 60
~~~

Sonuç: 5/5 geçti. `build/e2e-artifacts/vitals-hud.json` içinde 25/25 kontrol geçti.

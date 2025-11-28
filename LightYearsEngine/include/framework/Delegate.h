#pragma once
#include "framework/Core.h"  // weak_ptr, shared_ptr, List tanýmlarý
#include <functional> // std::function için

namespace ly
{
	// ============================================================================
	// FORWARD DECLARATION (Ýleriye Dönük Bildirim)
	// ============================================================================
	// Object sýnýfýnýn tam tanýmýný yazmadan sadece "var" olduðunu söylüyoruz.
	// Bu sayede circular dependency (döngüsel baðýmlýlýk) önlenir.
	// Sadece pointer/reference kullanýmý için yeterlidir.
	class Object;

	// ============================================================================
	// DELEGATE CLASS - Event/Observer Pattern Implementation
	// ============================================================================
	// 
	// TEMPLATE SYNTAX AÇIKLAMASI:
	// template<typename... Args>
	//          ^^^^^^^^ ^^^^ ^^^^
	//      |        |  ???? Parametre paketi adý (istediðin isim olabilir)
	//    |        ????????? Üç nokta = "variadic" (deðiþken sayýda)
	//          ?????????????????? "typename" veya "class" kullanýlabilir (ayný anlam)
	//
	// VARIADIC TEMPLATE:
	// - Ýstediðin kadar tip parametresi alabilir
	// - Compiler her kullaným için farklý kod üretir
	// 
	// ÖRNEKLER:
	// Delegate<>   ? Args = hiç parametre yok
	// Delegate<float>              ? Args = float
	// Delegate<float, int>    ? Args = float, int
	// Delegate<float, int, string>   ? Args = float, int, string
	//
	// Bu sayede ayný Delegate sýnýfýný farklý imzalardaki eventler için kullanabiliriz.
	
	template<typename... Args>
	class Delegate
	{
	public:
		// ====================================================================
		// BINDACTION - Üye Fonksiyonu Event'e Baðlama (weak_ptr versiyonu)
		// ====================================================================
		//
		// TEMPLATE FUNCTION SYNTAX:
		// template<typename ClassName>
		//   ^^^^^^^^ ^^^^^^^^^
		//          |        ???? Tip parametresi adý (compiler otomatik çýkarýr)
		//          ????????????? "typename" = bu bir tip olacak
		//
		// FUNCTION SIGNATURE AÇIKLAMASI:
		// void BindAction(weak_ptr<Object> obj, void(ClassName::* callback)(Args...))
		// ^^^^ ^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		// |    | |                 |
		// |    |           |        ?? Member function pointer parametresi
		// ||    ??????????????????????? Nesne referansý parametresi
		// |    ??????????????????????????????????? Fonksiyon adý
		// ???????????????????????????????????????? Dönüþ tipi (void = bir þey döndürmez)
		//
		// MEMBER FUNCTION POINTER SYNTAX DETAYI:
		// void(ClassName::* callback)(Args...)
		// ^^^^ ^^^^^^^^^^ ^ ^^^^^^^^  ^^^^^^^^
		// |    |          | |         ?????? Fonksiyonun parametreleri (variadic)
		// |    |          | ???????????????? Pointer deðiþken adý
		// |    |          ?????????????????? Pointer to member operator
		// |    ????????????????????????????? Hangi sýnýfýn üyesi
		// ?????????????????????????????????? Fonksiyonun dönüþ tipi
		//
		// ÖRNEK:
		// void(Spaceship::* callback)(float, float, float)
		// Bu: "Spaceship sýnýfýnýn, void dönen ve 3 float alan üye fonksiyonu"
		
		template<typename ClassName>
		void BindAction(weak_ptr<Object> obj, void(ClassName::* callback)(Args...))
		{
			// ================================================================
			// LAMBDA EXPRESSION - Anonim Fonksiyon
			// ================================================================
			//
			// LAMBDA SYNTAX:
			// [capture list](parameters) -> return_type { function body }
			//  ^^^^^^^^^^^^^ ^^^^^^^^^^    ^^^^^^^^^^^^   ^^^^^^^^^^^^^^
			//  |             |       |      ?? Fonksiyon gövdesi
			//  |             |      ????????????????? Dönüþ tipi (opsiyonel)
			//  |     ??????????????????????????????? Parametreler
			//  ????????????????????????????????????????????? Yakalanan deðiþkenler
			//
			// CAPTURE LIST AÇIKLAMASI:
			// [obj, callback] = "copy capture" (deðeri kopyala)
			//  ^^^  ^^^^^^^^
			//  |    ???????????? callback deðiþkenini yakala
			//  ????????????????? obj deðiþkenini yakala
			//
			// Alternatif capture yöntemleri:
			// []            = hiçbir þey yakalama
			// [=]       = tüm dýþ deðiþkenleri kopyala
			// [&]   = tüm dýþ deðiþkenleri referans olarak yakala
			// [obj, &count] = obj'yi kopyala, count'u referans al
			//
			// NEDEN COPY CAPTURE?
			// - obj ve callback deðerleri lambda oluþturulduðunda "dondurulur"
			// - Lambda daha sonra çaðrýldýðýnda bu deðerleri kullanýr
			// - BindAction fonksiyonu bittiðinde bile lambda içinde yaþarlar
			//
			// STD::FUNCTION SYNTAX:
			// std::function<bool(Args...)>
			//   ^^^^ ^^^^^^^^
			//          |    ?????????? Fonksiyonun parametreleri
			//  ??????????????? Fonksiyonun dönüþ tipi
			//
			// STD::FUNCTION NEDÝR?
			// - "Type erasure" (tip silme) kullanan bir wrapper
			// - Herhangi bir "callable" (çaðrýlabilir) nesneyi sarar:
			//   * Function pointer
			//   * Lambda expression
			//   * Functor (operator() overload eden class)
			//   * std::bind sonucu
			// - Farklý tiplerdeki callable'larý ayný tipte saklamamýzý saðlar
			//
			// BOOL DÖNÜÞ TÝPÝ NEDEN?
			// - true  = callback baþarýyla çalýþtý (nesne yaþýyor)
			// - false = callback baþarýsýz (nesne silinmiþ, bu listener'ý temizle)
			
			std::function<bool(Args...)> callBackFunc = [obj, callback](Args... args)->bool
			{
				// ============================================================
				// LAMBDA BODY - Lambda Fonksiyonunun Ýçi
				// ============================================================
				
				// WEAK_PTR::EXPIRED() AÇIKLAMASI:
				// - weak_ptr'nin gösterdiði nesne silinmiþ mi kontrol eder
				// - false = nesne yaþýyor, güvenle kullanabiliriz
				// - true  = nesne silinmiþ, kullanýrsak crash olur (dangling pointer)
				//
				// WEAK_PTR NEDEN KULLANILIR?
				// - shared_ptr kullanýrsak circular reference oluþur (memory leak)
				// - raw pointer kullanýrsak dangling pointer riski var (crash)
				// - weak_ptr: nesneyi "gözlemler" ama yaþam döngüsünü etkilemez
				if (!obj.expired())
				{
					// ========================================================
					// MEMBER FUNCTION POINTER CALL - En Karmaþýk Satýr!
					// ========================================================
					//
					// BU SATIRDA NE OLUYOR ADIM ADIM:
					//
					// 1. obj.lock()
					//    ^^^^^^^^^^
					//    weak_ptr'den geçici shared_ptr oluþturur
					//    Bu iþlem "atomic" (thread-safe) bir operasyondur
					//    Dönen shared_ptr, nesnenin silinmesini geçici olarak engeller
					//
					// 2. .get()
					//    ^^^^^^
					//    shared_ptr'den ham pointer (Object*) alýr
					//    shared_ptr'in kendisini deðil, içindeki pointer'ý döndürür
					//    Örnek: shared_ptr<int> sp; int* p = sp.get();
					//
					// 3. static_cast<ClassName*>(...)
					//    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
					//    DOWNCAST: Object* ? ClassName* dönüþümü
					//    - "static_cast" = compile-time type conversion
					//    - "dynamic_cast" deðil çünkü tipin doðru olduðundan eminiz
					//    - Daha hýzlý (runtime type check yok)
					//    - GÜVENLÝ çünkü callback orijinalinde ClassName'den geldi
					//
					// 4. ->*callback
					//    ^^^^^^^^^^^
					//    POINTER-TO-MEMBER OPERATOR
					//    - -> = pointer dereference (ok operatörü)
					//    - * = member pointer dereference
					//    - Birlikte: nesne üzerinde member function pointer çaðýrýr
					//Normal fonksiyon: obj->func()
					//    Member ptr:  obj->*funcPtr
					//
					// 5. (args...)
					//    ^^^^^^^^^
					//    PARAMETER PACK EXPANSION
					// - Args... þablonundaki tüm tipleri açar
					//    - Örnek: Args=<float,int> ise ? (arg1, arg2) olur
					//    - Compiler her tip kombinasyonu için ayrý kod üretir
					//
					// TOPARLAMA:
					// (static_cast<Spaceship*>(obj.lock().get())->*callback)(health, maxHealth);
					//  |      |     ||           |
					//  |         |    |????????????????
					//  |          |           ?? Parametreler
					//  |       ?????????????? Member func ptr
					//  ??????????????????????????????????????????????????????? Spaceship nesnesi
					(static_cast<ClassName*>(obj.lock().get())->*callback)(args...);
					return true;  // Ýþlem baþarýlý - listener hala aktif
				}
				return false;  // Nesne silinmiþ - bu listener'ý temizle
			};
			
			// ================================================================
			// LIST::PUSH_BACK - Listeye Ekleme
			// ================================================================
			// Lambda'yý (std::function'a otomatik dönüþtürülmüþ halde) listeye ekler
			// push_back: Container'ýn sonuna yeni eleman ekler
			// Lambda'nýn copy constructor'ý çaðrýlýr (capture edilen deðerler kopyalanýr)
			mCallbacks.push_back(callBackFunc);
		}

		// ====================================================================
		// BINDACTION - Üye Fonksiyonu Event'e Baðlama (raw pointer versiyonu)
		// ====================================================================
		// Player gibi shared_ptr ile yönetilmeyen ama yaþam süresi garanti
		// edilen nesneler için kullanýlýr. Lifetime kontrolü YAPILMAZ!
		template<typename ClassName>
		void BindAction(ClassName* obj, void(ClassName::* callback)(Args...))
		{
			std::function<bool(Args...)> callBackFunc = [obj, callback](Args... args)->bool
			{
				if (obj != nullptr)
				{
					(obj->*callback)(args...);
					return true;
				}
				return false;
			};
			
			mCallbacks.push_back(callBackFunc);
		}

		// ====================================================================
		// BROADCAST - Event'i Tüm Dinleyicilere Gönder
		// ====================================================================
		//
		// FUNCTION SIGNATURE:
		// void Broadcast(Args... args)
		// ^^^^ ^^^^^^^^^  ^^^^^^^^^^^
		// |    |      ?????????????? Variadic parameters (deðiþken sayýda)
		// |  ????????????????????????? Fonksiyon adý
		// ?????????????????????????????? Dönüþ tipi
		//
		// ARGS... NEDEN?
		// - Template parametresi Args'ý buraya "açýyoruz" (expand)
		// - Delegate<float,int> ise ? void Broadcast(float arg1, int arg2)
		// - Delegate<> ise ? void Broadcast()
		//
		// PERFECT FORWARDING:
		// - args parametreleri doðrudan callback'lere iletilir
		// - Gereksiz kopyalama yapýlmaz (performans)
		
		void Broadcast(Args... args)
		{
			// ================================================================
			// ITERATOR-BASED LOOP - Manuel Iterator Kontrolü
			// ================================================================
			//
			// NORMAL FOR LOOP NEDEN KULLANILMAZ?
			// ? for (auto& callback : mCallbacks)
			//    - Loop içinde eleman silme güvenli deðil!
			//    - Iterator invalidation oluþur
			//    - Undefined behavior (crash, yanlýþ veri, vb.)
			//
			// ? MANUAL ITERATOR LOOP:
			//    - Eleman silerken iterator'ü kontrollü güncelleriz
			//    - erase() yeni geçerli iterator döner
			//
			// AUTO KEYWORD:
			// - Compiler tip çýkarýmý yapar (type deduction)
			// - Gerçek tip: List<std::function<bool(Args...)>>::iterator
			// - Çok uzun tip adlarý yazmaktan kurtarýr
			// - C++11 ile geldi
			//
			// BEGIN() ve END():
			// - begin() = ilk elemaný gösteren iterator
			// - end()   = son elemandan sonraki "sentinel" iterator
			// - end() asla geçerli bir elemana iþaret etmez!
			// - [begin, end) = "half-open range" (matematik notasyonu)
			
			for (auto iter = mCallbacks.begin(); iter != mCallbacks.end();)
			{
				// ============================================================
				// CALLBACK ÇAÐRISI VE SONUÇ KONTROLÜ
				// ============================================================
				//
				// ITERATOR DEREFERENCE:
				// (*iter)
				//  ^^^^^
				// - * operatörü: iterator'ün gösterdiði deðeri al
				// - iter bir pointer gibi davranýr
				// - *iter = std::function<bool(Args...)> nesnesi
				//
				// FUNCTION CALL:
				// (*iter)(args...)
				//        ^^^^^^^^^
				// - std::function'ýn operator() çaðrýlýr
				// - args... = parameter pack expansion
				// - Delegate<float> ise: (*iter)(floatValue)
				// - Ýçindeki lambda çalýþýr
				//
				// RETURN VALUE:
				// bool result = (*iter)(args...);
				// - true  = callback baþarýlý (nesne yaþýyor)
				// - false = callback baþarýsýz (nesne ölmüþ)
				
				if ((*iter)(args...))
				{
					// ========================================================
					// BAÞARILI CALLBACK - Sonraki Elemana Geç
					// ========================================================
					//
					// PRE-INCREMENT vs POST-INCREMENT:
					// ++iter  (pre)  = önce artýr, sonra kullan
					// iter++  (post) = önce kullan, sonra artýr
					//
					// NEDEN PRE-INCREMENT?
					// - Post-increment geçici kopya oluþturur
					//temp = iter; iter += 1; return temp;
					// - Pre-increment doðrudan artýrýr
					//   iter += 1; return iter;
					// - Iterator'ler büyük nesneler olabilir (linked list vb.)
					// - Pre-increment daha verimli
					// - Modern C++ best practice
					++iter;
				}
				else
				{
					// ========================================================
					// BAÞARISIZ CALLBACK - Ölü Listener'ý Temizle
					// ========================================================
					//
					// LIST::ERASE() AÇIKLAMASI:
					// - Belirtilen pozisyondaki elemaný container'dan siler
					// - Element'in destructor'ý çaðrýlýr (std::function silinir)
					// - Container'ýn size'ý 1 azalýr
					// - Memory deallocate edilebilir
					//
					// KRÝTÝK: ITERATOR INVALIDATION!
					// - erase() çaðrýsýndan SONRA eski iterator GEÇERSÝZ!
					// - Kullanmaya devam edersen: UNDEFINED BEHAVIOR
					//   * Crash
					//   * Yanlýþ veri
					//   * Bellek bozulmasý
					//
					// ÇÖZÜM:
					// - erase() yeni geçerli iterator döner
					// - Silinen elemandan SONRAKÝ elemana iþaret eder
					// - Son elemaný sildiyse end() döner
					// - Bu yüzden iter = erase(iter) kullanýmý ZORUNLU!
					//
					// ÖRNEK:
					// [A, B, C, D]
					//     ^
					//     iter (B'yi gösteriyor)
					// iter = erase(iter);
					// [A, C, D]
					//     ^
					//     iter (C'yi gösteriyor - otomatik ilerledi!)
					iter = mCallbacks.erase(iter);
				}
			}
		}

	private:
		// ====================================================================
		// CALLBACK STORAGE - Dinleyici Listesi
		// ====================================================================
		//
		// LIST<T> TÝPÝ:
		// - Core.h'da tanýmlý custom container
		// - Muhtemelen std::list veya std::vector wrapper'ý
		// 
		// STD::LIST vs STD::VECTOR:
		// std::list (doubly linked list):
		//   ? Insertion/deletion: O(1)
		//   ? Iterator invalidation minimal
		//   ? Random access: O(n)
		//   ? Cache unfriendly
		//
		// std::vector (dynamic array):
		//   ? Random access: O(1)
		//   ? Cache friendly
		//   ? Insertion/deletion: O(n)
		//   ? Iterator invalidation riski
		//
		// BU DURUMDA HANGISI DAHA ÝYI?
		// - Eleman silme sýk yapýlýyor ? list daha iyi
		// - Callback sayýsý genelde az ? fark etmez
		//
		// STD::FUNCTION<BOOL(ARGS...)> DETAYLI AÇIKLAMA:
		// 
		// TYPE ERASURE (Tip Silme):
		// - Farklý tiplerdeki callable'larý ayný tipte saklar
		// - Her lambda aslýnda farklý bir tip (compiler unique type üretir)
		// - std::function hepsini "siler" ve tek tipte tutar
		//
		// NASIL ÇALIÞIR?
		// - Içte polymorphism kullanýr (virtual function)
		// - Küçük fonksiyonlar için "small buffer optimization"
		// - Büyük fonksiyonlar için heap allocation
		//
		// MALIYET:
		// - Virtual function call overhead (~5-10ns)
		// - Genelde ihmal edilebilir
		// - Kritik performans gerektiren yerler için alternatif: function pointer
		//
		// FUNCTION SIGNATURE: bool(Args...)
		// - bool = callback durumu (baþarýlý/baþarýsýz)
		// - Args... = event parametreleri
		//
		// ÖRNEK AÇILIM:
		// Delegate<float, int> için:
		// List<std::function<bool(float, int)>> mCallbacks;
		List<std::function<bool(Args...)>> mCallbacks;
	};
}

/*
 * ============================================================================
 * DETAYLI KULLANIM ÖRNEÐÝ
 * ============================================================================
 * 
 * // 1. EVENT TANIMI (Publisher - HealthComponent.h)
 * class HealthComponent
 * {
 * public:
 *     // Event: "Can deðiþtiðinde haber ver"
 * // 3 parametre: deðiþim miktarý, mevcut can, maksimum can
 *     Delegate<float, float, float> onHealthChanged;
 * };
 * 
 * // 2. DÝNLEYÝCÝ EKLEME (Subscriber - Spaceship.cpp)
 * void Spaceship::BeginPlay()
 * {
 *     // "Can deðiþtiðinde benim OnHealthChanged fonksiyonumu çaðýr"
 *     mHealthComponent.onHealthChanged.BindAction(
 *         GetWeakPtr(),              // Bu Spaceship nesnesi
 *       &Spaceship::OnHealthChanged      // Bu fonksiyon
 *     );
 * }
 * 
 * // 3. EVENT TETÝKLEME (Publisher - HealthComponent.cpp)
 * void HealthComponent::ChangeHealth(float amount)
 * {
 *     mHealth += amount;
 *     
 *     // Tüm dinleyicilere haber ver!
 *     onHealthChanged.Broadcast(amount, mHealth, mMaxHealth);
 *     //      ^^^^^^  ^^^^^^^^^^^^^^^^^
 *     //            Arg1    Arg2     Arg3
 * }
 * 
 * // 4. CALLBACK FONKSÝYONU (Subscriber - Spaceship.cpp)
 * void Spaceship::OnHealthChanged(float amt, float health, float maxHealth)
 * {
 * LOG("Health changed: %f/%f (change: %f)", health, maxHealth, amt);
 *     
 *     // Görsel efektler
 *     if (amt < 0.0f)  // Hasar aldý
 * {
 *         PlayHitAnimation();
 *  PlayHitSound();
 *     }
 *     
 *     // Can bitti mi?
 *   if (health <= 0.0f)
 *     {
 *         Destroy();
 *     }
 * }
 * 
 * ============================================================================
 * SYNTAX KISA ÖZET
 * ============================================================================
 * 
 * template<typename... Args>  ? Variadic template (deðiþken sayýda tip)
 * weak_ptr<Object>     ? Zayýf referans (nesne silinse bile güvenli)
 * void(ClassName::*)(Args...) ? Member function pointer
 * [obj, callback]  ? Lambda capture list (deðerleri yakala)
 * (Args... args)    ? Parameter pack expansion
 * ->bool    ? Lambda return type
 * (*iter)(args...)            ? Iterator dereference + function call
 * ++iter           ? Pre-increment (daha verimli)
 * iter = erase(iter)          ? Silme + iterator güncelleme
 * std::function<bool(Args...)>? Type-erased callable wrapper
 */
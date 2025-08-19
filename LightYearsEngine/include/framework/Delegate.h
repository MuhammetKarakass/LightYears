#pragma once
#include "framework/Core.h"  // weak_ptr, shared_ptr, List tanýmlarý için
#include <functional>        // std::function wrapper için

namespace ly
{
	// FORWARD DECLARATION - Ýleriye Dönük Bildirim
	// Object sýnýfýnýn tam tanýmýný header'da yazmak zorunda deðiliz
	// Sadece pointer veya reference kullanýmý için bu yeterli
	// Bu sayede circular dependency (döngüsel baðýmlýlýk) problemlerini önleriz
	class Object;

	// =========================================================================
	// TEMPLATE CLASS DELEGATE - EVENT/OBSERVER PATTERN IMPLEMENTASYONU
	// =========================================================================
	
	// VARIADIC TEMPLATE AÇIKLAMASI:
	// Args... = "Parameter Pack" - istediðin kadar tip parametresi alabilir
	// Örnekler:
	//   * Delegate<>              ? parametresiz callback'ler
	//   * Delegate<float>         ? tek float parametreli callback'ler  
	//   * Delegate<float, int>    ? iki parametreli callback'ler
	//   * Delegate<float, int, string> ? üç parametreli callback'ler
	//
	// Bu sistem sayesinde farklý imzalardaki fonksiyonlarý ayný Delegate sistemiyle yönetebiliriz
	template<typename... Args>
	class Delegate
	{
	public:
		// =====================================================================
		// MEMBER FUNCTION BINDING - Üye Fonksiyon Baðlama Ýþlemi
		// =====================================================================
		
		// TEMPLATE FUNCTION AÇIKLAMASI:
		// Bu fonksiyon da template - ClassName tipi otomatik olarak çýkarýlýr
		// Örnek: &SpaceShip::OnHealthChanged geçirdiðinde ClassName = SpaceShip olur
		//
		// PARAMETRELER:
		// 1. weak_ptr<Object> obj    ? Callback sahibi nesnenin güvenli referansý
		// 2. Member function pointer ? Çaðrýlacak üye fonksiyonun adresi
		//
		// MEMBER FUNCTION POINTER SYNTAX AÇIKLAMASI:
		// void(ClassName::* callback)(Args...)
		//   ?        ?         ?        ?
		//   ?        ?         ?        ?? Fonksiyonun parametreleri (variadic)
		//   ?        ?         ?? Fonksiyonun adý (pointer olarak)
		//   ?        ?? Hangi sýnýfýn üye fonksiyonu
		//   ?? Fonksiyonun dönüþ tipi
		template<typename ClassName>
		void BindAction(weak_ptr<Object> obj, void(ClassName::* callback)(Args...))
		{
			// =============================================================
			// STD::FUNCTION WRAPPER OLUÞTURMA
			// =============================================================
			
			// STD::FUNCTION NEDÝR?
			// - Herhangi bir "çaðrýlabilir" þeyi (function pointer, lambda, functor) sarabilen wrapper
			// - Type erasure kullanýr - farklý tiplerdeki callable'larý ayný tipte saklar
			// - Bu sayede vector/list içinde farklý tipte callback'leri saklayabiliriz
			//
			// BOOL DÖNÜÞ TÝPÝ NEDÝR?
			// - true  = callback baþarýyla çalýþtý (nesne hala yaþýyor)
			// - false = callback baþarýsýz (nesne silinmiþ, temizle)
			std::function<bool(Args...)> callBackFunc = [obj, callback](Args... args)->bool
			{
				// =========================================================
				// LAMBDA EXPRESSION AÇIKLAMASI
				// =========================================================
				
				// LAMBDA SYNTAX:
				// [capture list](parameters) -> return_type { body }
				//
				// [obj, callback] = CAPTURE LIST
				// - obj ve callback deðiþkenlerini lambda içine "kopyalar"
				// - Lambda oluþturulduðunda bu deðerler "dondurulur"
				// - Lambda her çaðrýldýðýnda ayný obj ve callback deðerleri kullanýlýr
				//
				// (Args... args) = PARAMETER LIST
				// - Lambda çaðrýldýðýnda alacaðý parametreler
				// - Args... = variadic template expansion
				// - Örnek: Delegate<float,int> için (float arg1, int arg2) olur
				//
				// ->bool = RETURN TYPE SPECIFICATION
				// - Lambda'nýn döneceði tip (açýk belirtim)
				// - Compiler otomatik çýkarsayabilir ama açýk yazmak daha net
				
				// =========================================================
				// WEAK_PTR GÜVENLÝK KONTROLÜ
				// =========================================================
				
				// WEAK_PTR NEDÝR?
				// - shared_ptr'ye "zayýf" referans - nesnenin yaþam döngüsünü etkilemez
				// - Nesne silindiðinde otomatik olarak "expired" (süresi dolmuþ) olur
				// - Circular reference problemlerini çözer
				//
				// EXPIRED() KONTROLÜ:
				// - false = nesne hala yaþýyor, güvenle kullanabiliriz
				// - true  = nesne silinmiþ, kullanamayzý (dangling pointer olur)
				if (!obj.expired())
				{
					// =====================================================
					// MEMBER FUNCTION POINTER ÇAÐRISI - EN KARMAÞIK KISIM
					// =====================================================
					
					// Bu satýr C++'ýn en karmaþýk syntax'larýndan biri!
					// Adým adým açýklayalým:
					//
					// 1. obj.lock()
					//    - weak_ptr'den geçici shared_ptr elde et
					//    - Bu iþlem "atomic" - thread-safe
					//    - shared_ptr nesnenin silinmesini engeller
					//
					// 2. .get()
					//    - shared_ptr'den ham pointer (Object*) al
					//    - shared_ptr'in kendisi deðil, içindeki pointer
					//
					// 3. static_cast<ClassName*>()
					//    - Object* tipinden ClassName* tipine dönüþtür
					//    - "Downcast" iþlemi - base'den derived'a geçiþ
					//    - GÜVENLÝ çünkü callback orijinalinde ClassName'den geldi
					//    - dynamic_cast yerine static_cast kullanýyoruz (daha hýzlý)
					//
					// 4. ->*callback
					//    - POINTER-TO-MEMBER OPERATOR
					//    - Nesne üzerinde member function pointer'ý çaðýrýr
					//    - -> = pointer dereference
					//    - * = member pointer dereference
					//    - callback = member function pointer deðiþkeni
					//
					// 5. (args...)
					//    - PARAMETER PACK EXPANSION
					//    - Args... tiplerindeki tüm parametreleri fonksiyona geçir
					//    - Compiler her tip için ayrý kod üretir
					//
					// ÖRNEK AÇILIM:
					// Delegate<float, int> için:
					// (static_cast<SpaceShip*>(obj.lock().get())->*callback)(arg1, arg2);
					(static_cast<ClassName*>(obj.lock().get())->*callback)(args...);
					return true;  // Ýþlem baþarýlý - listener hala aktif
				}
				return false;  // Nesne silinmiþ - bu listener'ý temizle
			};
			
			// =============================================================
			// CALLBACK LÝSTESÝNE EKLEME
			// =============================================================
			
			// PUSH_BACK() AÇIKLAMASI:
			// - Container'ýn (vector/list) sonuna yeni eleman ekler
			// - Lambda otomatik olarak std::function'a dönüþtürülür
			// - std::function'ýn type erasure özelliði sayesinde farklý lambda'lar ayný tipte saklanýr
			// - Memory allocation yapýlabilir (vector grow)
			mCallbacks.push_back(callBackFunc);
		}

		// =====================================================================
		// EVENT BROADCAST - Tüm Dinleyicilere Olay Gönderme
		// =====================================================================
		
		// BROADCAST NEDÝR?
		// - Tüm kayýtlý callback'leri sýrayla çaðýrýr
		// - "Multicast" - birden fazla dinleyiciye ayný anda haber verir
		// - Observer Pattern'ýn kalbi - publisher'dan subscriber'lara mesaj
		//
		// VARIADIC PARAMETERS:
		// Args... args - istediðin kadar parametre geçebilirsin
		// Compiler her kullaným için farklý kod üretir
		// Perfect forwarding ile performans korunur
		void Broadcast(Args... args)
		{
			// =============================================================
			// ITERATOR-BASED LOOP - Manuel Ýterator Kontrolü
			// =============================================================
			
			// NEDEN NORMAL FOR LOOP DEÐÝL?
			// - Çünkü loop sýrasýnda elemanlarý silmemiz gerekebilir
			// - Normal for(auto& item : container) ile silme güvenli deðil
			// - Iterator invalidation problemleri oluþur
			//
			// AUTO KEYWORD:
			// - Compiler iterator tipini otomatik çýkarýr
			// - Örnek gerçek tip: std::vector<std::function<bool(Args...)>>::iterator
			// - Çok uzun tip adlarý yazmaktan kurtarýr
			//
			// BEGIN() ve END():
			// - begin() = container'ýn ilk elemanýný iþaret eden iterator
			// - end()   = container'ýn son elemandan sonrasýný iþaret eden "sentinel" iterator
			// - end() asla valid bir elemana iþaret etmez - sadece "bitiþ" marker'ý
			for (auto iter = mCallbacks.begin(); iter != mCallbacks.end();)
			{
				// =========================================================
				// CALLBACK ÇAÐRISI VE GÜVENLÝK KONTROLÜ
				// =========================================================
				
				// ITERATOR DEREFERENCE:
				// (*iter) - iterator'ün iþaret ettiði std::function nesnesini al
				// operator* overload'u sayesinde mümkün
				//
				// FUNCTION CALL:
				// (args...) - std::function'ý çaðýr, parametreleri geçir
				// Parameter pack expansion burada da devreye girer
				//
				// RETURN VALUE CHECK:
				// std::function bool döner - callback baþarýlý mý?
				// true  = nesne yaþýyor, callback çalýþtý
				// false = nesne ölmüþ, callback çalýþmadý
				if ((*iter)(args...))
				{
					// =====================================================
					// BAÞARILI CALLBACK - Sonraki Elemana Geç
					// =====================================================
					
					// PRE-INCREMENT:
					// ++iter - iterator'ü bir sonraki pozisyona taþý
					// 
					// NEDEN POST-INCREMENT (iter++) DEÐÝL?
					// - Pre-increment daha verimli
					// - Post-increment geçici copy oluþturur
					// - Iterator'ler aðýr nesneler olabilir
					// - Modern C++ best practice
					++iter;
				}
				else
				{
					// =====================================================
					// BAÞARISIZ CALLBACK - Ölü Listener'ý Temizle
					// =====================================================
					
					// ERASE() OPERASYONU:
					// - Belirtilen pozisyondaki elemaný container'dan sil
					// - Element destructor'ý çaðrýlýr
					// - Container size azalýr
					// - Memory deallocate edilebilir
					//
					// KRÝTÝK NOKTA - ITERATOR INVALIDATION:
					// - erase() çaðrýsýndan sonra eski iterator GEÇERSÝZ olur!
					// - Kullanýrsan undefined behavior (crash, corruption)
					// - erase() yeni geçerli iterator döner
					// - Bu nedenle iter = erase(iter) kullanýmý ZORUNLU
					//
					// DÖNEYÝ DÝKKAT:
					// - erase() silinen elemandan SONRAKÝ elemana iþaret eden iterator döner
					// - Eðer son elemaný sildiysek end() döner
					// - Loop condition (iter != end()) ile otomatik kontrol
					iter = mCallbacks.erase(iter);
				}
			}
		}

	private:
		// =====================================================================
		// CALLBACK STORAGE - Dinleyici Listesi
		// =====================================================================
		
		// LIST<> CONTAINER:
		// - Custom container type (muhtemelen std::list veya std::vector wrapper'ý)
		// - std::list = doubly linked list (insertion/deletion O(1))
		// - std::vector = dynamic array (random access O(1), resize O(n))
		//
		// STD::FUNCTION<BOOL(ARGS...)> AÇIKLAMASI:
		// - Type-erased callable wrapper - "tip silici"
		// - Herhangi bir callable'ý (lambda, function pointer, functor) sarar
		// - Virtual function call overhead'i var ama tip güvenliði saðlar
		// - Function signature: bool(Args...)
		//   * bool return type  ? callback durumu (baþarýlý/baþarýsýz)
		//   * Args... parameters ? event data tipleri
		//
		// NEDEN TYPE ERASURE?
		// - Farklý lambda'lar aslýnda farklý tipler
		// - [obj1, callback1] ve [obj2, callback2] farklý closure tipleri
		// - std::function sayesinde ayný container'da saklayabiliriz
		// - Polymorphism benzeri davranýþ ama compile-time
		List<std::function<bool(Args...)>> mCallbacks;
	};
}

/*
 * =========================================================================
 * DETAYLI KULLANIM ÖRNEÐÝ VE AÇIKLAMALAR
 * =========================================================================
 * 
 * 1. EVENT TANIMI (Publisher tarafýnda - HealthComponent içinde):
 *    
 *    Delegate<float, float, float> onHealthChanged;
 *    
 *    Bu satýr þu anlama gelir:
 *    - onHealthChanged bir event'dir
 *    - Callback fonksiyonlarý 3 tane float parametre almalýdýr
 *    - Ýmza: void(float healthChange, float currentHealth, float maxHealth)
 * 
 * 2. ABONE OLMA (Subscriber tarafýnda - SpaceShip::BeginPlay içinde):
 *    
 *    mHealthComponent.onHealthChanged.BindAction(GetWeakPtr(), &SpaceShip::OnHealthChanged);
 *    
 *    Bu satýrda neler oluyor:
 *    - GetWeakPtr() = SpaceShip nesnesinin weak_ptr referansýný al
 *    - &SpaceShip::OnHealthChanged = SpaceShip'in OnHealthChanged metodunun adresi
 *    - BindAction template fonksiyonu ClassName=SpaceShip olarak çýkarýr
 *    - Member function pointer lambda ile sarýlýr
 *    - Lambda callback listesine eklenir
 * 
 * 3. EVENT TETÝKLEME (Publisher tarafýnda - HealthComponent::ChangeHealth içinde):
 *    
 *    onHealthChanged.Broadcast(amount, mHealth, mMaxHealth);
 *    
 *    Bu satýrda neler oluyor:
 *    - Tüm kayýtlý callback'ler sýrayla çaðrýlýr
 *    - Her callback'e 3 parametre geçirilir: (amount, mHealth, mMaxHealth)
 *    - Ölü referanslar (expired weak_ptr) otomatik temizlenir
 *    - Sadece yaþayan nesnelerin callback'leri çalýþýr
 * 
 * 4. CALLBACK FONKSÝYONU (Subscriber tarafýnda - SpaceShip içinde):
 *    
 *    void SpaceShip::OnHealthChanged(float amt, float health, float maxHealth)
 *    {
 *        LOG("SpaceShip health changed: %f/%f (change: %f)", health, maxHealth, amt);
 *        // Saðlýk deðiþimine göre aksiyon al (animasyon, ses, UI update vb.)
 *    }
 *    
 *    Bu fonksiyon:
 *    - Event tetiklendiðinde otomatik çaðrýlýr
 *    - SpaceShip silinirse artýk çaðrýlmaz (weak_ptr güvenliði)
 *    - Thread-safe deðil - ana thread'de çaðrýlýr
 * 
 * =========================================================================
 * MEMORY YÖNETÝMÝ VE GÜVENLÝK
 * =========================================================================
 * 
 * 1. WEAK_PTR GÜVENLÝÐÝ:
 *    - SpaceShip silinirse weak_ptr otomatik "expired" olur
 *    - Broadcast sýrasýnda expired callback'ler temizlenir
 *    - Dangling pointer crash'leri önlenir
 *    - Manual unsubscribe gerekmez
 * 
 * 2. EXCEPTION SAFETY:
 *    - Callback içinde exception atýlýrsa ne olur?
 *    - Þu anki kod exception'larý yakalamaz
 *    - Exception propagate olur, broadcast yarýda kesilir
 *    - Production'da try-catch eklemek gerekebilir
 * 
 * 3. THREAD SAFETY:
 *    - Bu implementasyon thread-safe DEÐÝL
 *    - Ayný anda BindAction ve Broadcast çaðrýlýrsa race condition
 *    - Multi-thread kullanýmda mutex gerekir
 * 
 * 4. PERFORMANCE:
 *    - std::function virtual call overhead'i var
 *    - weak_ptr.lock() atomic operasyon - maliyetli
 *    - Çok sýk çaðrýlan event'ler için optimize edilebilir
 *    - Ancak bu overhead çoðu zaman ihmal edilebilir
 */
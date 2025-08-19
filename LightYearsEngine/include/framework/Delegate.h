#pragma once
#include "framework/Core.h"  // weak_ptr, shared_ptr, List tan�mlar� i�in
#include <functional>        // std::function wrapper i�in

namespace ly
{
	// FORWARD DECLARATION - �leriye D�n�k Bildirim
	// Object s�n�f�n�n tam tan�m�n� header'da yazmak zorunda de�iliz
	// Sadece pointer veya reference kullan�m� i�in bu yeterli
	// Bu sayede circular dependency (d�ng�sel ba��ml�l�k) problemlerini �nleriz
	class Object;

	// =========================================================================
	// TEMPLATE CLASS DELEGATE - EVENT/OBSERVER PATTERN IMPLEMENTASYONU
	// =========================================================================
	
	// VARIADIC TEMPLATE A�IKLAMASI:
	// Args... = "Parameter Pack" - istedi�in kadar tip parametresi alabilir
	// �rnekler:
	//   * Delegate<>              ? parametresiz callback'ler
	//   * Delegate<float>         ? tek float parametreli callback'ler  
	//   * Delegate<float, int>    ? iki parametreli callback'ler
	//   * Delegate<float, int, string> ? �� parametreli callback'ler
	//
	// Bu sistem sayesinde farkl� imzalardaki fonksiyonlar� ayn� Delegate sistemiyle y�netebiliriz
	template<typename... Args>
	class Delegate
	{
	public:
		// =====================================================================
		// MEMBER FUNCTION BINDING - �ye Fonksiyon Ba�lama ��lemi
		// =====================================================================
		
		// TEMPLATE FUNCTION A�IKLAMASI:
		// Bu fonksiyon da template - ClassName tipi otomatik olarak ��kar�l�r
		// �rnek: &SpaceShip::OnHealthChanged ge�irdi�inde ClassName = SpaceShip olur
		//
		// PARAMETRELER:
		// 1. weak_ptr<Object> obj    ? Callback sahibi nesnenin g�venli referans�
		// 2. Member function pointer ? �a�r�lacak �ye fonksiyonun adresi
		//
		// MEMBER FUNCTION POINTER SYNTAX A�IKLAMASI:
		// void(ClassName::* callback)(Args...)
		//   ?        ?         ?        ?
		//   ?        ?         ?        ?? Fonksiyonun parametreleri (variadic)
		//   ?        ?         ?? Fonksiyonun ad� (pointer olarak)
		//   ?        ?? Hangi s�n�f�n �ye fonksiyonu
		//   ?? Fonksiyonun d�n�� tipi
		template<typename ClassName>
		void BindAction(weak_ptr<Object> obj, void(ClassName::* callback)(Args...))
		{
			// =============================================================
			// STD::FUNCTION WRAPPER OLU�TURMA
			// =============================================================
			
			// STD::FUNCTION NED�R?
			// - Herhangi bir "�a�r�labilir" �eyi (function pointer, lambda, functor) sarabilen wrapper
			// - Type erasure kullan�r - farkl� tiplerdeki callable'lar� ayn� tipte saklar
			// - Bu sayede vector/list i�inde farkl� tipte callback'leri saklayabiliriz
			//
			// BOOL D�N�� T�P� NED�R?
			// - true  = callback ba�ar�yla �al��t� (nesne hala ya��yor)
			// - false = callback ba�ar�s�z (nesne silinmi�, temizle)
			std::function<bool(Args...)> callBackFunc = [obj, callback](Args... args)->bool
			{
				// =========================================================
				// LAMBDA EXPRESSION A�IKLAMASI
				// =========================================================
				
				// LAMBDA SYNTAX:
				// [capture list](parameters) -> return_type { body }
				//
				// [obj, callback] = CAPTURE LIST
				// - obj ve callback de�i�kenlerini lambda i�ine "kopyalar"
				// - Lambda olu�turuldu�unda bu de�erler "dondurulur"
				// - Lambda her �a�r�ld���nda ayn� obj ve callback de�erleri kullan�l�r
				//
				// (Args... args) = PARAMETER LIST
				// - Lambda �a�r�ld���nda alaca�� parametreler
				// - Args... = variadic template expansion
				// - �rnek: Delegate<float,int> i�in (float arg1, int arg2) olur
				//
				// ->bool = RETURN TYPE SPECIFICATION
				// - Lambda'n�n d�nece�i tip (a��k belirtim)
				// - Compiler otomatik ��karsayabilir ama a��k yazmak daha net
				
				// =========================================================
				// WEAK_PTR G�VENL�K KONTROL�
				// =========================================================
				
				// WEAK_PTR NED�R?
				// - shared_ptr'ye "zay�f" referans - nesnenin ya�am d�ng�s�n� etkilemez
				// - Nesne silindi�inde otomatik olarak "expired" (s�resi dolmu�) olur
				// - Circular reference problemlerini ��zer
				//
				// EXPIRED() KONTROL�:
				// - false = nesne hala ya��yor, g�venle kullanabiliriz
				// - true  = nesne silinmi�, kullanamayz� (dangling pointer olur)
				if (!obj.expired())
				{
					// =====================================================
					// MEMBER FUNCTION POINTER �A�RISI - EN KARMA�IK KISIM
					// =====================================================
					
					// Bu sat�r C++'�n en karma��k syntax'lar�ndan biri!
					// Ad�m ad�m a��klayal�m:
					//
					// 1. obj.lock()
					//    - weak_ptr'den ge�ici shared_ptr elde et
					//    - Bu i�lem "atomic" - thread-safe
					//    - shared_ptr nesnenin silinmesini engeller
					//
					// 2. .get()
					//    - shared_ptr'den ham pointer (Object*) al
					//    - shared_ptr'in kendisi de�il, i�indeki pointer
					//
					// 3. static_cast<ClassName*>()
					//    - Object* tipinden ClassName* tipine d�n��t�r
					//    - "Downcast" i�lemi - base'den derived'a ge�i�
					//    - G�VENL� ��nk� callback orijinalinde ClassName'den geldi
					//    - dynamic_cast yerine static_cast kullan�yoruz (daha h�zl�)
					//
					// 4. ->*callback
					//    - POINTER-TO-MEMBER OPERATOR
					//    - Nesne �zerinde member function pointer'� �a��r�r
					//    - -> = pointer dereference
					//    - * = member pointer dereference
					//    - callback = member function pointer de�i�keni
					//
					// 5. (args...)
					//    - PARAMETER PACK EXPANSION
					//    - Args... tiplerindeki t�m parametreleri fonksiyona ge�ir
					//    - Compiler her tip i�in ayr� kod �retir
					//
					// �RNEK A�ILIM:
					// Delegate<float, int> i�in:
					// (static_cast<SpaceShip*>(obj.lock().get())->*callback)(arg1, arg2);
					(static_cast<ClassName*>(obj.lock().get())->*callback)(args...);
					return true;  // ��lem ba�ar�l� - listener hala aktif
				}
				return false;  // Nesne silinmi� - bu listener'� temizle
			};
			
			// =============================================================
			// CALLBACK L�STES�NE EKLEME
			// =============================================================
			
			// PUSH_BACK() A�IKLAMASI:
			// - Container'�n (vector/list) sonuna yeni eleman ekler
			// - Lambda otomatik olarak std::function'a d�n��t�r�l�r
			// - std::function'�n type erasure �zelli�i sayesinde farkl� lambda'lar ayn� tipte saklan�r
			// - Memory allocation yap�labilir (vector grow)
			mCallbacks.push_back(callBackFunc);
		}

		// =====================================================================
		// EVENT BROADCAST - T�m Dinleyicilere Olay G�nderme
		// =====================================================================
		
		// BROADCAST NED�R?
		// - T�m kay�tl� callback'leri s�rayla �a��r�r
		// - "Multicast" - birden fazla dinleyiciye ayn� anda haber verir
		// - Observer Pattern'�n kalbi - publisher'dan subscriber'lara mesaj
		//
		// VARIADIC PARAMETERS:
		// Args... args - istedi�in kadar parametre ge�ebilirsin
		// Compiler her kullan�m i�in farkl� kod �retir
		// Perfect forwarding ile performans korunur
		void Broadcast(Args... args)
		{
			// =============================================================
			// ITERATOR-BASED LOOP - Manuel �terator Kontrol�
			// =============================================================
			
			// NEDEN NORMAL FOR LOOP DE��L?
			// - ��nk� loop s�ras�nda elemanlar� silmemiz gerekebilir
			// - Normal for(auto& item : container) ile silme g�venli de�il
			// - Iterator invalidation problemleri olu�ur
			//
			// AUTO KEYWORD:
			// - Compiler iterator tipini otomatik ��kar�r
			// - �rnek ger�ek tip: std::vector<std::function<bool(Args...)>>::iterator
			// - �ok uzun tip adlar� yazmaktan kurtar�r
			//
			// BEGIN() ve END():
			// - begin() = container'�n ilk eleman�n� i�aret eden iterator
			// - end()   = container'�n son elemandan sonras�n� i�aret eden "sentinel" iterator
			// - end() asla valid bir elemana i�aret etmez - sadece "biti�" marker'�
			for (auto iter = mCallbacks.begin(); iter != mCallbacks.end();)
			{
				// =========================================================
				// CALLBACK �A�RISI VE G�VENL�K KONTROL�
				// =========================================================
				
				// ITERATOR DEREFERENCE:
				// (*iter) - iterator'�n i�aret etti�i std::function nesnesini al
				// operator* overload'u sayesinde m�mk�n
				//
				// FUNCTION CALL:
				// (args...) - std::function'� �a��r, parametreleri ge�ir
				// Parameter pack expansion burada da devreye girer
				//
				// RETURN VALUE CHECK:
				// std::function bool d�ner - callback ba�ar�l� m�?
				// true  = nesne ya��yor, callback �al��t�
				// false = nesne �lm��, callback �al��mad�
				if ((*iter)(args...))
				{
					// =====================================================
					// BA�ARILI CALLBACK - Sonraki Elemana Ge�
					// =====================================================
					
					// PRE-INCREMENT:
					// ++iter - iterator'� bir sonraki pozisyona ta��
					// 
					// NEDEN POST-INCREMENT (iter++) DE��L?
					// - Pre-increment daha verimli
					// - Post-increment ge�ici copy olu�turur
					// - Iterator'ler a��r nesneler olabilir
					// - Modern C++ best practice
					++iter;
				}
				else
				{
					// =====================================================
					// BA�ARISIZ CALLBACK - �l� Listener'� Temizle
					// =====================================================
					
					// ERASE() OPERASYONU:
					// - Belirtilen pozisyondaki eleman� container'dan sil
					// - Element destructor'� �a�r�l�r
					// - Container size azal�r
					// - Memory deallocate edilebilir
					//
					// KR�T�K NOKTA - ITERATOR INVALIDATION:
					// - erase() �a�r�s�ndan sonra eski iterator GE�ERS�Z olur!
					// - Kullan�rsan undefined behavior (crash, corruption)
					// - erase() yeni ge�erli iterator d�ner
					// - Bu nedenle iter = erase(iter) kullan�m� ZORUNLU
					//
					// D�NEY� D�KKAT:
					// - erase() silinen elemandan SONRAK� elemana i�aret eden iterator d�ner
					// - E�er son eleman� sildiysek end() d�ner
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
		// - Custom container type (muhtemelen std::list veya std::vector wrapper'�)
		// - std::list = doubly linked list (insertion/deletion O(1))
		// - std::vector = dynamic array (random access O(1), resize O(n))
		//
		// STD::FUNCTION<BOOL(ARGS...)> A�IKLAMASI:
		// - Type-erased callable wrapper - "tip silici"
		// - Herhangi bir callable'� (lambda, function pointer, functor) sarar
		// - Virtual function call overhead'i var ama tip g�venli�i sa�lar
		// - Function signature: bool(Args...)
		//   * bool return type  ? callback durumu (ba�ar�l�/ba�ar�s�z)
		//   * Args... parameters ? event data tipleri
		//
		// NEDEN TYPE ERASURE?
		// - Farkl� lambda'lar asl�nda farkl� tipler
		// - [obj1, callback1] ve [obj2, callback2] farkl� closure tipleri
		// - std::function sayesinde ayn� container'da saklayabiliriz
		// - Polymorphism benzeri davran�� ama compile-time
		List<std::function<bool(Args...)>> mCallbacks;
	};
}

/*
 * =========================================================================
 * DETAYLI KULLANIM �RNE�� VE A�IKLAMALAR
 * =========================================================================
 * 
 * 1. EVENT TANIMI (Publisher taraf�nda - HealthComponent i�inde):
 *    
 *    Delegate<float, float, float> onHealthChanged;
 *    
 *    Bu sat�r �u anlama gelir:
 *    - onHealthChanged bir event'dir
 *    - Callback fonksiyonlar� 3 tane float parametre almal�d�r
 *    - �mza: void(float healthChange, float currentHealth, float maxHealth)
 * 
 * 2. ABONE OLMA (Subscriber taraf�nda - SpaceShip::BeginPlay i�inde):
 *    
 *    mHealthComponent.onHealthChanged.BindAction(GetWeakPtr(), &SpaceShip::OnHealthChanged);
 *    
 *    Bu sat�rda neler oluyor:
 *    - GetWeakPtr() = SpaceShip nesnesinin weak_ptr referans�n� al
 *    - &SpaceShip::OnHealthChanged = SpaceShip'in OnHealthChanged metodunun adresi
 *    - BindAction template fonksiyonu ClassName=SpaceShip olarak ��kar�r
 *    - Member function pointer lambda ile sar�l�r
 *    - Lambda callback listesine eklenir
 * 
 * 3. EVENT TET�KLEME (Publisher taraf�nda - HealthComponent::ChangeHealth i�inde):
 *    
 *    onHealthChanged.Broadcast(amount, mHealth, mMaxHealth);
 *    
 *    Bu sat�rda neler oluyor:
 *    - T�m kay�tl� callback'ler s�rayla �a�r�l�r
 *    - Her callback'e 3 parametre ge�irilir: (amount, mHealth, mMaxHealth)
 *    - �l� referanslar (expired weak_ptr) otomatik temizlenir
 *    - Sadece ya�ayan nesnelerin callback'leri �al���r
 * 
 * 4. CALLBACK FONKS�YONU (Subscriber taraf�nda - SpaceShip i�inde):
 *    
 *    void SpaceShip::OnHealthChanged(float amt, float health, float maxHealth)
 *    {
 *        LOG("SpaceShip health changed: %f/%f (change: %f)", health, maxHealth, amt);
 *        // Sa�l�k de�i�imine g�re aksiyon al (animasyon, ses, UI update vb.)
 *    }
 *    
 *    Bu fonksiyon:
 *    - Event tetiklendi�inde otomatik �a�r�l�r
 *    - SpaceShip silinirse art�k �a�r�lmaz (weak_ptr g�venli�i)
 *    - Thread-safe de�il - ana thread'de �a�r�l�r
 * 
 * =========================================================================
 * MEMORY Y�NET�M� VE G�VENL�K
 * =========================================================================
 * 
 * 1. WEAK_PTR G�VENL���:
 *    - SpaceShip silinirse weak_ptr otomatik "expired" olur
 *    - Broadcast s�ras�nda expired callback'ler temizlenir
 *    - Dangling pointer crash'leri �nlenir
 *    - Manual unsubscribe gerekmez
 * 
 * 2. EXCEPTION SAFETY:
 *    - Callback i�inde exception at�l�rsa ne olur?
 *    - �u anki kod exception'lar� yakalamaz
 *    - Exception propagate olur, broadcast yar�da kesilir
 *    - Production'da try-catch eklemek gerekebilir
 * 
 * 3. THREAD SAFETY:
 *    - Bu implementasyon thread-safe DE��L
 *    - Ayn� anda BindAction ve Broadcast �a�r�l�rsa race condition
 *    - Multi-thread kullan�mda mutex gerekir
 * 
 * 4. PERFORMANCE:
 *    - std::function virtual call overhead'i var
 *    - weak_ptr.lock() atomic operasyon - maliyetli
 *    - �ok s�k �a�r�lan event'ler i�in optimize edilebilir
 *    - Ancak bu overhead �o�u zaman ihmal edilebilir
 */
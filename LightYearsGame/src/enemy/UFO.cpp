#include "enemy/UFO.h"
#include "weapon/BulletShooter.h"
#include <framework/World.h>

namespace ly
{
	UFO::UFO(World* owningWorld, const sf::Vector2f& velocity, const std::string& texturePath, float rotationSpeed) :
		EnemySpaceShip { owningWorld, texturePath },
		mShooter1{new BulletShooter{this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, sf::Vector2{ 0.f,0.f},60.f}},
		mShooter2{new BulletShooter{this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, sf::Vector2{ 0.f,0.f},-60.f}},
		mShooter3{new BulletShooter{this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, sf::Vector2{ 0.f,0.f},180.f}},
		mRotationSpeed{rotationSpeed}
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Plasma);
	}

	UFO::~UFO()
	{
		LOG("UFO destroyed");
	}

	void UFO::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
		AddActorRotationOffset(deltaTime * mRotationSpeed);

		CheckBounce();
	}

	void UFO::SetupCollisionLayers()
	{
		// UFO için özel collision setup
		SetCollisionLayer(CollisionLayer::Enemy);
		// 🎯 UFO'lar birbirleriyle de çarpışır (Enemy katmanını mask'e ekle)
		SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet | CollisionLayer::Enemy);
	}

	void UFO::OnActorBeginOverlap(Actor* otherActor)
	{
		// Base class'ı çağır (collision mask kontrolü için)
		SpaceShip::OnActorBeginOverlap(otherActor);
		
		if (!otherActor || !GetCanCollide()) return;
		
		// 🎯 Eğer çarpışan nesne de bir UFO ise
		UFO* otherUFO = dynamic_cast<UFO*>(otherActor);
		if (otherUFO)
		{
			// ═══════════════════════════════════════════════════════════════════════════════
			// FİZİK TABANLI ÇARPIŞMA SİSTEMİ
			// ═══════════════════════════════════════════════════════════════════════════════
			// Bu sistem gerçek fizik prensiplerini kullanır:
			// 1. Momentum Korunumu (p = m*v)
			// 2. Elastik/İnelastik Çarpışma (restitution coefficient)
			// 3. İmpuls-Momentum Teoremi (J = Δp)
			// ═══════════════════════════════════════════════════════════════════════════════
			
			LOG("UFO-UFO collision detected - physics-based bounce!");
			
			// ───────────────────────────────────────────────────────────────────────────────
			// ADIM 1: ÇARPIŞMA GEOMETRİSİNİ BELİRLE
			// ───────────────────────────────────────────────────────────────────────────────
			// Çarpışma geometrisi, iki cismin merkezleri arasındaki vektöre dayanır.
			// Bu vektör "çarpışma normal vektörü" olarak adlandırılır.
			
			// Pozisyonları al
			sf::Vector2f thisPos = GetActorLocation();   // Bu UFO'nun pozisyonu (P1)
			sf::Vector2f otherPos = otherUFO->GetActorLocation(); // Diğer UFO'nun pozisyonu (P2)
			
			// Çarpışma normal vektörünü hesapla
			// n = P1 - P2
			// Bu vektör, çarpışma noktasındaki "yüzey normalini" temsil eder
			sf::Vector2f collisionNormal = thisPos - otherPos;
			
			// ÖZEL DURUM: İki cisim tam üst üste ise (çok nadir)
			// Bu durumda rastgele bir yön seçeriz
			if (collisionNormal.x == 0.f && collisionNormal.y == 0.f)
			{
				collisionNormal = sf::Vector2f(RandRange(-1.f, 1.f), RandRange(-1.f, 1.f));
			}
			
			// Normal vektörü normalize et (birim vektör yap, uzunluk = 1)
			// n̂ = n / |n|
			// Normalize etme nedeni: Sadece YÖN bilgisi istiyoruz, büyüklük değil
			NormalizeVector(collisionNormal);
			
			// ───────────────────────────────────────────────────────────────────────────────
			// ADIM 2: HAREKETLİLİK DURUMUNU ANALİZ ET
			// ───────────────────────────────────────────────────────────────────────────────
			// Çarpışmanın gerçekten olup olmadığını kontrol etmek için
			// cisimlerin birbirine yaklaşıp yaklaşmadığını kontrol ederiz.
			
			// Hızları al
			sf::Vector2f v1 = GetVelocity();        // Bu UFO'nun hızı
			sf::Vector2f v2 = otherUFO->GetVelocity(); // Diğer UFO'nun hızı
			
			// Göreceli hız hesapla (relative velocity)
			// v_rel = v1 - v2
			// Bu, UFO1'in UFO2'ye göre ne kadar hızlı hareket ettiğini gösterir
			sf::Vector2f relativeVelocity = v1 - v2;
			
			// Göreceli hızın normal yöndeki bileşenini hesapla (Dot Product / İç Çarpım)
			// v_n = v_rel · n̂
			// İç çarpım formülü: A · B = Ax*Bx + Ay*By
			// 
			// Fiziksel Anlamı:
			// - v_n < 0: Cisimler birbirine yaklaşıyor (çarpışma var)
			// - v_n > 0: Cisimler birbirinden uzaklaşıyor (çarpışma bitti)
			// - v_n = 0: Cisimler yan yana hareket ediyor (접촉 yok)
			float velocityAlongNormal = relativeVelocity.x * collisionNormal.x + 
			 relativeVelocity.y * collisionNormal.y;
			
			// Eğer cisimler birbirinden uzaklaşıyorsa, çarpışma işlemi yapma
			// (Çünkü zaten çarpışma bitmiş ve ayrılıyorlar)
			if (velocityAlongNormal > 0.f)
			{
				return; // Erken çıkış
			}
			
			// ───────────────────────────────────────────────────────────────────────────────
			// ADIM 3: ELASTİKİYET KATSAYISINI BELİRLE
			// ───────────────────────────────────────────────────────────────────────────────
			// Restitution coefficient (e): Çarpışmanın ne kadar "elastik" olduğunu belirler
			//
			// Değer Aralığı: 0.0 ≤ e ≤ 1.0
			//
			// e = 0.0: Tamamen İnelastik Çarpışma
			//   - Cisimler çarpıştıktan sonra yapışırlar
			//   - Maksimum enerji kaybı
			//   - Örnek: Çamur topları, hamur
			//
			// e = 0.5: Orta Elastikiyet
			//   - Yarı-elastik çarpışma
			//   - %50 enerji kaybı
			//   - Örnek: Basketbol topu (sert zemin)
			//
			// e = 0.8: Yüksek Elastikiyet (ŞU ANKİ AYAR)
			//   - Çok az enerji kaybı (%20)
			//   - Cisimler güçlü bir şekilde sekiyor
			//   - Örnek: Çelik bilardo topları
			//
			// e = 1.0: Tamamen Elastik Çarpışma
			//   - Hiç enerji kaybı yok
			//   - Teorik ideal durum
			//   - Gerçek dünyada nadiren görülür
			//
			// Matematik:
			// e = -(v1' - v2') / (v1 - v2)
			// (v1', v2' = çarpışma sonrası hızlar)
			float restitution = 0.8f;
			
			// ───────────────────────────────────────────────────────────────────────────────
			// ADIM 4: İMPULS BÜYÜKLÜĞÜNÜ HESAPLA (MOMENTUM KORUNUMU)
			// ───────────────────────────────────────────────────────────────────────────────
			//
			// FİZİK TEORİSİ:
			// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
			//
			// 1. MOMENTUM KORUNUMU:
			//    m1*v1 + m2*v2 = m1*v1' + m2*v2'
			//    (Toplam momentum önce = Toplam momentum sonra)
			//
			// 2. RESTİTUTİON DENKLEMİ:
			//    v1' - v2' = -e * (v1 - v2)
			//    (Ayrılma hızı = -e × Yaklaşma hızı)
			//
			// 3. BU İKİ DENKLEMİ BİRLEŞTİREREK İMPULS FORMÜLÜNÜ ELDE EDERİZ:
			//
			//    İmpuls (J) = Δp = m * Δv
			//    
			//    Eşit kütleler için (m1 = m2 = m):
			//    J = -(1 + e) * m * v_rel_n / (1/m1 + 1/m2)
			//    J = -(1 + e) * m * v_rel_n / (2/m)
			//    J = -(1 + e) * v_rel_n / 2
			//
			// NEDEN (1 + e)?
			//   - 1: Momentum korunumundan gelir
			//   - e: Elastikiyetten gelir
			//   - Toplamı, hız değişiminin büyüklüğünü verir
			//
			// NEDEN / 2?
			//   - İki eşit kütleli cisim impulsü paylaşır
			//- Her biri impulun yarısını alır
			//   - Bu, Newton'un 3. yasasının sonucudur (aksiyon-reaksiyon)
			//
			// NEDEN NEGATİF?
			//   - velocityAlongNormal < 0 (cisimler yaklaşıyor)
			//   - İmpuls, cisimleri birbirinden UZAKLAŞTIRACAK yönde olmalı
			//   - Negatif işaret, yönü tersine çevirir
			//
			// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
			
			float impulseMagnitude = -(1.f + restitution) * velocityAlongNormal / 2.f;
			
			// İmpuls vektörünü hesapla
			// J_vec = j * n̂
			// (Büyüklük × Yön = Vektör)
			sf::Vector2f impulse = collisionNormal * impulseMagnitude;
			
			// ───────────────────────────────────────────────────────────────────────────────
			// ADIM 5: YENİ HIZLARI HESAPLA
			// ───────────────────────────────────────────────────────────────────────────────
			//
			// İMPULS-MOMENTUM TEOREMİ:
			// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
			//
			// J = Δp = m * Δv
			// Δv = J / m
			// v' = v + Δv = v + J/m
			//
			// Eşit kütleler için (m1 = m2):
			// v1' = v1 + J/m1 = v1 + J  (m = 1 varsayıyoruz, normalize edilmiş)
			// v2' = v2 - J/m2 = v2 - J  (Newton'un 3. yasası: Zıt yönler)
			//
			// NEWTON'UN 3. YASASI (Aksiyon-Reaksiyon):
			// UFO1'e uygulanan kuvvet = -UFO2'ye uygulanan kuvvet
			// Bu yüzden biri +J, diğeri -J alır
			//
			// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
			
			sf::Vector2f newVelocity1 = v1 + impulse;  // UFO1'in yeni hızı
			sf::Vector2f newVelocity2 = v2 - impulse;  // UFO2'nin yeni hızı (zıt yön)
			
			// Yeni hızları uygula
			SetVelocity(newVelocity1);
			otherUFO->SetVelocity(newVelocity2);
			
			// ───────────────────────────────────────────────────────────────────────────────
			// ADIM 6: PENETRASYON ÇÖZÜMÜ (Collision Resolution)
			// ───────────────────────────────────────────────────────────────────────────────
			//
			// SORUN:
			// Fizik simülasyonlarında, cisimler bazen birbirinin "içine girer"
			// Bu, discrete time stepping'den kaynaklanır (sürekli değil, adım adım)
			//
			// ÇÖZÜM:
			// Cisimleri birbirinden ayırmak için pozisyon düzeltmesi yapılır
			//
			// YÖNTEM:
			// 1. Minimal ayrılma mesafesi belirle (penetrationDepth)
			// 2. Bu mesafeyi çarpışma normal yönünde uygula
			// 3. Her iki cismi de eşit miktarda kaydır (adalet için)
			//
			// MATEMATİK:
			// separation = n̂ * d (d = penetrasyon derinliği)
			// P1_new = P1 + separation * 0.5
			// P2_new = P2 - separation * 0.5
			//
			// NEDEN 0.5?
			// Her iki UFO da yarı yarıya sorumluluk alır
			// Bu, daha dengeli ve kararlı bir simülasyon sağlar
			//
			float penetrationDepth = 10.f; // Minimal ayrılma mesafesi (piksel)
			sf::Vector2f separation = collisionNormal * penetrationDepth;
			
			// Pozisyon düzeltmesi uygula
			SetActorLocation(thisPos + separation * 0.5f);      // UFO1'i hafifçe iter
			otherUFO->SetActorLocation(otherPos - separation * 0.5f); // UFO2'yi hafifçe iter
			
			// ═══════════════════════════════════════════════════════════════════════════════
			// SONUÇ: FİZİKSEL OLARAK DOĞRU ÇARPIŞMA
			// ═══════════════════════════════════════════════════════════════════════════════
			// ✅ Momentum korundu
			// ✅ Enerji kısmen korundu (restitution'a bağlı)
			// ✅ Newton yasaları uygulandı
			// ✅ Penetrasyon çözüldü
			// ✅ Gerçekçi sekme efekti elde edildi
			// ═══════════════════════════════════════════════════════════════════════════════
			
			// Hasar yok - UFO'lar birbirlerine zarar vermez
			return;
		}
		
		// Diğer nesnelere (Player, PlayerBullet, diğer düşmanlar) normal hasar ver
		otherActor->ApplyDamage(GetCollisionDamage());
	}

	void UFO::CheckBounce()
	{
		// Pencere boyutunu al
		auto windowSize = GetWorld()->GetWindowSize();

		// UFO'nun pozisyonunu al
		sf::Vector2f currentPos = GetActorLocation();
		sf::Vector2f currentVelocity = GetVelocity();

		// Sprite boyutını al (collision için)
		float spriteWidth = GetActorGlobalBounds().size.x - 100.f;

		bool bounced = false;

		// 🔴 SOL SINIR KONTROLÜ (X ekseninde sekme)
		if (currentPos.x - spriteWidth / 2.f <= 0.f && currentVelocity.x < 0.f)
		{
			// X hızını tersine çevir (sekme efekti)
			currentVelocity.x = -currentVelocity.x;
			bounced = true;

			// Pozisyonu sınır içine çek
			currentPos.x = spriteWidth / 2.f;
		}
		// 🔴 SAĞ SINIR KONTROLÜ (X ekseninde sekme)
		else if (currentPos.x + spriteWidth / 2.f >= windowSize.x && currentVelocity.x > 0.f)
		{
			// X hızını tersine çevir (sekme efekti)
			currentVelocity.x = -currentVelocity.x;
			bounced = true;

			// Pozisyonu sınır içine çek
			currentPos.x = windowSize.x - spriteWidth / 2.f;
		}

		// Eğer sekme olduysa, yeni velocity ve position'ı uygula
		if (bounced)
		{
			SetVelocity(currentVelocity);
			SetActorLocation(currentPos);
		}
	}

	void UFO::Shoot()
	{
		mShooter1->Shoot();
		mShooter2->Shoot();
		mShooter3->Shoot();
	}
}
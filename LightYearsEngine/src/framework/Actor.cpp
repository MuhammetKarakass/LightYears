#include "framework/Actor.h"
#include "framework/AssetManager.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include <framework/PhysicsSystem.h>

namespace ly
{
	// Çağrıldığı Yer: World::SpawnActor<ActorType>() içinde new ActorType(this, args...) ile.
	// Açıklama: Actor nesnesi oluşturulduğunda çalışır. Üye değişkenleri başlatır ve texture yükler.
	Actor::Actor(World* OwningWorld, const std::string& TexturePath)
		: mOwningWorld{ OwningWorld },  // Bu actor'ın sahibi olan dünya
		mBeganPlay{ false },            // Henüz BeginPlay çağrılmadı
		mSprite{},                      // Grafik sprite (boş başlıyor)
		mTexture{},                     // Texture dosyası referansı
		mPhysicsBodyId{},               // Box2D fizik body'si (boş başlıyor)
		mPhysicsEnabled{ false },       // Fizik sistemi kapalı başlıyor
		mCollisionLayer{ CollisionLayer::None },  // Hangi katmanda çarpışır
		mCollisionMask{ CollisionLayer::None },   // Hangi katmanları algılar
		mCanCollide{ false },
		mTickWhenPaused{ false }
	{
		SetTexture(TexturePath);  // Verilen texture'ı yükle ve sprite'a ata
	}
	
	// Çağrıldığı Yer: C++ tarafından, Actor'ü tutan son shared_ptr yok olduğunda (World::CleanCycle sonrası).
	// Açıklama: Actor bellekten silinirken çalışır. Log mesajı yazdırır.
	Actor::~Actor()
	{
	
	}
	
	// Çağrıldığı Yer: World::TickInternal() - yeni oluşturulan Actor'lar için (mPendingActors'tan mActors'a taşınırken).
	// Açıklama: BeginPlay'in sadece bir kez çağrılmasını garanti eden wrapper fonksiyon.
	void Actor::BeginPlayInternal()
	{
		if (!mBeganPlay)  
		{
			mBeganPlay = true;    
			BeginPlay();          
		}
	}
	
	// Çağrıldığı Yer: Actor::BeginPlayInternal() içinde.
	// Açıklama: Türetilmiş sınıfların (Spaceship, Bullet vb.) Actor ilk kez oyuna girdiğinde
	// yapılacak özel işlemleri (event'lere abone olma, fizik başlatma vb.) tanımlaması için override edilir.
	void Actor::BeginPlay()
	{
		
	}

	// Çağrıldığı Yer: World::TickInternal() - her frame, tüm aktif Actor'lar için.
	// Açıklama: Actor'ün güncellenmesini yöneten ana fonksiyon. BeginPlay çağrılmışsa ve
	// Actor silinmek üzere işaretlenmemişse Tick() fonksiyonunu çağırır.
	void Actor::TickInternal(float deltaTime)
	{
		if (mBeganPlay && !GetIsPendingDestroy())
		{
			Tick(deltaTime);  // Gerçek Tick'i çağır (virtual)
		}
	}

	// Çağrıldığı Yer: Actor::TickInternal() içinde.
	// Açıklama: Türetilmiş sınıfların her frame'de yapılacak özel mantıklarını
	// (hareket, ateş etme, AI vb.) tanımlaması için override edilir.
	void Actor::Tick(float deltaTime)
	{
		// Base class'ta boş - türetilen sınıflar kendi mantığını ekler
	}
	
	// Çağrıldığı Yer: Genellikle türetilmiş Actor sınıfları tarafından (örn: can bittiğinde, ekran dışına çıktığında).
	// Açıklama: Actor'ü yok etme sürecini başlatır. Önce fizik sisteminden çıkarır, sonra Object::Destroy() çağırır.
	void Actor::Destroy()
	{
		UnInitializePhysics();  // Önce fizik sisteminden çıkar
		onActorDestroyed.Broadcast(this); // Yıkılma olayı yayınla
		Object::Destroy();    // Sonra base class destruction (mPendingDestroy = true)
	}
	
	// Çağrıldığı Yer: World::Render() - her frame, tüm aktif Actor'lar için.
	// Açıklama: Actor'ün sprite'ını ekrana çizer. Sprite yoksa veya Actor silinecekse çizmez.
	void Actor::Render(sf::RenderWindow& window)
	{
		if (!mSprite.has_value() || GetIsPendingDestroy()) return;
		window.draw(mSprite.value());  
	}
	
	// Çağrıldığı Yer: Genellikle türetilmiş Actor sınıflarının Tick() fonksiyonlarında (örn: Bullet, Enemy).
	// Açıklama: Actor'ün oyun penceresinin dışına çıkıp çıkmadığını kontrol eder.
	// allowance parametresi, ekran kenarlarından ne kadar dışarı çıkmasına izin verildiğini belirtir.
	bool Actor::IsActorOutOfWindow(float allowance) const
	{
		float windowWidth = GetWindowSize().x;   
		float windowHeight = GetWindowSize().y;  

		float width = GetActorGlobalBounds().size.x; 
		float height = GetActorGlobalBounds().size.y;  

		sf::Vector2f actorPosition = GetActorLocation();  

		if(actorPosition.x < -width-allowance-10.f)
			return true;

		if(actorPosition.x > windowWidth + width+allowance+10.f)
			return true;

		if (actorPosition.y < -height-allowance-10.f)
			return true;

		if (actorPosition.y > windowHeight + height+allowance+10.f)
			return true;

		return false;  
	}
	
	// Çağrıldığı Yer: Genellikle türetilmiş Actor sınıflarının constructor'ında veya BeginPlay'inde.
	// Açıklama: Actor'ün fizik sistemini açar veya kapatır.
	void Actor::SetEnablePhysics(bool enable)
	{
		mPhysicsEnabled = enable;

		if (mPhysicsEnabled)
		{
			InitializePhysics();    
		}
		else
		{
			UnInitializePhysics();  
		}
	}
	
	// Çağrıldığı Yer: Actor::SetEnablePhysics(true) içinde.
	// Açıklama: Actor'ü Box2D fizik sistemine ekler ve bir fizik body'si (b2BodyId) oluşturur.
	void Actor::InitializePhysics()
	{
		if (!mPhysicsBodyId)  
		{
			mPhysicsBodyId = PhysicsSystem::Get().AddListener(this);
		}
	}
	
	// Çağrıldığı Yer: Actor::SetEnablePhysics(false) veya Actor::Destroy() içinde.
	// Açıklama: Actor'ün fizik body'sini Box2D dünyasından kaldırır.
	void Actor::UnInitializePhysics()
	{
		if(mPhysicsBodyId)
		{
			PhysicsSystem::Get().RemoveListener(*mPhysicsBodyId);
			mPhysicsBodyId.reset();  
		}
	}

	// Çağrıldığı Yer: Actor::SetActorLocation() ve Actor::SetActorRotation() içinde.
	// Açıklama: SFML sprite'ının pozisyonunu ve rotasyonunu Box2D fizik body'sine senkronize eder.
	void Actor::UpdatePhysicsTransform()
	{
		if (mPhysicsBodyId)  // Fizik body'si varsa
		{
			float physicsRate = PhysicsSystem::Get().GetPhysicsRate();  // Ölçek faktörü
			sf::Vector2f actorLocation = GetActorLocation();            // Sprite pozisyonu
			float actorRotation = GetActorRotation();     // Sprite rotasyonu
			
			b2Vec2 position{ actorLocation.x * physicsRate, actorLocation.y * physicsRate };
			b2Rot rotation = b2MakeRot(DegreesToRadians(actorRotation));  // Derece → Radian
			
			b2Body_SetTransform(*mPhysicsBodyId, position, rotation);
		}
	}

	// Çağrıldığı Yer: Actor::OnActorBeginOverlap() ve Actor::OnActorEndOverlap() içinde.
	// Açıklama: Bu Actor'ün belirtilen diğer Actor ile çarpışıp çarpışamayacağını kontrol eder.
	// Collision mask sistemi: Bu Actor'ün maskesi, diğer Actor'ün katmanını içermeli.
	bool Actor::CanCollideWith(const Actor* other) const
	{
		if (other == nullptr) return false;
		return HasCollisionLayer(mCollisionMask, other->GetCollisionLayer());
	}

	// Çağrıldığı Yer: PhysicsSystem::ProcessContactEvents() - iki Actor'ün fizik body'leri çarpıştığında.
	// Açıklama: Çarpışma başladığında çağrılır. Collision layer/mask sistemi ile filtreleme yapar.
	// Türetilmiş sınıflar bu fonksiyonu override ederek çarpışma olaylarına tepki verebilir (örn: hasar verme).
	void Actor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (!otherActor) return;
		
		// Her iki taraf da birbirini maskelemeli (çift yönlü filtre)
		if (!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this))
		{
			return; 
		}

		mCanCollide = (CanCollideWith(otherActor) || otherActor->CanCollideWith(this));
	}

	// Çağrıldığı Yer: PhysicsSystem::ProcessContactEvents() - iki Actor'ün fizik body'leri ayrıldığında.
	// Açıklama: Çarpışma bittiğinde çağrılır. Collision layer/mask sistemi ile filtreleme yapar.
	void Actor::OnActorEndOverlap(Actor* otherActor)
	{
		if (!otherActor) return;
		
		if (!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this))
		{
			return; 
		}
	}

	// Çağrıldığı Yer: Genellikle Bullet::OnActorBeginOverlap() gibi çarpışma callback'lerinde.
	// Açıklama: Bu Actor'a hasar vermek için çağrılır. Türetilmiş sınıflar (Spaceship vb.)
	// override ederek hasar mantığını (HealthComponent'e iletme vb.) tanımlar.
	void Actor::ApplyDamage(float amt)
	{
		
	}

	// Çağrıldığı Yer: Actor::IsActorOutOfWindow() ve çarpışma kontrollerinde.
	// Açıklama: Sprite'ın ekrandaki sınırlayıcı kutusunu (bounding box) döndürür.
	sf::FloatRect Actor::GetActorGlobalBounds() const
	{
		return mSprite.value().getGlobalBounds(); 
	}
	
	// Çağrıldığı Yer: Actor constructor'ında.
	// Açıklama: AssetManager'dan texture yükler, sprite'a atar ve merkez pivot noktasını ayarlar.
	void Actor::SetTexture(const std::string& texturePath)
	{
		mTexture = AssetManager::GetAssetManager().LoadTexture(texturePath);
		if (!mTexture) return;  

		mSprite.emplace(*mTexture);

		int width = mTexture->getSize().x;
		int height = mTexture->getSize().y;
		mSprite.value().setTextureRect(sf::IntRect{ sf::Vector2i{}, sf::Vector2i{width,height} });
		
		CenterPivot();  
	}

	// Çağrıldığı Yer: Actor::IsActorOutOfWindow() ve türetilmiş sınıflarda.
	// Açıklama: Oyun penceresinin boyutunu döndürür. World üzerinden Application'a erişir.
	sf::Vector2u Actor::GetWindowSize() const
	{
		return mOwningWorld->GetWindowSize();
	}

	// Çağrıldığı Yer: Türetilmiş Actor sınıflarının Tick() fonksiyonlarında (hareket için).
	// Açıklama: Actor'ün dünya koordinatlarındaki pozisyonunu ayarlar ve fizik body'sini günceller.
	void Actor::SetActorLocation(const sf::Vector2f& newLoc)
	{
		mSprite.value().setPosition(newLoc); 
		UpdatePhysicsTransform(); 
	}
	
	// Çağrıldığı Yer: Türetilmiş Actor sınıflarında (döndürme için).
	// Açıklama: Actor'ün rotasyonunu (derece cinsinden) ayarlar ve fizik body'sini günceller.
	void Actor::SetActorRotation(float newRotation)
	{
		mSprite.value().setRotation(sf::degrees(newRotation));  
		UpdatePhysicsTransform();  
	}

	void Actor::SetTextureRepeated(bool repeated)
	{
		if (mTexture)
		{
			mTexture->setRepeated(repeated);
		}
	}
	
	// Çağrıldığı Yer: Türetilmiş Actor sınıflarının Tick() fonksiyonlarında (hareket için).
	// Açıklama: Actor'ün mevcut pozisyonuna bir offset (kayma) ekler.
	void Actor::AddActorLocationOffset(const sf::Vector2f& offset)
	{
		SetActorLocation(GetActorLocation() + offset);  
	}
	
	// Çağrıldığı Yer: Türetilmiş Actor sınıflarında (döndürme animasyonları için).
	// Açıklama: Actor'ün mevcut rotasyonuna bir offset ekler.
	void Actor::AddActorRotationOffset(float offset)
	{
		SetActorRotation(GetActorRotation() + offset);  
	}
	
	// Çağrıldığı Yer: Actor::SetTexture() içinde.
	// Açıklama: Sprite'ın merkez noktasını (origin) sprite'ın geometrik merkezine ayarlar.
	// Bu, rotasyon ve ölçeklendirme işlemlerinin sprite'ın merkezinden yapılmasını sağlar.
	void Actor::CenterPivot()
	{
		sf::FloatRect rectBounds = mSprite.value().getGlobalBounds();  
		mSprite.value().setOrigin(sf::Vector2f{rectBounds.size.x/2.f, rectBounds.size.y/2.f});
	}
	
	// Çağrıldığı Yer: Türetilmiş Actor sınıflarında ve Actor içinde (pozisyon hesaplamaları için).
	// Açıklama: Actor'ün dünya koordinatlarındaki mevcut pozisyonunu döndürür.
	sf::Vector2f Actor::GetActorLocation() const
	{
		return mSprite.value().getPosition();  
	}
	
	// Çağrıldığı Yer: Türetilmiş Actor sınıflarında (rotasyon hesaplamaları için).
	// Açıklama: Actor'ün mevcut rotasyonunu derece cinsinden döndürür.
	float Actor::GetActorRotation() const
	{
		return mSprite.value().getRotation().asDegrees(); 
	}
	
	// Çağrıldığı Yer: Türetilmiş Actor sınıflarında (ileri hareket, mermi ateşleme vb.).
	// Açıklama: Actor'ün "ileri" yönünü birim vektör olarak döndürür.
	// SFML'de sprite'lar varsayılan olarak sağa bakar, bu yüzden 90 derece çıkarılır.
	sf::Vector2f Actor::GetActorForwardDirection() const
	{
		return RotationToVector(GetActorRotation() - 90.f); 
	}
	
	// Çağrıldığı Yer: Türetilmiş Actor sınıflarında (yan hareket, yatay hizalama vb.).
	// Açıklama: Actor'ün "sağ" yönünü birim vektör olarak döndürür.
	sf::Vector2f Actor::GetActorRightDirection() const
	{
		return RotationToVector(GetActorRotation());  
	}
	
	// ========================================================================
	// LOCAL SPACE TRANSFORMATION SYSTEM - SPRITE'IN KENDİ KOORDİNAT SİSTEMİ
	// ========================================================================
	
	// Çağrıldığı Yer: Actor::AddActorLocalLocationOffset() içinde.
	// Açıklama: Actor'ün lokal koordinat sistemindeki bir offset'i (örn: "2 birim ileri, 1 birim sağa")
	// dünya koordinat sistemine dönüştürür. Bu, Actor döndürülmüş olsa bile doğru yönde hareket etmesini sağlar.
	sf::Vector2f Actor::TransformLocalToWorld(const sf::Vector2f& localOffset) const
	{

		sf::Vector2f right = GetActorRightDirection();        // Sağ = direkt rotasyon 
		sf::Vector2f forward = GetActorForwardDirection();    // İleri = rotasyon - 90°
		
		// Linear combination ile local offset'i world'e çevir
		return localOffset.x * right + localOffset.y * forward;
	}
	
	// Çağrıldığı Yer: Türetilmiş Actor sınıflarında (lokal koordinatlarda hareket için, örn: yan yana kayma).
	// Açıklama: Actor'ün kendi koordinat sisteminde bir hareket yapar.
	// Örneğin localOffset = (0, 10) demek "10 birim ileri git" demektir, Actor'ün rotasyonu ne olursa olsun.
	void Actor::AddActorLocalLocationOffset(const sf::Vector2f& localOffset)
	{
		sf::Vector2f worldOffset = TransformLocalToWorld(localOffset);  // Local → World dönüşümü
		AddActorLocationOffset(worldOffset);        // World space'de hareket ettir
	}
	
	// Çağrıldığı Yer: Genellikle AI veya hedef takip sistemlerinde.
	// Açıklama: Dünya koordinatlarındaki bir noktanın, Actor'ün lokal koordinat sisteminde nerede olduğunu hesaplar.
	// Örneğin, bir düşmanın oyuncuya göre "önünde mi, arkasında mı, sağında mı" olduğunu anlamak için kullanılır.
	sf::Vector2f Actor::GetActorLocalLocation(const sf::Vector2f& worldLocation) const
	{
		// INVERSE TRANSFORMATION
		sf::Vector2f deltaWorld = worldLocation - GetActorLocation();  // Dünya'da sprite'dan target'a vektör
		float rotation = GetActorRotation();
		float radians = DegreesToRadians(rotation - 90.f);            // MathUtility hazır fonksiyonunu kullan
		
		// INVERSE ROTATION MATRIX UYGULA
		float cosR = std::cos(-radians);  // Inverse rotation cosine
		float sinR = std::sin(-radians);  // Inverse rotation sine
		
		return sf::Vector2f{
			deltaWorld.x * cosR - deltaWorld.y * sinR,  // Local X (sprite'ın sağ/sol ekseni)
			deltaWorld.x * sinR + deltaWorld.y * cosR   // Local Y (sprite'ın ileri/geri ekseni)  
		};
	}
	void Actor::SetCollisionRadius(float radius)
	{
		if (!mPhysicsBodyId) return;
		PhysicsSystem::Get().SetCollisionRadius(*mPhysicsBodyId, radius);
	}
}
#include "enemy/UFO.h"
#include "weapon/BulletShooter.h"
#include <framework/World.h>
#include <cmath> // std::sqrt için (artık gerekmeyebilir ama kalsın)
#include <algorithm> // std::max, std::min için
#include <framework/MathUtility.h> // NormalizeVector için

namespace ly
{
	UFO::UFO(World* owningWorld, const sf::Vector2f& velocity, const std::string& texturePath, float rotationSpeed) :
		EnemySpaceShip{ owningWorld, texturePath }, // <-- Doku burada yükleniyor
		mShooter1{ new BulletShooter{this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, sf::Vector2{ 35.f, 20.f},60.f} },
		mShooter2{ new BulletShooter{this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, sf::Vector2{ -35.f, 20.f},-60.f} },
		mShooter3{ new BulletShooter{this, "SpaceShooterRedux/PNG/Lasers/laserRed01.png", 1.f, sf::Vector2{ 0.f, -40.f},180.f} },
		mRotationSpeed{ rotationSpeed }
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Plasma);

		float visualRadius = std::min(GetActorGlobalBounds().size.x, GetActorGlobalBounds().size.y) / 2.f;
		float collisionRadius = visualRadius * 0.4f;  // %40 küçült
		SetCollisionRadius(collisionRadius);
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
			// FİZİK TABANLI ÇARPIŞMA SİSTEMİ (Basitleştirilmiş)
			// ═══════════════════════════════════════════════════════════════════════════════
			// Constructor'da SetCollisionRadius ayarlandığı için,
			// bu fonksiyon zaten UFO'lar iç içe geçtiğinde çağrılır.
			// Artık karmaşık yarıçap hesaplarına gerek yok.

			LOG("UFO-UFO collision (Kucuk Radius ile) detected!");

			// ──────────────────────────────────────────────────────────────────
			// ADIM 1: ÇARPIŞMA GEOMETRİSİNİ BELİRLE (Basitleştirildi)
			// ──────────────────────────────────────────────────────────────────
			sf::Vector2f thisPos = GetActorLocation();
			sf::Vector2f otherPos = otherUFO->GetActorLocation();

			sf::Vector2f collisionNormal = thisPos - otherPos;

			// ÖZEL DURUM: İki cisim tam üst üste ise (çok nadir)
			if (collisionNormal.x == 0.f && collisionNormal.y == 0.f)
			{
				collisionNormal = sf::Vector2f(RandRange(-1.f, 1.f), RandRange(-1.f, 1.f));
			}

			NormalizeVector(collisionNormal); // Sadece yön lazım

			// ──────────────────────────────────────────────────────────────────
			// ADIM 2: HAREKETLİLİK ANALİZİ
			// ──────────────────────────────────────────────────────────────────
			sf::Vector2f v1 = GetVelocity();
			sf::Vector2f v2 = otherUFO->GetVelocity();
			sf::Vector2f relativeVelocity = v1 - v2;

			// Göreceli hızın normal yöndeki bileşenini hesapla (Dot Product)
			float velocityAlongNormal = relativeVelocity.x * collisionNormal.x + relativeVelocity.y * collisionNormal.y;

			// Eğer cisimler birbirinden uzaklaşıyorsa, çarpışma işlemi yapma
			if (velocityAlongNormal > 0.f)
			{
				return; // Zaten ayrılıyorlar
			}

			// ──────────────────────────────────────────────────────────────────
			// ADIM 3: ELASTİKİYET KATSAYISI
			// ──────────────────────────────────────────────────────────────────
			float restitution = 0.8f;

			// ──────────────────────────────────────────────────────────────────
			// ADIM 4: İMPULS HESAPLAMA
			// ──────────────────────────────────────────────────────────────────
			float impulseMagnitude = -(1.f + restitution) * velocityAlongNormal / 2.f;
			sf::Vector2f impulse = collisionNormal * impulseMagnitude;

			// ──────────────────────────────────────────────────────────────────
			// ADIM 5: YENİ HIZLARI UYGULA
			// ──────────────────────────────────────────────────────────────────
			SetVelocity(v1 + impulse);
			otherUFO->SetVelocity(v2 - impulse);

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
		// NOT: Buradaki -100.f'nin ne yaptığından emin olun.
		// Bu, kenar sekmesi için de küçültülmüş bir kutu kullanır.
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
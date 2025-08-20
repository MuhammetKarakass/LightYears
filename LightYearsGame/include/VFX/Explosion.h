#pragma once
#include <string>
#include <framework/Core.h>
#include <framework/MathUtility.h>

namespace ly
{
	class World;

	// RGB bileþenleri için min-max range yapýsý
	struct ColorRange
	{
		int min{0};
		int max{255};
		
		ColorRange() = default;
		ColorRange(int minVal, int maxVal) : min(minVal), max(maxVal) {}
		
		int Random() const {
			return RandRange(min, max);
		}
	};

	// Explosion parametrelerini yönetmek için struktur
	struct ExplosionParams
	{
		int particleCount{25};
		float lifeTimeMin{0.5f};
		float lifeTimeMax{1.5f};
		float sizeMin{1.0f};
		float sizeMax{2.5f};
		float speedMin{200.0f};
		float speedMax{400.0f};
		
		// RGB color ranges
		ColorRange redRange{255, 255};     // Kýrmýzý aralýðý
		ColorRange greenRange{128, 255};   // Yeþil aralýðý
		ColorRange blueRange{0, 50};       // Mavi aralýðý
		uint8_t alpha{255};                // Sabit alpha
		
		List<std::string> particleTextures{
			"SpaceShooterRedux/PNG/Effects/star1.png",
			"SpaceShooterRedux/PNG/Effects/star2.png",
			"SpaceShooterRedux/PNG/Effects/star3.png"
		};
		
		// Convenience method to generate random color
		sf::Color GenerateRandomColor() const {
			return sf::Color(
				redRange.Random(),
				greenRange.Random(),
				blueRange.Random(),
				alpha
			);
		}
	};

	// Düþman tipine göre explosion çeþitleri
	enum class ExplosionType
	{
		Tiny,           // Mermi çarpmasý
		Small,          // Küçük düþmanlar
		Medium,         // Orta düþmanlar
		Heavy,          // Aðýr düþmanlar
		Plasma,         // Plazma silahlarý
		Boss,           // Boss patlamalarý
		Environmental   // Çevre nesneleri
	};

	class Explosion
	{
	public:
		Explosion() = delete; // Instance yaratmayý engelle

		// Ana kullaným: Tip bazlý
		static void SpawnExplosion(World* world, const sf::Vector2f& location, ExplosionType type);
		
		// Özel parametrelerle (nadir durumlar)
		static void SpawnExplosion(World* world, const sf::Vector2f& location, const ExplosionParams& params);
		
		// Preset parametreleri al
		static ExplosionParams GetPreset(ExplosionType type);

	private:
		// Internal spawn helper
		static void SpawnExplosionWithParams(World* world, const sf::Vector2f& location, const ExplosionParams& params);
	};
}


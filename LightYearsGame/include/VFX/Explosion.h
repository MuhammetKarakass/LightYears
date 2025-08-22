#pragma once
#include <string>
#include <framework/Core.h>
#include <framework/MathUtility.h>

namespace ly
{
	class World;

	// RGB bile�enleri i�in min-max range yap�s�
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

	// Explosion parametrelerini y�netmek i�in struktur
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
		ColorRange redRange{255, 255};     // K�rm�z� aral���
		ColorRange greenRange{128, 255};   // Ye�il aral���
		ColorRange blueRange{0, 50};       // Mavi aral���
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

	// D��man tipine g�re explosion �e�itleri
	enum class ExplosionType
	{
		Tiny,           // Mermi �arpmas�
		Small,          // K���k d��manlar
		Medium,         // Orta d��manlar
		Heavy,          // A��r d��manlar
		Plasma,         // Plazma silahlar�
		Boss,           // Boss patlamalar�
		Environmental   // �evre nesneleri
	};

	class Explosion
	{
	public:
		Explosion() = delete; // Instance yaratmay� engelle

		// Ana kullan�m: Tip bazl�
		static void SpawnExplosion(World* world, const sf::Vector2f& location, ExplosionType type);
		
		// �zel parametrelerle (nadir durumlar)
		static void SpawnExplosion(World* world, const sf::Vector2f& location, const ExplosionParams& params);
		
		// Preset parametreleri al
		static ExplosionParams GetPreset(ExplosionType type);

	private:
		// Internal spawn helper
		static void SpawnExplosionWithParams(World* world, const sf::Vector2f& location, const ExplosionParams& params);
	};
}


#pragma once

#include <cstddef>

#include <SFML/Graphics.hpp>

namespace vega::sky
{
	// Sets shader uniforms for the per-frame draws without sf::Shader::setUniform.
	// SFML asks the driver for the bound program (glGetHandle) on every setUniform call. With NVIDIA's threaded
	// OpenGL driver each such query waits until the driver thread has drained its queue, so the few hundred uniforms
	// a frame sets turned into a few hundred CPU round trips; when other processes kept the cores busy, frames took
	// 50-300 ms. This writer binds the program once, writes through cached locations and puts back the program that
	// was bound before (SFML leaves none bound between draws), never querying GL state.
	// Writers nest. Falls back to sf::Shader::setUniform when the GL entry points or a current context are missing.
	class UniformWriter
	{
	public:
		explicit UniformWriter(sf::Shader& shader);
		~UniformWriter();
		UniformWriter(const UniformWriter&) = delete;
		UniformWriter& operator=(const UniformWriter&) = delete;

		void Set(const char* name, float value);
		void Set(const char* name, int value);
		void Set(const char* name, sf::Vector2f value);
		void Set(const char* name, const sf::Glsl::Vec3& value);
		void Set(const char* name, const sf::Glsl::Vec4& value);
		void SetArray(const char* name, const sf::Glsl::Vec3* values, std::size_t count);
		void SetArray(const char* name, const sf::Glsl::Vec4* values, std::size_t count);

		// Uniform locations are cached per program; call after shaders are (re)compiled, because GL may hand a new
		// program the handle of a deleted one.
		static void InvalidateLocations();

	private:
		int Location(const char* name);

		sf::Shader& mShader;
		unsigned int mProgram = 0;
		unsigned int mPreviousProgram = 0;
		bool mDirect = false;
	};
}

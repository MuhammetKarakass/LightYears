#include "visual/UniformWriter.h"

#include <string>
#include <type_traits>
#include <unordered_map>

#include <SFML/OpenGL.hpp>
#include <SFML/Window/Context.hpp>

#ifdef _WIN32
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#else
#ifndef APIENTRY
#define APIENTRY
#endif
#endif

namespace ly::visual
{
	namespace
	{
		using UseProgramFn = void(APIENTRY*)(GLuint);
		using GetUniformLocationFn = GLint(APIENTRY*)(GLuint, const char*);
		using Uniform1fFn = void(APIENTRY*)(GLint, GLfloat);
		using Uniform1iFn = void(APIENTRY*)(GLint, GLint);
		using Uniform2fFn = void(APIENTRY*)(GLint, GLfloat, GLfloat);
		using Uniform3fFn = void(APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat);
		using Uniform4fFn = void(APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
		using UniformFvFn = void(APIENTRY*)(GLint, GLsizei, const GLfloat*);

		struct GlFunctions
		{
			UseProgramFn useProgram = nullptr;
			GetUniformLocationFn getUniformLocation = nullptr;
			Uniform1fFn uniform1f = nullptr;
			Uniform1iFn uniform1i = nullptr;
			Uniform2fFn uniform2f = nullptr;
			Uniform3fFn uniform3f = nullptr;
			Uniform4fFn uniform4f = nullptr;
			UniformFvFn uniform3fv = nullptr;
			UniformFvFn uniform4fv = nullptr;
			bool loaded = false;
			bool complete = false;
		};

		GlFunctions& Functions()
		{
			static GlFunctions functions;
			if (!functions.loaded)
			{
#ifdef _WIN32
				// The entry points can only be resolved with a current context; retry on a later frame otherwise.
				if (wglGetCurrentContext() == nullptr)
				{
					return functions;
				}
#endif
				functions.loaded = true;
				auto load = [](auto& slot, const char* name)
				{
					slot = reinterpret_cast<std::remove_reference_t<decltype(slot)>>(sf::Context::getFunction(name));
				};
				load(functions.useProgram, "glUseProgram");
				load(functions.getUniformLocation, "glGetUniformLocation");
				load(functions.uniform1f, "glUniform1f");
				load(functions.uniform1i, "glUniform1i");
				load(functions.uniform2f, "glUniform2f");
				load(functions.uniform3f, "glUniform3f");
				load(functions.uniform4f, "glUniform4f");
				load(functions.uniform3fv, "glUniform3fv");
				load(functions.uniform4fv, "glUniform4fv");
				functions.complete = functions.useProgram && functions.getUniformLocation && functions.uniform1f && functions.uniform1i
					&& functions.uniform2f && functions.uniform3f && functions.uniform4f && functions.uniform3fv && functions.uniform4fv;
			}
			return functions;
		}

		bool HasCurrentContext()
		{
#ifdef _WIN32
			return wglGetCurrentContext() != nullptr;
#else
			return false;
#endif
		}

		// Program bound by the innermost live writer; 0 outside writers, which is what SFML leaves after each draw.
		unsigned int gBoundProgram = 0;

		std::unordered_map<unsigned int, std::unordered_map<std::string, int>>& LocationCache()
		{
			static std::unordered_map<unsigned int, std::unordered_map<std::string, int>> cache;
			return cache;
		}
	}

	UniformWriter::UniformWriter(sf::Shader& shader)
		: mShader(shader)
		, mProgram(shader.getNativeHandle())
	{
		const GlFunctions& gl = Functions();
		mDirect = mProgram != 0 && gl.complete && HasCurrentContext();
		if (mDirect)
		{
			mPreviousProgram = gBoundProgram;
			if (mPreviousProgram != mProgram)
			{
				gl.useProgram(mProgram);
			}
			gBoundProgram = mProgram;
		}
	}

	UniformWriter::~UniformWriter()
	{
		if (mDirect)
		{
			if (mPreviousProgram != mProgram)
			{
				Functions().useProgram(mPreviousProgram);
			}
			gBoundProgram = mPreviousProgram;
		}
	}

	void UniformWriter::InvalidateLocations()
	{
		LocationCache().clear();
	}

	int UniformWriter::Location(const char* name)
	{
		std::unordered_map<std::string, int>& locations = LocationCache()[mProgram];
		const auto it = locations.find(name);
		if (it != locations.end())
		{
			return it->second;
		}
		// One query per uniform name and program, not one per write.
		const int location = Functions().getUniformLocation(mProgram, name);
		locations.emplace(name, location);
		return location;
	}

	void UniformWriter::Set(const char* name, float value)
	{
		if (!mDirect)
		{
			mShader.setUniform(name, value);
			return;
		}
		if (const int location = Location(name); location != -1)
		{
			Functions().uniform1f(location, value);
		}
	}

	void UniformWriter::Set(const char* name, int value)
	{
		if (!mDirect)
		{
			mShader.setUniform(name, value);
			return;
		}
		if (const int location = Location(name); location != -1)
		{
			Functions().uniform1i(location, value);
		}
	}

	void UniformWriter::Set(const char* name, sf::Vector2f value)
	{
		if (!mDirect)
		{
			mShader.setUniform(name, sf::Glsl::Vec2(value));
			return;
		}
		if (const int location = Location(name); location != -1)
		{
			Functions().uniform2f(location, value.x, value.y);
		}
	}

	void UniformWriter::Set(const char* name, const sf::Glsl::Vec3& value)
	{
		if (!mDirect)
		{
			mShader.setUniform(name, value);
			return;
		}
		if (const int location = Location(name); location != -1)
		{
			Functions().uniform3f(location, value.x, value.y, value.z);
		}
	}

	void UniformWriter::Set(const char* name, const sf::Glsl::Vec4& value)
	{
		if (!mDirect)
		{
			mShader.setUniform(name, value);
			return;
		}
		if (const int location = Location(name); location != -1)
		{
			Functions().uniform4f(location, value.x, value.y, value.z, value.w);
		}
	}

	void UniformWriter::SetArray(const char* name, const sf::Glsl::Vec3* values, std::size_t count)
	{
		if (!mDirect)
		{
			mShader.setUniformArray(name, values, count);
			return;
		}
		// Arrays are looked up by their base name, as sf::Shader::setUniformArray does.
		if (const int location = Location(name); location != -1)
		{
			Functions().uniform3fv(location, static_cast<GLsizei>(count), &values[0].x);
		}
	}

	void UniformWriter::SetArray(const char* name, const sf::Glsl::Vec4* values, std::size_t count)
	{
		if (!mDirect)
		{
			mShader.setUniformArray(name, values, count);
			return;
		}
		if (const int location = Location(name); location != -1)
		{
			Functions().uniform4fv(location, static_cast<GLsizei>(count), &values[0].x);
		}
	}
}
